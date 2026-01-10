// Fill out your copyright notice in the Description page of Project Settings.


#include "SKGameplayTags.h"
#include "AbilitySystemComponent.h"

namespace SKGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(Character_Class, "Character.Class");

	UE_DEFINE_GAMEPLAY_TAG(Skill_Action, "Skill.Action");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Buff, "Skill.Buff");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Debuff, "Skill.Debuff");

	UE_DEFINE_GAMEPLAY_TAG(Lock_Move, "Lock.Move");
	UE_DEFINE_GAMEPLAY_TAG(Lock_Look, "Lock.Look");

	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Actor, "GameplayCue.Actor");

	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Static, "GameplayCue.Static");


	FGameplayTag SKGameplayTags::GetActionSkillTag(const UAbilitySystemComponent* ASC)
	{
		if (ASC == nullptr)
			return FGameplayTag::EmptyTag;

		FGameplayTagContainer OwnedTags;
		ASC->GetOwnedGameplayTags(OwnedTags);

		FGameplayTag ParentTag = FGameplayTag::RequestGameplayTag(FName("Skill.Action"));

		for (const FGameplayTag& Tag : OwnedTags)
		{
			if (Tag.MatchesTag(ParentTag))
			{
				return Tag;
			}
		}

		return FGameplayTag::EmptyTag;
	}
}