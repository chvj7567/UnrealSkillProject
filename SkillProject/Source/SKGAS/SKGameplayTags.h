// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h"

class UAbilitySystemComponent;

// ============================================================
// SKGameplayTags — SKGAS 플러그인 공통 태그
// 게임에 종속되지 않는 재사용 가능한 부모/루트 태그를 정의합니다.
// 게임별 자식 태그는 각 프로젝트의 네임스페이스에서 확장하세요.
// ============================================================
namespace SKGameplayTags
{
	//# 입력 잠금
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Lock_Input);

	//# 캐릭터 클래스
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Class);

	//# 캐릭터 상태
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Alive);
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_DeathStart);
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Death);
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Skill);
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_SuperArmor);
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Parry);
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Combo);

	//# 액션 스킬
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Action);
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Skill_Action);

	//# 이동 스킬
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Move);

	//# 버프 스킬
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Buff);
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Buff_Heal);

	//# 디버프 스킬
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Debuff);

	//# GameplayCue
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Actor);
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Static);

	//# 피격 방향
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Hit_Left);
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Hit_Right);
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Hit_Front);
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Hit_Back);

	//# 패링
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Parry_Hit);

	SKGAS_API FGameplayTag GetSkillActionTag(const UAbilitySystemComponent* ASC);
	SKGAS_API FGameplayTag GetEffectSkillActionTag(const UAbilitySystemComponent* ASC);
}