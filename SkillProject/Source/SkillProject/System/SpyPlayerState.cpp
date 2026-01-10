// Fill out your copyright notice in the Description page of Project Settings.


#include "System/SpyPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Character/SpyCharacter.h"
#include "UI/SpyHPBar.h"
#include "UI/SpyUserWidget.h"
#include "Util/SpyGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyPlayerState)

ASpyPlayerState::ASpyPlayerState()
{
}

void ASpyPlayerState::Multicast_Death_Implementation()
{
	RemoveState(SpyGameplayTags::Character_State_Survival_Alive);
	OwnerCharacter->Death();
}

void ASpyPlayerState::Initialize()
{
	if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetPawn()))
	{
		OwnerCharacter = SpyCharacter;
	}
}

bool ASpyPlayerState::HasState(FGameplayTag Tag)
{
	if (OwnerCharacter == nullptr)
		return false;

	if (OwnerCharacter->GetSKAbilitySystemComponent()->HasMatchingGameplayTag(Tag))
		return true;

	return false;
}

void ASpyPlayerState::AddState(FGameplayTag Tag)
{
	if (OwnerCharacter == nullptr)
		return;

	if (OwnerCharacter->HasAuthority() == false)
		return;

	if (HasState(Tag))
		return;

	UE_LOG(LogTemp, Warning, TEXT("# Server AddState %s"), *Tag.ToString());
	OwnerCharacter->GetSKAbilitySystemComponent()->AddReplicatedLooseGameplayTag(Tag);
}

void ASpyPlayerState::RemoveState(FGameplayTag Tag)
{
	if (OwnerCharacter == nullptr)
		return;

	if (OwnerCharacter->HasAuthority() == false)
		return;

	if (HasState(Tag) == false)
		return;

	UE_LOG(LogTemp, Warning, TEXT("# Server RemoveState %s"), * Tag.ToString());
	OwnerCharacter->GetSKAbilitySystemComponent()->RemoveReplicatedLooseGameplayTag(Tag);
}

void ASpyPlayerState::ToggleState(FGameplayTag Tag)
{
	if (HasState(Tag))
	{
		RemoveState(Tag);
	}
	else
	{
		AddState(Tag);
	}
}
