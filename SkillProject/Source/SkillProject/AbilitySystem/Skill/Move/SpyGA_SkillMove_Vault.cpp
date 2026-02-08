// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyGA_SkillMove_Vault.h"
#include "ManagerComponent/SpyParkourManagerComponent.h"
#include "GameFramework/Character.h"
#include "AbilitySystem/Task/SpyAbilityTask_MotionWarping.h"
#include "MotionWarpingComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGA_SkillMove_Vault)

void USpyGA_SkillMove_Vault::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (CommitAbility(Handle, ActorInfo, ActivationInfo) == false)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (USpyParkourManagerComponent* ParkourComponent = OwnerCharacter->FindComponentByClass<USpyParkourManagerComponent>())
		{
			if (ParkourComponent->CanVaultAction())
			{
				ParkourComponent->OnVaultMotionWarpingData.AddDynamic(this, &USpyGA_SkillMove_Vault::OnSyncMotionWarpingData);
				ParkourComponent->SetVaultMotionWarpingData();
			}
			else
			{
				EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			}
		}
	}
}

void USpyGA_SkillMove_Vault::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
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

void USpyGA_SkillMove_Vault::OnSyncMotionWarpingData(FVaultMotionWarpingData InVaultData)
{
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (USpyParkourManagerComponent* ParkourComponent = OwnerCharacter->FindComponentByClass<USpyParkourManagerComponent>())
		{
			ParkourComponent->OnVaultMotionWarpingData.RemoveDynamic(this, &USpyGA_SkillMove_Vault::OnSyncMotionWarpingData);

			if (HasAuthority(&CurrentActivationInfo))
			{
				ParkourComponent->bFreeMoveMode = true;
			}
		}

		if (UMotionWarpingComponent* MotionWarpingComponent = OwnerCharacter->FindComponentByClass<UMotionWarpingComponent>())
		{
			MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(MotionWarpingStartName, InVaultData.StartLoc, InVaultData.StartRot);
			MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(MotionWarpingEndName, InVaultData.EndLoc, InVaultData.EndRot);
		}

		SetMoveState(true);
		PlayMontage();
	}
}
