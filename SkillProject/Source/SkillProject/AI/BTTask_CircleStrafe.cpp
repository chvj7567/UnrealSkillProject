// Fill out your copyright notice in the Description page of Project Settings.

#include "BTTask_CircleStrafe.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "DrawDebugHelpers.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "SpyAIUtils.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(BTTask_CircleStrafe)

UBTTask_CircleStrafe::UBTTask_CircleStrafe()
{
	NodeName = TEXT("Circle Strafe");
	bCreateNodeInstance = true;

	TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_CircleStrafe, TargetKey), AActor::StaticClass());
	StrafeLeftKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_CircleStrafe, StrafeLeftKey));
}

EBTNodeResult::Type UBTTask_CircleStrafe::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();

	if (!IsValid(AIController) || !IsValid(BB))
	{
		return EBTNodeResult::Failed;
	}

	if (!IsValid(EQSQuery))
	{
		UE_LOG(LogTemp, Error, TEXT("[CircleStrafe] ExecuteTask FAIL: EQSQuery is NULL"));
		return EBTNodeResult::Failed;
	}

	ACharacter* Target = Cast<ACharacter>(BB->GetValueAsObject(TargetKey.SelectedKeyName));
	if (!IsValid(Target))
	{
		return EBTNodeResult::Failed;
	}

	if (OriginalMaxWalkSpeed > 0.f)
	{
		SetStrafeSpeed(AIController, OriginalMaxWalkSpeed);
	}

	UE_LOG(LogTemp, Warning, TEXT("[CircleStrafe] START — Duration=%.1f"), StrafeDuration);

	bTaskActive = true;
	StartTime = GetWorld()->GetTimeSeconds();
	RunEQS(OwnerComp);

	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UBTTask_CircleStrafe::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	bTaskActive = false;
	GetWorld()->GetTimerManager().ClearTimer(RetryTimerHandle);

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

void UBTTask_CircleStrafe::OnMessage(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
                                     FName Message, int32 RequestID, bool bSuccess)
{
	// Super 호출 금지 — Super는 FinishLatentTask를 호출해 태스크를 종료시킴
	SetStrafeSpeed(OwnerComp.GetAIOwner(), OriginalMaxWalkSpeed);

	const float Elapsed = GetWorld()->GetTimeSeconds() - StartTime;
	if (Elapsed < StrafeDuration)
	{
		TWeakObjectPtr<UBehaviorTreeComponent> RetryOwner(&OwnerComp);
		GetWorld()->GetTimerManager().SetTimer(RetryTimerHandle,
			FTimerDelegate::CreateWeakLambda(this, [this, RetryOwner]()
			{
				if (bTaskActive && RetryOwner.IsValid())
				{
					RunEQS(*RetryOwner.Get());
				}
			}), 0.3f, false);
	}
	else
	{
		FinishStrafe(OwnerComp, EBTNodeResult::Succeeded);
	}
}

void UBTTask_CircleStrafe::RunEQS(UBehaviorTreeComponent& OwnerComp)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();

	if (!IsValid(AIController) || !IsValid(BB))
	{
		FinishStrafe(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	APawn* Pawn = AIController->GetPawn();
	if (!IsValid(Pawn))
	{
		FinishStrafe(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	BB->SetValueAsBool(StrafeLeftKey.SelectedKeyName, FMath::RandBool());

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

	if (!WeakOwner.IsValid() || !bTaskActive)
	{
		return;
	}

	UBehaviorTreeComponent* OwnerComp = WeakOwner.Get();

	DrawDebugEQSResults(GetWorld(), Result);

	if (!Result.IsValid() || !Result->IsSuccessful() || Result->Items.IsEmpty())
	{
		TWeakObjectPtr<UBehaviorTreeComponent> RetryOwner(OwnerComp);
		GetWorld()->GetTimerManager().SetTimer(RetryTimerHandle,
			FTimerDelegate::CreateWeakLambda(this, [this, RetryOwner]()
			{
				if (bTaskActive && RetryOwner.IsValid())
				{
					RunEQS(*RetryOwner.Get());
				}
			}), 0.5f, false);
		return;
	}

	FVector StrafeLocation = Result->GetItemAsLocation(0);

	AAIController* AIController = OwnerComp->GetAIOwner();
	UBlackboardComponent* BB = OwnerComp->GetBlackboardComponent();

	if (!IsValid(AIController) || !IsValid(BB))
	{
		FinishStrafe(*OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 스킬 시전 중(Lock_Input_Move)이면 이동 보류
	if (!SpyAIUtils::CanMove(AIController))
	{
		TWeakObjectPtr<UBehaviorTreeComponent> RetryOwner(OwnerComp);
		GetWorld()->GetTimerManager().SetTimer(RetryTimerHandle,
			FTimerDelegate::CreateWeakLambda(this, [this, RetryOwner]()
			{
				if (bTaskActive && RetryOwner.IsValid())
				{
					RunEQS(*RetryOwner.Get());
				}
			}), 0.2f, false);
		return;
	}

	ACharacter* Target = Cast<ACharacter>(BB->GetValueAsObject(TargetKey.SelectedKeyName));
	if (IsValid(Target))
	{
		AIController->SetFocus(Target);
	}

	SetStrafeSpeed(AIController, StrafeWalkSpeed);

	FAIMoveRequest MoveReq(StrafeLocation);
	MoveReq.SetAcceptanceRadius(StrafeAcceptanceRadius);
	MoveReq.SetUsePathfinding(true);

	FPathFollowingRequestResult MoveResult = AIController->MoveTo(MoveReq);

	if (MoveResult.Code == EPathFollowingRequestResult::RequestSuccessful)
	{
		WaitForMessage(*OwnerComp, UBrainComponent::AIMessage_MoveFinished);
	}
	else if (MoveResult.Code == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		SetStrafeSpeed(AIController, OriginalMaxWalkSpeed);
		const float Elapsed = GetWorld()->GetTimeSeconds() - StartTime;
		if (Elapsed < StrafeDuration)
		{
			TWeakObjectPtr<UBehaviorTreeComponent> RetryOwner(OwnerComp);
			GetWorld()->GetTimerManager().SetTimer(RetryTimerHandle,
				FTimerDelegate::CreateWeakLambda(this, [this, RetryOwner]()
				{
					if (bTaskActive && RetryOwner.IsValid())
					{
						RunEQS(*RetryOwner.Get());
					}
				}), 0.3f, false);
		}
		else
		{
			FinishStrafe(*OwnerComp, EBTNodeResult::Succeeded);
		}
	}
	else
	{
		SetStrafeSpeed(AIController, OriginalMaxWalkSpeed);
		FinishStrafe(*OwnerComp, EBTNodeResult::Failed);
	}
}

void UBTTask_CircleStrafe::SetStrafeSpeed(AAIController* InController, float Speed)
{
	if (!IsValid(InController)) return;

	ACharacter* Char = Cast<ACharacter>(InController->GetPawn());
	if (!IsValid(Char)) return;

	UCharacterMovementComponent* CMC = Char->GetCharacterMovement();
	if (!IsValid(CMC)) return;

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
	GetWorld()->GetTimerManager().ClearTimer(RetryTimerHandle);
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
	if (!World || !Result.IsValid())
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
