// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterAttributeSet.h"
#include "Net/UnrealNetwork.h"

void UCharacterAttributeSet::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	DOREPLIFETIME(UCharacterAttributeSet, Health);
	DOREPLIFETIME(UCharacterAttributeSet, Mana);
	DOREPLIFETIME(UCharacterAttributeSet, MaxHealth);
	DOREPLIFETIME(UCharacterAttributeSet, MaxMana);
}
