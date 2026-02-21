// Fill out your copyright notice in the Description page of Project Settings.


#include "SKGameplayAbility_SkillAction.h"
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
#include "SKGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKGameplayAbility_SkillAction)

USKGameplayAbility_SkillAction::USKGameplayAbility_SkillAction()
{
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

    //# Context에 정보 추가
    CustomContext->AddInstigator(CurrentActorInfo->OwnerActor.Get(), CurrentActorInfo->AvatarActor.Get());
    CustomContext->AddSourceObject(GetSourceObject(CurrentSpecHandle, CurrentActorInfo));
    CustomContext->SetHitDirectionTag(Payload.TargetTags.First());

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

    if (HasAuthority(&CurrentActivationInfo))
    {
        if (UAbilityTask_WaitGameplayEvent* WaitEffectSkillTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, WaitEffectSkillTag, nullptr, false, false))
        {
            WaitEffectSkillTask->EventReceived.AddDynamic(this, &USKGameplayAbility_SkillAction::OnWaitGameplayEvent);
            WaitEffectSkillTask->ReadyForActivation();
        }

        //# 서버에서 Hit 검사
        ScheduleServerDetect();
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

void USKGameplayAbility_SkillAction::CheckDetect()
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
    
    switch (DetectRangeType)
    {
        case EDetectRangeType::Weapon:
            SendTagToTargetByWeapon(OwnerCharacter, EffectSkillActionTag);
            break;
        case EDetectRangeType::Sphere:
            SendTagToTargetBySphere(OwnerCharacter, EffectSkillActionTag);
            break;
        default:
            break;
    }
}

void USKGameplayAbility_SkillAction::ScheduleServerDetect()
{
    if (DetectTimes.Num() == 0)
        return;

    DetectTimes.Sort();

    for (int32 Index = 0; Index < DetectTimes.Num(); ++Index)
    {
        const float HitTime = DetectTimes[Index];

        UAbilityTask_WaitDelay* WaitTask =
            UAbilityTask_WaitDelay::WaitDelay(this, HitTime);

        if (!WaitTask)
            continue;

        WaitTask->OnFinish.AddDynamic(this, &USKGameplayAbility_SkillAction::CheckDetect);
        WaitTask->ReadyForActivation();
    }
}

void USKGameplayAbility_SkillAction::SendTagToTargetByWeapon(ACharacter* OwnerCharacter, FGameplayTag EffectSkillActionTag)
{
    FVector CurrentStart = OwnerCharacter->GetMesh()->GetSocketLocation(StartWeaponSocketName);
    FVector CurrentEnd = OwnerCharacter->GetMesh()->GetSocketLocation(EndWeaponSocketName);

    TArray<FHitResult> OutHits;
    FCollisionShape SweepShape = FCollisionShape::MakeSphere(Radius);

    FCollisionQueryParams QueryParams;

    switch (DetectTargetType)
    {
        case EDetectTargetType::ExcludeMe:
            QueryParams.AddIgnoredActor(OwnerCharacter);
            break;
        default:
            break;
    }

    OwnerCharacter->GetWorld()->SweepMultiByChannel(
        OutHits, CurrentStart, CurrentEnd,
        FQuat::Identity, ECC_Pawn, SweepShape, QueryParams);

    //# 디버그용
    bool bInvalidCharacter = false;

    //# 중복 액터 체크
    TArray<AActor*> CheckActors;
    for (const FHitResult& Overlap : OutHits)
    {
        if (AActor* TargetActor = Overlap.GetActor())
        {
            if (CheckActors.Contains(TargetActor))
                continue;

            CheckActors.Add(TargetActor);

            if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor))
            {
                bInvalidCharacter = true;

                FGameplayEventData Payload;
                Payload.Instigator = OwnerCharacter;
                Payload.Target = TargetCharacter;

                TArray<FGameplayTag> HitDirectionTags;
                HitDirectionTags.Add(SKGameplayTags::Skill_Hit_Front);
                HitDirectionTags.Add(SKGameplayTags::Skill_Hit_Back);
                HitDirectionTags.Add(SKGameplayTags::Skill_Hit_Left);
                HitDirectionTags.Add(SKGameplayTags::Skill_Hit_Right);

                //# 랜덤 방향 Hit 태그
                int32 RandomIndex = FMath::RandRange(0, HitDirectionTags.Num() - 1);
                FGameplayTag RandomHitTag = HitDirectionTags[RandomIndex];

                Payload.TargetTags.AddTag(RandomHitTag);

                UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerCharacter, EffectSkillActionTag, Payload);
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

void USKGameplayAbility_SkillAction::SendTagToTargetBySphere(ACharacter* OwnerCharacter, FGameplayTag EffectSkillActionTag)
{
    FVector TargetLoc = OwnerCharacter->GetActorLocation();
    TArray<FOverlapResult> OutHits;
    FCollisionShape CollisionShape = FCollisionShape::MakeSphere(Radius);
    FCollisionQueryParams QueryParams;

    switch (DetectTargetType)
    {
        case EDetectTargetType::ExcludeMe:
            QueryParams.AddIgnoredActor(OwnerCharacter);
            break;
        default:
            break;
    }

    OwnerCharacter->GetWorld()->OverlapMultiByChannel(
        OutHits,
        TargetLoc,
        FQuat::Identity,
        ECC_Pawn,
        CollisionShape,
        QueryParams
    );

    //# 디버그용
    bool bInvalidCharacter = false;

    //# 중복 액터 체크
    TArray<AActor*> CheckActors;
    for (const FOverlapResult& Overlap : OutHits)
    {
        if (AActor* TargetActor = Overlap.GetActor())
        {
            if (CheckActors.Contains(TargetActor))
                continue;

            CheckActors.Add(TargetActor);

            if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor))
            {
                bInvalidCharacter = true;

                FGameplayEventData Payload;
                Payload.Instigator = OwnerCharacter;
                Payload.Target = TargetCharacter;

                TArray<FGameplayTag> HitDirectionTags;
                HitDirectionTags.Add(SKGameplayTags::Skill_Hit_Front);
                HitDirectionTags.Add(SKGameplayTags::Skill_Hit_Back);
                HitDirectionTags.Add(SKGameplayTags::Skill_Hit_Left);
                HitDirectionTags.Add(SKGameplayTags::Skill_Hit_Right);

                //# 랜덤 방향 Hit 태그
                int32 RandomIndex = FMath::RandRange(0, HitDirectionTags.Num() - 1);
                FGameplayTag RandomHitTag = HitDirectionTags[RandomIndex];

                Payload.TargetTags.AddTag(RandomHitTag);

                UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerCharacter, EffectSkillActionTag, Payload);
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
