// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyAnimManagerComponent.h"
#include "Character/AnimInstance/SpyCharacterAnimInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyAnimManagerComponent)

USpyAnimManagerComponent::USpyAnimManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void USpyAnimManagerComponent::BeginPlay()
{
	Super::BeginPlay();

}

void USpyAnimManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void USpyAnimManagerComponent::Initialize(USpyCharacterAnimInstance* InAnimInstance)
{
	AnimInstance = InAnimInstance;
}

float USpyAnimManagerComponent::CalcDirectionFromVelocity(
	const FVector& WorldVelocity, const FRotator& ActorRotation)
{
	if (WorldVelocity.IsNearlyZero()) return 0.f;
	FVector LocalVel = ActorRotation.UnrotateVector(WorldVelocity);
	return FMath::RadiansToDegrees(FMath::Atan2(LocalVel.Y, LocalVel.X));
}

