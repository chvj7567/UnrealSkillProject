// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyParkourManagerComponent.h"
#include "Character/SpyCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

USpyParkourManagerComponent::USpyParkourManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USpyParkourManagerComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USpyParkourManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    CheckAbleWallClimbing();
}

void USpyParkourManagerComponent::CheckAbleWallClimbing()
{
    ASpyCharacter* OwnerChar = Cast<ASpyCharacter>(GetOwner());
    if (!OwnerChar) return;

    FVector Start = OwnerChar->GetActorLocation() + OwnerChar->GetActorForwardVector() * 20.f;
    FVector End = Start + OwnerChar->GetActorForwardVector() * 40.f;

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(OwnerChar);

    bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

    FColor LineColor = bHit ? FColor::Green : FColor::Red;
    float Lifetime = 1.0f;      // 화면에 얼마나 오래 보일지 (초)
    float Thickness = 2.0f;     // 라인 두께

    DrawDebugLine(
        GetWorld(),
        Start,
        End,
        LineColor,
        false,      // PersistentLines, false = 일정 시간 후 사라짐
        Lifetime,
        0,          // DepthPriority
        Thickness
    );

    if (!bHit)
    {
        // 벽에서 손 놓기
        bIsWallClimbing = false;
    }
    else
    {
        // 벽 유지 중
        // 여기서 Hit.Normal 등으로 각도 체크 가능
        float Dot = FVector::DotProduct(Hit.Normal, -OwnerChar->GetActorForwardVector());
        bIsWallClimbing = true;
    }
}

