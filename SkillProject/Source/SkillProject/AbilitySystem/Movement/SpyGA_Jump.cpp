// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyGA_Jump.h"
#include "Character/SpyCharacter.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"

void USpyGA_Jump::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	CharacterJumpStart();

	UAbilityTask_WaitDelay* WaitDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, 0.15f);

	WaitDelayTask->OnFinish.AddDynamic(this, &USpyGA_Jump::CharacterJumpStop);
	WaitDelayTask->ReadyForActivation();
}

bool USpyGA_Jump::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	bool bResult = Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
	if (bResult == false)
		return false;

	const ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(ActorInfo->AvatarActor.Get());
	if (SpyCharacter == nullptr || SpyCharacter->CanJump() == false)
		return false;

	return true;
}

void USpyGA_Jump::CharacterJumpStart()
{
	if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetAvatarActorFromActorInfo()))
	{
		if (UCharacterMovementComponent* MoveComp = SpyCharacter->GetCharacterMovement())
		{
			SpyCharacter->Jump();
		}
	}
}

void USpyGA_Jump::CharacterJumpStop()
{
	if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetAvatarActorFromActorInfo()))
	{
		if (SpyCharacter->IsLocallyControlled() && SpyCharacter->bPressedJump)
		{
			SpyCharacter->StopJumping();
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}