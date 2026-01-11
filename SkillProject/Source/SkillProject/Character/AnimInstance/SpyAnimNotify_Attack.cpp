// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/AnimInstance/SpyAnimNotify_Attack.h"
#include "AbilitySystemBlueprintLibrary.h"

void USpyAnimNotify_Attack::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (AActor* Owner = MeshComp->GetOwner())
	{
		FGameplayEventData EventData;
		EventData.Instigator = Owner;
		EventData.EventTag = EventTag;

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, EventTag, EventData);
	}
}
