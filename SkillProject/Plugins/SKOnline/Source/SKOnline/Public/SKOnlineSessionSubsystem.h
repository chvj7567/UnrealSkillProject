// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "SKOnlineTypes.h"

#include "SKOnlineSessionSubsystem.generated.h"

class FOnlineSessionSearch;

//# ⚠ OSS Null 은 완료 콜백을 명령 호출 안에서 동기 발화한다 — HostSession() 이 반환되기 전에 broadcast 될 수 있다.
//# 소비자는 반드시 명령을 호출하기 *전에* 구독할 것. 호출 후 구독하면 영영 못 받는다.

//# 방 목록 갱신 — 검색 성공 시(결과 0건 포함) 브로드캐스트
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSKSessionsFound, const TArray<FSKSessionInfo>&);

//# 방 생성 완료 — 소비자가 리슨 서버로 트래블한다(플러그인은 트래블하지 않는다)
DECLARE_MULTICAST_DELEGATE(FOnSKHostReady);

//# 조인 완료 — 해석된 접속 문자열을 넘긴다. 소비자가 ClientTravel 한다
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSKJoinReady, const FString&);

//# 작업 실패 — 사용자 문구는 소비자가 정한다. Detail 은 로그용 원문
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnSKSessionError, ESKSessionOp, ESKSessionError, const FString&);

//# 검색 취소 완료 — 소비자가 막아 뒀던 호스팅 흐름을 이어간다
DECLARE_MULTICAST_DELEGATE(FOnSKFindCancelled);

//# OnlineSubsystem 세션 파이프라인. UI·트래블을 모르고 델리게이트만 브로드캐스트한다.
UCLASS()
class SKONLINE_API USKOnlineSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	//# 설정 기본값으로 즉시 방을 만든다(옵션 입력 화면 없음)
	void HostSession();

	//# 접속 가능한 방을 검색한다
	void FindSessions();

	//# 직전 검색 결과의 Index 번째 방에 조인한다
	void JoinSessionByIndex(int32 Index);

	//# 현재 세션을 파괴한다(조인 실패 복구·정리용)
	void DestroyCurrentSession();

	//# 진행 중인 검색을 취소한다. 끝나면 OnFindCancelled 로 통지해 소비자가 호스팅을 잇게 한다.
	void CancelFindForHost();

	//# 남아 있는 세션을 완료 대기 없이 파괴한다(시작·종료·방 목록 진입 청소용). op 상태를 점유하지 않는다.
	void DestroyLingeringSession(const TCHAR* Reason);

	ESKSessionOp GetCurrentOp() const
	{
		return CurrentOp;
	}

public:
	FOnSKSessionsFound OnSessionsFound;
	FOnSKHostReady OnHostReady;
	FOnSKJoinReady OnJoinReady;
	FOnSKSessionError OnSessionError;
	FOnSKFindCancelled OnFindCancelled;

protected:
	//# 세션 인터페이스 획득 — 없으면 NoOnlineSubsystem 으로 실패 통지 후 nullptr
	IOnlineSessionPtr GetSessionInterfaceChecked(ESKSessionOp Op);

	//# 작업 시작 가드 + 상태 전이. 거부되면 Busy 로 실패 통지 후 false
	bool BeginOp(ESKSessionOp RequestedOp);

	//# 작업 종료 — 상태를 None 으로 되돌린다
	void EndOp();

	//# 실패 통지 + 상태 복구를 한 번에
	void FailOp(ESKSessionOp Op, ESKSessionError Error, const FString& Detail);

	//# 호스트명 기반 방 이름 생성 (설정의 포맷 사용)
	FString BuildRoomName() const;

	//# 취소된 검색의 잔여 상태 정리 — 취소로 인한 실패는 사용자에게 통지하지 않는다
	void FinishPreemptedFind();

protected:
	//# OSS 콜백
	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleFindSessionsComplete(bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleCancelFindSessionsComplete(bool bWasSuccessful);

	//# 조인 직전 잔존 세션 파괴 완료 — 여기서 실제 조인을 잇는다(Steam 의 비동기 파괴 대응)
	void HandleDestroyBeforeJoinComplete(FName SessionName, bool bWasSuccessful);

	//# 등록한 델리게이트 핸들 해제
	void ClearDelegateHandles(IOnlineSessionPtr Sessions);

	//# 호스트 로컬 플레이어를 세션 인원에 등록한다 — 등록 없이는 남은 자리가 최대치로 광고된다
	void RegisterLocalPlayerInSession();

	//# 실제 조인 실행 — 잔존 세션 정리가 끝난 뒤에만 호출한다
	void JoinSessionInternal(int32 Index);

protected:
	//# 동시에 하나만 — USKSessionOpRules 가 판정한다
	ESKSessionOp CurrentOp = ESKSessionOp::None;

	//# 직전 검색 결과 — JoinSessionByIndex 가 인덱스로 되찾는다
	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	//# 취소 완료 후 OnFindCancelled 를 통지할지 — 취소 중 도착하는 검색 실패를 삼키는 판정도 겸한다
	bool bNotifyHostAfterCancelFind = false;

	//# 잔존 세션 파괴 완료 후 조인할 검색 결과 인덱스. INDEX_NONE 이면 대기 중인 조인이 없다
	int32 PendingJoinIndex = INDEX_NONE;

	FDelegateHandle CreateSessionCompleteHandle;
	FDelegateHandle FindSessionsCompleteHandle;
	FDelegateHandle JoinSessionCompleteHandle;
	FDelegateHandle DestroySessionCompleteHandle;
	FDelegateHandle CancelFindSessionsCompleteHandle;
	FDelegateHandle DestroyBeforeJoinCompleteHandle;
};
