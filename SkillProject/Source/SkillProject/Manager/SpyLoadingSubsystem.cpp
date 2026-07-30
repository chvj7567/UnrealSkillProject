// Fill out your copyright notice in the Description page of Project Settings.

#include "Manager/SpyLoadingSubsystem.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "SKAssetManager.h"
#include "SKOnlineSessionSubsystem.h"
#include "SKAssetData.h"
#include "Data/SpyLoadingConfig.h"
#include "Data/SpyAssetNames.h"
#include "Manager/SpyAssetManager.h"
#include "Manager/SpyUIManager.h"
#include "Util/DefineEnum.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyLoadingSubsystem)

bool USpyLoadingSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	//# 데디케이티드 서버는 ServerDefaultMap 으로 직행하므로 로딩 서브시스템이 필요 없다
	if (IsRunningDedicatedServer())
	{
		return false;
	}

	return Super::ShouldCreateSubsystem(Outer);
}

float USpyLoadingSubsystem::CombineProgress(int32 Loaded, int32 Total, float MapPercent, float Weight)
{
	//# 1단계 대상이 0개면 완료로 본다
	const float Phase1Ratio = (Total > 0) ? ((float)Loaded / (float)Total) : 1.f;

	//# GetAsyncLoadPercentage 는 미시작 시 -1 을 반환한다 — 반드시 0 으로 바닥을 잡는다
	const float Phase2Ratio = FMath::Max(MapPercent, 0.f) / 100.f;

	const float ClampedWeight = FMath::Clamp(Weight, 0.f, 1.f);

	return FMath::Clamp(Phase1Ratio * ClampedWeight + Phase2Ratio * (1.f - ClampedWeight), 0.f, 1.f);
}

float USpyLoadingSubsystem::ClampDisplayed(float Raw, float Elapsed, float MinDisplaySeconds)
{
	const float ClampedRaw = FMath::Clamp(Raw, 0.f, 1.f);

	//# 최소 표시 시간이 없으면 Raw 를 그대로 보여준다
	if (MinDisplaySeconds <= 0.f)
	{
		return ClampedRaw;
	}

	return FMath::Min(ClampedRaw, Elapsed / MinDisplaySeconds);
}

bool USpyLoadingSubsystem::ShouldTransition(float Raw, float Elapsed, float MinDisplaySeconds)
{
	return (Raw >= 1.f) && (Elapsed >= MinDisplaySeconds);
}

bool USpyLoadingSubsystem::ShouldConnectToServer(const FString& ServerAddress)
{
	return ServerAddress.IsEmpty() == false;
}

bool USpyLoadingSubsystem::IsLoadingMapName(FName LoadedMapName, FName LoadingMapName)
{
	return LoadedMapName == LoadingMapName;
}

bool USpyLoadingSubsystem::HasConnectTimedOut(float ConnectElapsed, float TimeoutSeconds)
{
	if (TimeoutSeconds <= 0.f)
	{
		return false;
	}

	return ConnectElapsed >= TimeoutSeconds;
}

float USpyLoadingSubsystem::ConnectPhaseDisplayed(float PreloadWeight, float ConnectElapsed, float ConnectPacingSeconds)
{
	//# 도착 전에는 1.0 에 닿지 않게 남은 구간의 95% 까지만 채운다
	const float ConnectDisplayCap = 0.95f;
	const float Remaining = FMath::Clamp(1.f - PreloadWeight, 0.f, 1.f);

	//# 페이싱이 0 이하이면 즉시 상한
	const float Fraction = (ConnectPacingSeconds > 0.f)
		? FMath::Clamp(ConnectElapsed / ConnectPacingSeconds, 0.f, 1.f)
		: 1.f;

	return FMath::Clamp(PreloadWeight, 0.f, 1.f) + Remaining * Fraction * ConnectDisplayCap;
}

FString USpyLoadingSubsystem::ResolveTravelAddress(const FString& OverrideAddress, const FString& ConfigAddress)
{
	//# 방 목록에서 조인한 주소가 항상 우선한다
	if (OverrideAddress.IsEmpty() == false)
		return OverrideAddress;

	return ConfigAddress;
}

