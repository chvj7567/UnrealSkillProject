// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyGameplayTags.h"
#include "Util/DefineEnum.h"

namespace SpyGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(Lock_Input_All, "Lock.Input.All");
	UE_DEFINE_GAMEPLAY_TAG(Lock_Input_Move, "Lock.Input.Move");
	UE_DEFINE_GAMEPLAY_TAG(Lock_Input_Look, "Lock.Input.Look");

	UE_DEFINE_GAMEPLAY_TAG(InitState_Spawned, "InitState.Spawned");
	UE_DEFINE_GAMEPLAY_TAG(InitState_DataAvailable, "InitState.DataAvailable");
	UE_DEFINE_GAMEPLAY_TAG(InitState_DataInitialized, "InitState.DataInitialized");
	UE_DEFINE_GAMEPLAY_TAG(InitState_GameplayReady, "InitState.GameplayReady");

	UE_DEFINE_GAMEPLAY_TAG(Character_Class_AI, "Character.Class.AI");
	UE_DEFINE_GAMEPLAY_TAG(Character_Class_Normal, "Character.Class.Normal");

	UE_DEFINE_GAMEPLAY_TAG(Character_State_Targeting, "Character.State.Targeting");
	UE_DEFINE_GAMEPLAY_TAG(Character_State_Movement_Normal, "Character.State.Movement.Normal");
	UE_DEFINE_GAMEPLAY_TAG(Character_State_Movement_Climb, "Character.State.Movement.Climb");
	UE_DEFINE_GAMEPLAY_TAG(Character_State_Grapple, "Character.State.Grapple");

	UE_DEFINE_GAMEPLAY_TAG(Input_Native_Move, "Input.Native.Move");
	UE_DEFINE_GAMEPLAY_TAG(Input_Native_Look, "Input.Native.Look");
	UE_DEFINE_GAMEPLAY_TAG(Input_Native_CursorToggle, "Input.Native.CursorToggle");

	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_Skill_1, "Input.Ability.Skill.1");
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_Skill_2, "Input.Ability.Skill.2");
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_Skill_3, "Input.Ability.Skill.3");
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_Skill_4, "Input.Ability.Skill.4");
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_Skill_5, "Input.Ability.Skill.5");
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_Skill_6, "Input.Ability.Skill.6");
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_Skill_7, "Input.Ability.Skill.7");
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_Skill_8, "Input.Ability.Skill.8");
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_Skill_9, "Input.Ability.Skill.9");
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_Skill_10, "Input.Ability.Skill.10");
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_Skill_11, "Input.Ability.Skill.11");
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_Parry, "Input.Ability.Parry");

	UE_DEFINE_GAMEPLAY_TAG(Skill_Action_A, "Skill.Action.A");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Skill_Action_A, "Effect.Skill.Action.A");
	UE_DEFINE_GAMEPLAY_TAG(Notify_Skill_Action_A, "Notify.Skill.Action.A");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Skill_Action_A, "Cooldown.Skill.Action.A");

	UE_DEFINE_GAMEPLAY_TAG(Skill_Action_B, "Skill.Action.B");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Skill_Action_B, "Effect.Skill.Action.B");
	UE_DEFINE_GAMEPLAY_TAG(Notify_Skill_Action_B, "Notify.Skill.Action.B");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Skill_Action_B, "Cooldown.Skill.Action.B");

	UE_DEFINE_GAMEPLAY_TAG(Skill_Action_C, "Skill.Action.C");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Skill_Action_C, "Effect.Skill.Action.C");
	UE_DEFINE_GAMEPLAY_TAG(Notify_Skill_Action_C, "Notify.Skill.Action.C");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Skill_Action_C, "Cooldown.Skill.Action.C");

	UE_DEFINE_GAMEPLAY_TAG(Skill_Action_D, "Skill.Action.D");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Skill_Action_D, "Effect.Skill.Action.D");
	UE_DEFINE_GAMEPLAY_TAG(Notify_Skill_Action_D, "Notify.Skill.Action.D");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Skill_Action_D, "Cooldown.Skill.Action.D");

	UE_DEFINE_GAMEPLAY_TAG(Skill_Action_E, "Skill.Action.E");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Skill_Action_E, "Effect.Skill.Action.E");
	UE_DEFINE_GAMEPLAY_TAG(Notify_Skill_Action_E, "Notify.Skill.Action.E");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Skill_Action_E, "Cooldown.Skill.Action.E");

	UE_DEFINE_GAMEPLAY_TAG(Skill_Action_F, "Skill.Action.F");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Skill_Action_F, "Effect.Skill.Action.F");
	UE_DEFINE_GAMEPLAY_TAG(Notify_Skill_Action_F, "Notify.Skill.Action.F");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Skill_Action_F, "Cooldown.Skill.Action.F");

	UE_DEFINE_GAMEPLAY_TAG(Skill_Move_Vault, "Skill.Move.Vault");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Move_Climb, "Skill.Move.Climb");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Move_HangUp, "Skill.Move.HangUp");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Move_Jump, "Skill.Move.Jump");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Move_GrappleHook, "Skill.Move.GrappleHook");

	UE_DEFINE_GAMEPLAY_TAG(Skill_Util_Death, "Skill.Util.Death");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Util_Combo1, "Skill.Util.Combo1");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Util_Combo2, "Skill.Util.Combo2");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Util_Combo3, "Skill.Util.Combo3");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Util_Combo4, "Skill.Util.Combo4");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Util_Combo5, "Skill.Util.Combo5");

	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Actor_Attack, "GameplayCue.Actor.Attack");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Actor_Target, "GameplayCue.Actor.Target");

	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Static_Hit, "GameplayCue.Static.Hit");

	UE_DEFINE_GAMEPLAY_TAG(Movement_Mode_Walking, "Movement.Mode.Walking");
	UE_DEFINE_GAMEPLAY_TAG(Movement_Mode_NavWalking, "Movement.Mode.NavWalking");
	UE_DEFINE_GAMEPLAY_TAG(Movement_Mode_Falling, "Movement.Mode.Falling");
	UE_DEFINE_GAMEPLAY_TAG(Movement_Mode_Swimming, "Movement.Mode.Swimming");
	UE_DEFINE_GAMEPLAY_TAG(Movement_Mode_Flying, "Movement.Mode.Flying");

	UE_DEFINE_GAMEPLAY_TAG(Movement_Mode_Custom, "Movement.Mode.Custom");
	UE_DEFINE_GAMEPLAY_TAG(Movement_Mode_WallClimb, "Movement.Mode.WallClimb");

	//# SetByCaller 데이터 태그
	UE_DEFINE_GAMEPLAY_TAG(Data_Experience_Gain, "Data.Experience.Gain");
	UE_DEFINE_GAMEPLAY_TAG(Data_Level_MaxHealthGrowth, "Data.Level.MaxHealthGrowth");
	UE_DEFINE_GAMEPLAY_TAG(Data_Level_MaxManaGrowth, "Data.Level.MaxManaGrowth");
	UE_DEFINE_GAMEPLAY_TAG(Data_Cost_Mana, "Data.Cost.Mana");

	//# 미션 진행 이벤트 태그
	UE_DEFINE_GAMEPLAY_TAG(Event_Mission_Kill, "Event.Mission.Kill");
	UE_DEFINE_GAMEPLAY_TAG(Event_Mission_Combo, "Event.Mission.Combo");
	UE_DEFINE_GAMEPLAY_TAG(Event_Mission_Level, "Event.Mission.Level");

	const FGameplayTagContainer& GetComboTags()
	{
		//# 태그 정의(UE_DEFINE_GAMEPLAY_TAG)가 끝난 뒤 최초 호출 시점에 1회만 만든다
		static FGameplayTagContainer ComboTags = []()
		{
			FGameplayTagContainer Container;
			Container.AddTag(Skill_Util_Combo1);
			Container.AddTag(Skill_Util_Combo2);
			Container.AddTag(Skill_Util_Combo3);
			Container.AddTag(Skill_Util_Combo4);
			Container.AddTag(Skill_Util_Combo5);

			return Container;
		}();

		return ComboTags;
	}

	const TMap<uint8, FGameplayTag> MovementModeTagMap =
	{
		{ MOVE_Walking, Movement_Mode_Walking },
		{ MOVE_NavWalking, Movement_Mode_NavWalking },
		{ MOVE_Falling, Movement_Mode_Falling },
		{ MOVE_Swimming, Movement_Mode_Swimming },
		{ MOVE_Flying, Movement_Mode_Flying },
		{ MOVE_Custom, Movement_Mode_Custom }
	};

	const TMap<uint8, FGameplayTag> CustomMovementModeTagMap =
	{
		{ ECustomMovementMode::MOVE_WallClimb, Movement_Mode_WallClimb }
	};
}