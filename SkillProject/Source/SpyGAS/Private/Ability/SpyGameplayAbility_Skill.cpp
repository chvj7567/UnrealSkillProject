// Fill out your copyright notice in the Description page of Project Settings.


#include "Ability/SpyGameplayAbility_Skill.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "SpyGameplayEffectContext.h"
#include "SpyAbilitySystemComponent.h"
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGameplayAbility_Skill)

USpyGameplayAbility_Skill::USpyGameplayAbility_Skill()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;
    ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
}

void USpyGameplayAbility_Skill::OnMontageCompleted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USpyGameplayAbility_Skill::OnMontageCancelled()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USpyGameplayAbility_Skill::OnWaitGameplayEvent(FGameplayEventData Payload)
{
    if (GameplayEffectClass == nullptr)
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
            FActiveGameplayEffectHandle AppliedHandle = ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
            if (AppliedHandle.WasSuccessfullyApplied())
            {
                UE_LOG(LogTemp, Warning, TEXT("[SERVER] %s GE Successfully Applied! Effect: %s"), *Payload.EventTag.ToString(), *SpecHandle.Data.Get()->Def->GetName());
            }
            else
            {
                // 1. 타겟 액터가 파괴 중인지 확인
                bool bIsPendingKill = TargetActor->IsPendingKillPending();

                // 2. 타겟의 태그 상태 확인
                FGameplayTagContainer TargetTags;
                TargetASC->GetOwnedGameplayTags(TargetTags);

                UE_LOG(LogTemp, Error, TEXT("[FAILED] Target: %s | PendingKill: %d | Tags: %s"),
                    *TargetActor->GetName(),
                    bIsPendingKill ? 1 : 0,
                    *TargetTags.ToString());

                // 3. 만약 서버/클라이언트 문제라면
                if (!ASC->IsOwnerActorAuthoritative())
                {
                    UE_LOG(LogTemp, Warning, TEXT("Warning: Client tried to apply GE but failed (Normal Behavior if Predicted)"));
                }

                UE_LOG(LogTemp, Error, TEXT("[SERVER] %s GE Application Failed!"), *Payload.EventTag.ToString());
            }
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

        if (SkillMontage)
        {
            if (UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, SkillMontage))
            {
                MontageTask->OnCompleted.AddDynamic(this, &USpyGameplayAbility_Skill::OnMontageCompleted);
                MontageTask->OnInterrupted.AddDynamic(this, &USpyGameplayAbility_Skill::OnMontageCancelled);
                MontageTask->OnCancelled.AddDynamic(this, &USpyGameplayAbility_Skill::OnMontageCancelled);
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

void USpyGameplayAbility_Skill::CheckHit()
{
    if (IsActive() == false)
        return;

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    USpyAbilitySystemComponent* OwnerASC = Cast<USpyAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get());

    if (OwnerCharacter == nullptr || OwnerASC == nullptr)
        return;

    float Radius = 10.f;
    FName StartWeaponSocketName = "LeftWeaponPos0";
    FName EndWeaponSocketName = "LeftWeaponPos0";

    FVector CenterPos = OwnerCharacter->GetActorLocation();
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
                Payload.EventTag = OwnerASC->GetCurrentActiveSkillTag();
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

    UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, 0.1f);
    DelayTask->OnFinish.AddDynamic(this, &USpyGameplayAbility_Skill::Test);
    DelayTask->ReadyForActivation();
}