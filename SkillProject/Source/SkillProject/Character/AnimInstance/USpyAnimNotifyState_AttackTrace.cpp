// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/AnimInstance/USpyAnimNotifyState_AttackTrace.h"
#include "Character/SpyCharacter.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "SKGameplayTags.h"

void UUSpyAnimNotifyState_AttackTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp == nullptr)
		return;

	AActor* Owner = MeshComp->GetOwner();
	if (Owner == nullptr)
		return;

	if (Owner->HasAuthority() == false)
		return;

	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
	{
		ASC->AddReplicatedLooseGameplayTag(SkillTag);
	}
}

void UUSpyAnimNotifyState_AttackTrace::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp == nullptr)
		return;

	AActor* Owner = MeshComp->GetOwner();
	if (Owner == nullptr)
		return;

	if (Owner->HasAuthority() == false)
		return;

	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
	{
		ASC->RemoveReplicatedLooseGameplayTag(SkillTag);
	}
}