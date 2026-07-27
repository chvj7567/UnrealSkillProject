// Fill out your copyright notice in the Description page of Project Settings.

#include "SpyAIUtils.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Util/SpyGameplayTags.h"

bool SpyAIUtils::CanMove(AAIController* InAIController)
{
    if (InAIController == nullptr) return false;

    APawn* Pawn = InAIController->GetPawn();
    if (Pawn == nullptr) return false;

    APlayerState* PS = Pawn->GetPlayerState();
    if (PS == nullptr) return false;

    UAbilitySystemComponent* ASC = PS->FindComponentByClass<UAbilitySystemComponent>();
    if (ASC == nullptr) return false;

    if (ASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Death)) return false;
    if (ASC->HasMatchingGameplayTag(SpyGameplayTags::Lock_Input_Move)) return false;

    return true;
}

bool SpyAIUtils::IsDead(AAIController* InAIController)
{
	if (InAIController == nullptr)
		return false;

	APawn* Pawn = InAIController->GetPawn();
	if (Pawn == nullptr)
		return false;

	APlayerState* PS = Pawn->GetPlayerState();
	if (PS == nullptr)
		return false;

	UAbilitySystemComponent* ASC = PS->FindComponentByClass<UAbilitySystemComponent>();
	if (ASC == nullptr)
		return false;

	return ASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Death);
}

bool SpyAIUtils::CanTargetAttack(ACharacter* InTarget, UBlackboardComponent* InBlackboard, FName TargetKeyName)
{
    if (InTarget == nullptr) return false;

    APlayerState* PS = InTarget->GetPlayerState();
    if (PS == nullptr) return false;

    UAbilitySystemComponent* ASC = PS->FindComponentByClass<UAbilitySystemComponent>();
    if (ASC == nullptr) return false;

    //# Death 시에도 BB는 직접 건드리지 않음 — RefreshBlackboardTarget이 단일 진입점
    if (ASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Death))
    {
        return false;
    }

    return true;
}
