// Fill out your copyright notice in the Description page of Project Settings.

#include "ManagerComponent/SpyNavigationComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "Data/SpyMissionConfig.h"
#include "Manager/SpyAssetManager.h"
#include "System/CommonInterface.System.h"
#include "System/SpyMissionComponent.h"
#include "System/SpyMissionTargetRegistrySubsystem.h"
#include "System/SpyNavPathMath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyNavigationComponent)

USpyNavigationComponent::USpyNavigationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USpyNavigationComponent::BeginPlay()
{
	Super::BeginPlay();

	const APawn* OwningPawn = Cast<APawn>(GetOwner());

	if (ShouldActivateForOwningPawn(OwningPawn) == false)
		return;

	PathSpline = NewObject<USplineComponent>(GetOwner(), TEXT("SpyNavigationPathSpline"));
	PathSpline->SetupAttachment(GetOwner()->GetRootComponent());
	PathSpline->RegisterComponent();
	PathSpline->SetVisibility(false);

	if (AutoDiscoverAndBindMissionComponent())
		return;

	//# 클라이언트에서는 이 시점에 Controller/PlayerState 가 아직 없을 수 있다 —
	//# SpyMainHUD::TryBindMissionComponent 와 동일한 재시도 타이머 패턴
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(BindRetryTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]() {
											  if (AutoDiscoverAndBindMissionComponent() == false)
												  return;

											  if (UWorld* InnerWorld = GetWorld())
												  InnerWorld->GetTimerManager().ClearTimer(BindRetryTimerHandle);
										  }),
										  0.2f, true);
	}
}

bool USpyNavigationComponent::ShouldActivateForOwningPawn(const APawn* InPawn)
{
	if (InPawn == nullptr)
		return false;

	//# IsLocallyControlled() 는 네트워크 role 기준이라 standalone/listen server 에서는 AI 봇도 true 다.
	//# Controller 타입으로 실제 플레이어 조종만 통과시킨다(IsPlayerControlled() 는 PlayerState 기준이라 부적합).
	if (InPawn->IsLocallyControlled() == false)
		return false;

	return Cast<APlayerController>(InPawn->GetController()) != nullptr;
}

void USpyNavigationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(BindRetryTimerHandle);

	StopPath();
	UnbindMissionComponent();

	Super::EndPlay(EndPlayReason);
}

bool USpyNavigationComponent::AutoDiscoverAndBindMissionComponent()
{
	if (BoundMissionComponent != nullptr)
		return true;

	const APawn* OwningPawn = Cast<APawn>(GetOwner());
	if (OwningPawn == nullptr)
		return false;

	AController* OwningController = OwningPawn->GetController();
	if (OwningController == nullptr)
		return false;

	APlayerState* OwningState = OwningController->PlayerState;
	if (OwningState == nullptr)
		return false;

	USpyMissionComponent* MissionComponent = USpyMissionComponent::FindMissionComponent(OwningState);
	if (MissionComponent == nullptr)
		return false;

	BindMissionComponent(MissionComponent);

	return true;
}

void USpyNavigationComponent::BindMissionComponent(USpyMissionComponent* InMissionComponent)
{
	if (InMissionComponent == nullptr || InMissionComponent == BoundMissionComponent)
		return;

	UnbindMissionComponent();

	BoundMissionComponent = InMissionComponent;
	BoundMissionComponent->OnMissionProgressChanged.AddDynamic(this, &USpyNavigationComponent::HandleMissionProgressChanged);

	//# design npc-mission-dialogue.md §6-2(a) 결함 B / mission-ground-navigation.md §5-8 —
	//# 구독 전 상태를 놓치지 않도록 바인드 직후 즉시 1회 현재 상태 기준으로 호출한다.
	HandleMissionProgressChanged(BoundMissionComponent, BoundMissionComponent->GetMissionIndex(),
								  BoundMissionComponent->GetCount(), BoundMissionComponent->GetTargetCount());
}