FString USpyLoadingSubsystem::MakeListenTravelURL(const FString& InMapPackageName)
{
	if (InMapPackageName.IsEmpty())
		return FString();

	//# 이미 리슨 옵션이 붙어 있으면 그대로 둔다
	if (InMapPackageName.Contains(TEXT("?listen")))
		return InMapPackageName;

	return InMapPackageName + TEXT("?listen");
}

bool USpyLoadingSubsystem::ShouldShowSessionBrowser(const FString& ConfigAddress)
{
	//# 주소가 지정돼 있으면 그쪽이 우선한다(데디서버 붙이기·CI 용 경로 보존)
	return ConfigAddress.IsEmpty();
}

FName USpyLoadingSubsystem::GetGameplayMapPackageName(const USpyLoadingConfig* InConfig)
{
	if (InConfig == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("# [SpyLoadingSubsystem] LoadingConfig 를 찾을 수 없습니다 — 맵 전환을 중단합니다"));
		return NAME_None;
	}

	if (InConfig->GameplayMap.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("# [SpyLoadingSubsystem] LoadingConfig 의 GameplayMap 이 비어 있습니다 — 맵 전환을 중단합니다"));
		return NAME_None;
	}

	return FName(*InConfig->GameplayMap.ToSoftObjectPath().GetLongPackageName());
}

FName USpyLoadingSubsystem::ResolveGameplayMapPackageName()
{
	//# 하드코딩 맵으로 폴백하지 않는다 — 무효하면 NAME_None 을 돌려 호출부가 중단하게 한다
	const USpyLoadingConfig* Config = USpyAssetManager::GetAssetByName<USpyLoadingConfig>(SpyAssetNames::LoadingConfig);
	return GetGameplayMapPackageName(Config);
}

bool USpyLoadingSubsystem::ApplyConfig(const USpyLoadingConfig* InConfig)
{
	const FName ResolvedMapName = GetGameplayMapPackageName(InConfig);
	if (ResolvedMapName.IsNone())
		return false;

	LoadingConfig = InConfig;
	MapPackageName = ResolvedMapName;

	//# 접속 설정 복사
	ServerAddress = InConfig->ServerAddress;
	ConnectTimeoutSeconds = InConfig->ConnectTimeoutSeconds;

	return true;
}

void USpyLoadingSubsystem::StartLoading()
{
	if (bLoading)
	{
		return;
	}

	const USpyLoadingConfig* Config = USpyAssetManager::GetAssetByName<USpyLoadingConfig>(SpyAssetNames::LoadingConfig);
	if (ApplyConfig(Config) == false)
	{
		//# Config 이상 — 임의 맵 이름으로 fallback 하지 않는다 (하드코딩 금지)
		return;
	}

	//# 데디 서버 트래블은 ASpyLoadingGameMode::BeginPlay 가 담당한다 — 이 서브시스템은 데디에서
	//# ShouldCreateSubsystem 으로 생성이 막히므로 여기에 그 분기를 두면 도달할 수 없다.

	//# 1단계 대상 = 이름 맵에 등록된 secondary 에셋 전체
	TArray<FSoftObjectPath> AssetPaths;
	USKAssetManager::Get().GetAssetData().GetAllAssetPaths(AssetPaths);

	AssetTotalCount = AssetPaths.Num();
	AssetLoadedCount = 0;
	ElapsedSeconds = 0.f;
	DisplayedProgress = 0.f;
	bAssetPhaseComplete = false;
	bMapLoadComplete = false;
	bTransitionStarted = false;
	bConnecting = false;
	bReadyToEnter = false;
	bMapPhase = false;

	//# 도착 판정용 — 현재(로딩맵) 월드 이름을 PIE 접두어 제거해 저장
	if (const UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
	{
		LoadingMapName = FName(*UWorld::RemovePIEPrefix(World->GetName()));
	}

	//# 로딩 UI 를 먼저 띄운다 — persistent 경로라 트래블을 넘어 유지되고, 동기 로드라 즉시 표시된다.
	//# 데디케이티드 서버는 이 서브시스템이 생성되지 않으므로 여기 도달하지 않는다.
	if (USpyUIManager* UIManager = USpyUIManager::Get(GetGameInstance()))
	{
		UIManager->OpenPersistentSpyUI(ESpyUIType::Loading);
	}

	//# 트래블 완료를 받아 UI 를 내린다
	if (PostLoadMapHandle.IsValid() == false)
	{
		PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &USpyLoadingSubsystem::HandlePostLoadMap);
	}

	//# 콜백이 동기 실행될 수 있으므로 마지막에 켠다
	bLoading = true;

	UE_LOG(LogTemp, Log, TEXT("# [SpyLoadingSubsystem] 로딩 시작 — 1단계 대상 %d 개, 대상 맵 %s"), AssetTotalCount, *MapPackageName.ToString());

	USKAssetManager::Get().LoadAssetsAsync(
		AssetPaths,
		FSKAssetBatchProgressDelegate::CreateUObject(this, &USpyLoadingSubsystem::HandleAssetProgress),
		FSimpleDelegate::CreateUObject(this, &USpyLoadingSubsystem::HandleAssetPhaseComplete));
}

