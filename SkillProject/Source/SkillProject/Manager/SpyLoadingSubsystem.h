// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/EngineTypes.h"

#include "SpyLoadingSubsystem.generated.h"

class USpyLoadingConfig;
class UNetDriver;

//# 표시용 진행률(0~1) 변경 알림 — 위젯이 구독한다
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLoadingProgressChanged, float);

//# 접속 실패 알림(사유 문자열) — 위젯이 에러 UI 를 띄운다
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLoadingConnectFailed, const FString&);

//# 로딩 완료(전환 준비) 알림 — 위젯이 "접속" 버튼을 띄운다. 자동 전환하지 않는다(데모).
DECLARE_MULTICAST_DELEGATE(FOnLoadingReadyToEnter);

//# 로딩 씬 파이프라인 소유주. 위젯을 알지 못하고 델리게이트만 브로드캐스트한다.
UCLASS()
class SKILLPROJECT_API USpyLoadingSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	//# 데디케이티드 서버는 로딩 화면을 거치지 않으므로 생성 자체를 막는다
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Deinitialize() override;

public:
	//# FTickableGameObject — 로딩 중일 때만 틱한다(전환 후에는 GameInstance 에 남아도 유휴 상태)
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override
	{
		return bLoading;
	}
	virtual bool IsTickableInEditor() const override
	{
		return false;
	}
	virtual bool IsTickableWhenPaused() const override
	{
		return true;
	}
	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(USpyLoadingSubsystem, STATGROUP_Tickables);
	}

public:
	//# 로딩 파이프라인 시작 — 1단계(에셋 프리로드) → 2단계(맵 스트리밍) → 전환
	void StartLoading();

	//# 접속 실패 후 재시도 — 위젯 버튼이 호출
	void RetryConnect();

	//# 게임플레이 전환 개시. OverrideAddress 가 비어 있으면 기존 config 경로(자동 접속/오프라인 폴백)를 탄다.
	//# 방 목록에서 조인하면 해석된 접속 문자열이 여기로 들어온다.
	void EnterGameplay(const FString& OverrideAddress = TEXT(""));

	//# 방 만들기 — 맵 로드 후 리슨 서버로 자기 월드를 연다
	void HostAndEnter();

public:
	//# 진행률 합성 — Raw(0~1). MapPercent 는 -1(미시작)을 0 으로 바닥 잡는다
	static float CombineProgress(int32 Loaded, int32 Total, float MapPercent, float Weight);

	//# 표시용 진행률 — Raw 를 경과 시간으로 클램프. MinDisplaySeconds <= 0 이면 클램프 생략
	static float ClampDisplayed(float Raw, float Elapsed, float MinDisplaySeconds);

	//# 전환 조건 — Raw 가 1.0 이고 최소 표시 시간을 채웠을 때만 true
	static bool ShouldTransition(float Raw, float Elapsed, float MinDisplaySeconds);

	//# 서버 주소가 지정돼 있으면 접속 모드, 비어 있으면 오프라인 폴백
	static bool ShouldConnectToServer(const FString& ServerAddress);

	//# 도착 판정 보조 — 로드된 맵이 로딩맵과 같으면 true(도착 아님, 무시)
	static bool IsLoadingMapName(FName LoadedMapName, FName LoadingMapName);

	//# 접속 타임아웃 판정 — 경과가 타임아웃 이상이면 true. 타임아웃 0 이하이면 무제한(false)
	static bool HasConnectTimedOut(float ConnectElapsed, float TimeoutSeconds);

	//# 접속 단계 표시 진행률 — 프리로드 가중치에서 시작해 도착 전에는 1.0 에 못 닿는다(상한 0.95 비율)
	static float ConnectPhaseDisplayed(float PreloadWeight, float ConnectElapsed, float ConnectPacingSeconds);

	//# 트래블 주소 결정 — override(조인) 가 config(자동 접속) 보다 우선. 둘 다 비면 빈 문자열
	static FString ResolveTravelAddress(const FString& OverrideAddress, const FString& ConfigAddress);

	//# 리슨 서버 트래블 URL — 맵 패키지명에 ?listen 을 붙인다(중복 방지)
	static FString MakeListenTravelURL(const FString& InMapPackageName);

	//# 방 목록을 띄울지 — config 주소가 비어 있을 때만. 채워져 있으면 기존 자동 접속을 유지한다
	static bool ShouldShowSessionBrowser(const FString& ConfigAddress);

	//# config → 게임플레이 맵 롱 패키지명. 무효하면 NAME_None (호출부가 전환을 중단한다)
	static FName GetGameplayMapPackageName(const USpyLoadingConfig* InConfig);

	//# 에셋에서 LoadingConfig 를 읽어 맵 패키지명을 해석한다 — 서브시스템이 없는 데디 서버도 쓴다
	static FName ResolveGameplayMapPackageName();

