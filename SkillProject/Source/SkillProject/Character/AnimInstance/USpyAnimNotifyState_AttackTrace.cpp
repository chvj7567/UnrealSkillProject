// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/AnimInstance/USpyAnimNotifyState_AttackTrace.h"
#include "Engine/OverlapResult.h"
#include "Character/SpyCharacter.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Item/SpyWeapon.h"

void UUSpyAnimNotifyState_AttackTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
}

void UUSpyAnimNotifyState_AttackTrace::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

    CumulativeTime += FrameDeltaTime;

    if (CumulativeTime >= CheckInterval)
    {
        CumulativeTime = 0.0f;

        AActor* Owner = MeshComp->GetOwner();
        if (Owner == nullptr || Owner->HasAuthority() == false)
            return;

        ASpyCharacter* OwnerCharacter = Cast<ASpyCharacter>(Owner);
        if (OwnerCharacter == nullptr)
            return;

        FVector CenterPos = Owner->GetActorLocation();
        float Radius = 100.0f;

        TArray<FOverlapResult> OutOverlaps;

        FCollisionShape MySphere = FCollisionShape::MakeSphere(Radius);
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(Owner);

        bool bHit = MeshComp->GetWorld()->OverlapMultiByChannel(
            OutOverlaps,
            CenterPos,
            FQuat::Identity,
            ECC_Pawn,
            MySphere,
            QueryParams
        );

        bool findCharacter = false;

        if (bHit)
        {
            for (const FOverlapResult& Overlap : OutOverlaps)
            {
                if (AActor* TargetActor = Overlap.GetActor())
                {
                    if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(TargetActor))
                    {
                        findCharacter = true;

                        ASpyWeapon* SpyWeapon = OwnerCharacter->GetSpyWeapon();
                        if (SpyWeapon == nullptr)
                            return;

                        FGameplayEventData Payload;
                        Payload.EventTag = SpyWeapon->CurrentSkillTag;
                        Payload.Instigator = Owner;
                        Payload.Target = SpyCharacter;

                        UE_LOG(LogTemp, Warning, TEXT("OnHit %s %s %s"), *Owner->GetName(), *SpyCharacter->GetName(), *SpyWeapon->CurrentSkillTag.ToString());
                        UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, SpyWeapon->CurrentSkillTag, Payload);
                    }
                }
            }
        }
        
        if (findCharacter)
        {
            DrawDebugSphere(MeshComp->GetWorld(), CenterPos, Radius, 12, FColor::Red, false, 1.0f);
        }
        else
        {
            DrawDebugSphere(MeshComp->GetWorld(), CenterPos, Radius, 12, FColor::Green, false, 1.0f);
        }
    }
}

void UUSpyAnimNotifyState_AttackTrace::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);
}
