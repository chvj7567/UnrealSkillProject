// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyGA_SkillMove_HangUp.h"
#include "GameFramework/Character.h"
#include "ManagerComponent/SpyParkourManagerComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGA_SkillMove_HangUp)

void USpyGA_SkillMove_HangUp::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (CommitAbility(Handle, ActorInfo, ActivationInfo) == false)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	SetMoveState(true);

	if (HasAuthority(&CurrentActivationInfo))
	{
		if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
		{
			if (USpyParkourManagerComponent* ParkourComponent = OwnerCharacter->FindComponentByClass<USpyParkourManagerComponent>())
			{
				ParkourComponent->bFreeMoveMode = true;
			}
		}
	}

	PlayMontage();
}

void USpyGA_SkillMove_HangUp::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (HasAuthority(&CurrentActivationInfo))
	{
		if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
		{
			if (USpyParkourManagerComponent* ParkourComponent = OwnerCharacter->FindComponentByClass<USpyParkourManagerComponent>())
			{
				ParkourComponent->bFreeMoveMode = false;
			}
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
}
