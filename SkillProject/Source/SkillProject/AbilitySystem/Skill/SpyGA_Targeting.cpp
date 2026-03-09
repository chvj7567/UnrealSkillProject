// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyGA_Targeting.h"
#include "GameFramework/Character.h"
#include "ManagerComponent/SpyTargetingManagerComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGA_Targeting)

void USpyGA_Targeting::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (HasAuthority(&CurrentActivationInfo))
	{
		if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
		{
			//# 서버에서 타겟 Find
			if (USpyTargetingManagerComponent* TargetingComp = OwnerCharacter->FindComponentByClass<USpyTargetingManagerComponent>())
			{
				TargetingComp->FindTarget(100.f);
			}
		}
	}
}
