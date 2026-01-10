// Fill out your copyright notice in the Description page of Project Settings.


#include "GameAbility/Skill/Move/SpyGA_SkillMove_Vault.h"
#include "ManagerComponent/SpyParkourManagerComponent.h"
#include "Character/SpyCharacter.h"

bool USpyGA_SkillMove_Vault::CommitAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, OUT FGameplayTagContainer* OptionalRelevantTags)
{
	bool Result = Super::CommitAbility(Handle, ActorInfo, ActivationInfo, OptionalRelevantTags);

	if (ASpyCharacter* OwnerCharacter = Cast<ASpyCharacter>(GetAvatarActorFromActorInfo()))
	{
		if (USpyParkourManagerComponent* ParkourComponent = OwnerCharacter->GetSpyParkourManagerComponent())
		{
			return Result && ParkourComponent->CanVaultAction();
		}
	}

	return Result;
}
