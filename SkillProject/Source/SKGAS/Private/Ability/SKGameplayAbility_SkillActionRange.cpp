// Fill out your copyright notice in the Description page of Project Settings.


#include "Ability/SKGameplayAbility_SkillActionRange.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "SKGameplayEffectContext.h"
#include "SKAbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Actor.h"
#include "Engine/OverlapResult.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKGameplayAbility_SkillActionRange)

USKGameplayAbility_SkillActionRange::USKGameplayAbility_SkillActionRange()
{

}

void USKGameplayAbility_SkillActionRange::OnMontageCompleted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USKGameplayAbility_SkillActionRange::OnMontageCancelled()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USKGameplayAbility_SkillActionRange::OnWaitGameplayEvent(FGameplayEventData Payload)
{
    if (DamageEffectClass == nullptr)
        return;

    UAbilitySystemComponent* ASC = CurrentActorInfo->AbilitySystemComponent.Get();
    if (ASC == nullptr)
        return;

    if (ASC->IsOwnerActorAuthoritative() == false)
        return;

    AActor* TargetActor = const_cast<AActor*>(Payload.Target.Get());
    if (TargetActor == nullptr)
        return;

    FGameplayEffectContextHandle EffectContext = MakeEffectContext(CurrentSpecHandle, CurrentActorInfo);
    FSKGameplayEffectContext* CustomContext = FSKGameplayEffectContext::ExtractEffectContext(EffectContext);
    if (CustomContext == nullptr)
        return;

    CustomContext->AddInstigator(CurrentActorInfo->OwnerActor.Get(), CurrentActorInfo->AvatarActor.Get());
    CustomContext->AddSourceObject(GetSourceObject(CurrentSpecHandle, CurrentActorInfo));

    FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), EffectContext);
    if (SpecHandle.IsValid())
    {
        if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor))
        {
            FActiveGameplayEffectHandle AppliedHandle = ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
            if (AppliedHandle.WasSuccessfullyApplied())
            {
                UE_LOG(LogTemp, Warning, TEXT("# [Server] %s GE Successfully Applied! Effect: %s"), *Payload.EventTag.ToString(), *SpecHandle.Data.Get()->Def->GetName());
            }
        }
    }
}

void USKGameplayAbility_SkillActionRange::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        CurrentSpecHandle = Handle;
        CurrentActorInfo = ActorInfo;
        CurrentActivationInfo = ActivationInfo;

        if (UAbilityTask_WaitGameplayEvent* WaitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, WaitSkillTag, nullptr, false, false))
        {
            WaitTask->EventReceived.AddDynamic(this, &USKGameplayAbility_SkillActionRange::OnWaitGameplayEvent);
            WaitTask->ReadyForActivation();
        }

        if (SkillMontage)
        {
            if (UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, SkillMontage))
            {
                MontageTask->OnCompleted.AddDynamic(this, &USKGameplayAbility_SkillActionRange::OnMontageCompleted);
                MontageTask->OnInterrupted.AddDynamic(this, &USKGameplayAbility_SkillActionRange::OnMontageCancelled);
                MontageTask->OnCancelled.AddDynamic(this, &USKGameplayAbility_SkillActionRange::OnMontageCancelled);
                MontageTask->ReadyForActivation();
            }
        }

        if (IsPredictingClient() == false)
        {
            CheckHit();
        }
    }
    else
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
    }
}

void USKGameplayAbility_SkillActionRange::CheckHit()
{
    if (IsActive() == false)
        return;

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    USKAbilitySystemComponent* OwnerASC = Cast<USKAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get());

    if (OwnerCharacter == nullptr || OwnerASC == nullptr)
        return;

    FGameplayTag SkillTag = OwnerASC->GetSkillActionTag();

    if (SkillTag != FGameplayTag::EmptyTag)
    {
        FVector TargetLoc = OwnerCharacter->GetActorLocation();
        TArray<FOverlapResult> OutHits;
        FCollisionShape CollisionShape = FCollisionShape::MakeSphere(SphereRadius);
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(OwnerCharacter);

        OwnerCharacter->GetWorld()->OverlapMultiByChannel(
            OutHits,
            TargetLoc,
            FQuat::Identity,
            ECC_Pawn,
            CollisionShape,
            QueryParams
        );

        bool bInvalidCharacter = false;

        for (const FOverlapResult& Overlap : OutHits)
        {
            if (AActor* TargetActor = Overlap.GetActor())
            {
                if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor))
                {
                    bInvalidCharacter = true;

                    FGameplayEventData Payload;
                    Payload.EventTag = SkillTag;
                    Payload.Instigator = OwnerCharacter;
                    Payload.Target = TargetCharacter;

                    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerCharacter, Payload.EventTag, Payload);
                }
            }
        }

        if (bInvalidCharacter)
        {
            DrawDebugSphere(OwnerCharacter->GetWorld(), TargetLoc, SphereRadius, 12, FColor::Red, false, 1.0f);
        }
        else
        {
            DrawDebugSphere(OwnerCharacter->GetWorld(), TargetLoc, SphereRadius, 12, FColor::Green, false, 1.0f);
        }
    }

    UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, IntervalTime);
    DelayTask->OnFinish.AddDynamic(this, &USKGameplayAbility_SkillActionRange::CheckHit);
    DelayTask->ReadyForActivation();
}