void USpyNavigationComponent::UnbindMissionComponent()
{
	if (BoundMissionComponent == nullptr)
		return;

	BoundMissionComponent->OnMissionProgressChanged.RemoveDynamic(this, &USpyNavigationComponent::HandleMissionProgressChanged);

	BoundMissionComponent = nullptr;

	//# 재바인드 시 새 컴포넌트의 초기 상태를 반드시 재계산하도록 가드를 리셋한다
	LastResolvedMissionIndex = INDEX_NONE;
	bLastResolvedAccepted = false;
}

void USpyNavigationComponent::HandleMissionProgressChanged(USpyMissionComponent* MissionComponent, int32 MissionIndex, int32 Count, int32 TargetCount)
{
	if (BoundMissionComponent == nullptr)
		return;

	const bool bAccepted = BoundMissionComponent->IsCurrentAccepted();

	//# design §5-8 과잉 재계산 가드 — Count 만 바뀌어도 이 델리게이트가 발화하므로
	//# (MissionIndex, bAccepted) 가 직전과 동일하면 레지스트리 재질의를 생략한다.
	if (MissionIndex == LastResolvedMissionIndex && bAccepted == bLastResolvedAccepted)
		return;

	LastResolvedMissionIndex = MissionIndex;
	bLastResolvedAccepted = bAccepted;

	const FSpyMissionRow* Entry = BoundMissionComponent->GetMissionEntry(MissionIndex);
	if (Entry == nullptr)
	{
		StopPath();

		return;
	}

	if (bAccepted == false)
	{
		//# design §5-2-1 — 미수락 상태는 MissionType 과 무관하게 담당 NPC 위치를 가리킨다
		bPendingIsDialogue = true;
		PendingNPCId = BoundMissionComponent->GetCurrentNPCId();
	}
	else
	{
		//# design §5-5 — 수락됨이면 MissionType 이 1차 게이트다 (NPCId 유무로 분기하지 않는다)
		bPendingIsDialogue = (Entry->MissionType == ESpyMissionType::Dialogue);
		PendingNPCId = Entry->NPCId;
	}

	PendingMatchTag = Entry->MatchTag;
	TargetRetryCount = 0;

	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(TargetRetryTimerHandle);

	TryResolveTarget();
}

void USpyNavigationComponent::StartPathTo(const FVector& InTargetLocation, AActor* InTargetActor)
{
	CurrentTargetLocation = InTargetLocation;
	bPathActive = true;

	//# 콜드 스타트를 "이전에 보이고 있었다"로 시드한다 — design §4-3 콜드 스타트 조항 참조.
	bPathVisible = true;

	BindHideTrigger(InTargetActor);

	RecomputePath();

	if (UWorld* World = GetWorld())
		World->GetTimerManager().SetTimer(RepathTimerHandle, this, &USpyNavigationComponent::RecomputePath, UpdateIntervalSeconds, true);
}

void USpyNavigationComponent::StopPath()
{
	bPathActive = false;
	CurrentTargetLocation = FVector::ZeroVector;

	UnbindHideTrigger();

	//# design §5-6 — 대기 중인 좌표 재시도 타이머도 함께 정리한다. 완료→수락이 같은 OnRep 안에서
	//# 연달아 일어날 때(§2-3), 낡은 재시도가 새 미션에 잘못된 좌표를 주입하는 경합을 막는다.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RepathTimerHandle);
		World->GetTimerManager().ClearTimer(TargetRetryTimerHandle);
	}

	if (PathSpline != nullptr)
		PathSpline->ClearSplinePoints(true);

	HideVisual();
}

USpyMissionTargetRegistrySubsystem* USpyNavigationComponent::GetMissionTargetRegistry()
{
	if (OverrideTargetRegistry != nullptr)
		return OverrideTargetRegistry;

	UWorld* World = GetWorld();
	if (World == nullptr)
		return nullptr;

	return World->GetSubsystem<USpyMissionTargetRegistrySubsystem>();
}

