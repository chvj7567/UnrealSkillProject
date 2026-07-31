// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyGA_WallClimb.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"
#include "Util/DefineEnum.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "Character/SpyCharacter.h"
#include "Character/SpyCharacterMovementComponent.h"
#include "Character/CommonInterface.Character.h"
#include "System/SpyPlayerController.h"
#include "Input/SpyInputComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "System/SpyMissionComponent.h"
#include "Util/SpyGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGA_WallClimb)

USpyGA_WallClimb::USpyGA_WallClimb()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void USpyGA_WallClimb::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	CurrentSpecHandle = Handle;
	CurrentActorInfo = ActorInfo;
	CurrentActivationInfo = ActivationInfo;

	//# 등반 중에는 캐릭터가 벽에 밀착해 SpringArm 프로브가 벽 안에서 시작되고 팔 길이가 붕괴한다.
	//# bDoCollisionTest 는 레플리케이트되지 않는 로컬 카메라 연출이라 Authority 블록 밖에서 처리한다.
	//# 어떤 조기 종료 경로로 빠져도 EndAbility 가 짝을 맞추도록 등반 판정보다 먼저 건다.
	if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetAvatarActorFromActorInfo()))
	{
		SpyCharacter->PushCameraCollisionSuppress();
		CameraSuppressedCharacter = SpyCharacter;
	}

	if (TryToggleClimbAction() == false)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void USpyGA_WallClimb::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	EndWallClimb();

	//# 등반은 FreeMoveMode 를 켜지 않는다 (MOVE_Custom/WallClimb 로만 동작).
	//# 따라서 여기서 SetFreeMoveMode(false) 를 부르면 켠 적 없는 상태를 끄는 셈이 되어,
	//# 벽이 없는 공중 입력 시에도 낙하 중 캐릭터를 MOVE_Walking 으로 스냅시킨다.
	//# 이동 모드 복구는 EndWallClimb(bWallClimbing 가드) 가 담당한다.

	//# 취소·중단·사망 경로도 전부 EndAbility 로 흘러오므로 여기서만 해제하면 카운트가 새지 않는다.
	if (ASpyCharacter* SuppressedCharacter = CameraSuppressedCharacter.Get())
	{
		SuppressedCharacter->PopCameraCollisionSuppress();
	}
	CameraSuppressedCharacter = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USpyGA_WallClimb::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
    //# 토글 기능 — 같은 프레임의 Held 루프가 재활성화 시키지 못하도록 입력을 먼저 소비한다.
    if (IsActive() == false)
        return;

    if (USpyAbilitySystemComponent* SpyASC = Cast<USpyAbilitySystemComponent>(ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr))
    {
        SpyASC->ConsumeInputForHandle(Handle);
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

bool USpyGA_WallClimb::TryToggleClimbAction()
{
	if (ISpyCharacterRoot* RootPtr = Cast<ISpyCharacterRoot>(GetAvatarActorFromActorInfo()))
	{
		if (ISpyParkourHost* Parkour = RootPtr->GetParkourHost().GetInterface())
		{
			Parkour->OnClimb().AddUniqueDynamic(this, &USpyGA_WallClimb::StartWallClimb);
			return Parkour->TryToggleClimbAction();
		}
	}

	return false;
}

void USpyGA_WallClimb::StartWallClimb(const FClimbData& InClimbData, const FClimbWallData& InClimbWallData)
{
    if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
    {
        if (USpyCharacterMovementComponent* MoveComp = Cast<USpyCharacterMovementComponent>(OwnerCharacter->GetCharacterMovement()))
        {
            MoveComp->StartWallClimb(InClimbData, InClimbWallData);
        }

        if (IsPredictingClient())
        {
            if (USpyInputComponent* InputComp = USpyInputComponent::FindInputComponent(OwnerCharacter))
            {
                InputComp->AddMappingContext(InputMappingContext, 99);
            }
        }

		//# 미션 진행 — 등반이 실제로 시작된 시점. 벽 트레이스가 적중해야만 도달한다.
		//# TryToggleClimbAction 의 반환값을 쓰지 않는 이유: 그 함수는 진행 신호가 아니라 트레이스 결과이고,
		//# 등반 종료는 InputPressed → EndAbility 경로라 이 콜백은 1회 등반당 한 번만 불린다.
		//# 서버 발화 근거: TryToggleClimbAction 이 HasAuthority 분기에서 OnRep_ClimbWallData 를 직접 호출한다
		//# (SpyParkourManagerComponent.cpp:112-127)
		if (HasAuthority(&CurrentActivationInfo))
		{
			if (USpyMissionComponent* MissionComp = USpyMissionComponent::FindMissionComponent(OwnerCharacter->GetPlayerState()))
			{
				MissionComp->AddProgress(SpyGameplayTags::Skill_Move_Climb, 1);
			}
		}
	}
}

void USpyGA_WallClimb::EndWallClimb()
{
    if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
    {
        if (USpyCharacterMovementComponent* MoveComp = Cast<USpyCharacterMovementComponent>(OwnerCharacter->GetCharacterMovement()))
        {
            MoveComp->EndWallClimb();
        }

        if (IsPredictingClient())
        {
            if (USpyInputComponent* InputComp = USpyInputComponent::FindInputComponent(OwnerCharacter))
            {
                InputComp->RemoveMappingContext(InputMappingContext);
            }
        }

		if (ISpyCharacterRoot* RootPtr = Cast<ISpyCharacterRoot>(OwnerCharacter))
		{
			if (ISpyParkourHost* Parkour = RootPtr->GetParkourHost().GetInterface())
			{
				Parkour->OnClimb().RemoveDynamic(this, &USpyGA_WallClimb::StartWallClimb);
			}
		}
    }
}