void USpyLoadingSubsystem::HandleAssetProgress(int32 Loaded, int32 Total)
{
	AssetLoadedCount = Loaded;
	AssetTotalCount = Total;
}

void USpyLoadingSubsystem::HandleAssetPhaseComplete()
{
	//# 자동 phase 2 를 돌리지 않는다. 오프라인 맵 로드는 "접속" 버튼(EnterGameplay) 뒤로 미룬다.
	//# 접속 모드도 로컬 스트리밍을 하지 않고 ClientTravel 로 서버에서 받는다.
	bAssetPhaseComplete = true;
}

void USpyLoadingSubsystem::HandleMapPackageLoaded(const FName& PackageName, UPackage* Package, EAsyncLoadingResult::Type Result)
{
	if (Result != EAsyncLoadingResult::Succeeded)
	{
		UE_LOG(LogTemp, Error, TEXT("# [SpyLoadingSubsystem] 맵 패키지 비동기 로드 실패: %s"), *PackageName.ToString());
	}

	//# 성공/실패 무관하게 2단계는 종료로 처리한다 (진행률이 멈추면 안 된다)
	bMapLoadComplete = true;
}

float USpyLoadingSubsystem::GetMapPercent() const
{
	//# 2단계 완료 — GetAsyncLoadPercentage 는 완료 후에도 -1 을 반환하므로 플래그로 판정한다
	if (bMapLoadComplete)
	{
		return 100.f;
	}

	//# 1단계가 끝나기 전에는 2단계가 시작되지 않았으므로 0
	if (bAssetPhaseComplete == false)
	{
		return 0.f;
	}

	return GetAsyncLoadPercentage(MapPackageName);
}

