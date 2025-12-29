// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h"

class UAbilitySystemComponent;

namespace SpyGameplayTags
{
	//# 캐릭터 클래스
	SPYGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Normal);

	//# 스킬 종류
	SPYGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_A);
	SPYGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_B);
	SPYGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_C);
	SPYGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_D);
	SPYGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_E);

	//# Lock
	SPYGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Lock_Move);
	SPYGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Lock_Look);

	//# Actor 이펙트
	SPYGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Actor_Fire);
	SPYGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Actor_Heal);

	//# Static 이펙트
	SPYGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Static_Hit);


	SPYGAS_API FGameplayTag GetActiveSkillTag(const UAbilitySystemComponent* ASC);
}