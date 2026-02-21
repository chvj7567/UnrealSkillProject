// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyGA_SkillHit.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "SKAbilitySystemComponent.h"
#include "SKGameplayEffectContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGA_SkillHit)

void USpyGA_SkillHit::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    UAnimMontage* AbilityMontage = nullptr;

    if (TriggerEventData)
    {
        FGameplayTag EventTag = TriggerEventData->EventTag;

        if (EventTag.MatchesTag(SKGameplayTags::Skill_Hit_Left))
        {
            AbilityMontage = HitLeftAbilityMontage;
        }
        else if (EventTag.MatchesTag(SKGameplayTags::Skill_Hit_Right))
        {
            AbilityMontage = HitRightAbilityMontage;
        }
        else if (EventTag.MatchesTag(SKGameplayTags::Skill_Hit_Front))
        {
            AbilityMontage = HitFrontAbilityMontage;
        }
        else if (EventTag.MatchesTag(SKGameplayTags::Skill_Hit_Back))
        {
            AbilityMontage = HitBackAbilityMontage;
        }
    }

    if (AbilityMontage)
    {
        if (UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, AbilityMontage))
        {
            MontageTask->OnCompleted.AddDynamic(this, &USKGameplayAbility::OnMontageCompleted);
            MontageTask->OnInterrupted.AddDynamic(this, &USKGameplayAbility::OnMontageCancelled);
            MontageTask->OnCancelled.AddDynamic(this, &USKGameplayAbility::OnMontageCancelled);
            MontageTask->ReadyForActivation();
        }
    }
}