void USpyLoadingSubsystem::Tick(float DeltaTime)
{
	if (bLoading == false)
	{
		return;
	}

	if (LoadingConfig == nullptr)
	{
		bLoading = false;
		return;
	}

	ElapsedSeconds += DeltaTime;

	//# ── phase 2: 맵 로딩바 (접속 버튼 입력 후) ──
	if (bMapPhase)
	{
		const float MapRatio = FMath::Max(GetMapPercent(), 0.f) / 100.f;
		const float NewDisplayed = ClampDisplayed(MapRatio, ElapsedSeconds, LoadingConfig->MinDisplaySeconds);
		if (NewDisplayed > DisplayedProgress)
		{
			DisplayedProgress = NewDisplayed;
			OnProgressChanged.Broadcast(DisplayedProgress);
		}

		//# 맵 로드 완료 + 최소 표시 시간 → 전환(OpenLevel)
		if (bMapLoadComplete && ElapsedSeconds >= LoadingConfig->MinDisplaySeconds && bTransitionStarted == false)
		{
			TransitionToGameplayMap();
		}
		return;
	}

	//# ── phase 1: 에셋 로딩바 ──
	const float AssetRatio = (AssetTotalCount > 0) ? ((float)AssetLoadedCount / (float)AssetTotalCount) : 1.f;
	const float NewDisplayed = ClampDisplayed(AssetRatio, ElapsedSeconds, LoadingConfig->MinDisplaySeconds);
	if (NewDisplayed > DisplayedProgress)
	{
		DisplayedProgress = NewDisplayed;
		OnProgressChanged.Broadcast(DisplayedProgress);
	}

	//# 에셋 완료 + 최소 표시 시간 → "접속" 버튼 대기(자동 전환 안 함)
	if (bReadyToEnter == false && bAssetPhaseComplete && ElapsedSeconds >= LoadingConfig->MinDisplaySeconds)
	{
		bReadyToEnter = true;
		bLoading = false;

		//# phase 1 바를 100% 로 확정
		if (DisplayedProgress < 1.f)
		{
			DisplayedProgress = 1.f;
			OnProgressChanged.Broadcast(DisplayedProgress);
		}

		OnReadyToEnter.Broadcast();
		UE_LOG(LogTemp, Log, TEXT("# [SpyLoadingSubsystem] 에셋 로딩 완료 — 접속 버튼 대기"));
	}
}

void USpyLoadingSubsystem::EnterGameplay(const FString& OverrideAddress)
{
	//# 에셋 로딩이 끝나 버튼이 떠 있을 때만, 중복 없이
	if (bReadyToEnter == false || bMapPhase)
		return;

	//# 조인 경로면 해석된 접속 문자열을 기억해 둔다(전환 시 config 보다 우선)
	PendingOverrideAddress = OverrideAddress;

	UE_LOG(LogTemp, Log, TEXT("# [SpyLoadingSubsystem] 접속 개시 — 맵 로딩 시작 (override=%s)"),
		OverrideAddress.IsEmpty() ? TEXT("<none>") : *OverrideAddress);

	//# phase 2 진입 — 바를 0 으로 리셋해 맵 로딩바가 새로 차오르게 한다
	bReadyToEnter = false;
	bMapPhase = true;
	ElapsedSeconds = 0.f;
	DisplayedProgress = 0.f;
	OnProgressChanged.Broadcast(DisplayedProgress);

	//# 맵 패키지 비동기 로드 시작
	bMapLoadComplete = false;
	const int32 RequestId = LoadPackageAsync(
		MapPackageName.ToString(),
		FLoadPackageAsyncDelegate::CreateUObject(this, &USpyLoadingSubsystem::HandleMapPackageLoaded));
	if (RequestId == INDEX_NONE)
	{
		UE_LOG(LogTemp, Error, TEXT("# [SpyLoadingSubsystem] 맵 패키지 로드 요청 실패: %s"), *MapPackageName.ToString());
		bMapLoadComplete = true;
	}

	//# 틱 재개 — phase 2 바 구동
	bLoading = true;
}

void USpyLoadingSubsystem::HostAndEnter()
{
	if (bReadyToEnter == false || bMapPhase)
		return;

	//# 리슨 서버 플래그를 세운 뒤 공용 전환 경로를 탄다
	bHostingListenServer = true;

	UE_LOG(LogTemp, Log, TEXT("# [SpyLoadingSubsystem] 방 만들기 — 리슨 서버 전환 개시"));
	EnterGameplay(TEXT(""));
}

void USpyLoadingSubsystem::RestoreAfterAbortedTransition()
{
	//# 중단 지점에서 상태가 굳으면 어떤 입력으로도 복구되지 않는다 — 버튼/목록 대기 상태로 되돌린다
	bHostingListenServer = false;
	bMapPhase = false;
	bTransitionStarted = false;
	bReadyToEnter = true;
}

