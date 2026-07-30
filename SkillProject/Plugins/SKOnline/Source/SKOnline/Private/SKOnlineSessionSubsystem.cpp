// Fill out your copyright notice in the Description page of Project Settings.

#include "SKOnlineSessionSubsystem.h"

#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "SKOnlineSettings.h"
#include "SKSessionOpRules.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKOnlineSessionSubsystem)

IOnlineSessionPtr USKOnlineSessionSubsystem::GetSessionInterfaceChecked(ESKSessionOp Op)
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem == nullptr)
	{
		FailOp(Op, ESKSessionError::NoOnlineSubsystem, TEXT("IOnlineSubsystem::Get() returned null"));
		return nullptr;
	}

	IOnlineSessionPtr Sessions = Subsystem->GetSessionInterface();
	if (Sessions.IsValid() == false)
	{
		FailOp(Op, ESKSessionError::NoOnlineSubsystem, TEXT("Session interface is invalid"));
		return nullptr;
	}

	return Sessions;
}

bool USKOnlineSessionSubsystem::BeginOp(ESKSessionOp RequestedOp)
{
	if (USKSessionOpRules::CanStartOp(CurrentOp, RequestedOp) == false)
	{
		//# 진행 중 작업이 있으므로 조용히 거부하되 소비자가 알 수 있게 통지한다
		OnSessionError.Broadcast(RequestedOp, ESKSessionError::Busy, TEXT("Another session operation is in progress"));
		return false;
	}

	CurrentOp = RequestedOp;
	return true;
}

void USKOnlineSessionSubsystem::EndOp()
{
	CurrentOp = ESKSessionOp::None;
}

void USKOnlineSessionSubsystem::FailOp(ESKSessionOp Op, ESKSessionError Error, const FString& Detail)
{
	UE_LOG(LogTemp, Error, TEXT("# [SKOnlineSessionSubsystem] 세션 작업 실패 (Op=%d, Error=%d): %s"),
		(int32)Op, (int32)Error, *Detail);

	EndOp();
	OnSessionError.Broadcast(Op, Error, Detail);
}

FString USKOnlineSessionSubsystem::BuildRoomName() const
{
	const USKOnlineSettings* Settings = GetDefault<USKOnlineSettings>();

	FString HostName = TEXT("Player");
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const ULocalPlayer* LocalPlayer = GameInstance->GetFirstGamePlayer())
		{
			const FString NickName = LocalPlayer->GetNickname();
			if (NickName.IsEmpty() == false)
			{
				HostName = NickName;
			}
		}
	}

	if (Settings == nullptr || Settings->DefaultRoomNameFormat.IsEmpty())
		return HostName;

	return FString::Format(*Settings->DefaultRoomNameFormat, { HostName });
}

void USKOnlineSessionSubsystem::CancelFindForHost()
{
	//# 취소할 검색이 없으면 곧바로 통지해 소비자 흐름이 멈추지 않게 한다
	if (USKSessionOpRules::ShouldPreemptFindForHost(CurrentOp) == false)
	{
		OnFindCancelled.Broadcast();
		return;
	}

	IOnlineSessionPtr Sessions = GetSessionInterfaceChecked(ESKSessionOp::Finding);
	if (Sessions.IsValid() == false)
		return;

	//# 취소 완료 콜백에서 통지한다. 이 플래그는 취소 중 도착하는 검색 실패를 삼키는 판정도 겸한다.
	bNotifyHostAfterCancelFind = true;

	CancelFindSessionsCompleteHandle = Sessions->AddOnCancelFindSessionsCompleteDelegate_Handle(
		FOnCancelFindSessionsCompleteDelegate::CreateUObject(this, &USKOnlineSessionSubsystem::HandleCancelFindSessionsComplete));

	UE_LOG(LogTemp, Log, TEXT("# [SKOnlineSessionSubsystem] 진행 중인 검색을 취소합니다 — 방 만들기 요청"));

	if (Sessions->CancelFindSessions() == false)
	{
		//# 취소가 거부됐다(취소할 검색이 없는 등). 콜백이 이미 동기 실행됐으면 플래그가 내려가 있다.
		if (bNotifyHostAfterCancelFind)
		{
			Sessions->ClearOnCancelFindSessionsCompleteDelegate_Handle(CancelFindSessionsCompleteHandle);
			CancelFindSessionsCompleteHandle.Reset();
			bNotifyHostAfterCancelFind = false;

			//# 검색 상태를 직접 정리하고 곧바로 통지한다
			FinishPreemptedFind();
			OnFindCancelled.Broadcast();
		}
	}
}

