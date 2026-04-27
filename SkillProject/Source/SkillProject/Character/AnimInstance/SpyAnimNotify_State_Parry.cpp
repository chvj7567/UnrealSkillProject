// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyAnimNotify_State_Parry.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "SKGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyAnimNotify_State_Parry)

void USpyAnimNotify_State_Parry::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp->GetOwner()))
		{
			ASC->AddLooseGameplayTag(SKGameplayTags::Character_State_Parry);
		}
	}
}

void USpyAnimNotify_State_Parry::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp->GetOwner()))
		{
			ASC->RemoveLooseGameplayTag(SKGameplayTags::Character_State_Parry);
		}
	}
}