void USpyLoadingSubsystem::TransitionToGameplayMap()
{
	bTransitionStarted = true;

	//# 로딩 단계 틱 종료 — 이후는 접속(타이머) 또는 오프라인 OpenLevel 이 담당
	bLoading = false;

	//# 리슨 서버 호스팅 — 자기 월드를 서버로 연다. 로딩맵은 NM_Standalone(권한 보유)이라 ServerTravel 이 성립한다.
	if (bHostingListenServer)
	{
		const FString ListenURL = MakeListenTravelURL(MapPackageName.ToString());
		if (ListenURL.IsEmpty())
		{
			UE_LOG(LogTemp, Error, TEXT("# [SpyLoadingSubsystem] 리슨 트래블 URL 이 비어 있습니다 — 전환 중단"));
			RestoreAfterAbortedTransition();
			return;
		}

		UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
		if (World == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("# [SpyLoadingSubsystem] 월드가 없습니다 — 리슨 전환 중단"));
			RestoreAfterAbortedTransition();
			return;
		}

		if (DisplayedProgress < 1.f)
		{
			DisplayedProgress = 1.f;
			OnProgressChanged.Broadcast(DisplayedProgress);
		}

		UE_LOG(LogTemp, Log, TEXT("# [SpyLoadingSubsystem] 리슨 서버 전환: %s"), *ListenURL);
		World->ServerTravel(ListenURL);
		return;
	}

	//# 접속 모드 — override(조인) 또는 config 주소로 ClientTravel. 클라는 자기 월드를 열지 않고 서버에 합류한다.
	const FString TravelAddress = ResolveTravelAddress(PendingOverrideAddress, ServerAddress);
	if (ShouldConnectToServer(TravelAddress))
	{
		APlayerController* PC = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController() : nullptr;
		if (PC == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("# [SpyLoadingSubsystem] 접속할 PlayerController 가 없습니다 — 접속 실패 처리"));
			HandleConnectFailed(TEXT("No local PlayerController"));
			return;
		}

		bConnecting = true;
		ConnectStartTime = FPlatformTime::Seconds();
		StartConnectWatch();

		UE_LOG(LogTemp, Log, TEXT("# [SpyLoadingSubsystem] 서버 접속 개시: %s"), *TravelAddress);
		PC->ClientTravel(TravelAddress, ETravelType::TRAVEL_Absolute);
		return;
	}

	//# 오프라인 폴백 — NM_Standalone(단일 authority)에서만 OpenLevel. 네트워크 클라는 절대 여기서 열지 않는다.
	const UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (World != nullptr && World->GetNetMode() == NM_Standalone)
	{
		//# 마지막 프레임에 100% 를 확실히 보여준다
		if (DisplayedProgress < 1.f)
		{
			DisplayedProgress = 1.f;
			OnProgressChanged.Broadcast(DisplayedProgress);
		}

		UE_LOG(LogTemp, Log, TEXT("# [SpyLoadingSubsystem] 오프라인 전환(Standalone): %s"), *MapPackageName.ToString());
		UGameplayStatics::OpenLevel(GetGameInstance(), MapPackageName);
		return;
	}

	//# ServerAddress 비어 있는데 네트워크 클라 — OpenLevel 하면 서버와 끊긴다. 금지.
	UE_LOG(LogTemp, Error, TEXT("# [SpyLoadingSubsystem] ServerAddress 미설정 + 비-Standalone — 전환을 중단합니다(클라 OpenLevel 금지)"));
}

void USpyLoadingSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	//# 아직 전환을 시작하지 않았으면 로딩맵 자신의 로드다 — 무시
	if (bTransitionStarted == false)
	{
		return;
	}

	//# 로드된 월드가 로딩맵이면(접속 중 재브라우즈 등) 도착이 아니다 — 무시
	if (LoadedWorld != nullptr)
	{
		const FName LoadedName = FName(*UWorld::RemovePIEPrefix(LoadedWorld->GetName()));
		if (IsLoadingMapName(LoadedName, LoadingMapName))
		{
			return;
		}
	}

	//# 도착 — 접속 감시 종료, 100% 반영, UI 종료
	StopConnectWatch();
	bConnecting = false;

	//# 리셋보다 먼저 잡아둔다 — 이번 도착이 호스팅이었는지 판단할 유일한 근거다
	const bool bArrivedAsListenHost = bHostingListenServer;

	if (DisplayedProgress < 1.f)
	{
		DisplayedProgress = 1.f;
		OnProgressChanged.Broadcast(DisplayedProgress);
	}

	if (USpyUIManager* UIManager = USpyUIManager::Get(GetGameInstance()))
	{
		UIManager->ClosePersistentSpyUI(ESpyUIType::Loading);
	}

	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}

	UE_LOG(LogTemp, Log, TEXT("# [SpyLoadingSubsystem] 도착 — 로딩 UI 종료"));

	//# 호스팅 도착이면 여기서 세션을 만든다(리셋보다 먼저 트리거한다)
	if (bArrivedAsListenHost)
	{
		ScheduleHostSessionAfterArrival();
	}

	//# 이번 전환은 끝났다 — 다음 전환이 잔여 상태를 물려받지 않게 되돌린다
	bHostingListenServer = false;
	PendingOverrideAddress.Reset();
}

void USpyLoadingSubsystem::ScheduleHostSessionAfterArrival()
{
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance == nullptr)
		return;

	//# 엔진 LoadMap 스택(PostLoadMapWithWorld) 안에서 OSS 작업을 재진입시키지 않도록 한 틱 미룬다.
	//# 리슨 NetDriver 는 이 시점 이미 준비돼 있다(LoadMap 이 Listen 을 먼저 수행한다).
	GameInstance->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		UGameInstance* OwningGameInstance = GetGameInstance();
		if (OwningGameInstance == nullptr)
			return;

		USKOnlineSessionSubsystem* SessionSubsystem = OwningGameInstance->GetSubsystem<USKOnlineSessionSubsystem>();
		if (SessionSubsystem == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("# [SpyLoadingSubsystem] 세션 서브시스템이 없어 방을 만들지 못했습니다"));
			return;
		}

		//# 여기서 생성이 실패해도 되돌리지 않는다 — 이미 인게임이고 브라우저가 없다.
		//# 귀결은 "방이 목록에 뜨지 않을 뿐 플레이는 가능" 이며, 실패는 SKOnline 이 Error 로 남긴다.
		UE_LOG(LogTemp, Log, TEXT("# [SpyLoadingSubsystem] 리슨 서버 도착 — 세션 생성 요청"));
		SessionSubsystem->HostSession();
	}));
}

void USpyLoadingSubsystem::StartConnectWatch()
{
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance == nullptr)
	{
		return;
	}

	bConnectFailed = false;

	//# 엔진 실패 델리게이트 구독(중복 방지)
	if (GEngine != nullptr)
	{
		if (NetworkFailureHandle.IsValid() == false)
		{
			NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &USpyLoadingSubsystem::HandleNetworkFailure);
		}
		if (TravelFailureHandle.IsValid() == false)
		{
			TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &USpyLoadingSubsystem::HandleTravelFailure);
		}
	}

	//# GameInstance 타이머 — 월드가 바뀌어도 살아남는다
	GameInstance->GetTimerManager().SetTimer(
		ConnectWatchTimer, this, &USpyLoadingSubsystem::TickConnectWatch, 0.05f, true);
}

void USpyLoadingSubsystem::StopConnectWatch()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		GameInstance->GetTimerManager().ClearTimer(ConnectWatchTimer);
	}

	if (GEngine != nullptr)
	{
		if (NetworkFailureHandle.IsValid())
		{
			GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
			NetworkFailureHandle.Reset();
		}
		if (TravelFailureHandle.IsValid())
		{
			GEngine->OnTravelFailure().Remove(TravelFailureHandle);
			TravelFailureHandle.Reset();
		}
	}
}

