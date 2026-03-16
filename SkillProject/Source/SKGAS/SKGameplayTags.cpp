// Fill out your copyright notice in the Description page of Project Settings.


#include "SKGameplayTags.h"
#include "AbilitySystemComponent.h"

namespace SKGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(Lock_Input, "Lock.Input");

	UE_DEFINE_GAMEPLAY_TAG(Character_Class, "Character.Class");

	UE_DEFINE_GAMEPLAY_TAG(Character_State_Alive, "Character.State.Alive");
	UE_DEFINE_GAMEPLAY_TAG(Character_State_DeathStart, "Character.State.DeathStart");
	UE_DEFINE_GAMEPLAY_TAG(Character_State_Death, "Character.State.Death");
	UE_DEFINE_GAMEPLAY_TAG(Character_State_Skill, "Character.State.Skill");
	UE_DEFINE_GAMEPLAY_TAG(Character_State_SuperArmor, "Character.State.SuperArmor");
	UE_DEFINE_GAMEPLAY_TAG(Character_State_Combo, "Character.State.Combo");

	UE_DEFINE_GAMEPLAY_TAG(Skill_Action, "Skill.Action");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Skill_Action, "Effect.Skill.Action");

	UE_DEFINE_GAMEPLAY_TAG(Skill_Move, "Skill.Move");

	UE_DEFINE_GAMEPLAY_TAG(Skill_Buff, "Skill.Buff");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Buff_Heal, "Skill.Buff.Heal");

	UE_DEFINE_GAMEPLAY_TAG(Skill_Debuff, "Skill.Debuff");

	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Actor, "GameplayCue.Actor");

	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Static, "GameplayCue.Static");

	UE_DEFINE_GAMEPLAY_TAG(Skill_Hit_Left, "Skill.Hit.Left");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Hit_Right, "Skill.Hit.Right");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Hit_Front, "Skill.Hit.Front");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Hit_Back, "Skill.Hit.Back");

	FGameplayTag SKGameplayTags::GetSkillActionTag(const UAbilitySystemComponent* ASC)
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

	FGameplayTag SKGameplayTags::GetEffectSkillActionTag(const UAbilitySystemComponent* ASC)
	{
		if (ASC == nullptr)
			return FGameplayTag::EmptyTag;

		FGameplayTagContainer OwnedTags;
		ASC->GetOwnedGameplayTags(OwnedTags);

		FGameplayTag ParentTag = FGameplayTag::RequestGameplayTag(FName("Effect.Skill.Action"));

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