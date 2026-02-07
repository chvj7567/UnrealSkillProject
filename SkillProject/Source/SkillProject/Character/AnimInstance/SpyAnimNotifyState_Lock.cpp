// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyAnimNotifyState_Lock.h"
#include "Character/SpyCharacter.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Util/SpyGameplayTags.h"
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyAnimNotifyState_Lock)

void USpyAnimNotifyState_Lock::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp->GetOwner()))
		{
			if (bLockMove)
			{
				ASC->AddLooseGameplayTag(SpyGameplayTags::Lock_Input_Move);
			}

			if (bLockLook)
			{
				ASC->AddLooseGameplayTag(SpyGameplayTags::Lock_Input_Look);
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
				ASC->RemoveLooseGameplayTag(SpyGameplayTags::Lock_Input_Move);
			}

			if (bLockLook)
			{
				ASC->RemoveLooseGameplayTag(SpyGameplayTags::Lock_Input_Look);
			}
		}
	}
}