public:
	//# Config 유효성 검사 + 내부 상태 반영. 무효하면 Error 로그 후 false
	bool ApplyConfig(const USpyLoadingConfig* InConfig);

	float GetDisplayedProgress() const
	{
		return DisplayedProgress;
	}

	//# 위젯이 분기에 쓰는 조회 — config 에 서버 주소가 지정돼 있는가
	bool HasConfiguredServerAddress() const
	{
		return ShouldShowSessionBrowser(ServerAddress) == false;
	}

public:
	FOnLoadingProgressChanged OnProgressChanged;
	FOnLoadingConnectFailed OnConnectionFailed;
	FOnLoadingReadyToEnter OnReadyToEnter;

protected:
	//# 1단계 진행률 콜백
	void HandleAssetProgress(int32 Loaded, int32 Total);

	//# 1단계 완료 → 2단계(맵 패키지) 비동기 로드 시작
	void HandleAssetPhaseComplete();

	//# 2단계 완료 콜백
	void HandleMapPackageLoaded(const FName& PackageName, UPackage* Package, EAsyncLoadingResult::Type Result);

	//# 게임플레이 맵으로 전환
	void TransitionToGameplayMap();

	//# 전환을 시작했다가 중단했을 때 — 다시 시도할 수 있게 진입 상태를 되돌린다
	void RestoreAfterAbortedTransition();

	//# 리슨 서버 도착 후 세션 생성 — CreateSession 이 NetDriver 포트를 세션에 박으므로 트래블 뒤에 만든다
	void ScheduleHostSessionAfterArrival();

	//# 트래블 완료 시점 — 로딩 UI 를 내린다
	void HandlePostLoadMap(UWorld* LoadedWorld);

	//# 접속 감시 시작(타이머) / 종료 / 접속 실패 처리
	void StartConnectWatch();
	void StopConnectWatch();
	void HandleConnectFailed(const FString& Reason);

	//# 접속 감시 타이머 콜백
	void TickConnectWatch();

	//# 엔진 네트워크/트래블 실패 어댑터
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);

	//# 현재 2단계 진행률(%) — 미시작 0, 완료 100
	float GetMapPercent() const;

protected:
	UPROPERTY(Transient)
	TObjectPtr<const USpyLoadingConfig> LoadingConfig;

	//# 전환 대상 맵의 롱 패키지명 (GetAsyncLoadPercentage / OpenLevel 공용)
	FName MapPackageName;

	float DisplayedProgress = 0.f;

	bool bLoading = false;
	bool bAssetPhaseComplete = false;
	bool bMapLoadComplete = false;
	bool bTransitionStarted = false;

	int32 AssetLoadedCount = 0;
	int32 AssetTotalCount = 0;

	float ElapsedSeconds = 0.f;

	//# Config 에서 복사한 접속 설정
	FString ServerAddress;
	float ConnectTimeoutSeconds = 0.f;

	//# 로딩을 시작한 맵(로딩맵)의 PIE 접두어 제거 이름 — 도착 판정에 쓴다
	FName LoadingMapName;

	//# 접속 진행 중 여부 — 전환(접속 개시) 후 도착/실패까지 true
	bool bConnecting = false;

	//# 로딩 완료(버튼 대기) 여부 — 자동 전환 대신 버튼을 기다린다
	bool bReadyToEnter = false;

	//# 맵 로딩 단계(phase 2) 활성 — "접속" 버튼 입력 후 DevMap 로드 바를 그린다
	bool bMapPhase = false;

	//# 이번 전환이 리슨 서버 호스팅인지 — true 면 ClientTravel 대신 ServerTravel(?listen)
	bool bHostingListenServer = false;

	//# 조인이 넘긴 접속 문자열. 비어 있으면 config ServerAddress 를 쓴다
	FString PendingOverrideAddress;

	//# 접속 개시 시각(월드 무관, FPlatformTime 기준)
	double ConnectStartTime = 0.0;

	//# 접속 감시 타이머(GameInstance 타이머 — 트래블 넘어 생존)
	FTimerHandle ConnectWatchTimer;

	//# 엔진 네트워크/트래블 실패 구독 핸들
	FDelegateHandle NetworkFailureHandle;
	FDelegateHandle TravelFailureHandle;

	//# 접속 실패 상태(재시도 대기) — 타임아웃/실패 콜백 중복 방지
	bool bConnectFailed = false;

	//# PostLoadMapWithWorld 구독 핸들
	FDelegateHandle PostLoadMapHandle;
};
