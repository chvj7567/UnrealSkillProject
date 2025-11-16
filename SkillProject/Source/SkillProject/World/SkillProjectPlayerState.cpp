// Fill out your copyright notice in the Description page of Project Settings.


#include "World/SkillProjectPlayerState.h"
#include "Net/UnrealNetwork.h"

ASkillProjectPlayerState::ASkillProjectPlayerState()
{
	PlayerFlags = ESpyPlayerStateFlags::None;
}

void ASkillProjectPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASkillProjectPlayerState, PlayerFlags);
}

void ASkillProjectPlayerState::Multicast_Death_Implementation()
{
	RemoveState(ESpyPlayerStateFlags::IsAlive);
}

bool ASkillProjectPlayerState::HasState(ESpyPlayerStateFlags Flag) const
{
	return EnumHasAllFlags(PlayerFlags, Flag);
}

void ASkillProjectPlayerState::AddState(ESpyPlayerStateFlags Flag)
{
	PlayerFlags |= Flag;
}

void ASkillProjectPlayerState::RemoveState(ESpyPlayerStateFlags Flag)
{
	PlayerFlags &= ~Flag;
}

void ASkillProjectPlayerState::ToggleState(ESpyPlayerStateFlags Flag)
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