void USKOnlineSessionSubsystem::FinishPreemptedFind()
{
	//# 취소된 검색의 완료 콜백은 받지 않는다 — 취소로 인한 실패를 사용자에게 통지하지 않기 위해서다
	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		if (IOnlineSessionPtr Sessions = Subsystem->GetSessionInterface())
		{
			if (FindSessionsCompleteHandle.IsValid())
			{
				Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
			}
		}
	}
	FindSessionsCompleteHandle.Reset();

	//# 취소된 검색 결과는 조인 인덱스로 쓸 수 없다
	SessionSearch.Reset();

	if (CurrentOp == ESKSessionOp::Finding)
	{
		EndOp();
	}
}

void USKOnlineSessionSubsystem::HandleCancelFindSessionsComplete(bool bWasSuccessful)
{
	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		if (IOnlineSessionPtr Sessions = Subsystem->GetSessionInterface())
		{
			Sessions->ClearOnCancelFindSessionsCompleteDelegate_Handle(CancelFindSessionsCompleteHandle);
		}
	}
	CancelFindSessionsCompleteHandle.Reset();

	const bool bNotifyHost = bNotifyHostAfterCancelFind;
	bNotifyHostAfterCancelFind = false;

	//# 취소 성공/실패와 무관하게 검색 상태는 정리한다 — 실패해도 사용자가 기다릴 이유가 없다
	FinishPreemptedFind();

	if (bNotifyHost == false)
		return;

	UE_LOG(LogTemp, Log, TEXT("# [SKOnlineSessionSubsystem] 검색 취소 완료(성공=%d) — 방 만들기 진행"), bWasSuccessful ? 1 : 0);
	OnFindCancelled.Broadcast();
}

void USKOnlineSessionSubsystem::HostSession()
{
	if (BeginOp(ESKSessionOp::Hosting) == false)
		return;

	IOnlineSessionPtr Sessions = GetSessionInterfaceChecked(ESKSessionOp::Hosting);
	if (Sessions.IsValid() == false)
		return;

	//# 이전 세션이 남아 있으면 생성이 실패한다 — 먼저 지운다
	if (Sessions->GetNamedSession(NAME_GameSession) != nullptr)
	{
		Sessions->DestroySession(NAME_GameSession);
	}

	const USKOnlineSettings* Settings = GetDefault<USKOnlineSettings>();
	if (Settings == nullptr)
	{
		FailOp(ESKSessionOp::Hosting, ESKSessionError::CreateFailed, TEXT("USKOnlineSettings CDO is null"));
		return;
	}

	FOnlineSessionSettings SessionSettings;
	SessionSettings.NumPublicConnections = Settings->MaxPlayers;
	SessionSettings.NumPrivateConnections = 0;
	SessionSettings.bIsLANMatch = Settings->bIsLanMatch;
	SessionSettings.bShouldAdvertise = Settings->bShouldAdvertise;
	SessionSettings.bAllowJoinInProgress = Settings->bAllowJoinInProgress;
	SessionSettings.bAllowJoinViaPresence = Settings->bUsesPresence;
	SessionSettings.bUsesPresence = Settings->bUsesPresence;
	SessionSettings.bUseLobbiesIfAvailable = Settings->bUseLobbiesIfAvailable;
	SessionSettings.bIsDedicated = false;

	//# 방 이름을 광고에 실어 목록에 표시한다
	SessionSettings.Set(
		SKOnlineKeys::RoomName,
		BuildRoomName(),
		EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	CreateSessionCompleteHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &USKOnlineSessionSubsystem::HandleCreateSessionComplete));

	if (Sessions->CreateSession(0, NAME_GameSession, SessionSettings) == false)
	{
		//# Null 은 실패 콜백을 이 호출 안에서 이미 실행했을 수 있다. 그때는 EndOp 로 op 가 None 이라 중복 통지하지 않는다.
		//# 아직 Hosting 이면 콜백이 오지 않은 것이라 여기서 핸들을 걷고 정리한다.
		if (CurrentOp == ESKSessionOp::Hosting)
		{
			Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
			CreateSessionCompleteHandle.Reset();
			FailOp(ESKSessionOp::Hosting, ESKSessionError::CreateFailed, TEXT("CreateSession returned false"));
		}
	}
}

