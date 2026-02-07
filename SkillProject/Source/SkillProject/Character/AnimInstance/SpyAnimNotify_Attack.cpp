// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyAnimNotify_Attack.h"
#include "AbilitySystemBlueprintLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyAnimNotify_Attack)

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
