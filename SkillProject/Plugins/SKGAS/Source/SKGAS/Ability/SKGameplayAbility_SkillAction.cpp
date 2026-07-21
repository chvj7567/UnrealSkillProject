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
#include "DrawDebugHelpers.h"
#include "SKDebug.h"
#include "SKGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKGameplayAbility_SkillAction)

//# 타겟의 정면 기준으로 공격자가 어느 방향에 있는지 4분할
FGameplayTag CalcHitDirectionTag(const FVector& AttackerLocation, const ACharacter* TargetCharacter)
{
    if (TargetCharacter == nullptr)
        return SKGameplayTags::Skill_Hit_Front;

    const FVector TargetLocation = TargetCharacter->GetActorLocation();
    const FVector ToAttacker = (AttackerLocation - TargetLocation).GetSafeNormal2D();
    const FVector TargetForward = TargetCharacter->GetActorForwardVector().GetSafeNormal2D();

    const float Dot = FVector::DotProduct(TargetForward, ToAttacker);
    const float CrossZ = FVector::CrossProduct(TargetForward, ToAttacker).Z;
    const float AngleDeg = FMath::RadiansToDegrees(FMath::Atan2(CrossZ, Dot));

    FGameplayTag ResultTag = SKGameplayTags::Skill_Hit_Front;
    if (FMath::Abs(AngleDeg) <= 45.0f)
        ResultTag = SKGameplayTags::Skill_Hit_Front;
    else if (FMath::Abs(AngleDeg) >= 135.0f)
        ResultTag = SKGameplayTags::Skill_Hit_Back;
    else if (AngleDeg > 0.0f)
        ResultTag = SKGameplayTags::Skill_Hit_Right;
    else
        ResultTag = SKGameplayTags::Skill_Hit_Left;

    if (SKDebugDrawEnabled())
    {
        UE_LOG(LogTemp, Warning, TEXT("[HitDir] Target=%s Angle=%.1f° → %s"),
            *TargetCharacter->GetName(), AngleDeg, *ResultTag.ToString());

        if (UWorld* World = TargetCharacter->GetWorld())
        {
            const FVector LineBase = TargetLocation + FVector(0, 0, 90.0f);
            DrawDebugLine(World, LineBase, LineBase + TargetForward * 200.0f, FColor::Blue, false, 2.0f, 0, 3.0f);
            DrawDebugLine(World, LineBase, LineBase + ToAttacker * 200.0f, FColor::Red, false, 2.0f, 0, 3.0f);
        }
    }

    return ResultTag;
}

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
    CustomContext->AddActors({ TWeakObjectPtr<AActor>(TargetActor) });
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
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
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
            MontageTask->OnInterrupted.AddDynamic(this, &USKGameplayAbility_SkillAction::OnMontageInterrupted);
            MontageTask->OnCancelled.AddDynamic(this, &USKGameplayAbility_SkillAction::OnMontageCancelled);
            MontageTask->OnBlendOut.AddDynamic(this, &USKGameplayAbility_SkillAction::OnMontageBlendOut);
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

        if (UAbilityTask_WaitDelay* WaitTask = UAbilityTask_WaitDelay::WaitDelay(this, HitTime))
        {
            WaitTask->OnFinish.AddDynamic(this, &USKGameplayAbility_SkillAction::CheckDetect);
            WaitTask->ReadyForActivation();

            UE_LOG(LogTemp, Log, TEXT("# [GA_SkillAction] Server Hit Time %f"), HitTime);
        }
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
                UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetCharacter);
                if (TargetASC && TargetASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Death))
                    continue;

                if (bIsHeal == false && TargetASC && TargetASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Parry))
                {
                    FVector DefenderForward = TargetCharacter->GetActorForwardVector();
                    FVector ToAttacker = (OwnerCharacter->GetActorLocation() - TargetCharacter->GetActorLocation()).GetSafeNormal();
                    if (FVector::DotProduct(DefenderForward, ToAttacker) > 0.0f)
                    {
                        FGameplayEventData ParryPayload;
                        ParryPayload.Instigator = OwnerCharacter;
                        ParryPayload.Target = TargetCharacter;
                        UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetCharacter, SKGameplayTags::Skill_Parry_Hit, ParryPayload);
                        bInvalidCharacter = true;
                        continue;
                    }
                }

                bInvalidCharacter = true;

                FGameplayEventData Payload;
                Payload.Instigator = OwnerCharacter;
                Payload.Target = TargetCharacter;

                if (bIsHeal)
                {
                    Payload.TargetTags.AddTag(SKGameplayTags::Skill_Buff_Heal);
                }
                else
                {
                    //# 타겟 정면 기준 공격자 위치로 4방향 Hit 태그 결정
                    Payload.TargetTags.AddTag(CalcHitDirectionTag(OwnerCharacter->GetActorLocation(), TargetCharacter));
                }

                UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerCharacter, EffectSkillActionTag, Payload);
            }
        }
    }

    if (SKDebugDrawEnabled())
    {
        const FColor Color = bInvalidCharacter ? FColor::Red : FColor::Green;
        DrawDebugCapsule(OwnerCharacter->GetWorld(), (CurrentStart + CurrentEnd) * 0.5f,
            FVector::Dist(CurrentStart, CurrentEnd) * 0.5f + Radius, Radius,
            FRotationMatrix::MakeFromZ(CurrentStart - CurrentEnd).ToQuat(), Color, false, 1.0f);
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
                UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetCharacter);
                if (TargetASC && TargetASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Death))
                    continue;

                FVector TargetVector = TargetCharacter->GetActorLocation() - OwnerCharacter->GetActorLocation();
                float TargetDegree = FMath::RadiansToDegrees(FMath::Acos(OwnerCharacter->GetActorForwardVector().CosineAngle2D(TargetVector)));

                UE_LOG(LogTemp, Log, TEXT("# [GA_SkillAction] TargetDegree %f"), TargetDegree);
                if (Degree < TargetDegree)
                    continue;

                if (bIsHeal == false && TargetASC && TargetASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Parry))
                {
                    FVector DefenderForward = TargetCharacter->GetActorForwardVector();
                    FVector ToAttacker = (OwnerCharacter->GetActorLocation() - TargetCharacter->GetActorLocation()).GetSafeNormal();
                    if (FVector::DotProduct(DefenderForward, ToAttacker) > 0.0f)
                    {
                        FGameplayEventData ParryPayload;
                        ParryPayload.Instigator = OwnerCharacter;
                        ParryPayload.Target = TargetCharacter;
                        UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetCharacter, SKGameplayTags::Skill_Parry_Hit, ParryPayload);
                        bInvalidCharacter = true;
                        continue;
                    }
                }

                bInvalidCharacter = true;

                FGameplayEventData Payload;
                Payload.Instigator = OwnerCharacter;
                Payload.Target = TargetCharacter;

                if (bIsHeal)
                {
                    Payload.TargetTags.AddTag(SKGameplayTags::Skill_Buff_Heal);
                }
                else
                {
                    //# 타겟 정면 기준 공격자 위치로 4방향 Hit 태그 결정
                    Payload.TargetTags.AddTag(CalcHitDirectionTag(OwnerCharacter->GetActorLocation(), TargetCharacter));
                }

                UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerCharacter, EffectSkillActionTag, Payload);
            }
        }
    }

    if (SKDebugDrawEnabled())
    {
        const FColor Color = bInvalidCharacter ? FColor::Red : FColor::Green;
        DrawDebugSphere(OwnerCharacter->GetWorld(), TargetLoc, Radius, 12, Color, false, 1.0f);
    }
}
