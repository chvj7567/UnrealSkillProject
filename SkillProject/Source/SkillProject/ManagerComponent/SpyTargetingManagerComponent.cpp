// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyTargetingManagerComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Character/SpyCharacter.h"
#include "AIController.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"
#include "GameplayTagContainer.h"
#include "Util/SpyGameplayTags.h"
#include "Character/SpyCharacter.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "SKGameplayTags.h"

USpyTargetingManagerComponent::USpyTargetingManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USpyTargetingManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(USpyTargetingManagerComponent, CurrentTarget);
}

void USpyTargetingManagerComponent::SetCurrentTarget(AActor* NewTarget)
{
    AActor* MyActor = GetOwner();
    if (MyActor == nullptr)
        return;

    ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(MyActor);
    if (SpyCharacter == nullptr)
        return;

	if (NewTarget)
	{
        if (CurrentTarget.IsValid())
        {
            //# 이전 타겟 GE 제거
            if (USpyAbilitySystemComponent* ASC = Cast<ASpyCharacter>(CurrentTarget.Get())->GetSpyAbilitySystemComponent())
            {
                FGameplayTagContainer TagContainer;
                TagContainer.AddTag(SpyGameplayTags::Character_State_Targeting);

                ASC->RemoveActiveEffectsWithGrantedTags(TagContainer);
            }
        }

		CurrentTarget = NewTarget;

        UE_LOG(LogTemp, Warning, TEXT("# [SpyTargeting] Target: %s"), *CurrentTarget.Get()->GetName());
	}
	else
	{
		CurrentTarget.Reset();
	}
}

bool USpyTargetingManagerComponent::IsTargetValid() const
{
    return IsPotentialTargetValid(CurrentTarget.Get());
}

bool USpyTargetingManagerComponent::FindTarget(float Radius)
{
    AActor* Owner = GetOwner();
    if (Owner == nullptr)
        return false;

    //# 감지할 오브젝트 타입을 Pawn으로 설정
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

    //# 자기 자신 제외
    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(Owner);

    TArray<AActor*> OutActors;

    bool bFound = UKismetSystemLibrary::SphereOverlapActors(
        GetWorld(),
        Owner->GetActorLocation(),
        Radius,
        ObjectTypes,
        nullptr,
        ActorsToIgnore,
        OutActors
    );

    DrawDebugSphere(
        GetWorld(),
        Owner->GetActorLocation(),
        Radius,
        24,                // 세그먼트 수 (구의 디테일)
        FColor::Blue,       // 선 색상
        false,             // 영구 지속 여부
        0.5f,              // 지속 시간 (초)
        0,                 // Depth Priority
        1.0f               // 선 두께
    );

    AActor* Target = nullptr;
    float ClosestDistanceSq = FMath::Square(Radius);

    if (bFound)
    {
        for (AActor* FoundActor : OutActors)
        {
            if (IsPotentialTargetValid(FoundActor))
            {
                //# DistSquared는 루트 계산이 없어 일반 Dist보다 빠름
                float DistSq = FVector::DistSquared(Owner->GetActorLocation(), FoundActor->GetActorLocation());
                if (DistSq < ClosestDistanceSq)
                {
                    ClosestDistanceSq = DistSq;
                    Target = FoundActor;
                }
            }
        }
    }

    SetCurrentTarget(Target);

    return Target != nullptr;
}

bool USpyTargetingManagerComponent::IsPotentialTargetValid(AActor* PotentialTarget) const
{
    if (PotentialTarget == nullptr)
        return false;

    if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(PotentialTarget))
    {
        bool bIsDeath = SpyCharacter->GetSpyAbilitySystemComponent()->HasMatchingGameplayTag(SKGameplayTags::Character_State_Death);
        bool bIsAI = SpyCharacter->IsPlayerControlled() == false;
        return bIsDeath == false && bIsAI;
    }

    return false;
}
