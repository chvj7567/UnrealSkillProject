// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/SpyCharacterAttributeSet.h"
#include "Net/UnrealNetwork.h"

void USpyCharacterAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USpyCharacterAttributeSet, MoveNormalSpeed);
}

void USpyCharacterAttributeSet::OnRep_MoveNormalSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USpyCharacterAttributeSet, MoveNormalSpeed, OldValue);

	OnMoveNormalSpeedChanged.Broadcast(nullptr, nullptr, nullptr, GetMoveNormalSpeed() - OldValue.GetCurrentValue(), OldValue.GetCurrentValue(), GetMoveNormalSpeed());
}