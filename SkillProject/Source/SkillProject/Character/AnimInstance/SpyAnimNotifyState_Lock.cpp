// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/AnimInstance/SpyAnimNotifyState_Lock.h"
#include "Character/SpyCharacter.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "SKGameplayTags.h"

void USpyAnimNotifyState_Lock::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp->GetOwner()))
		{
			if (bLockMove)
			{
				ASC->AddLooseGameplayTag(SKGameplayTags::Lock_Move);
			}

			if (bLockLook)
			{
				ASC->AddLooseGameplayTag(SKGameplayTags::Lock_Look);
			}
		}
	}
}

void USpyAnimNotifyState_Lock::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp->GetOwner()))
		{
			if (bLockMove)
			{
				ASC->RemoveLooseGameplayTag(SKGameplayTags::Lock_Move);
			}

			if (bLockLook)
			{
				ASC->RemoveLooseGameplayTag(SKGameplayTags::Lock_Look);
			}
		}
	}
}
