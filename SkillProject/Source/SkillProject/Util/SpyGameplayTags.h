// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h"

namespace SpyGameplayTags
{
	//# 캐릭터 클래스
	SKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Class_Normal);

	//# 액션 스킬
	SKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Action_A);
	SKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Action_B);
	SKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Action_C);
	SKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Action_D);
	SKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Action_E);

	//# Actor 이펙트
	SKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Actor_Fire);
	SKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Actor_Heal);

	//# Static 이펙트
	SKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Static_Hit);
}