void USKOnlineSessionSubsystem::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	//# 잔류 핸들의 뒤늦은 발화·다른 op 문맥의 콜백을 무시한다. 핸들 정리는 등록한 쪽이 책임진다.
	if (CurrentOp != ESKSessionOp::Hosting || SessionName != NAME_GameSession)
		return;

	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		if (IOnlineSessionPtr Sessions = Subsystem->GetSessionInterface())
		{
			Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
		}
	}
	CreateSessionCompleteHandle.Reset();

	if (bWasSuccessful == false)
	{
		FailOp(ESKSessionOp::Hosting, ESKSessionError::CreateFailed, TEXT("CreateSession completed unsuccessfully"));
		return;
	}

	EndOp();

	UE_LOG(LogTemp, Log, TEXT("# [SKOnlineSessionSubsystem] 방 생성 완료 — 리슨 서버 전환 대기"));
	OnHostReady.Broadcast();
}

void USKOnlineSessionSubsystem::FindSessions()
{
	if (BeginOp(ESKSessionOp::Finding) == false)
		return;

	IOnlineSessionPtr Sessions = GetSessionInterfaceChecked(ESKSessionOp::Finding);
	if (Sessions.IsValid() == false)
		return;

	const USKOnlineSettings* Settings = GetDefault<USKOnlineSettings>();
	if (Settings == nullptr)
	{
		FailOp(ESKSessionOp::Finding, ESKSessionError::FindFailed, TEXT("USKOnlineSettings CDO is null"));
		return;
	}

	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->bIsLanQuery = Settings->bIsLanMatch;
	SessionSearch->MaxSearchResults = Settings->MaxSearchResults;

	//# 로비 검색 질의 — Steam 은 이 키로 로비 경로를 탄다. Null 은 bIsLanQuery 가 라우팅하므로 무관.
	if (Settings->bUseLobbiesIfAvailable)
	{
		SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	}

	FindSessionsCompleteHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &USKOnlineSessionSubsystem::HandleFindSessionsComplete));

	if (Sessions->FindSessions(0, SessionSearch.ToSharedRef()) == false)
	{
		//# Null 은 실패 콜백을 이 호출 안에서 이미 실행했을 수 있다 — 중복 통지 방지(HostSession 과 동일 규약)
		if (CurrentOp == ESKSessionOp::Finding)
		{
			Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
			FindSessionsCompleteHandle.Reset();
			FailOp(ESKSessionOp::Finding, ESKSessionError::FindFailed, TEXT("FindSessions returned false"));
		}
	}
}

void USKOnlineSessionSubsystem::HandleFindSessionsComplete(bool bWasSuccessful)
{
	//# 선점 취소 중 도착한 완료 콜백은 사용자 통지 대상이 아니다 — 취소가 만든 실패이므로 삼킨다
	if (bNotifyHostAfterCancelFind)
	{
		UE_LOG(LogTemp, Log, TEXT("# [SKOnlineSessionSubsystem] 검색 취소 중 완료 콜백 — 무시 (성공=%d)"), bWasSuccessful ? 1 : 0);
		return;
	}

	//# 잔류 핸들의 뒤늦은 발화·다른 op 문맥의 콜백을 무시한다. 핸들 정리는 등록한 쪽이 책임진다.
	if (CurrentOp != ESKSessionOp::Finding)
		return;

	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		if (IOnlineSessionPtr Sessions = Subsystem->GetSessionInterface())
		{
			Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
		}
	}
	FindSessionsCompleteHandle.Reset();

	if (bWasSuccessful == false || SessionSearch.IsValid() == false)
	{
		FailOp(ESKSessionOp::Finding, ESKSessionError::FindFailed, TEXT("FindSessions completed unsuccessfully"));
		return;
	}

	TArray<FSKSessionInfo> Infos;
	Infos.Reserve(SessionSearch->SearchResults.Num());

	for (int32 Index = 0; Index < SessionSearch->SearchResults.Num(); ++Index)
	{
		const FOnlineSessionSearchResult& Result = SessionSearch->SearchResults[Index];
		if (Result.IsValid() == false)
			continue;

		FString RoomName;
		Result.Session.SessionSettings.Get(SKOnlineKeys::RoomName, RoomName);

		Infos.Add(FSKSessionInfo::Make(
			RoomName,
			Result.Session.OwningUserName,
			Result.Session.SessionSettings.NumPublicConnections,
			Result.Session.NumOpenPublicConnections,
			Result.PingInMs,
			Index));
	}

	EndOp();

	UE_LOG(LogTemp, Log, TEXT("# [SKOnlineSessionSubsystem] 방 검색 완료 — %d 건"), Infos.Num());
	OnSessionsFound.Broadcast(Infos);
}

