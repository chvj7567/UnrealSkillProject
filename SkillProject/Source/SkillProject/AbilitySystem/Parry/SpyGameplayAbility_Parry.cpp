// Fill out your copyright notice in the Description page of Project Settings.

#include "SpyGameplayAbility_Parry.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameFramework/Character.h"
#include "SKGameplayTags.h"
#include "System/SpyPlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGameplayAbility_Parry)

USpyGameplayAbility_Parry::USpyGameplayAbility_Parry()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void USpyGameplayAbility_Parry::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (CommitAbility(Handle, ActorInfo, ActivationInfo) == false)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (HasAuthority(&ActivationInfo))
	{
		ActorInfo->AbilitySystemComponent->AddLooseGameplayTag(SKGameplayTags::Character_State_Parry);

		if (UAbilityTask_WaitGameplayEvent* WaitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, SKGameplayTags::Skill_Parry_Hit, nullptr, false, false))
		{
			WaitTask->EventReceived.AddDynamic(this, &USpyGameplayAbility_Parry::OnWaitGameplayEvent);
			WaitTask->ReadyForActivation();
		}
	}

	if (ParryMontage)
	{
		if (UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, ParryMontage))
		{
			MontageTask->OnInterrupted.AddDynamic(this, &USpyGameplayAbility_Parry::OnMontageInterrupted);
			MontageTask->OnCancelled.AddDynamic(this, &USpyGameplayAbility_Parry::OnMontageCancelled);
			MontageTask->ReadyForActivation();
		}
	}
}

void USpyGameplayAbility_Parry::InputReleased(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	CancelAbility(Handle, ActorInfo, ActivationInfo, true);
}

void USpyGameplayAbility_Parry::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (HasAuthority(&ActivationInfo))
	{
		UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
		if (ASC && ASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Parry))
		{
			ASC->RemoveLooseGameplayTag(SKGameplayTags::Character_State_Parry);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USpyGameplayAbility_Parry::OnWaitGameplayEvent(FGameplayEventData Payload)
{
	// Super::OnWaitGameplayEvent 호출 금지 — 기본 구현은 EndAbility를 호출함
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (OwnerCharacter == nullptr)
		return;

	if (HasAuthority(&CurrentActivationInfo) == false)
		return;

	FVector KnockbackDir = -OwnerCharacter->GetActorForwardVector();
	OwnerCharacter->LaunchCharacter(KnockbackDir * KnockbackForce, true, false);

	// 패링 성공 시 방어자/공격자 양쪽 카메라 쉐이크 — 로컬 컨트롤러는 RPC 우회
	if (ASpyPlayerController* DefenderPC = Cast<ASpyPlayerController>(OwnerCharacter->GetController()))
	{
		if (DefenderPC->IsLocalController())
			DefenderPC->TriggerShakeLocal(false, true);
		else
			DefenderPC->Client_TriggerShake(false, true);
	}

	AActor* InstigatorActor = const_cast<AActor*>(Payload.Instigator.Get());
	if (APawn* InstigatorPawn = Cast<APawn>(InstigatorActor))
	{
		if (ASpyPlayerController* AttackerPC = Cast<ASpyPlayerController>(InstigatorPawn->GetController()))
		{
			if (AttackerPC->IsLocalController())
				AttackerPC->TriggerShakeLocal(false, false);
			else
				AttackerPC->Client_TriggerShake(false, false);
		}
	}
}
