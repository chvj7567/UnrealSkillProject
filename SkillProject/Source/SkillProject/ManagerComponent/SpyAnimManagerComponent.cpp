// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyAnimManagerComponent.h"
#include "Character/AnimInstance/CharacterAnimInstance.h"

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

void USpyAnimManagerComponent::Initialize(UCharacterAnimInstance* InAnimInstance)
{
	AnimInstance = InAnimInstance;
}