void USKOnlineSessionSubsystem::JoinSessionByIndex(int32 Index)
{
	if (BeginOp(ESKSessionOp::Joining) == false)
		return;

	if (SessionSearch.IsValid() == false || SessionSearch->SearchResults.IsValidIndex(Index) == false)
	{
		FailOp(ESKSessionOp::Joining, ESKSessionError::InvalidIndex, TEXT("Search result index is out of range"));
		return;
	}

	IOnlineSessionPtr Sessions = GetSessionInterfaceChecked(ESKSessionOp::Joining);
	if (Sessions.IsValid() == false)
		return;

	JoinSessionCompleteHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &USKOnlineSessionSubsystem::HandleJoinSessionComplete));

	if (Sessions->JoinSession(0, NAME_GameSession, SessionSearch->SearchResults[Index]) == false)
	{
		//# Null 은 실패 콜백을 이 호출 안에서 이미 실행했을 수 있다 — 중복 통지 방지(HostSession 과 동일 규약)
		if (CurrentOp == ESKSessionOp::Joining)
		{
			Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
			JoinSessionCompleteHandle.Reset();
			FailOp(ESKSessionOp::Joining, ESKSessionError::JoinFailed, TEXT("JoinSession returned false"));
		}
	}
}

void USKOnlineSessionSubsystem::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	//# 잔류 핸들의 뒤늦은 발화·다른 op 문맥의 콜백을 무시한다. 핸들 정리는 등록한 쪽이 책임진다.
	if (CurrentOp != ESKSessionOp::Joining || SessionName != NAME_GameSession)
		return;

	IOnlineSessionPtr Sessions = nullptr;
	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		Sessions = Subsystem->GetSessionInterface();
	}

	if (Sessions.IsValid())
	{
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
	}
	JoinSessionCompleteHandle.Reset();

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		//# 실패한 세션이 남아 있으면 다음 조인이 막힌다 — 정리한다
		if (Sessions.IsValid())
		{
			Sessions->DestroySession(NAME_GameSession);
		}

		FailOp(ESKSessionOp::Joining, ESKSessionError::JoinFailed,
			FString::Printf(TEXT("JoinSession result: %d"), (int32)Result));
		return;
	}

	FString ConnectString;
	if (Sessions.IsValid() == false || Sessions->GetResolvedConnectString(NAME_GameSession, ConnectString) == false
		|| ConnectString.IsEmpty())
	{
		if (Sessions.IsValid())
		{
			Sessions->DestroySession(NAME_GameSession);
		}

		FailOp(ESKSessionOp::Joining, ESKSessionError::ResolveFailed, TEXT("GetResolvedConnectString failed"));
		return;
	}

	EndOp();

	UE_LOG(LogTemp, Log, TEXT("# [SKOnlineSessionSubsystem] 조인 완료 — 접속 문자열: %s"), *ConnectString);
	OnJoinReady.Broadcast(ConnectString);
}

