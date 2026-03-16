// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h"

class UAbilitySystemComponent;

namespace SKGameplayTags
{
	//# 잠금 관련
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Lock_Input);

	//# 캐릭터 클래스
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Class);

	//# 캐릭터 상태
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Alive);
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_DeathStart);
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Death);
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Skill);
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_SuperArmor);
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Combo);

	//# 액션 스킬
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Action);
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Skill_Action);

	//# 이동 스킬
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Move);
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Move);

	//# 버프 스킬
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Buff);
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Buff_Heal);

	//# 디버프 스킬
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Debuff);

	//# Actor 이펙트
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Actor);

	//# Static 이펙트
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Static);

	//# Hit
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Hit_Left);
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Hit_Right);
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Hit_Front);
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Hit_Back);

	SKGAS_API FGameplayTag GetSkillActionTag(const UAbilitySystemComponent* ASC);
	SKGAS_API FGameplayTag GetEffectSkillActionTag(const UAbilitySystemComponent* ASC);
}