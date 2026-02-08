// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyGA_SkillMove_HangUp.h"
#include "GameFramework/Character.h"
#include "ManagerComponent/SpyParkourManagerComponent.h"
#include "MotionWarpingComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGA_SkillMove_HangUp)

void USpyGA_SkillMove_HangUp::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (CommitAbility(Handle, ActorInfo, ActivationInfo) == false)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (TriggerEventData && TriggerEventData->TargetData.Num() > 0)
	{
		FVector LedgeLocation = TriggerEventData->TargetData.Get(0)->GetEndPoint();

		if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
		{
			if (USpyParkourManagerComponent* ParkourComponent = OwnerCharacter->FindComponentByClass<USpyParkourManagerComponent>())
			{
				if (OwnerCharacter->GetLocalRole() == ROLE_Authority)
					UE_LOG(LogTemp, Warning, TEXT("## ROLE_Authority"));
				if (OwnerCharacter->GetLocalRole() == ROLE_AutonomousProxy)
					UE_LOG(LogTemp, Warning, TEXT("## ROLE_AutonomousProxy"));

				ParkourComponent->OnHangUpMotionWarpingData.AddDynamic(this, &USpyGA_SkillMove_HangUp::OnSyncMotionWarpingData);
				ParkourComponent->SetHangUpMotionWarpingData(LedgeLocation);
			}
		}
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
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
void USpyGA_SkillMove_HangUp::OnSyncMotionWarpingData(FMotionWarpingData InHangUpData)
{
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (USpyParkourManagerComponent* ParkourComponent = OwnerCharacter->FindComponentByClass<USpyParkourManagerComponent>())
		{
			ParkourComponent->OnHangUpMotionWarpingData.RemoveDynamic(this, &USpyGA_SkillMove_HangUp::OnSyncMotionWarpingData);

			if (HasAuthority(&CurrentActivationInfo))
			{
				ParkourComponent->bFreeMoveMode = true;
			}
		}

		if (UMotionWarpingComponent* MotionWarpingComponent = OwnerCharacter->FindComponentByClass<UMotionWarpingComponent>())
		{
			MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(MotionWarpingStartName, InHangUpData.StartLoc, InHangUpData.StartRot);
			MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(MotionWarpingEndName, InHangUpData.EndLoc, InHangUpData.EndRot);
		}

		SetMoveState(true);
		PlayMontage();
	}
}