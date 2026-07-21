// Fill out your copyright notice in the Description page of Project Settings.

#include "BTTask_CircleStrafe.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "DrawDebugHelpers.h"
#include "SKDebug.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "SpyAIUtils.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(BTTask_CircleStrafe)

UBTTask_CircleStrafe::UBTTask_CircleStrafe()
{
	NodeName = TEXT("Circle Strafe");
	bCreateNodeInstance = true;
	bNotifyTick = true;

	TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_CircleStrafe, TargetKey), AActor::StaticClass());
	StrafeLeftKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_CircleStrafe, StrafeLeftKey));
}

EBTNodeResult::Type UBTTask_CircleStrafe::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();

	if (IsValid(AIController) == false || IsValid(BB) == false)
	{
		return EBTNodeResult::Failed;
	}

	if (IsValid(EQSQuery) == false)
	{
		UE_LOG(LogTemp, Error, TEXT("[CircleStrafe] ExecuteTask FAIL: EQSQuery is NULL"));
		return EBTNodeResult::Failed;
	}

	ACharacter* Target = Cast<ACharacter>(BB->GetValueAsObject(TargetKey.SelectedKeyName));
	if (IsValid(Target) == false)
	{
		return EBTNodeResult::Failed;
	}

	bTaskActive = true;
	bEQSPending = false;
	EQSAccumulator = 0.f;
	StartTime = GetWorld()->GetTimeSeconds();

	//# 추격 시 본체가 타겟을 보도록 focus 지정
	AIController->SetFocus(Target);

	//# 스트레이프 속도로 변경
	SetStrafeSpeed(AIController, StrafeWalkSpeed);

	//# StrafeDuration 후 강제 종료 — EQS retry나 MoveTo 미완료로 인한 InProgress 무한대기 방지
	TWeakObjectPtr<UBehaviorTreeComponent> WeakOwnerForTimeout(&OwnerComp);
	GetWorld()->GetTimerManager().SetTimer(DurationTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this, WeakOwnerForTimeout]()
		{
			if (bTaskActive && WeakOwnerForTimeout.IsValid())
			{
				FinishStrafe(*WeakOwnerForTimeout.Get(), EBTNodeResult::Succeeded);
			}
		}), StrafeDuration, false);

	//# 첫 EQS 즉시 발사
	RunEQS(OwnerComp);

	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UBTTask_CircleStrafe::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	bTaskActive = false;
	GetWorld()->GetTimerManager().ClearTimer(DurationTimerHandle);

	if (ActiveQueryId != INDEX_NONE)
	{
		UEnvQueryManager* EQSManager = UEnvQueryManager::GetCurrent(GetWorld());
		if (IsValid(EQSManager))
		{
			EQSManager->AbortQuery(ActiveQueryId);
		}
		ActiveQueryId = INDEX_NONE;
	}

	AAIController* AIController = OwnerComp.GetAIOwner();
	if (IsValid(AIController))
	{
		SetStrafeSpeed(AIController, OriginalMaxWalkSpeed);
		AIController->StopMovement();
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}

	return Super::AbortTask(OwnerComp, NodeMemory);
}

void UBTTask_CircleStrafe::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	if (bTaskActive == false)
		return;

	AAIController* AIC = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (IsValid(AIC) == false || IsValid(BB) == false)
		return;

	APawn* MyPawn = AIC->GetPawn();
	ACharacter* CurTarget = Cast<ACharacter>(BB->GetValueAsObject(TargetKey.SelectedKeyName));
	if (IsValid(MyPawn) == false || IsValid(CurTarget) == false)
	{
		FinishStrafe(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	//# 스킬 시전(Lock_Input_Move) / 죽음 중에는 타겟 회전·시선 락 모두 해제
	if (SpyAIUtils::CanMove(AIC) == false)
	{
		AIC->ClearFocus(EAIFocusPriority::Gameplay);
	}
	else
	{
		//# 행동 가능 상태면 시선 락 복구 후 매 프레임 타겟 방향으로 본체 회전
		if (AIC->GetFocusActor() != CurTarget)
		{
			AIC->SetFocus(CurTarget);
		}

		FVector ToTarget = CurTarget->GetActorLocation() - MyPawn->GetActorLocation();
		ToTarget.Z = 0.f;
		if (ToTarget.IsNearlyZero() == false)
		{
			const FRotator NewRot = ToTarget.Rotation();
			AIC->SetControlRotation(NewRot);
			MyPawn->SetActorRotation(NewRot);
		}
	}

	//# 0.3초마다 새로운 EQS 위치로 재요청 — bEQSPending stuck 방지를 위해 강제 발사
	EQSAccumulator += DeltaSeconds;
	if (EQSAccumulator >= 0.3f)
	{
		EQSAccumulator = 0.f;
		RunEQS(OwnerComp);
	}
}

void UBTTask_CircleStrafe::RunEQS(UBehaviorTreeComponent& OwnerComp)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();

	if (IsValid(AIController) == false || IsValid(BB) == false)
		return;

	APawn* Pawn = AIController->GetPawn();
	if (IsValid(Pawn) == false)
		return;

	BB->SetValueAsBool(StrafeLeftKey.SelectedKeyName, FMath::RandBool());

	bEQSPending = true;
	TWeakObjectPtr<UBehaviorTreeComponent> WeakOwner(&OwnerComp);
	FEnvQueryRequest QueryRequest(EQSQuery, Pawn);
	FQueryFinishedSignature FinishedDelegate = FQueryFinishedSignature::CreateLambda(
		[this, WeakOwner](TSharedPtr<FEnvQueryResult> Result)
		{
			OnQueryFinished(Result, WeakOwner);
		});

	ActiveQueryId = QueryRequest.Execute(EEnvQueryRunMode::SingleResult, FinishedDelegate);
}

