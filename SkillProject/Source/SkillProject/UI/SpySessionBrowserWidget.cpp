// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SpySessionBrowserWidget.h"

#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "SKOnlineSessionSubsystem.h"
#include "SKSessionOpRules.h"
#include "Manager/SpyLoadingSubsystem.h"
#include "UI/SpySessionRowWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpySessionBrowserWidget)

FString USpySessionBrowserWidget::MakeStatusMessage(ESKSessionOp Op, ESKSessionError Error)
{
	//# 중복 입력 가드는 사용자 잘못이 아니므로 아무것도 보이지 않는다
	if (Error == ESKSessionError::Busy || Error == ESKSessionError::None)
		return FString();

	if (Error == ESKSessionError::NoOnlineSubsystem)
		return TEXT("온라인 기능을 사용할 수 없습니다");

	switch (Op)
	{
	case ESKSessionOp::Hosting:
		return TEXT("방을 만들지 못했습니다");

	case ESKSessionOp::Finding:
		return TEXT("방 목록을 불러오지 못했습니다");

	case ESKSessionOp::Joining:
		return TEXT("방에 들어가지 못했습니다");

	default:
		return TEXT("온라인 요청을 처리하지 못했습니다");
	}
}

FString USpySessionBrowserWidget::MakeProgressMessage(ESKSessionOp CurrentOp, ESKSessionOp RequestedOp)
{
	//# 방 만들기는 진행 중인 검색을 선점하므로 요청한 쪽이 사실이 된다. 그 외에는 진행 중인 작업이 사실이다.
	const bool bPreempts = (RequestedOp == ESKSessionOp::Hosting) && USKSessionOpRules::ShouldPreemptFindForHost(CurrentOp);
	const ESKSessionOp DisplayOp = (CurrentOp == ESKSessionOp::None || bPreempts) ? RequestedOp : CurrentOp;

	switch (DisplayOp)
	{
	case ESKSessionOp::Finding:
		return TEXT("방 목록을 불러오는 중입니다");

	case ESKSessionOp::Hosting:
		return TEXT("방을 만드는 중입니다");

	case ESKSessionOp::Joining:
		return TEXT("방에 들어가는 중입니다");

	case ESKSessionOp::Destroying:
		return TEXT("이전 방을 정리하는 중입니다");

	default:
		return FString();
	}
}

USKOnlineSessionSubsystem* USpySessionBrowserWidget::GetSessionSubsystem() const
{
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance == nullptr)
		return nullptr;

	return GameInstance->GetSubsystem<USKOnlineSessionSubsystem>();
}

void USpySessionBrowserWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(RefreshButton))
	{
		RefreshButton->OnClicked.AddDynamic(this, &USpySessionBrowserWidget::OnRefreshClicked);
	}
	if (IsValid(HostButton))
	{
		HostButton->OnClicked.AddDynamic(this, &USpySessionBrowserWidget::OnHostClicked);
	}

	//# 캐시된 위젯이 재오픈될 수 있다 — 이전 검색 결과가 남지 않게 먼저 비운다
	if (IsValid(SessionListBox))
	{
		SessionListBox->ClearChildren();
	}

	SetStatus(FString());

	USKOnlineSessionSubsystem* SessionSubsystem = GetSessionSubsystem();
	if (SessionSubsystem == nullptr)
	{
		SetStatus(MakeStatusMessage(ESKSessionOp::Finding, ESKSessionError::NoOnlineSubsystem));
		return;
	}

	//# OSS Null 은 완료 콜백을 명령 호출 안에서 동기 발화한다 — 반드시 구독이 먼저다
	SessionsFoundHandle = SessionSubsystem->OnSessionsFound.AddUObject(this, &USpySessionBrowserWidget::HandleSessionsFound);
	HostReadyHandle = SessionSubsystem->OnHostReady.AddUObject(this, &USpySessionBrowserWidget::HandleHostReady);
	JoinReadyHandle = SessionSubsystem->OnJoinReady.AddUObject(this, &USpySessionBrowserWidget::HandleJoinReady);
	SessionErrorHandle = SessionSubsystem->OnSessionError.AddUObject(this, &USpySessionBrowserWidget::HandleSessionError);
	FindCancelledHandle = SessionSubsystem->OnFindCancelled.AddUObject(this, &USpySessionBrowserWidget::HandleFindCancelled);

	//# 진행 문구는 명령보다 먼저 — 명령 안에서 결과 문구가 이미 세팅될 수 있다
	SetStatus(MakeProgressMessage(SessionSubsystem->GetCurrentOp(), ESKSessionOp::Finding));

	//# 화면이 뜨자마자 한 번 검색한다 — 사용자가 새로고침을 먼저 누르지 않아도 되게
	SessionSubsystem->FindSessions();
}

