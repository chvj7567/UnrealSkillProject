// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyGA_SkillMove_HangUp.h"
#include "GameFramework/Character.h"
#include "ManagerComponent/SpyParkourManagerComponent.h"
#include "MotionWarpingComponent.h"
#include "Character/SpyCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGA_SkillMove_HangUp)

void USpyGA_SkillMove_HangUp::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	//# 매달려 오르는 동안 캐릭터가 벽에 밀착해 SpringArm 프로브가 벽 안에서 시작되고 팔 길이가 붕괴한다.
	//# bDoCollisionTest 는 레플리케이트되지 않는 로컬 카메라 연출이라 Authority 블록 밖에서 처리한다.
	//# 어떤 조기 종료 경로로 빠져도 EndAbility 가 짝을 맞추도록 커밋 판정보다 먼저 건다.
	if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetAvatarActorFromActorInfo()))
	{
		SpyCharacter->PushCameraCollisionSuppress();
		CameraSuppressedCharacter = SpyCharacter;
	}

	if (CommitAbility(Handle, ActorInfo, ActivationInfo) == false)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (TriggerEventData && TriggerEventData->TargetData.Num() > 0)
	{
		FVector LedgeLocation = TriggerEventData->TargetData.Get(0)->GetEndPoint();

		if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
		{
			if (USpyParkourManagerComponent* ParkourComponent = OwnerCharacter->FindComponentByClass<USpyParkourManagerComponent>())
			{
				ParkourComponent->OnHangUpMotionWarpingData.AddDynamic(this, &USpyGA_SkillMove_HangUp::OnSyncMotionWarpingData);
				ParkourComponent->SetHangUpMotionWarpingData(LedgeLocation);
			}
		}
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
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
				//# 이 GA 가 FreeMoveMode 를 켠 적이 있을 때만 되돌린다.
				//# (매달리기가 성립하지 않아 워핑 데이터가 산출되지 않은 경우에는
				//#  켠 적이 없으므로 이동 모드·캡슐 충돌을 건드리지 않는다)
				if (bFreeMoveEngaged)
				{
					ParkourComponent->SetFreeMoveMode(false);
				}
			}
		}
	}

	bFreeMoveEngaged = false;

	//# 취소·중단·사망 경로도 전부 EndAbility 로 흘러오므로 여기서만 해제하면 카운트가 새지 않는다.
	if (ASpyCharacter* SuppressedCharacter = CameraSuppressedCharacter.Get())
	{
		SuppressedCharacter->PopCameraCollisionSuppress();
	}
	CameraSuppressedCharacter = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
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
				ParkourComponent->SetFreeMoveMode(true);
				bFreeMoveEngaged = true;
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