void UBTTask_CircleStrafe::OnQueryFinished(TSharedPtr<FEnvQueryResult> Result,
                                           TWeakObjectPtr<UBehaviorTreeComponent> WeakOwner)
{
	ActiveQueryId = INDEX_NONE;
	bEQSPending = false;

	if (WeakOwner.IsValid() == false || bTaskActive == false)
		return;

	UBehaviorTreeComponent* OwnerComp = WeakOwner.Get();
	AAIController* AIController = OwnerComp->GetAIOwner();

	if (SKDebugDrawEnabled())
		DrawDebugEQSResults(GetWorld(), Result);

	//# 빈 결과 — C++ 자체 계산으로 fallback
	if (Result.IsValid() == false || Result->IsSuccessful() == false || Result->Items.IsEmpty())
	{
		if (IsValid(AIController) == false)
			return;
		if (SpyAIUtils::CanMove(AIController) == false)
			return;

		APawn* MyPawn = AIController->GetPawn();
		UBlackboardComponent* BB = OwnerComp->GetBlackboardComponent();
		ACharacter* CurTarget = (IsValid(BB) && IsValid(MyPawn))
			? Cast<ACharacter>(BB->GetValueAsObject(TargetKey.SelectedKeyName))
			: nullptr;
		if (IsValid(CurTarget) == false || IsValid(MyPawn) == false)
			return;

		//# 타겟 반대 방향 + ±30° 랜덤 각도, 100~200 유닛 거리
		FVector AwayDir = MyPawn->GetActorLocation() - CurTarget->GetActorLocation();
		AwayDir.Z = 0.f;
		if (AwayDir.Normalize() == false)
			return;

		const float RandomAngle = FMath::RandRange(-30.f, 30.f);
		const FVector StrafeDir = FRotator(0.f, RandomAngle, 0.f).RotateVector(AwayDir);
		const float Radius = FMath::RandRange(100.f, 200.f);
		const FVector RawLoc = MyPawn->GetActorLocation() + StrafeDir * Radius;

		//# NavMesh로 투영
		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
		FNavLocation ProjectedLoc;
		if (IsValid(NavSys) && NavSys->ProjectPointToNavigation(RawLoc, ProjectedLoc, FVector(100.f, 100.f, 200.f)))
		{
			FAIMoveRequest MoveReq(ProjectedLoc.Location);
			MoveReq.SetAcceptanceRadius(StrafeAcceptanceRadius);
			MoveReq.SetUsePathfinding(true);
			AIController->MoveTo(MoveReq);
		}
		return;
	}

	if (IsValid(AIController) == false)
		return;

	//# 스킬 시전 중(Lock_Input_Move)이면 이번 프레임 이동 보류 — TickTask가 다시 시도
	if (SpyAIUtils::CanMove(AIController) == false)
	{
		return;
	}

	const FVector StrafeLocation = Result->GetItemAsLocation(0);

	FAIMoveRequest MoveReq(StrafeLocation);
	MoveReq.SetAcceptanceRadius(StrafeAcceptanceRadius);
	MoveReq.SetUsePathfinding(true);

	AIController->MoveTo(MoveReq);
}

void UBTTask_CircleStrafe::SetStrafeSpeed(AAIController* InController, float Speed)
{
	if (IsValid(InController) == false) return;

	ACharacter* Char = Cast<ACharacter>(InController->GetPawn());
	if (IsValid(Char) == false) return;

	UCharacterMovementComponent* CMC = Char->GetCharacterMovement();
	if (IsValid(CMC) == false) return;

	if (Speed == StrafeWalkSpeed)
	{
		if (OriginalMaxWalkSpeed <= 0.f)
		{
			OriginalMaxWalkSpeed = CMC->MaxWalkSpeed;
		}
		CMC->MaxWalkSpeed = Speed;
	}
	else
	{
		if (OriginalMaxWalkSpeed > 0.f)
		{
			CMC->MaxWalkSpeed = OriginalMaxWalkSpeed;
			OriginalMaxWalkSpeed = 0.f;
		}
	}
}

void UBTTask_CircleStrafe::FinishStrafe(UBehaviorTreeComponent& OwnerComp, EBTNodeResult::Type Result)
{
	bTaskActive = false;
	GetWorld()->GetTimerManager().ClearTimer(DurationTimerHandle);
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (IsValid(AIController))
	{
		SetStrafeSpeed(AIController, OriginalMaxWalkSpeed);
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}
	FinishLatentTask(OwnerComp, Result);
}

void UBTTask_CircleStrafe::DrawDebugEQSResults(UWorld* World,
                                               const TSharedPtr<FEnvQueryResult>& Result) const
{
#if ENABLE_DRAW_DEBUG
	if (World == nullptr || Result.IsValid() == false)
	{
		return;
	}

	for (int32 Idx = 0; Idx < Result->Items.Num(); ++Idx)
	{
		const FEnvQueryItem& Item = Result->Items[Idx];
		FVector ItemLocation = Result->GetItemAsLocation(Idx);

		if (Idx == 0)
		{
			DrawDebugSphere(World, ItemLocation, 50.f, 12, FColor::Yellow, false, DebugDrawDuration);
		}

		FColor DrawColor = Item.IsValid() ? FColor::Green : FColor::Red;
		DrawDebugSphere(World, ItemLocation, 20.f, 8, DrawColor, false, DebugDrawDuration);

		DrawDebugString(World, ItemLocation + FVector(0.f, 0.f, 30.f),
			FString::Printf(TEXT("%.2f"), Item.Score),
			nullptr, FColor::White, DebugDrawDuration);
	}
#endif
}