void USpySessionBrowserWidget::NativeDestruct()
{
	if (USKOnlineSessionSubsystem* SessionSubsystem = GetSessionSubsystem())
	{
		SessionSubsystem->OnSessionsFound.Remove(SessionsFoundHandle);
		SessionSubsystem->OnHostReady.Remove(HostReadyHandle);
		SessionSubsystem->OnJoinReady.Remove(JoinReadyHandle);
		SessionSubsystem->OnSessionError.Remove(SessionErrorHandle);
		SessionSubsystem->OnFindCancelled.Remove(FindCancelledHandle);
	}

	SessionsFoundHandle.Reset();
	HostReadyHandle.Reset();
	JoinReadyHandle.Reset();
	SessionErrorHandle.Reset();
	FindCancelledHandle.Reset();

	if (IsValid(RefreshButton))
	{
		RefreshButton->OnClicked.RemoveDynamic(this, &USpySessionBrowserWidget::OnRefreshClicked);
	}
	if (IsValid(HostButton))
	{
		HostButton->OnClicked.RemoveDynamic(this, &USpySessionBrowserWidget::OnHostClicked);
	}

	Super::NativeDestruct();
}

void USpySessionBrowserWidget::SetStatus(const FString& Message)
{
	if (IsValid(StatusText) == false)
		return;

	if (Message.IsEmpty())
	{
		StatusText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	StatusText->SetText(FText::FromString(Message));
	StatusText->SetVisibility(ESlateVisibility::Visible);
}

void USpySessionBrowserWidget::HandleSessionsFound(const TArray<FSKSessionInfo>& Infos)
{
	if (IsValid(SessionListBox) == false)
		return;

	SessionListBox->ClearChildren();

	if (Infos.Num() == 0)
	{
		//# 결과 0건은 에러가 아니다 — 안내만 띄우고 새로고침을 남겨 둔다
		SetStatus(TEXT("방이 없습니다"));
		return;
	}

	SetStatus(FString());

	if (RowWidgetClass == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("# [SpySessionBrowserWidget] RowWidgetClass 가 지정되지 않았습니다"));
		return;
	}

	for (const FSKSessionInfo& Info : Infos)
	{
		USpySessionRowWidget* Row = CreateWidget<USpySessionRowWidget>(this, RowWidgetClass);
		if (Row == nullptr)
			continue;

		Row->SetSessionInfo(Info);
		Row->OnRowClicked.AddUObject(this, &USpySessionBrowserWidget::HandleRowClicked);

		//# 패널 공용 API — VerticalBox/ScrollBox 어느 쪽이든 동작한다
		SessionListBox->AddChild(Row);
	}
}

void USpySessionBrowserWidget::OnRefreshClicked()
{
	USKOnlineSessionSubsystem* SessionSubsystem = GetSessionSubsystem();
	if (SessionSubsystem == nullptr)
		return;

	//# 문구를 먼저 세우고 명령을 보낸다 — 명령 호출 뒤에는 결과가 이미 반영돼 있을 수 있다
	SetStatus(MakeProgressMessage(SessionSubsystem->GetCurrentOp(), ESKSessionOp::Finding));

	SessionSubsystem->FindSessions();
}

