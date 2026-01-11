// Fill out your copyright notice in the Description page of Project Settings.


#include "GameAbility/Skill/Move/SpyGA_SkillMove_Vault.h"
#include "ManagerComponent/SpyParkourManagerComponent.h"
#include "Character/SpyCharacter.h"
#include "GameAbility/Task/SpyAbilityTask_MotionWarping.h"
#include "MotionWarpingComponent.h"

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

void USpyGA_SkillMove_Vault::PrePlayMontage()
{
	if (HasAuthority(&CurrentActivationInfo))
	{
		if (ASpyCharacter* OwnerCharacter = Cast<ASpyCharacter>(GetAvatarActorFromActorInfo()))
		{
			if (USpyParkourManagerComponent* ParkourComponent = OwnerCharacter->GetSpyParkourManagerComponent())
			{
				FVaultMotionWarpingData MotionWarpingData = ParkourComponent->GetVaultMotionWarpingData();

				USpyAbilityTask_MotionWarping* SyncTask = USpyAbilityTask_MotionWarping::CreateMotionWarpingSyncTask(
					this, MotionWarpingData.StartLoc, MotionWarpingData.StartRot, MotionWarpingData.EndLoc, MotionWarpingData.EndRot);
				SyncTask->OnSyncMotionWarpingData.AddDynamic(this, &USpyGA_SkillMove_Vault::OnSyncMotionWarpingData);
				SyncTask->ReadyForActivation();
			}
		}
	}
}

void USpyGA_SkillMove_Vault::OnSyncMotionWarpingData(FVector StartLoc, FRotator StartRot, FVector EndLoc, FRotator EndRot)
{
	if (ASpyCharacter* OwnerCharacter = Cast<ASpyCharacter>(GetAvatarActorFromActorInfo()))
	{
		if (UMotionWarpingComponent* MotionWarpingComponent = OwnerCharacter->GetMotionWarpingComponent())
		{
			MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(MotionWarpingStartName, StartLoc, StartRot);
			MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(MotionWarpingEndName, EndLoc, EndRot);
		}
	}
}
