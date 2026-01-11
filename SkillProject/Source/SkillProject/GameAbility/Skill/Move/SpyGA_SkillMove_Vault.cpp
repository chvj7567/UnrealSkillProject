// Fill out your copyright notice in the Description page of Project Settings.


#include "GameAbility/Skill/Move/SpyGA_SkillMove_Vault.h"
#include "ManagerComponent/SpyParkourManagerComponent.h"
#include "Character/SpyCharacter.h"
#include "GameAbility/Task/SpyAbilityTask_MotionWarping.h"
#include "MotionWarpingComponent.h"

bool USpyGA_SkillMove_Vault::CommitAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, OUT FGameplayTagContainer* OptionalRelevantTags)
{
	bool Result = Super::CommitAbility(Handle, ActorInfo, ActivationInfo, OptionalRelevantTags);

	//# 서버에서만 Vault 정보 계산
	if (HasAuthority(&CurrentActivationInfo))
	{
		if (ASpyCharacter* OwnerCharacter = Cast<ASpyCharacter>(GetAvatarActorFromActorInfo()))
		{
			if (USpyParkourManagerComponent* ParkourComponent = OwnerCharacter->GetSpyParkourManagerComponent())
			{
				return Result && ParkourComponent->CanVaultAction();
			}
		}
	}

	return Result;
}

void USpyGA_SkillMove_Vault::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (CommitAbility(Handle, ActorInfo, ActivationInfo) == false)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (ASpyCharacter* OwnerCharacter = Cast<ASpyCharacter>(GetAvatarActorFromActorInfo()))
	{
		if (USpyParkourManagerComponent* ParkourComponent = OwnerCharacter->GetSpyParkourManagerComponent())
		{
			ParkourComponent->OnVaultMotionWarpingData.AddDynamic(this, &USpyGA_SkillMove_Vault::OnSyncMotionWarpingData);

			SetMoveState(true);
			FVaultMotionWarpingData MotionWarpingData = ParkourComponent->GetVaultMotionWarpingData();
		}
	}
}

void USpyGA_SkillMove_Vault::OnSyncMotionWarpingData(FVaultMotionWarpingData InVaultData)
{

	if (ASpyCharacter* OwnerCharacter = Cast<ASpyCharacter>(GetAvatarActorFromActorInfo()))
	{
		if (USpyParkourManagerComponent* ParkourComponent = OwnerCharacter->GetSpyParkourManagerComponent())
		{
			ParkourComponent->OnVaultMotionWarpingData.RemoveDynamic(this, &USpyGA_SkillMove_Vault::OnSyncMotionWarpingData);
		}

		if (UMotionWarpingComponent* MotionWarpingComponent = OwnerCharacter->GetMotionWarpingComponent())
		{
			MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(MotionWarpingStartName, InVaultData.StartLoc, InVaultData.StartRot);
			MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(MotionWarpingEndName, InVaultData.EndLoc, InVaultData.EndRot);
		}
	}

	PlayMontage();
}
