// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyGA_SkillMove_Vault.h"
#include "ManagerComponent/SpyParkourManagerComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"
#include "MotionWarpingComponent.h"
#include "Perception/AISense_Hearing.h"
#include "Data/SpyAssetNames.h"
#include "System/SpyMissionComponent.h"
#include "Util/SpyGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGA_SkillMove_Vault)

void USpyGA_SkillMove_Vault::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (CommitAbility(Handle, ActorInfo, ActivationInfo) == false)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
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

	AActor* Avatar = ActorInfo->AvatarActor.Get();
	if (Avatar == nullptr) return;

	UAISense_Hearing::ReportNoiseEvent(
		Avatar->GetWorld(),
		Avatar->GetActorLocation(),
		2.0f,
		Avatar,
		0.f,
		SpyAINames::AttackNoise
	);
}

void USpyGA_SkillMove_Vault::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (HasAuthority(&CurrentActivationInfo))
	{
		if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
		{
			if (USpyParkourManagerComponent* ParkourComponent = OwnerCharacter->FindComponentByClass<USpyParkourManagerComponent>())
			{
				//# 이 GA 가 FreeMoveMode 를 켠 적이 있을 때만 되돌린다.
				//# (넘기가 성립하지 않아 워핑 데이터가 나오지 않은 경우 — 공중 입력 등 — 에는
				//#  켠 적이 없으므로 이동 모드·캡슐 충돌을 건드리지 않는다)
				if (bFreeMoveEngaged)
				{
					ParkourComponent->SetFreeMoveMode(false);
				}
			}
		}
	}

	bFreeMoveEngaged = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USpyGA_SkillMove_Vault::OnSyncMotionWarpingData(FMotionWarpingData InVaultData)
{
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (USpyParkourManagerComponent* ParkourComponent = OwnerCharacter->FindComponentByClass<USpyParkourManagerComponent>())
		{
			ParkourComponent->OnVaultMotionWarpingData.RemoveDynamic(this, &USpyGA_SkillMove_Vault::OnSyncMotionWarpingData);

			if (HasAuthority(&CurrentActivationInfo))
			{
				ParkourComponent->SetFreeMoveMode(true);
				bFreeMoveEngaged = true;

				//# 미션 진행 — 워핑 데이터가 실제로 산출된 시점이라 "넘기를 수행했다"가 성립한다.
				//# 활성화 시점(ActivateAbility)은 벽이 없어도 진입하므로 쓰지 않는다.
				//# 서버에서는 SetVaultMotionWarpingData 가 HasAuthority 분기에서 OnRep_VaultMotionWarpingData 를
				//# 직접 호출하므로 이 콜백이 서버에서도 확실히 발화한다 (SpyParkourManagerComponent.cpp:233-244)
				if (USpyMissionComponent* MissionComp = USpyMissionComponent::FindMissionComponent(OwnerCharacter->GetPlayerState()))
				{
					MissionComp->AddProgress(SpyGameplayTags::Skill_Move_Vault, 1);
				}
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
