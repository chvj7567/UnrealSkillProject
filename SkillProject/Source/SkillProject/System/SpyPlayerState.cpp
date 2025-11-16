// Fill out your copyright notice in the Description page of Project Settings.


#include "System/SpyPlayerState.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyPlayerState)

ASpyPlayerState::ASpyPlayerState()
{
	PlayerFlags = ESpyPlayerStateFlags::None;
}

void ASpyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASpyPlayerState, PlayerFlags);
}

void ASpyPlayerState::Multicast_Death_Implementation()
{
	RemoveState(ESpyPlayerStateFlags::IsAlive);
}

bool ASpyPlayerState::HasState(ESpyPlayerStateFlags Flag) const
{
	return EnumHasAllFlags(PlayerFlags, Flag);
}

void ASpyPlayerState::AddState(ESpyPlayerStateFlags Flag)
{
	PlayerFlags |= Flag;
}

void ASpyPlayerState::RemoveState(ESpyPlayerStateFlags Flag)
{
	PlayerFlags &= ~Flag;
}

void ASpyPlayerState::ToggleState(ESpyPlayerStateFlags Flag)
{
	if (HasState(Flag))
	{
		RemoveState(Flag);
	}
	else
	{
		AddState(Flag);
	}
}