void USpyNavigationComponent::TryResolveTarget()
{
	//# design §5-6 — NoNPCId(sentinel) 는 재시도해도 채워지지 않는다. 재시도 없이 즉시 포기한다.
	if (bPendingIsDialogue && PendingNPCId == FSpyMissionRow::NoNPCId)
	{
		if (UWorld* World = GetWorld())
			World->GetTimerManager().ClearTimer(TargetRetryTimerHandle);

		return;
	}

	USpyMissionTargetRegistrySubsystem* Registry = GetMissionTargetRegistry();

	FVector TargetLocation = FVector::ZeroVector;
	bool bFound = false;

	if (Registry != nullptr)
	{
		//# design §5-5 — Dialogue 는 NPCId 키 공간, 그 외(Gameplay/Interact)는 MatchTag 키 공간(§5-4)
		bFound = bPendingIsDialogue
			? (PendingNPCId != FSpyMissionRow::NoNPCId && Registry->FindNPCLocation(PendingNPCId, TargetLocation))
			: (PendingMatchTag.IsValid() && Registry->FindMissionTargetLocation(PendingMatchTag, TargetLocation));
	}

	if (bFound)
	{
		if (UWorld* World = GetWorld())
			World->GetTimerManager().ClearTimer(TargetRetryTimerHandle);

		AActor* TargetActor = bPendingIsDialogue
			? Registry->FindNPCActor(PendingNPCId)
			: Registry->FindMissionTargetActor(PendingMatchTag);

		StartPathTo(TargetLocation, TargetActor);

		return;
	}

	++TargetRetryCount;

	if (TargetRetryCount == 1)
	{
		//# 최초 시도 실패 — 이후 0.2초 간격으로 재시도한다(design §5-6)
		if (UWorld* World = GetWorld())
			World->GetTimerManager().SetTimer(TargetRetryTimerHandle, this, &USpyNavigationComponent::TryResolveTarget, TargetRetryIntervalSeconds, true);

		return;
	}

	if (TargetRetryCount < TargetRetryMaxCount)
		return;

	//# design §5-6 — 약 2초 안에 못 찾으면 포기. Dialogue 는 데이터 버그 가능성이 높아 경고 1회,
	//# Gameplay/Interact 는 장소 무관 미션과 구분할 매직 넘버가 없어 조용히 포기한다.
	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(TargetRetryTimerHandle);

	if (bPendingIsDialogue && bWarnedMissingNPCLocation == false)
	{
		bWarnedMissingNPCLocation = true;

		UE_LOG(LogTemp, Warning, TEXT("# [SpyNavigationComponent] NPCId %d 위치를 레지스트리에서 찾지 못했습니다 (NPC 미배치 의심): %s"), PendingNPCId, *GetNameSafe(GetOwner()));
	}
}

void USpyNavigationComponent::HideVisual()
{
	bPathVisible = false;

	if (PathSpline != nullptr)
		PathSpline->SetVisibility(false);

	for (USplineMeshComponent* Segment : PathSegmentPool)
	{
		if (Segment != nullptr)
			Segment->SetVisibility(false);
	}
}

void USpyNavigationComponent::RecomputePath()
{
	if (bPathActive == false)
		return;

	//# design 2026-08-10 §6 — 트리거 활성 타겟은 NavMesh 쿼리 자체를 생략한다(거리 계산 불필요)
	if (bInsideHideTrigger)
	{
		HideVisual();

		return;
	}

	AActor* Owner = GetOwner();
	if (Owner == nullptr)
		return;

	UWorld* World = GetWorld();
	if (World == nullptr)
		return;

	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(World);
	if (NavSystem == nullptr)
	{
		//# design §7-5 항목6 — 질의 자체가 불가능한 프레임도 "낡은 경로 유지"가 아니라 숨김으로 처리
		HideVisual();

		return;
	}

	UNavigationPath* NavPath = NavSystem->FindPathToLocationSynchronously(World, Owner->GetActorLocation(), CurrentTargetLocation);
	if (NavPath == nullptr || NavPath->IsValid() == false)
	{
		//# 목표 좌표가 NavMesh 밖(§8 조건 3·4 위반 등)이면 매번 이 경로를 탄다 — 낡은 경로를
		//# 가리킨 채 영구 정지하는 무증상 실패를 막는다(design §2 와 같은 계열, §7-5 항목6)
		HideVisual();

		return;
	}

	ApplyPathPoints(NavPath->PathPoints);
}

