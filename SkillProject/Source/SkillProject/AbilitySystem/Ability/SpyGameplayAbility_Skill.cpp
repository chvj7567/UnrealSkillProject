// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Ability/SpyGameplayAbility_Skill.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/SpyGameplayEffectContext.h"
#include "AbilitySystemComponent.h"
#include "Character/SpyCharacter.h"
#include "Item/SpyWeapon.h"
#include "Manager/SpyAssetManager.h"
#include "Data/SpyCharacterAssetData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGameplayAbility_Skill)

void USpyGameplayAbility_Skill::OnMontageCompleted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USpyGameplayAbility_Skill::OnMontageCancelled()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USpyGameplayAbility_Skill::OnWaitGameplayEvent(FGameplayEventData Payload)
{
    if (GameplayEffectClass == nullptr)
        return;

    AActor* TargetActor = const_cast<AActor*>(Payload.Target.Get());
    if (TargetActor == nullptr)
        return;

    UAbilitySystemComponent* ASC = CurrentActorInfo->AbilitySystemComponent.Get();
    if (ASC == nullptr)
        return;

    FGameplayEffectContextHandle EffectContext = MakeEffectContext(CurrentSpecHandle, CurrentActorInfo);
    FSpyGameplayEffectContext* CustomContext = FSpyGameplayEffectContext::ExtractEffectContext(EffectContext);
    if (CustomContext == nullptr)
        return;

    CustomContext->AddInstigator(CurrentActorInfo->OwnerActor.Get(), CurrentActorInfo->AvatarActor.Get());
    CustomContext->AddSourceObject(GetSourceObject(CurrentSpecHandle, CurrentActorInfo));

    FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(GameplayEffectClass, GetAbilityLevel(), EffectContext);
    if (SpecHandle.IsValid())
    {
        if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor))
        {
            ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
        }
    }
}

void USpyGameplayAbility_Skill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        CurrentSpecHandle = Handle;
        CurrentActorInfo = ActorInfo;
        CurrentActivationInfo = ActivationInfo;

        if (UAbilityTask_WaitGameplayEvent* WaitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, WaitGameplayTag, nullptr, false, false))
        {
            WaitTask->EventReceived.AddDynamic(this, &USpyGameplayAbility_Skill::OnWaitGameplayEvent);
            WaitTask->ReadyForActivation();
        }

        FName SkillAName = USpyAssetManager::Get().GetSkillAssetNameByType(CharacterType, WaitGameplayTag);
        if (UAnimMontage* Montage = USpyAssetManager::GetAssetByName<UAnimMontage>(SkillAName))
        {
            if (UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage))
            {
                MontageTask->OnCompleted.AddDynamic(this, &USpyGameplayAbility_Skill::OnMontageCompleted);
                MontageTask->OnInterrupted.AddDynamic(this, &USpyGameplayAbility_Skill::OnMontageCancelled);
                MontageTask->OnCancelled.AddDynamic(this, &USpyGameplayAbility_Skill::OnMontageCancelled);
                MontageTask->ReadyForActivation();
            }
        }

        SetWeaponSkillTag();
    }
    else
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
    }
}

void USpyGameplayAbility_Skill::SetWeaponSkillTag()
{
    if (const ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetAvatarActorFromActorInfo()))
    {
        if (ASpyWeapon* SpyWeapon = SpyCharacter->GetSpyWeapon())
        {
            SpyWeapon->SetCurrentSkillTag(WaitGameplayTag);
        }
    }
}
