// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyGameplayTags.h"
#include "AbilitySystemComponent.h"

namespace SpyGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(Character_A, "Character.A");
	UE_DEFINE_GAMEPLAY_TAG(Character_B, "Character.B");
	UE_DEFINE_GAMEPLAY_TAG(Character_C, "Character.C");
	UE_DEFINE_GAMEPLAY_TAG(Character_D, "Character.D");
	UE_DEFINE_GAMEPLAY_TAG(Character_E, "Character.E");

	UE_DEFINE_GAMEPLAY_TAG(Skill_A, "Skill.A");
	UE_DEFINE_GAMEPLAY_TAG(Skill_B, "Skill.B");
	UE_DEFINE_GAMEPLAY_TAG(Skill_C, "Skill.C");
	UE_DEFINE_GAMEPLAY_TAG(Skill_D, "Skill.D");
	UE_DEFINE_GAMEPLAY_TAG(Skill_E, "Skill.E");

	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Actor_Fire, "GameplayCue.Actor.Fire");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Actor_Heal, "GameplayCue.Actor.Heal");

	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Static_Hit, "GameplayCue.Static.Hit");

	FGameplayTag SpyGameplayTags::GetActiveSkillTag(const UAbilitySystemComponent* ASC)
	{
		if (ASC == nullptr)
			return FGameplayTag::EmptyTag;

		static TArray<FGameplayTag> SkillTags = {
			Skill_A,
			Skill_B,
			Skill_C,
			Skill_D,
			Skill_E };

		for (const FGameplayTag& Tag : SkillTags)
		{
			if (ASC->HasMatchingGameplayTag(Tag))
			{
				return Tag;
			}
		}

		return FGameplayTag::EmptyTag;
	}
}