void USpyNavigationComponent::ApplyPathPoints(const TArray<FVector>& InPathPoints)
{
	if (PathSpline == nullptr)
		return;

	//# 트리밍 전 "원본" 경로 길이로 히스테리시스를 먼저 판정한다 — design §4-3 규칙 순서 참조.
	//# 트리거 바인딩 여부와 무관하게 아래 TrimDistanceCm 계산에서 항상 필요해 스코프 밖에 둔다.
	const float RemainingPathLength = SpyNavPathMath::ComputePathLength(InPathPoints);

	if (BoundHideTrigger.IsValid())
	{
		//# 트리거 바인딩 타겟은 히스테리시스 자체를 쓰지 않는다 — RecomputePath 가드가
		//# "안"을 이미 걸러내므로 여기 도달한 것 자체가 "밖"이라는 뜻이다(design 2026-08-10 §6 개정).
		bPathVisible = true;
	}
	else
	{
		bPathVisible = SpyNavPathMath::EvaluateHysteresisVisibility(RemainingPathLength, ArrivalHideDistanceCm, ArrivalReshowDistanceCm, bPathVisible);
	}

	if (bPathVisible == false)
	{
		HideVisual();

		return;
	}

	//# 오프셋은 조건부다 — 남은 길이가 ArrivalReshowDistanceCm 미만이면 0으로 낮춘다(design §4-3 규칙2).
	const float TrimDistanceCm = (RemainingPathLength >= ArrivalReshowDistanceCm) ? StartOffsetDistanceCm : 0.f;
	const TArray<FVector> TrimmedPoints = SpyNavPathMath::TrimLeadingDistance(InPathPoints, TrimDistanceCm);

	//# §7-5 항목4 — 지면 z-fighting 방지용 Z 오프셋을 렌더용 사본에만 적용(NavMesh 질의 좌표 자체는 불변)
	TArray<FVector> OffsetPoints;
	OffsetPoints.Reserve(TrimmedPoints.Num());
	for (const FVector& Point : TrimmedPoints)
	{
		OffsetPoints.Add(Point + FVector(0.f, 0.f, GroundZOffsetCm));
	}

	PathSpline->SetVisibility(true);
	PathSpline->ClearSplinePoints(false);
	for (const FVector& Point : OffsetPoints)
	{
		PathSpline->AddSplinePoint(Point, ESplineCoordinateSpace::World, false);
	}
	PathSpline->UpdateSpline();

	//# 원본 경로점을 직선으로 잇지 않고, PathSpline 이 계산한 Catmull-Rom 곡선을 따라
	//# 일정 간격으로 재샘플링한 점으로 세그먼트를 만든다 — 코너가 곡선으로 표시된다.
	TArray<FVector> CurvePoints;
	//# CurvePoints 와 인덱스 1:1 대응하는 스플라인상 거리 — 세그먼트별로 실제 접선을
	//# 조회하기 위한 키(아래 bHasReliableSpline 분기 참조).
	TArray<float> CurveDistances;
	const float SplineLength = PathSpline->GetSplineLength();
	const bool bHasReliableSpline = (SplineLength > KINDA_SMALL_NUMBER);
	if (bHasReliableSpline)
	{
		//# 하한(디자이너 오설정 방지) + 상한(긴 경로에서 세그먼트/컴포넌트 폭증 방지)을
		//# 둘 다 적용한 유효 간격.
		float EffectiveSampleIntervalCm = FMath::Max(CurveSampleIntervalCm, MinCurveSampleIntervalCm);
		if (SplineLength / EffectiveSampleIntervalCm > MaxCurveSampleCount)
			EffectiveSampleIntervalCm = SplineLength / static_cast<float>(MaxCurveSampleCount);

		for (float Distance = 0.f; Distance < SplineLength - MinTailSegmentCm; Distance += EffectiveSampleIntervalCm)
		{
			CurvePoints.Add(PathSpline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World));
			CurveDistances.Add(Distance);
		}
		CurvePoints.Add(PathSpline->GetLocationAtDistanceAlongSpline(SplineLength, ESplineCoordinateSpace::World));
		CurveDistances.Add(SplineLength);
	}
	else
	{
		CurvePoints = OffsetPoints;
	}

	const TArray<TPair<FVector, FVector>> Segments = SpyNavPathMath::BuildSplineSegments(CurvePoints);
	EnsureSegmentPoolSize(Segments.Num());

	//# 세그먼트마다 UV 가 0부터 리셋되는 머티리얼 특성 때문에 화살표 위상을 이어주는 누적 거리.
	//# 플레이어(움직임) 대신 목표(월드 고정) 쪽을 위상 0으로 앵커하기 위해 -SplineLength 에서 시작한다.
	float RunningDistanceCm = -SplineLength;
	for (int32 Index = 0; Index < Segments.Num(); ++Index)
	{
		USplineMeshComponent* Segment = PathSegmentPool[Index];
		const FVector ChordTangent = (Segments[Index].Value - Segments[Index].Key);

		//# 세그먼트 자신의 직선 방향 대신 PathSpline 의 실제 접선을 쓴다 — 인접 세그먼트가
		//# 공유점에서 같은 접선을 갖게 돼 코너의 빈틈/겹침이 사라진다.
		FVector StartTangent = ChordTangent;
		FVector EndTangent = ChordTangent;
		if (bHasReliableSpline)
		{
			StartTangent = PathSpline->GetTangentAtDistanceAlongSpline(CurveDistances[Index], ESplineCoordinateSpace::World);
			EndTangent = PathSpline->GetTangentAtDistanceAlongSpline(CurveDistances[Index + 1], ESplineCoordinateSpace::World);
		}

		Segment->SetVisibility(true);
		Segment->SetStartAndEnd(Segments[Index].Key, StartTangent, Segments[Index].Value, EndTangent, true);

		if (UMaterialInstanceDynamic* DynMat = PathSegmentMaterialPool[Index])
		{
			DynMat->SetScalarParameterValue(TEXT("SegmentWorldLength"), ChordTangent.Size());
			DynMat->SetScalarParameterValue(TEXT("SegmentStartDistanceCm"), RunningDistanceCm);
			DynMat->SetScalarParameterValue(TEXT("TotalPathLengthCm"), SplineLength);
		}

		RunningDistanceCm += ChordTangent.Size();
	}

	for (int32 Index = Segments.Num(); Index < PathSegmentPool.Num(); ++Index)
	{
		PathSegmentPool[Index]->SetVisibility(false);
	}
}

