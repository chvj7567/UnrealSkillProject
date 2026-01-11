// Fill out your copyright notice in the Description page of Project Settings.


#include "Ability/SKGameplayAbility_SkillActionWeapon.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "SKGameplayEffectContext.h"
#include "SKAbilitySystemComponent.h"
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKGameplayAbility_SkillActionWeapon)

USKGameplayAbility_SkillActionWeapon::USKGameplayAbility_SkillActionWeapon()
{
}

void USKGameplayAbility_SkillActionWeapon::OnMontageCompleted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USKGameplayAbility_SkillActionWeapon::OnMontageCancelled()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USKGameplayAbility_SkillActionWeapon::OnWaitGameplayEvent(FGameplayEventData Payload)
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

void USKGameplayAbility_SkillActionWeapon::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        CurrentSpecHandle = Handle;
        CurrentActorInfo = ActorInfo;
        CurrentActivationInfo = ActivationInfo;

        if (UAbilityTask_WaitGameplayEvent* WaitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, WaitSkillTag, nullptr, false, false))
        {
            WaitTask->EventReceived.AddDynamic(this, &USKGameplayAbility_SkillActionWeapon::OnWaitGameplayEvent);
            WaitTask->ReadyForActivation();
        }

        if (SkillMontage)
        {
            if (UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, SkillMontage))
            {
                MontageTask->OnCompleted.AddDynamic(this, &USKGameplayAbility_SkillActionWeapon::OnMontageCompleted);
                MontageTask->OnInterrupted.AddDynamic(this, &USKGameplayAbility_SkillActionWeapon::OnMontageCancelled);
                MontageTask->OnCancelled.AddDynamic(this, &USKGameplayAbility_SkillActionWeapon::OnMontageCancelled);
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

void USKGameplayAbility_SkillActionWeapon::CheckHit()
{
    if (IsActive() == false)
        return;

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    USKAbilitySystemComponent* OwnerASC = Cast<USKAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get());

    if (OwnerCharacter == nullptr || OwnerASC == nullptr)
        return;

    FGameplayTag EffectSkillActionTag = OwnerASC->GetEffectSkillActionTag();

    if (EffectSkillActionTag != FGameplayTag::EmptyTag)
    {
        OwnerCharacter->GetMesh()->RefreshBoneTransforms();

        FVector CurrentStart = OwnerCharacter->GetMesh()->GetSocketLocation(StartWeaponSocketName);
        FVector CurrentEnd = OwnerCharacter->GetMesh()->GetSocketLocation(EndWeaponSocketName);

        TArray<FHitResult> OutHits;
        FCollisionShape SweepShape = FCollisionShape::MakeSphere(SphereRadius);
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(OwnerCharacter);

        OwnerCharacter->GetWorld()->SweepMultiByChannel(
            OutHits, CurrentStart, CurrentEnd,
            FQuat::Identity, ECC_Pawn, SweepShape, QueryParams);

        bool bInvalidCharacter = false;

        for (const FHitResult& Overlap : OutHits)
        {
            if (AActor* TargetActor = Overlap.GetActor())
            {
                if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor))
                {
                    bInvalidCharacter = true;

                    FGameplayEventData Payload;
                    Payload.EventTag = EffectSkillActionTag;
                    Payload.Instigator = OwnerCharacter;
                    Payload.Target = TargetCharacter;

                    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerCharacter, Payload.EventTag, Payload);
                }
            }
        }

        if (bInvalidCharacter)
        {
            DrawDebugCapsule(OwnerCharacter->GetWorld(), (CurrentStart + CurrentEnd) * 0.5f,
                FVector::Dist(CurrentStart, CurrentEnd) * 0.5f + SphereRadius, SphereRadius,
                FRotationMatrix::MakeFromZ(CurrentStart - CurrentEnd).ToQuat(), FColor::Red, false, 1.0f);
        }
        else
        {
            DrawDebugCapsule(OwnerCharacter->GetWorld(), (CurrentStart + CurrentEnd) * 0.5f,
                FVector::Dist(CurrentStart, CurrentEnd) * 0.5f + SphereRadius, SphereRadius,
                FRotationMatrix::MakeFromZ(CurrentStart - CurrentEnd).ToQuat(), FColor::Green, false, 1.0f);
        }
    }

    UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, IntervalTime);
    DelayTask->OnFinish.AddDynamic(this, &USKGameplayAbility_SkillActionWeapon::CheckHit);
    DelayTask->ReadyForActivation();
}