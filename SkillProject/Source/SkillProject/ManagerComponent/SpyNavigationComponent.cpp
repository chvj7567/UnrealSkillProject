// Fill out your copyright notice in the Description page of Project Settings.

#include "ManagerComponent/SpyNavigationComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "Data/SpyMissionConfig.h"
#include "Manager/SpyAssetManager.h"
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

	if (OwningPawn == nullptr || OwningPawn->IsLocallyControlled() == false)
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

void USpyNavigationComponent::StartPathTo(const FVector& InTargetLocation)
{
	CurrentTargetLocation = InTargetLocation;
	bPathActive = true;

	//# 콜드 스타트를 "이전에 보이고 있었다"로 시드한다 — design §4-3 콜드 스타트 조항 참조.
	bPathVisible = true;

	RecomputePath();

	if (UWorld* World = GetWorld())
		World->GetTimerManager().SetTimer(RepathTimerHandle, this, &USpyNavigationComponent::RecomputePath, UpdateIntervalSeconds, true);
}

void USpyNavigationComponent::StopPath()
{
	bPathActive = false;
	CurrentTargetLocation = FVector::ZeroVector;

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

		StartPathTo(TargetLocation);

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
	const float RemainingPathLength = SpyNavPathMath::ComputePathLength(InPathPoints);
	bPathVisible = SpyNavPathMath::EvaluateHysteresisVisibility(RemainingPathLength, ArrivalHideDistanceCm, ArrivalReshowDistanceCm, bPathVisible);

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

	const TArray<TPair<FVector, FVector>> Segments = SpyNavPathMath::BuildSplineSegments(OffsetPoints);
	EnsureSegmentPoolSize(Segments.Num());

	for (int32 Index = 0; Index < Segments.Num(); ++Index)
	{
		USplineMeshComponent* Segment = PathSegmentPool[Index];
		const FVector Tangent = (Segments[Index].Value - Segments[Index].Key);

		Segment->SetVisibility(true);
		Segment->SetStartAndEnd(Segments[Index].Key, Tangent, Segments[Index].Value, Tangent, true);

		if (UMaterialInstanceDynamic* DynMat = PathSegmentMaterialPool[Index])
			DynMat->SetScalarParameterValue(TEXT("SegmentWorldLength"), Tangent.Size());
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
