// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyGameplayTags.h"

namespace SpyGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(InitState_Spawned, "InitState.Spawned");
	UE_DEFINE_GAMEPLAY_TAG(InitState_DataAvailable, "InitState.DataAvailable");
	UE_DEFINE_GAMEPLAY_TAG(InitState_DataInitialized, "InitState.DataInitialized");
	UE_DEFINE_GAMEPLAY_TAG(InitState_GameplayReady, "InitState.GameplayReady");

	UE_DEFINE_GAMEPLAY_TAG(Character_Class_Normal, "Character.Class.Normal");

	UE_DEFINE_GAMEPLAY_TAG(Character_State_Survival_Alive, "Character.State.Survival.Alive");

	UE_DEFINE_GAMEPLAY_TAG(Character_State_Movement_Normal, "Character.State.Movement.Normal");
	UE_DEFINE_GAMEPLAY_TAG(Character_State_Movement_Climb, "Character.State.Movement.Climb");

	UE_DEFINE_GAMEPLAY_TAG(Input_Skill_1, "Input.Skill.1");
	UE_DEFINE_GAMEPLAY_TAG(Input_Skill_2, "Input.Skill.2");
	UE_DEFINE_GAMEPLAY_TAG(Input_Skill_3, "Input.Skill.3");
	UE_DEFINE_GAMEPLAY_TAG(Input_Skill_4, "Input.Skill.4");
	UE_DEFINE_GAMEPLAY_TAG(Input_Skill_5, "Input.Skill.5");

	UE_DEFINE_GAMEPLAY_TAG(Skill_Action_A, "Skill.Action.A");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Skill_Action_A, "Effect.Skill.Action.A");
	UE_DEFINE_GAMEPLAY_TAG(Notify_Skill_Action_A, "Notify.Skill.Action.A");

	UE_DEFINE_GAMEPLAY_TAG(Skill_Action_B, "Skill.Action.B");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Skill_Action_B, "Effect.Skill.Action.B");
	UE_DEFINE_GAMEPLAY_TAG(Notify_Skill_Action_B, "Notify.Skill.Action.B");

	UE_DEFINE_GAMEPLAY_TAG(Skill_Action_C, "Skill.Action.C");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Skill_Action_C, "Effect.Skill.Action.C");
	UE_DEFINE_GAMEPLAY_TAG(Notify_Skill_Action_C, "Notify.Skill.Action.C");

	UE_DEFINE_GAMEPLAY_TAG(Skill_Action_D, "Skill.Action.D");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Skill_Action_D, "Effect.Skill.Action.D");
	UE_DEFINE_GAMEPLAY_TAG(Notify_Skill_Action_D, "Notify.Skill.Action.D");
	
	UE_DEFINE_GAMEPLAY_TAG(Skill_Action_E, "Skill.Action.E");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Skill_Action_E, "Effect.Skill.Action.E");
	UE_DEFINE_GAMEPLAY_TAG(Notify_Skill_Action_E, "Notify.Skill.Action.E");

	UE_DEFINE_GAMEPLAY_TAG(Skill_Move_Vault, "Skill.Move.Vault");

	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Actor_Fire, "GameplayCue.Actor.Fire");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Actor_Heal, "GameplayCue.Actor.Heal");

	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Static_Hit, "GameplayCue.Static.Hit");
}