// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_ActivateAbility.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BTTask_ActivateAbility)

UBTTask_ActivateAbility::UBTTask_ActivateAbility()
{
    NodeName = "Activate Ability";

    TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ActivateAbility, TargetKey), AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_ActivateAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (AIController == nullptr)
        return EBTNodeResult::Failed;

    UBlackboardComponent* BlackBoardComp = OwnerComp.GetBlackboardComponent();
    if (BlackBoardComp == nullptr)
        return EBTNodeResult::Failed;

    APawn* Pawn = AIController->GetPawn();
    if (Pawn == nullptr)
        return EBTNodeResult::Failed;

    APlayerState* PS = Pawn->GetPlayerState();
    if (PS == nullptr)
        return EBTNodeResult::Failed;

    UAbilitySystemComponent* ASC = PS->FindComponentByClass<UAbilitySystemComponent>();
    if (ASC == nullptr)
        return EBTNodeResult::Failed;

    if (AbilityTags.Num() <= 0)
        return EBTNodeResult::Failed;

    AIController->StopMovement();

    //# 어빌리티 발동 직전에 타겟 방향으로 즉시 회전
    if (TargetKey.SelectedKeyName != NAME_None)
    {
        if (AActor* TargetActor = Cast<AActor>(BlackBoardComp->GetValueAsObject(TargetKey.SelectedKeyName)))
        {
            FVector ToTarget = TargetActor->GetActorLocation() - Pawn->GetActorLocation();
            ToTarget.Z = 0.f;
            if (!ToTarget.IsNearlyZero())
            {
                const FRotator NewRot = ToTarget.Rotation();
                AIController->SetControlRotation(NewRot);
                Pawn->SetActorRotation(NewRot);
            }
        }
    }

    int32 RandomIndex = FMath::RandRange(0, AbilityTags.Num() - 1);
    FGameplayTag RandomTag = AbilityTags[RandomIndex];

    FGameplayTagContainer TagContainer;
    TagContainer.AddTag(RandomTag);

    TArray<FGameplayAbilitySpec*> ActiveAbilities;
    ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(TagContainer, ActiveAbilities);

    //# GA 실행 중이면 중복 실행하지 않고 성공 처리
    for (const FGameplayAbilitySpec* Spec : ActiveAbilities)
    {
        if (Spec->IsActive())
        {
            return EBTNodeResult::Succeeded;
        }
    }

    //# 태그를 통해 GA 실행 시도 — 쿨타임 등으로 실패해도 다음 단계(CircleStrafe)로 넘어가도록 항상 Succeeded
    ASC->TryActivateAbilitiesByTag(TagContainer);

    return EBTNodeResult::Succeeded;
}