void USpyLoadingSubsystem::TickConnectWatch()
{
	if (bConnecting == false || bConnectFailed)
	{
		return;
	}

	const float ConnectElapsed = (float)(FPlatformTime::Seconds() - ConnectStartTime);

	//# 접속 단계 표시 진행률 — 프리로드 지점에서 도착 전까지 서서히(1.0 미만) 차오른다
	const float PreloadWeight = LoadingConfig ? LoadingConfig->AssetPhaseWeight : 0.9f;
	const float NewDisplayed = ConnectPhaseDisplayed(PreloadWeight, ConnectElapsed, ConnectTimeoutSeconds);
	if (NewDisplayed > DisplayedProgress)
	{
		DisplayedProgress = NewDisplayed;
		OnProgressChanged.Broadcast(DisplayedProgress);
	}

	//# 타임아웃 — 도착 없이 상한 시간 초과
	if (HasConnectTimedOut(ConnectElapsed, ConnectTimeoutSeconds))
	{
		HandleConnectFailed(TEXT("Connect timed out"));
	}
}

void USpyLoadingSubsystem::HandleConnectFailed(const FString& Reason)
{
	//# 중복 실패 콜백 무시(타임아웃 + 네트워크 실패가 겹칠 수 있다)
	if (bConnectFailed)
	{
		return;
	}

	bConnectFailed = true;
	bConnecting = false;

	StopConnectWatch();

	//# 방 목록 경로는 재시도 버튼이 아니라 목록 복귀로 회복하므로, 다시 조인할 수 있게 상태를 되돌린다
	if (ShouldShowSessionBrowser(ServerAddress))
	{
		bMapPhase = false;
		bTransitionStarted = false;
		bHostingListenServer = false;
		PendingOverrideAddress.Reset();
		bReadyToEnter = true;
	}

	UE_LOG(LogTemp, Error, TEXT("# [SpyLoadingSubsystem] 접속 실패: %s"), *Reason);
	OnConnectionFailed.Broadcast(Reason);
}

void USpyLoadingSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	if (bConnecting == false)
	{
		return;
	}

	HandleConnectFailed(FString::Printf(TEXT("NetworkFailure: %s"), *ErrorString));
}

void USpyLoadingSubsystem::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	if (bConnecting == false)
	{
		return;
	}

	HandleConnectFailed(FString::Printf(TEXT("TravelFailure: %s"), *ErrorString));
}

void USpyLoadingSubsystem::RetryConnect()
{
	//# 조인 주소가 있으면 그쪽으로 재시도한다 — config 주소로 가면 엉뚱한 서버에 붙는다
	const FString TravelAddress = ResolveTravelAddress(PendingOverrideAddress, ServerAddress);
	if (ShouldConnectToServer(TravelAddress) == false)
	{
		UE_LOG(LogTemp, Error, TEXT("# [SpyLoadingSubsystem] RetryConnect — 접속 주소가 없습니다"));
		return;
	}

	APlayerController* PC = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController() : nullptr;
	if (PC == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("# [SpyLoadingSubsystem] RetryConnect — PlayerController 가 없습니다"));
		return;
	}

	//# 진행률을 프리로드 지점으로 되돌린다 — 실패 시 0.995 에 동결됐으므로,
	//# 리셋하지 않으면 단조 가드(NewDisplayed > DisplayedProgress)가 creep 을 막아 바가 100% 로 남는다.
	const float PreloadWeight = LoadingConfig ? LoadingConfig->AssetPhaseWeight : 0.9f;
	DisplayedProgress = PreloadWeight;
	OnProgressChanged.Broadcast(DisplayedProgress);

	bConnectFailed = false;
	bConnecting = true;
	ConnectStartTime = FPlatformTime::Seconds();
	StartConnectWatch();

	UE_LOG(LogTemp, Log, TEXT("# [SpyLoadingSubsystem] 접속 재시도: %s"), *TravelAddress);
	PC->ClientTravel(TravelAddress, ETravelType::TRAVEL_Absolute);
}

void USpyLoadingSubsystem::Deinitialize()
{
	StopConnectWatch();

	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}

	bLoading = false;
	bConnecting = false;
	bReadyToEnter = false;
	bMapPhase = false;
	bHostingListenServer = false;
	PendingOverrideAddress.Reset();
	OnProgressChanged.Clear();
	OnConnectionFailed.Clear();
	OnReadyToEnter.Clear();
	LoadingConfig = nullptr;

	Super::Deinitialize();
}