void USpyNavigationComponent::EnsureSegmentPoolSize(int32 InRequiredCount)
{
	while (PathSegmentPool.Num() < InRequiredCount)
	{
		USplineMeshComponent* Segment = NewObject<USplineMeshComponent>(GetOwner());
		Segment->SetMobility(EComponentMobility::Movable);
		Segment->SetupAttachment(GetOwner()->GetRootComponent());

		//# SetStartAndEnd 는 로컬 좌표를 받는다 — 절대 트랜스폼으로 전환해 월드 좌표를 그대로 쓴다.
		//# 불변조건: 이후 아무도 이 세그먼트의 Relative Location/Rotation/Scale3D 를 건드리면 안 된다.
		Segment->SetUsingAbsoluteLocation(true);
		Segment->SetUsingAbsoluteRotation(true);
		Segment->SetUsingAbsoluteScale(true);

		Segment->RegisterComponent();

		//# USplineMeshComponent 는 StaticMesh 가 없으면 아무것도 그리지 않는다 — 아트 작업 전
		//# 임시 지오메트리를 SegmentMesh 에디터 필드로 지정할 수 있게 한다
		if (SegmentMesh != nullptr)
			Segment->SetStaticMesh(SegmentMesh);

		//# 글로우 머티리얼 에셋 제작은 이번 스펙 범위 밖(아트 작업) — 미등록 시 엔진 기본 머티리얼로 표시된다.
		//# 세그먼트별 SegmentWorldLength 파라미터를 세팅하려면 다이나믹 인스턴스가 필요하다.
		UMaterialInstanceDynamic* DynMat = nullptr;
		if (UMaterialInterface* GlowMaterial = USpyAssetManager::GetAssetByName<UMaterialInterface>(TEXT("NavPathGlow")))
			DynMat = Segment->CreateDynamicMaterialInstance(0, GlowMaterial);

		PathSegmentPool.Add(Segment);
		PathSegmentMaterialPool.Add(DynMat);
	}
}

