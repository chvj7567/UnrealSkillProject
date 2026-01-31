// Fill out your copyright notice in the Description page of Project Settings.


#include "Ability/SKGameplayAbility_SkillAction.h"
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

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKGameplayAbility_SkillAction)

USKGameplayAbility_SkillAction::USKGameplayAbility_SkillAction()
{
}

void USKGameplayAbility_SkillAction::OnMontageCompleted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USKGameplayAbility_SkillAction::OnMontageCancelled()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USKGameplayAbility_SkillAction::OnWaitGameplayEvent(FGameplayEventData Payload)
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
                UE_LOG(LogTemp, Warning, TEXT("# [SKSkillAction] %s GE Successfully Applied! Effect: %s"), *Payload.EventTag.ToString(), *SpecHandle.Data.Get()->Def->GetName());
            }
        }
    }
}

void USKGameplayAbility_SkillAction::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (CommitAbility(Handle, ActorInfo, ActivationInfo) == false)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    CurrentSpecHandle = Handle;
    CurrentActorInfo = ActorInfo;
    CurrentActivationInfo = ActivationInfo;

    if (HasAuthority(&CurrentActivationInfo))
    {
        if (UAbilityTask_WaitGameplayEvent* WaitEffectSkillTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, WaitEffectSkillTag, nullptr, false, false))
        {
            WaitEffectSkillTask->EventReceived.AddDynamic(this, &USKGameplayAbility_SkillAction::OnWaitGameplayEvent);
            WaitEffectSkillTask->ReadyForActivation();
        }

        //# 서버에서 Hit 검사
        ScheduleServerHits();
    }

    if (AbilityMontage)
    {
        if (UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, AbilityMontage))
        {
            MontageTask->OnCompleted.AddDynamic(this, &USKGameplayAbility_SkillAction::OnMontageCompleted);
            MontageTask->OnInterrupted.AddDynamic(this, &USKGameplayAbility_SkillAction::OnMontageCancelled);
            MontageTask->OnCancelled.AddDynamic(this, &USKGameplayAbility_SkillAction::OnMontageCancelled);
            MontageTask->ReadyForActivation();
        }
    }
}

void USKGameplayAbility_SkillAction::CheckHit()
{
    if (IsActive() == false)
        return;

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    USKAbilitySystemComponent* OwnerASC = Cast<USKAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get());
    if (OwnerCharacter == nullptr || OwnerASC == nullptr)
        return;

    if (OwnerCharacter->HasAuthority() == false)
        return;

    FGameplayTag EffectSkillActionTag = OwnerASC->GetEffectSkillActionTag();
    if (EffectSkillActionTag == FGameplayTag::EmptyTag)
        return;
    
    switch (AttackType)
    {
        case EAttackType::WeaponRange:
            SendTagToTargetByWeaponRange(OwnerCharacter, EffectSkillActionTag);
            break;
        case EAttackType::SphereRange:
            SendTagToTargetBySphereRange(OwnerCharacter, EffectSkillActionTag);
            break;
        default:
            break;
    }
}

void USKGameplayAbility_SkillAction::ScheduleServerHits()
{
    if (HitTimes.Num() == 0)
        return;

    HitTimes.Sort();

    for (int32 Index = 0; Index < HitTimes.Num(); ++Index)
    {
        const float HitTime = HitTimes[Index];

        UAbilityTask_WaitDelay* WaitTask =
            UAbilityTask_WaitDelay::WaitDelay(this, HitTime);

        if (!WaitTask)
            continue;

        WaitTask->OnFinish.AddDynamic(this, &USKGameplayAbility_SkillAction::CheckHit);
        WaitTask->ReadyForActivation();
    }
}

void USKGameplayAbility_SkillAction::SendTagToTargetByWeaponRange(ACharacter* OwnerCharacter, FGameplayTag EffectSkillActionTag)
{
    FVector CurrentStart = OwnerCharacter->GetMesh()->GetSocketLocation(StartWeaponSocketName);
    FVector CurrentEnd = OwnerCharacter->GetMesh()->GetSocketLocation(EndWeaponSocketName);

    TArray<FHitResult> OutHits;
    FCollisionShape SweepShape = FCollisionShape::MakeSphere(Radius);
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
            FVector::Dist(CurrentStart, CurrentEnd) * 0.5f + Radius, Radius,
            FRotationMatrix::MakeFromZ(CurrentStart - CurrentEnd).ToQuat(), FColor::Red, false, 1.0f);
    }
    else
    {
        DrawDebugCapsule(OwnerCharacter->GetWorld(), (CurrentStart + CurrentEnd) * 0.5f,
            FVector::Dist(CurrentStart, CurrentEnd) * 0.5f + Radius, Radius,
            FRotationMatrix::MakeFromZ(CurrentStart - CurrentEnd).ToQuat(), FColor::Green, false, 1.0f);
    }
}

void USKGameplayAbility_SkillAction::SendTagToTargetBySphereRange(ACharacter* OwnerCharacter, FGameplayTag EffectSkillActionTag)
{
    FVector TargetLoc = OwnerCharacter->GetActorLocation();
    TArray<FOverlapResult> OutHits;
    FCollisionShape CollisionShape = FCollisionShape::MakeSphere(Radius);
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
                Payload.EventTag = EffectSkillActionTag;
                Payload.Instigator = OwnerCharacter;
                Payload.Target = TargetCharacter;

                UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerCharacter, Payload.EventTag, Payload);
            }
        }
    }

    if (bInvalidCharacter)
    {
        DrawDebugSphere(OwnerCharacter->GetWorld(), TargetLoc, Radius, 12, FColor::Red, false, 1.0f);
    }
    else
    {
        DrawDebugSphere(OwnerCharacter->GetWorld(), TargetLoc, Radius, 12, FColor::Green, false, 1.0f);
    }
}