void USpySessionBrowserWidget::OnHostClicked()
{
	USKOnlineSessionSubsystem* SessionSubsystem = GetSessionSubsystem();
	if (SessionSubsystem == nullptr)
		return;

	SetStatus(MakeProgressMessage(SessionSubsystem->GetCurrentOp(), ESKSessionOp::Hosting));

	//# 진행 중인 검색을 먼저 끊는다 — 트래블 중 검색이 살아 있으면 안 된다.
	//# 취소 완료는 OnFindCancelled 로 돌아와 HandleFindCancelled 가 전환을 잇는다.
	if (USKSessionOpRules::ShouldPreemptFindForHost(SessionSubsystem->GetCurrentOp()))
	{
		SessionSubsystem->CancelFindForHost();
		return;
	}

	BeginHostTravel();
}

void USpySessionBrowserWidget::HandleFindCancelled()
{
	BeginHostTravel();
}

void USpySessionBrowserWidget::BeginHostTravel()
{
	CloseForTravel();

	//# 세션은 여기서 만들지 않는다 — 리슨 서버가 뜬 뒤(도착 후) SpyLoadingSubsystem 이 만든다.
	//# CreateSession 이 그 시점의 NetDriver 포트를 세션 정보에 박기 때문이다(포트 0 조인 실패 방지).
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USpyLoadingSubsystem* LoadingSubsystem = GameInstance->GetSubsystem<USpyLoadingSubsystem>())
		{
			LoadingSubsystem->HostAndEnter();
		}
	}
}

void USpySessionBrowserWidget::HandleRowClicked(int32 SearchResultIndex)
{
	USKOnlineSessionSubsystem* SessionSubsystem = GetSessionSubsystem();
	if (SessionSubsystem == nullptr)
		return;

	SetStatus(MakeProgressMessage(SessionSubsystem->GetCurrentOp(), ESKSessionOp::Joining));

	SessionSubsystem->JoinSessionByIndex(SearchResultIndex);
}

void USpySessionBrowserWidget::HandleHostReady()
{
	//# 세션 생성은 이제 도착 후에 일어나므로 이 통지는 브라우저가 닫힌 뒤에 온다 — 기록만 남긴다.
	//# 구독을 유지하는 이유는 방 목록으로 되돌아온 상태에서 생성이 완료되는 경우를 놓치지 않기 위해서다.
	UE_LOG(LogTemp, Log, TEXT("# [SpySessionBrowserWidget] 방 생성 완료 통지 수신"));
}

void USpySessionBrowserWidget::HandleJoinReady(const FString& ConnectString)
{
	CloseForTravel();

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USpyLoadingSubsystem* LoadingSubsystem = GameInstance->GetSubsystem<USpyLoadingSubsystem>())
		{
			LoadingSubsystem->EnterGameplay(ConnectString);
		}
	}
}

void USpySessionBrowserWidget::HandleSessionError(ESKSessionOp Op, ESKSessionError Error, const FString& Detail)
{
	//# Busy 는 중복 입력 가드가 정상 동작한 것이다 — Error 로 찍으면 사용자가 오류로 오해한다
	if (Error == ESKSessionError::Busy)
	{
		UE_LOG(LogTemp, Verbose, TEXT("# [SpySessionBrowserWidget] 진행 중인 작업에 막힌 요청: %s"), *Detail);
		return;
	}

	//# 원문 사유는 로그로만 남기고 화면에는 고정 한국어를 쓴다
	UE_LOG(LogTemp, Error, TEXT("# [SpySessionBrowserWidget] 세션 오류: %s"), *Detail);

	//# 무음 사유(None)는 진행 문구를 지우지 않는다 (기획서 §5-2-1)
	const FString Message = MakeStatusMessage(Op, Error);
	if (Message.IsEmpty())
		return;

	SetStatus(Message);
}

void USpySessionBrowserWidget::CloseForTravel()
{
	//# 트래블로 월드와 함께 사라지는 데 기대지 않고 명시적으로 닫는다(전환 전 잔상·오클릭 방지)
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (APlayerController* PC = GameInstance->GetFirstLocalPlayerController())
		{
			PC->bShowMouseCursor = false;
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
		}
	}

	Close();
}
