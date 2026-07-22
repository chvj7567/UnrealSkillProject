// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyGA_WallClimb.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"
#include "Util/DefineEnum.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "Character/SpyCharacterMovementComponent.h"
#include "ManagerComponent/SpyParkourManagerComponent.h"
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

    if (TryToggleClimbAction() == false)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
    }
}

void USpyGA_WallClimb::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    EndWallClimb();

    if (HasAuthority(&CurrentActivationInfo))
    {
        if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
        {
            if (USpyParkourManagerComponent* ParkourComponent = OwnerCharacter->FindComponentByClass<USpyParkourManagerComponent>())
            {
                ParkourComponent->SetFreeMoveMode(false);
            }
        }
    }

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
    if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
    {
        if (USpyParkourManagerComponent* ParkourComponent = OwnerCharacter->FindComponentByClass<USpyParkourManagerComponent>())
        {
            ParkourComponent->OnClimbData.AddUniqueDynamic(this, &USpyGA_WallClimb::StartWallClimb);
            return ParkourComponent->TryToggleClimbAction();
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

        if (USpyParkourManagerComponent* ParkourComponent = OwnerCharacter->FindComponentByClass<USpyParkourManagerComponent>())
        {
            ParkourComponent->OnClimbData.RemoveDynamic(this, &USpyGA_WallClimb::StartWallClimb);
        }
    }
}