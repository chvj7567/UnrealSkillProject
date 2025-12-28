// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/AnimInstance/USpyAnimNotifyState_AttackTrace.h"
#include "Engine/OverlapResult.h"
#include "Character/SpyCharacter.h"
#include "AbilitySystemBlueprintLibrary.h"

void UUSpyAnimNotifyState_AttackTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    //# 첫 공격은 딜레이 없이 들어가기 위해서 임시 값 지정
    CumulativeTime = 100.0f;
}

void UUSpyAnimNotifyState_AttackTrace::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

    CumulativeTime += FrameDeltaTime;

    if (CumulativeTime >= CheckInterval)
    {
        CumulativeTime = 0.0f;

        SendGameplayEventToOwner(MeshComp->GetOwner());
    }
}

void UUSpyAnimNotifyState_AttackTrace::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);
}

void UUSpyAnimNotifyState_AttackTrace::SendGameplayEventToOwner(AActor* InOwner)
{
    if (InOwner == nullptr)
        return;

    if (ASpyCharacter* OwnerCharacter = Cast<ASpyCharacter>(InOwner))
    {
        if (USpyAbilitySystemComponent* OwnerASC = OwnerCharacter->GetSpyAbilitySystemComponent())
        {
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
                    if (ASpyCharacter* TargetCharacter = Cast<ASpyCharacter>(TargetActor))
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

            if (bShowCollision)
            {
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
        }
    }
}