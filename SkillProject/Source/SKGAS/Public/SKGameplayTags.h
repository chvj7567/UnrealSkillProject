// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h"

class UAbilitySystemComponent;

namespace SKGameplayTags
{
	//# 캐릭터 클래스
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Class);

	//# 액션 스킬
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Action);

	//# 버프 스킬
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Buff);

	//# 디버프 스킬
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Debuff);

	//# Lock
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Lock_Move);
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Lock_Look);

	//# Actor 이펙트
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Actor);

	//# Static 이펙트
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Static);


	SKGAS_API FGameplayTag GetActionSkillTag(const UAbilitySystemComponent* ASC);
}