void USpyNavigationComponent::BindHideTrigger(AActor* TargetActor)
{
	UnbindHideTrigger();

	//# GetObject() 널체크로는 인터페이스 미구현 여부를 걸러낼 수 없다 — Cast<Interface> 로 먼저 판정한다
	//# (Interactable/SpyInteractableObject.cpp 의 동일 근거 주석 참조).
	ISpyMissionTargetHideVolume* HideVolumeHost = Cast<ISpyMissionTargetHideVolume>(TargetActor);
	if (HideVolumeHost == nullptr)
		return;

	UPrimitiveComponent* TriggerComponent = HideVolumeHost->GetHideTriggerComponent();
	if (TriggerComponent == nullptr)
		return;

	BoundHideTrigger = TriggerComponent;
	TriggerComponent->OnComponentBeginOverlap.AddDynamic(this, &USpyNavigationComponent::HandleHideTriggerBeginOverlap);
	TriggerComponent->OnComponentEndOverlap.AddDynamic(this, &USpyNavigationComponent::HandleHideTriggerEndOverlap);

	//# 구독 시점에 이미 트리거 안에 서 있는 경우(예: 트리거 안에서 미션을 수락) 대비 —
	//# BeginOverlap 이벤트는 발화하지 않으므로 현재 상태를 직접 조회해 동기화한다.
	AActor* Owner = GetOwner();
	bInsideHideTrigger = (Owner != nullptr) && TriggerComponent->IsOverlappingActor(Owner);
}

void USpyNavigationComponent::UnbindHideTrigger()
{
	if (UPrimitiveComponent* TriggerComponent = BoundHideTrigger.Get())
	{
		TriggerComponent->OnComponentBeginOverlap.RemoveDynamic(this, &USpyNavigationComponent::HandleHideTriggerBeginOverlap);
		TriggerComponent->OnComponentEndOverlap.RemoveDynamic(this, &USpyNavigationComponent::HandleHideTriggerEndOverlap);
	}

	BoundHideTrigger = nullptr;
	bInsideHideTrigger = false;
}

void USpyNavigationComponent::HandleHideTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
															  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	NotifyHideTriggerEntered(OtherActor);
}

void USpyNavigationComponent::HandleHideTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
														   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	NotifyHideTriggerExited(OtherActor);
}

void USpyNavigationComponent::NotifyHideTriggerEntered(AActor* OtherActor)
{
	if (OtherActor != GetOwner())
		return;

	bInsideHideTrigger = true;
	HideVisual();
}

void USpyNavigationComponent::NotifyHideTriggerExited(AActor* OtherActor)
{
	if (OtherActor != GetOwner())
		return;

	bInsideHideTrigger = false;

	//# World/NavSystem 없어 RecomputePath 가 조기반환해도 "밖" 상태를 동기 반영(design §6).
	bPathVisible = true;
	RecomputePath();
}