void USKOnlineSessionSubsystem::DestroyCurrentSession()
{
	if (BeginOp(ESKSessionOp::Destroying) == false)
		return;

	IOnlineSessionPtr Sessions = GetSessionInterfaceChecked(ESKSessionOp::Destroying);
	if (Sessions.IsValid() == false)
		return;

	//# 지울 세션이 없으면 성공으로 본다
	if (Sessions->GetNamedSession(NAME_GameSession) == nullptr)
	{
		EndOp();
		return;
	}

	DestroySessionCompleteHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &USKOnlineSessionSubsystem::HandleDestroySessionComplete));

	if (Sessions->DestroySession(NAME_GameSession) == false)
	{
		//# Null 은 실패 콜백을 이 호출 안에서 이미 실행했을 수 있다 — 중복 통지 방지(HostSession 과 동일 규약)
		if (CurrentOp == ESKSessionOp::Destroying)
		{
			Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
			DestroySessionCompleteHandle.Reset();
			FailOp(ESKSessionOp::Destroying, ESKSessionError::DestroyFailed, TEXT("DestroySession returned false"));
		}
	}
}

void USKOnlineSessionSubsystem::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	//# 잔류 핸들의 뒤늦은 발화·다른 op 문맥의 콜백을 무시한다. 핸들 정리는 등록한 쪽이 책임진다.
	if (CurrentOp != ESKSessionOp::Destroying || SessionName != NAME_GameSession)
		return;

	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		if (IOnlineSessionPtr Sessions = Subsystem->GetSessionInterface())
		{
			Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
		}
	}
	DestroySessionCompleteHandle.Reset();

	if (bWasSuccessful == false)
	{
		FailOp(ESKSessionOp::Destroying, ESKSessionError::DestroyFailed, TEXT("DestroySession completed unsuccessfully"));
		return;
	}

	EndOp();
}

void USKOnlineSessionSubsystem::ClearDelegateHandles(IOnlineSessionPtr Sessions)
{
	if (Sessions.IsValid() == false)
		return;

	if (CreateSessionCompleteHandle.IsValid())
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
		CreateSessionCompleteHandle.Reset();
	}
	if (FindSessionsCompleteHandle.IsValid())
	{
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
		FindSessionsCompleteHandle.Reset();
	}
	if (JoinSessionCompleteHandle.IsValid())
	{
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
		JoinSessionCompleteHandle.Reset();
	}
	if (DestroySessionCompleteHandle.IsValid())
	{
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
		DestroySessionCompleteHandle.Reset();
	}
	if (CancelFindSessionsCompleteHandle.IsValid())
	{
		Sessions->ClearOnCancelFindSessionsCompleteDelegate_Handle(CancelFindSessionsCompleteHandle);
		CancelFindSessionsCompleteHandle.Reset();
	}
}

void USKOnlineSessionSubsystem::DestroyLingeringSession(const TCHAR* Reason)
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem == nullptr)
		return;

	IOnlineSessionPtr Sessions = Subsystem->GetSessionInterface();
	if (Sessions.IsValid() == false)
		return;

	if (Sessions->GetNamedSession(NAME_GameSession) == nullptr)
		return;

	//# 완료 델리게이트를 걸지 않는다 — 종료 경로에서는 서브시스템이 사라지는 중이라 콜백을 받을 수 없고,
	//# 시작 경로에서도 결과를 기다릴 이유가 없다(목적은 LAN 광고를 즉시 끊는 것이다).
	UE_LOG(LogTemp, Warning, TEXT("# [SKOnlineSessionSubsystem] 잔존 세션을 정리합니다 — %s"), Reason);
	Sessions->DestroySession(NAME_GameSession);
}

void USKOnlineSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	//# 에디터는 PIE 가 끝나도 OSS 인스턴스를 유지한다 — 이전 실행의 세션이 LAN 에 계속 광고돼
	//# 아무도 만들지 않은 방(포트 0)이 목록에 뜬다. 검색만 하는 실행도 깨끗하게 시작하도록 지운다.
	DestroyLingeringSession(TEXT("서브시스템 시작"));
}

void USKOnlineSessionSubsystem::Deinitialize()
{
	//# op 상태와 무관하게 무조건 정리한다 — 종료 경로에 BeginOp 가드를 태우면 남을 수 있다
	DestroyLingeringSession(TEXT("서브시스템 종료"));

	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		ClearDelegateHandles(Subsystem->GetSessionInterface());
	}

	SessionSearch.Reset();
	CurrentOp = ESKSessionOp::None;
	bNotifyHostAfterCancelFind = false;

	OnSessionsFound.Clear();
	OnHostReady.Clear();
	OnJoinReady.Clear();
	OnSessionError.Clear();
	OnFindCancelled.Clear();

	Super::Deinitialize();
}
