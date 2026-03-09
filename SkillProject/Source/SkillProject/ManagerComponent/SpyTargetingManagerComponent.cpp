// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyTargetingManagerComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/Character.h"
#include "AIController.h"
#include "Net/UnrealNetwork.h"

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
    //# 서버만 타겟 세팅
    if (GetOwner() && GetOwner()->HasAuthority() == false)
        return;

	if (NewTarget)
	{
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

void USpyTargetingManagerComponent::FindTarget(float Radius)
{
    AActor* Owner = GetOwner();
    if (Owner == nullptr)
        return;

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
}

bool USpyTargetingManagerComponent::IsPotentialTargetValid(AActor* PotentialTarget) const
{
    if (PotentialTarget == nullptr)
        return false;

    if (APawn* Pawn = Cast<APawn>(PotentialTarget))
    {
        AController* Con = Pawn->GetController();
        return (Con && Con->IsA<AAIController>());
    }

    return false;
}
