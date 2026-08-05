// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Data/SpyMissionConfig.h"
#include "Engine/DataTable.h"
#include "Util/SpyGameplayTags.h"

//# 테스트 전용 헬퍼 — MissionId(=FSpyMissionRow.MissionId)별 보상 행을 MissionRewardTable 에 추가한다.
//# 보상은 더 이상 FSpyMissionRow 필드가 아니라 관계 테이블이다 (Task 1, cpp-style §14-1).
//# 널이면 새로 만든다 (SpyNPCDialogueTests.cpp 의 인메모리 DataTable 픽스처 패턴과 동일)
static void SpyMissionTests_AddReward(USpyMissionConfig* Config, int32 MissionId, float Reward)
{
	if (Config->MissionRewardTable == nullptr)
	{
		UDataTable* Table = NewObject<UDataTable>();
		Table->RowStruct = FSpyMissionRewardRow::StaticStruct();
		Config->MissionRewardTable = Table;
	}

	FSpyMissionRewardRow Row;
	Row.MissionId = MissionId;
	Row.ExperienceReward = Reward;

	Config->MissionRewardTable->AddRow(FName(*FString::Printf(TEXT("Reward_%d"), MissionId)), Row);
}

//# 테스트 전용 헬퍼 — 완성된 FSpyMissionRow 를 MissionTable 에 추가한다.
//# 널이면 새로 만든다 (SpyMissionTests_AddReward 와 동일 패턴)
static void SpyMissionTests_AddMissionRow(USpyMissionConfig* Config, const FSpyMissionRow& Row)
{
	if (Config->MissionTable == nullptr)
	{
		UDataTable* Table = NewObject<UDataTable>();
		Table->RowStruct = FSpyMissionRow::StaticStruct();
		Config->MissionTable = Table;
	}

	Config->MissionTable->AddRow(FName(*FString::Printf(TEXT("Mission_%d"), Row.MissionId)), Row);
}

//# 픽스처(1-based) — [1] Vault 3회(Accumulate), [2] Climb 3 도달(Threshold)
static USpyMissionConfig* SpyMissionTests_MakeConfig()
{
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();

	FSpyMissionRow Vault;
	Vault.MissionId = 1;
	Vault.MatchTag = SpyGameplayTags::Skill_Move_Vault;
	Vault.Mode = ESpyMissionMode::Accumulate;
	Vault.TargetCount = 3;
	SpyMissionTests_AddMissionRow(Config, Vault);
	SpyMissionTests_AddReward(Config, 1, 10.f);

	FSpyMissionRow Level;
	Level.MissionId = 2;
	Level.MatchTag = SpyGameplayTags::Skill_Move_Climb;
	Level.Mode = ESpyMissionMode::Threshold;
	Level.TargetCount = 3;
	SpyMissionTests_AddMissionRow(Config, Level);
	SpyMissionTests_AddReward(Config, 2, 20.f);

	return Config;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionTagMismatchTest,
	"SkillProject.System.Mission.TagMismatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionTagMismatchTest::RunTest(const FString& Parameters)
{
	const USpyMissionConfig* Config = SpyMissionTests_MakeConfig();

	//# 현재 미션은 Vault(1) 인데 Climb 이벤트가 들어옴
	const FSpyMissionProgressResult Result = Config->ResolveMissionProgress(1, 1, SpyGameplayTags::Skill_Move_Climb, 1);

	TestEqual(TEXT("Index unchanged"), Result.MissionIndex, 1);
	TestEqual(TEXT("Count unchanged"), Result.Count, 1);
	TestFalse(TEXT("Not completed"), Result.bCompletedNow);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionAccumulatePartialTest,
	"SkillProject.System.Mission.AccumulatePartial",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionAccumulatePartialTest::RunTest(const FString& Parameters)
{
	const USpyMissionConfig* Config = SpyMissionTests_MakeConfig();

	const FSpyMissionProgressResult Result = Config->ResolveMissionProgress(1, 1, SpyGameplayTags::Skill_Move_Vault, 1);

	TestEqual(TEXT("Count increased"), Result.Count, 2);
	TestEqual(TEXT("Index unchanged"), Result.MissionIndex, 1);
	TestFalse(TEXT("Not completed"), Result.bCompletedNow);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionAccumulateExactTest,
	"SkillProject.System.Mission.AccumulateExact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionAccumulateExactTest::RunTest(const FString& Parameters)
{
	const USpyMissionConfig* Config = SpyMissionTests_MakeConfig();

	const FSpyMissionProgressResult Result = Config->ResolveMissionProgress(1, 2, SpyGameplayTags::Skill_Move_Vault, 1);

	TestTrue(TEXT("Completed"), Result.bCompletedNow);
	TestEqual(TEXT("Advanced to next mission"), Result.MissionIndex, 2);
	TestEqual(TEXT("Count reset"), Result.Count, 0);
	TestFalse(TEXT("Not all completed"), Result.bAllCompleted);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionAccumulateOvershootTest,
	"SkillProject.System.Mission.AccumulateOvershoot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionAccumulateOvershootTest::RunTest(const FString& Parameters)
{
	const USpyMissionConfig* Config = SpyMissionTests_MakeConfig();

	//# 목표 3인데 한 번에 10 이 들어옴 — 초과분은 다음 미션으로 이월하지 않는다
	const FSpyMissionProgressResult Result = Config->ResolveMissionProgress(1, 0, SpyGameplayTags::Skill_Move_Vault, 10);

	TestTrue(TEXT("Completed"), Result.bCompletedNow);
	TestEqual(TEXT("Advanced to next mission"), Result.MissionIndex, 2);
	TestEqual(TEXT("No carry-over"), Result.Count, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionThresholdTest,
	"SkillProject.System.Mission.Threshold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionThresholdTest::RunTest(const FString& Parameters)
{
	const USpyMissionConfig* Config = SpyMissionTests_MakeConfig();

	//# Threshold 는 누적하지 않고 값을 대치한다 — 미달
	const FSpyMissionProgressResult Below = Config->ResolveMissionProgress(2, 0, SpyGameplayTags::Skill_Move_Climb, 2);
	TestEqual(TEXT("Value replaced"), Below.Count, 2);
	TestFalse(TEXT("Not completed"), Below.bCompletedNow);

	//# 도달 — 마지막 미션이므로 전체 완료
	const FSpyMissionProgressResult Reached = Config->ResolveMissionProgress(2, 2, SpyGameplayTags::Skill_Move_Climb, 3);
	TestTrue(TEXT("Completed"), Reached.bCompletedNow);
	TestTrue(TEXT("All completed"), Reached.bAllCompleted);
	TestEqual(TEXT("Index past the end"), Reached.MissionIndex, 3);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionThresholdNoAccumulateTest,
	"SkillProject.System.Mission.ThresholdNoAccumulate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionThresholdNoAccumulateTest::RunTest(const FString& Parameters)
{
	const USpyMissionConfig* Config = SpyMissionTests_MakeConfig();

	//# 2가 세 번 들어와도 누적되지 않으므로 목표 3에 도달하지 않는다
	FSpyMissionProgressResult Result = Config->ResolveMissionProgress(2, 0, SpyGameplayTags::Skill_Move_Climb, 2);
	Result = Config->ResolveMissionProgress(Result.MissionIndex, Result.Count, SpyGameplayTags::Skill_Move_Climb, 2);
	Result = Config->ResolveMissionProgress(Result.MissionIndex, Result.Count, SpyGameplayTags::Skill_Move_Climb, 2);

	TestEqual(TEXT("Still replaced, not accumulated"), Result.Count, 2);
	TestFalse(TEXT("Never completed"), Result.bCompletedNow);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionAfterAllCompletedTest,
	"SkillProject.System.Mission.AfterAllCompleted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionAfterAllCompletedTest::RunTest(const FString& Parameters)
{
	const USpyMissionConfig* Config = SpyMissionTests_MakeConfig();

	//# 인덱스가 배열 범위를 벗어난 상태(1-based, 2개 미션이므로 3이 past-the-end) — 추가 이벤트에 반응하지 않는다
	const FSpyMissionProgressResult Result = Config->ResolveMissionProgress(3, 0, SpyGameplayTags::Skill_Move_Vault, 5);

	TestEqual(TEXT("Index unchanged"), Result.MissionIndex, 3);
	TestEqual(TEXT("Count unchanged"), Result.Count, 0);
	TestFalse(TEXT("Nothing completed"), Result.bCompletedNow);
	TestTrue(TEXT("Reported as all completed"), Result.bAllCompleted);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionEmptyConfigTest,
	"SkillProject.System.Mission.EmptyConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionEmptyConfigTest::RunTest(const FString& Parameters)
{
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();

	TestEqual(TEXT("No missions"), Config->GetMissionCount(), 0);

	const FSpyMissionProgressResult Result = Config->ResolveMissionProgress(0, 0, SpyGameplayTags::Skill_Move_Vault, 1);

	TestTrue(TEXT("All completed with empty config"), Result.bAllCompleted);
	TestFalse(TEXT("Nothing completed"), Result.bCompletedNow);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionHierarchicalTagTest,
	"SkillProject.System.Mission.HierarchicalTag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionHierarchicalTagTest::RunTest(const FString& Parameters)
{
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();

	//# 부모 태그 Skill.Move 로 하위 전체를 묶는다.
	//# ⚠ 이 테스트는 판정 함수의 계층 매칭 능력만 검증한다. 실제 미션 데이터는
	//#   6종 전부 leaf 태그를 지정한다 (mission-system.md §6-1) —
	//#   부모 태그를 쓰면 넘기·벽타기·그래플이 서로의 미션에 섞여 집계된다
	FSpyMissionRow AnyMove;
	AnyMove.MissionId = 1;
	AnyMove.MatchTag = SKGameplayTags::Skill_Move;
	AnyMove.Mode = ESpyMissionMode::Accumulate;
	AnyMove.TargetCount = 2;
	SpyMissionTests_AddMissionRow(Config, AnyMove);

	//# 자식 태그 이벤트가 부모 미션에 반영돼야 한다
	FSpyMissionProgressResult Result = Config->ResolveMissionProgress(1, 0, SpyGameplayTags::Skill_Move_Vault, 1);
	TestEqual(TEXT("Child tag matched parent mission"), Result.Count, 1);

	Result = Config->ResolveMissionProgress(Result.MissionIndex, Result.Count, SpyGameplayTags::Skill_Move_GrappleHook, 1);
	TestTrue(TEXT("Completed by a different child tag"), Result.bCompletedNow);

	return true;
}

//# ─────────────────────────────────────────────────────────────────────────────
//# 이하 test-engineer 확장 — 기획 확정 테이블 회귀 · 엣지 케이스 망라
//# ─────────────────────────────────────────────────────────────────────────────

//# 기획서(docs/design/mission-system.md §3-1) 확정 테이블 픽스처(1-based, MissionId 1~6).
//# ⚠ 이 값은 DA_SpyMissionConfig 에디터에 입력될 값이지 C++ 기본값이 아니다.
//#   따라서 이 픽스처는 에디터 DataAsset 을 추적하지 않는다 — 판정 함수가
//#   기획이 의도한 순서대로 체인을 굴려 주는지를 고정하는 것이 목적이다
static USpyMissionConfig* SpyMissionTests_MakeDesignConfig()
{
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();

	//# 1 — 적 1명 처치
	FSpyMissionRow Kill;
	Kill.MissionId = 1;
	Kill.MatchTag = SpyGameplayTags::Event_Mission_Kill;
	Kill.Mode = ESpyMissionMode::Accumulate;
	Kill.TargetCount = 1;
	Kill.DisplayName = FText::FromString(TEXT("적 1명 처치"));
	SpyMissionTests_AddMissionRow(Config, Kill);
	SpyMissionTests_AddReward(Config, 1, 20.f);

	//# 2 — 레벨 3 달성 (Threshold)
	FSpyMissionRow Level;
	Level.MissionId = 2;
	Level.MatchTag = SpyGameplayTags::Event_Mission_Level;
	Level.Mode = ESpyMissionMode::Threshold;
	Level.TargetCount = 3;
	Level.DisplayName = FText::FromString(TEXT("레벨 3 달성"));
	SpyMissionTests_AddMissionRow(Config, Level);
	SpyMissionTests_AddReward(Config, 2, 10.f);

	//# 3 — 콤보 4회 연결
	FSpyMissionRow Combo;
	Combo.MissionId = 3;
	Combo.MatchTag = SpyGameplayTags::Event_Mission_Combo;
	Combo.Mode = ESpyMissionMode::Accumulate;
	Combo.TargetCount = 4;
	Combo.DisplayName = FText::FromString(TEXT("콤보 4회 연결"));
	SpyMissionTests_AddMissionRow(Config, Combo);
	SpyMissionTests_AddReward(Config, 3, 10.f);

	//# 4 — 장애물 넘기 5회
	FSpyMissionRow Vault;
	Vault.MissionId = 4;
	Vault.MatchTag = SpyGameplayTags::Skill_Move_Vault;
	Vault.Mode = ESpyMissionMode::Accumulate;
	Vault.TargetCount = 5;
	Vault.DisplayName = FText::FromString(TEXT("장애물 넘기 5회"));
	SpyMissionTests_AddMissionRow(Config, Vault);
	SpyMissionTests_AddReward(Config, 4, 10.f);

	//# 5 — 벽 타기 3회
	FSpyMissionRow Climb;
	Climb.MissionId = 5;
	Climb.MatchTag = SpyGameplayTags::Skill_Move_Climb;
	Climb.Mode = ESpyMissionMode::Accumulate;
	Climb.TargetCount = 3;
	Climb.DisplayName = FText::FromString(TEXT("벽 타기 3회"));
	SpyMissionTests_AddMissionRow(Config, Climb);
	SpyMissionTests_AddReward(Config, 5, 15.f);

	//# 6 — 그래플링 3회
	FSpyMissionRow Grapple;
	Grapple.MissionId = 6;
	Grapple.MatchTag = SpyGameplayTags::Skill_Move_GrappleHook;
	Grapple.Mode = ESpyMissionMode::Accumulate;
	Grapple.TargetCount = 3;
	Grapple.DisplayName = FText::FromString(TEXT("그래플링 3회"));
	SpyMissionTests_AddMissionRow(Config, Grapple);
	SpyMissionTests_AddReward(Config, 6, 15.f);

	return Config;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionDesignChainTest,
	"SkillProject.System.Mission.DesignChain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionDesignChainTest::RunTest(const FString& Parameters)
{
	const USpyMissionConfig* Config = SpyMissionTests_MakeDesignConfig();

	TestEqual(TEXT("Design table has 6 missions"), Config->GetMissionCount(), 6);

	//# ResolveMissionProgress 는 순수 함수라 상태를 갖지 않는다 — 결과를 다음 호출로 되먹인다.
	//# 기본 생성 State.MissionIndex == 1 (1-based 첫 미션)에서 출발한다
	FSpyMissionProgressResult State;

	//# ── 킬 1 — 기획서 §4-1 처리 순서: 경험치 GE(→Lv2 승급 신호) 가 킬 이벤트보다 먼저 도착한다.
	//#    현재 미션이 처치(id 1)이므로 Lv2 신호는 태그 불일치로 버려져야 한다 (§2 경로 B 의 핵심)
	State = Config->ResolveMissionProgress(State.MissionIndex, State.Count, SpyGameplayTags::Event_Mission_Level, 2);
	TestEqual(TEXT("Lv2 signal discarded — still on kill mission"), State.MissionIndex, 1);
	TestEqual(TEXT("Lv2 signal left the count untouched"), State.Count, 0);
	TestFalse(TEXT("Lv2 signal completed nothing"), State.bCompletedNow);

	//# 킬 이벤트 — 목표 1 이므로 즉시 완료
	State = Config->ResolveMissionProgress(State.MissionIndex, State.Count, SpyGameplayTags::Event_Mission_Kill, 1);
	TestTrue(TEXT("Mission 1 (kill) completed"), State.bCompletedNow);
	TestEqual(TEXT("Advanced to level mission"), State.MissionIndex, 2);
	TestEqual(TEXT("Count reset for level mission"), State.Count, 0);

	//# ── 킬 2 — 누적 60 XP 로 Lv3 승급. 이번에는 레벨 미션이 현재 미션이라 Threshold 로 완료된다
	State = Config->ResolveMissionProgress(State.MissionIndex, State.Count, SpyGameplayTags::Event_Mission_Level, 3);
	TestTrue(TEXT("Mission 2 (level) completed by Lv3 signal"), State.bCompletedNow);
	TestEqual(TEXT("Advanced to combo mission"), State.MissionIndex, 3);

	//# ── 콤보 4회 — 4번째에서 완료
	for (int32 Index = 0; Index < 4; ++Index)
	{
		State = Config->ResolveMissionProgress(State.MissionIndex, State.Count, SpyGameplayTags::Event_Mission_Combo, 1);
	}
	TestTrue(TEXT("Mission 3 (combo) completed on the 4th"), State.bCompletedNow);
	TestEqual(TEXT("Advanced to vault mission"), State.MissionIndex, 4);

	//# ── 넘기 5회
	for (int32 Index = 0; Index < 4; ++Index)
	{
		State = Config->ResolveMissionProgress(State.MissionIndex, State.Count, SpyGameplayTags::Skill_Move_Vault, 1);
	}
	TestEqual(TEXT("Vault at 4 of 5"), State.Count, 4);
	TestFalse(TEXT("Vault not completed at 4"), State.bCompletedNow);

	State = Config->ResolveMissionProgress(State.MissionIndex, State.Count, SpyGameplayTags::Skill_Move_Vault, 1);
	TestTrue(TEXT("Mission 4 (vault) completed on the 5th"), State.bCompletedNow);
	TestEqual(TEXT("Advanced to climb mission"), State.MissionIndex, 5);

	//# ── 벽타기 3회
	for (int32 Index = 0; Index < 3; ++Index)
	{
		State = Config->ResolveMissionProgress(State.MissionIndex, State.Count, SpyGameplayTags::Skill_Move_Climb, 1);
	}
	TestTrue(TEXT("Mission 5 (climb) completed on the 3rd"), State.bCompletedNow);
	TestEqual(TEXT("Advanced to grapple mission"), State.MissionIndex, 6);
	TestFalse(TEXT("Not all completed yet"), State.bAllCompleted);

	//# ── 그래플 3회 — 마지막 미션이므로 전체 완료
	for (int32 Index = 0; Index < 3; ++Index)
	{
		State = Config->ResolveMissionProgress(State.MissionIndex, State.Count, SpyGameplayTags::Skill_Move_GrappleHook, 1);
	}
	TestTrue(TEXT("Mission 6 (grapple) completed on the 3rd"), State.bCompletedNow);
	TestEqual(TEXT("Index past the end"), State.MissionIndex, 7);
	TestTrue(TEXT("All missions completed"), State.bAllCompleted);

	//# 완주 후 어떤 이벤트도 상태를 되돌리지 못한다
	State = Config->ResolveMissionProgress(State.MissionIndex, State.Count, SpyGameplayTags::Event_Mission_Kill, 1);
	TestEqual(TEXT("Index stays past the end"), State.MissionIndex, 7);
	TestTrue(TEXT("Still all completed"), State.bAllCompleted);
	TestFalse(TEXT("Nothing completed again"), State.bCompletedNow);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionDesignTableTest,
	"SkillProject.System.Mission.DesignTable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionDesignTableTest::RunTest(const FString& Parameters)
{
	//# 기획서 §3-1 확정 테이블을 밸런스 회귀로 박제한다.
	//# 픽스처와 기획 문서가 어긋나면 이 테스트가 먼저 알려준다
	const USpyMissionConfig* Config = SpyMissionTests_MakeDesignConfig();

	const FGameplayTag ExpectedTags[] = {
		SpyGameplayTags::Event_Mission_Kill,
		SpyGameplayTags::Event_Mission_Level,
		SpyGameplayTags::Event_Mission_Combo,
		SpyGameplayTags::Skill_Move_Vault,
		SpyGameplayTags::Skill_Move_Climb,
		SpyGameplayTags::Skill_Move_GrappleHook,
	};
	const ESpyMissionMode ExpectedModes[] = {
		ESpyMissionMode::Accumulate,
		ESpyMissionMode::Threshold,
		ESpyMissionMode::Accumulate,
		ESpyMissionMode::Accumulate,
		ESpyMissionMode::Accumulate,
		ESpyMissionMode::Accumulate,
	};
	const int32 ExpectedTargets[] = { 1, 3, 4, 5, 3, 3 };
	const float ExpectedRewards[] = { 20.f, 10.f, 10.f, 10.f, 15.f, 15.f };

	TestEqual(TEXT("Mission count"), Config->GetMissionCount(), 6);

	float RewardSum = 0.f;
	for (int32 Index = 0; Index < 6; ++Index)
	{
		//# 배열 Index 는 0-based, MissionId 는 1-based — 조회는 항상 Index + 1
		const int32 MissionId = Index + 1;
		const FSpyMissionRow* Entry = Config->GetMission(MissionId);
		if (Entry == nullptr)
		{
			AddError(FString::Printf(TEXT("Mission %d is missing"), MissionId));

			continue;
		}

		TestTrue(FString::Printf(TEXT("Mission %d match tag"), MissionId), Entry->MatchTag == ExpectedTags[Index]);
		TestTrue(FString::Printf(TEXT("Mission %d mode"), MissionId), Entry->Mode == ExpectedModes[Index]);
		TestEqual(FString::Printf(TEXT("Mission %d target count"), MissionId), Entry->TargetCount, ExpectedTargets[Index]);
		TestEqual(FString::Printf(TEXT("Mission %d reward"), MissionId), Config->GetMissionReward(MissionId), ExpectedRewards[Index]);
		TestFalse(FString::Printf(TEXT("Mission %d display name is not empty"), MissionId), Entry->DisplayName.IsEmpty());

		RewardSum += Config->GetMissionReward(MissionId);
	}

	//# §3-3 — 최소 필요 킬 2회(40 XP) + 보상 합 80 = 120 = Lv4 도달 누적치
	TestEqual(TEXT("Reward sum is 80"), RewardSum, 80.f);

	//# 레벨 미션만 Threshold 라는 것이 §2 교착 방지 논리의 전제다
	const FSpyMissionRow* LevelEntry = Config->GetMission(2);
	TestTrue(TEXT("Level mission is Threshold"), (LevelEntry != nullptr) && (LevelEntry->Mode == ESpyMissionMode::Threshold));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionCppDefaultsEmptyTest,
	"SkillProject.System.Mission.CppDefaultsEmpty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionCppDefaultsEmptyTest::RunTest(const FString& Parameters)
{
	//# 미션 값은 에디터 DataTable 이 단일 진실이다 (§6-3).
	//# C++ 에서 기본 미션을 채우기 시작하면 두 곳에 진실이 생기므로 비어 있음을 고정한다
	const USpyMissionConfig* Config = NewObject<USpyMissionConfig>();

	TestEqual(TEXT("Fresh config has no missions in C++"), Config->GetMissionCount(), 0);
	TestFalse(TEXT("Index 0 is invalid on a fresh config"), Config->IsValidMissionIndex(0));

	//# 로우 구조체 기본값도 함께 고정한다. MissionId 기본값은 0으로 둔다 — 1-based 체계에서
	//# 0은 어떤 실제 미션도 가질 수 없는 값이라, 미설정 행이 실제 미션(1)과 충돌하지 않는다
	const FSpyMissionRow Default;
	TestEqual(TEXT("Default MissionId"), Default.MissionId, 0);
	TestTrue(TEXT("Default mode is Accumulate"), Default.Mode == ESpyMissionMode::Accumulate);
	TestEqual(TEXT("Default target count"), Default.TargetCount, 1);
	TestFalse(TEXT("Default match tag is invalid"), Default.MatchTag.IsValid());
	TestEqual(TEXT("Default NPCId is the system-quest sentinel"), Default.NPCId, 9999);

	//# 보상은 더 이상 FSpyMissionRow 필드가 아니라 MissionRewardTable 관계다 —
	//# 테이블 미지정 상태에서는 "관계 없음"으로 0.f 를 반환해야 한다 (§4-3)
	TestEqual(TEXT("Reward is 0 with no MissionRewardTable"), Config->GetMissionReward(0), 0.f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionEventBeforeItsTurnTest,
	"SkillProject.System.Mission.EventBeforeItsTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionEventBeforeItsTurnTest::RunTest(const FString& Parameters)
{
	//# §2 경로 B — 아직 자기 차례가 아닌 미션의 이벤트는 보관되지 않고 버려진다.
	//# 이 성질이 "레벨 미션을 체인 앞쪽에 두어야 한다"는 배치 원칙의 근거다
	const USpyMissionConfig* Config = SpyMissionTests_MakeDesignConfig();

	FSpyMissionProgressResult State;

	//# 처치 미션(1) 진행 중 레벨 승급 신호가 세 번 도착해도 전부 폐기된다
	State = Config->ResolveMissionProgress(1, 0, SpyGameplayTags::Event_Mission_Level, 2);
	State = Config->ResolveMissionProgress(State.MissionIndex, State.Count, SpyGameplayTags::Event_Mission_Level, 3);
	State = Config->ResolveMissionProgress(State.MissionIndex, State.Count, SpyGameplayTags::Event_Mission_Level, 4);

	TestEqual(TEXT("Still on the kill mission"), State.MissionIndex, 1);
	TestEqual(TEXT("Kill progress untouched"), State.Count, 0);
	TestFalse(TEXT("No mission completed by out-of-turn events"), State.bCompletedNow);

	//# 처치 미션이 끝나 레벨 미션이 현재 미션이 되어도, 이미 지나간 승급은 진행도로 남지 않는다.
	//# HUD 가 0/3 으로 표시되는 §5·§8-2 의 "알려진 어긋남"을 코드 성질로 고정한다
	State = Config->ResolveMissionProgress(State.MissionIndex, State.Count, SpyGameplayTags::Event_Mission_Kill, 1);
	TestEqual(TEXT("Level mission is now current"), State.MissionIndex, 2);
	TestEqual(TEXT("Level mission starts at 0, not at the past level"), State.Count, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionThresholdJumpTest,
	"SkillProject.System.Mission.ThresholdJump",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionThresholdJumpTest::RunTest(const FString& Parameters)
{
	//# 레벨 2 → 4 로 한 번에 뛰어도(목표 3 초과) 완료 처리되고 초과분은 이월되지 않는다
	const USpyMissionConfig* Config = SpyMissionTests_MakeDesignConfig();

	const FSpyMissionProgressResult Result = Config->ResolveMissionProgress(2, 0, SpyGameplayTags::Event_Mission_Level, 4);

	TestTrue(TEXT("Completed by overshooting threshold"), Result.bCompletedNow);
	TestEqual(TEXT("Advanced to combo mission"), Result.MissionIndex, 3);
	TestEqual(TEXT("No carry-over into the next mission"), Result.Count, 0);
	TestFalse(TEXT("Not all completed"), Result.bAllCompleted);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionSameTagAfterCompleteTest,
	"SkillProject.System.Mission.SameTagAfterComplete",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionSameTagAfterCompleteTest::RunTest(const FString& Parameters)
{
	//# 완료 직후 같은 태그 이벤트가 또 도착 — 다음 미션이 다른 태그면 무시돼야 한다.
	//# 봇을 연속 처치했을 때 두 번째 킬이 레벨 미션에 잘못 집계되지 않는지 확인한다
	const USpyMissionConfig* Config = SpyMissionTests_MakeDesignConfig();

	FSpyMissionProgressResult State = Config->ResolveMissionProgress(1, 0, SpyGameplayTags::Event_Mission_Kill, 1);
	TestEqual(TEXT("Kill mission completed"), State.MissionIndex, 2);

	State = Config->ResolveMissionProgress(State.MissionIndex, State.Count, SpyGameplayTags::Event_Mission_Kill, 1);
	TestEqual(TEXT("Second kill did not touch the level mission"), State.Count, 0);
	TestEqual(TEXT("Index unchanged"), State.MissionIndex, 2);
	TestFalse(TEXT("Nothing completed"), State.bCompletedNow);

	//# 파쿠르 미션 사이에서도 같은 성질이 성립한다 — 넘기 완료 후 남은 넘기 입력이 벽타기에 새지 않는다
	FSpyMissionProgressResult Parkour = Config->ResolveMissionProgress(4, 4, SpyGameplayTags::Skill_Move_Vault, 1);
	TestEqual(TEXT("Advanced to climb mission"), Parkour.MissionIndex, 5);

	Parkour = Config->ResolveMissionProgress(Parkour.MissionIndex, Parkour.Count, SpyGameplayTags::Skill_Move_Vault, 1);
	TestEqual(TEXT("Extra vault did not leak into climb"), Parkour.Count, 0);
	TestFalse(TEXT("Climb not completed by a vault"), Parkour.bCompletedNow);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionNonPositiveAmountTest,
	"SkillProject.System.Mission.NonPositiveAmount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionNonPositiveAmountTest::RunTest(const FString& Parameters)
{
	const USpyMissionConfig* Config = SpyMissionTests_MakeDesignConfig();

	//# Accumulate — Amount 0 은 진행을 만들지 않는다
	const FSpyMissionProgressResult Zero = Config->ResolveMissionProgress(3, 1, SpyGameplayTags::Event_Mission_Combo, 0);
	TestEqual(TEXT("Zero amount keeps the count"), Zero.Count, 1);
	TestFalse(TEXT("Zero amount completes nothing"), Zero.bCompletedNow);

	//# Accumulate — 음수 Amount 도 0 으로 clamp 되어 진행도를 깎지 않는다
	const FSpyMissionProgressResult Negative = Config->ResolveMissionProgress(3, 1, SpyGameplayTags::Event_Mission_Combo, -5);
	TestEqual(TEXT("Negative amount does not decrease the count"), Negative.Count, 1);
	TestFalse(TEXT("Negative amount completes nothing"), Negative.bCompletedNow);

	//# Threshold — 대치 모드라 0/음수는 진행도를 0 으로 되돌린다 (목표 3 미달이므로 완료 없음)
	const FSpyMissionProgressResult ThresholdZero = Config->ResolveMissionProgress(2, 2, SpyGameplayTags::Event_Mission_Level, 0);
	TestEqual(TEXT("Threshold replaced with 0"), ThresholdZero.Count, 0);
	TestFalse(TEXT("Threshold not completed"), ThresholdZero.bCompletedNow);

	const FSpyMissionProgressResult ThresholdNegative = Config->ResolveMissionProgress(2, 2, SpyGameplayTags::Event_Mission_Level, -3);
	TestEqual(TEXT("Negative threshold clamped to 0"), ThresholdNegative.Count, 0);
	TestFalse(TEXT("Threshold not completed by a negative value"), ThresholdNegative.bCompletedNow);

	//# 음수 진행도가 입력으로 들어와도 0 으로 clamp 된다
	const FSpyMissionProgressResult NegativeCount = Config->ResolveMissionProgress(3, -4, SpyGameplayTags::Event_Mission_Combo, 1);
	TestEqual(TEXT("Negative in-count clamped to 0 then accumulated"), NegativeCount.Count, 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionInvalidMatchTagTest,
	"SkillProject.System.Mission.InvalidMatchTag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionInvalidMatchTagTest::RunTest(const FString& Parameters)
{
	//# MatchTag 를 비워 둔 미션 항목 — 에디터 입력 누락 시나리오.
	//# 어떤 이벤트로도 진행되지 않아 체인이 무증상 교착된다는 사실을 고정한다
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();

	FSpyMissionRow Broken;
	Broken.MissionId = 1;
	Broken.MatchTag = FGameplayTag();
	Broken.Mode = ESpyMissionMode::Accumulate;
	Broken.TargetCount = 2;
	SpyMissionTests_AddMissionRow(Config, Broken);

	const FSpyMissionRow* BrokenEntry = Config->GetMission(1);
	if (BrokenEntry == nullptr)
	{
		AddError(TEXT("Fixture entry was not added"));

		return false;
	}

	TestFalse(TEXT("Entry match tag is invalid"), BrokenEntry->MatchTag.IsValid());

	FSpyMissionProgressResult State;
	State = Config->ResolveMissionProgress(1, 0, SpyGameplayTags::Event_Mission_Kill, 1);
	State = Config->ResolveMissionProgress(State.MissionIndex, State.Count, SpyGameplayTags::Skill_Move_Vault, 1);

	TestEqual(TEXT("Stalled at index 1"), State.MissionIndex, 1);
	TestEqual(TEXT("No progress accumulated"), State.Count, 0);
	TestFalse(TEXT("Never completes"), State.bCompletedNow);
	TestFalse(TEXT("Never reports all completed"), State.bAllCompleted);

	//# 반대 방향 — 이벤트 태그가 무효면 유효한 미션도 진행되지 않는다
	const USpyMissionConfig* Design = SpyMissionTests_MakeDesignConfig();
	const FSpyMissionProgressResult InvalidEvent = Design->ResolveMissionProgress(1, 0, FGameplayTag(), 1);
	TestEqual(TEXT("Invalid event tag makes no progress"), InvalidEvent.Count, 0);
	TestFalse(TEXT("Invalid event tag completes nothing"), InvalidEvent.bCompletedNow);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionNegativeIndexTest,
	"SkillProject.System.Mission.NegativeIndex",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionNegativeIndexTest::RunTest(const FString& Parameters)
{
	//# 음수 인덱스는 무시가 아니라 1 로 clamp 된 뒤 정상 판정된다.
	//# (ResolveMissionProgress 의 FMath::Max(1, InIndex)) — 복제 초기화 이전 상태 등에서
	//# 음수가 들어와도 첫 미션(1)으로 되돌아갈 뿐 크래시하지 않는다
	const USpyMissionConfig* Config = SpyMissionTests_MakeDesignConfig();

	const FSpyMissionProgressResult Clamped = Config->ResolveMissionProgress(-1, 0, SpyGameplayTags::Event_Mission_Kill, 1);
	TestTrue(TEXT("Clamped to mission 1 and completed it"), Clamped.bCompletedNow);
	TestEqual(TEXT("Advanced from the clamped index"), Clamped.MissionIndex, 2);

	//# 훨씬 작은 음수도 동일하게 1 로 clamp 된다
	const FSpyMissionProgressResult DeepNegative = Config->ResolveMissionProgress(-100, 0, SpyGameplayTags::Event_Mission_Combo, 1);
	TestEqual(TEXT("Deep negative clamped to mission 1"), DeepNegative.MissionIndex, 1);
	TestEqual(TEXT("Combo event does not match mission 1"), DeepNegative.Count, 0);
	TestFalse(TEXT("Nothing completed"), DeepNegative.bCompletedNow);
	TestFalse(TEXT("Clamped index is a valid mission, so not all completed"), DeepNegative.bAllCompleted);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionIndexBoundaryTest,
	"SkillProject.System.Mission.IndexBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionIndexBoundaryTest::RunTest(const FString& Parameters)
{
	const USpyMissionConfig* Config = SpyMissionTests_MakeDesignConfig();

	TestEqual(TEXT("Mission count"), Config->GetMissionCount(), 6);

	TestFalse(TEXT("Index -1 invalid"), Config->IsValidMissionIndex(-1));
	TestFalse(TEXT("Index 0 invalid (new floor boundary under 1-based)"), Config->IsValidMissionIndex(0));
	TestTrue(TEXT("Index 1 valid (first)"), Config->IsValidMissionIndex(1));
	TestTrue(TEXT("Index 6 valid (last)"), Config->IsValidMissionIndex(6));
	TestFalse(TEXT("Index 7 invalid (one past the end)"), Config->IsValidMissionIndex(7));

	const FSpyMissionRow* First = Config->GetMission(1);
	const FSpyMissionRow* Last = Config->GetMission(6);

	TestTrue(TEXT("GetMission(-1) is null"), Config->GetMission(-1) == nullptr);
	TestTrue(TEXT("GetMission(0) is null (new floor boundary)"), Config->GetMission(0) == nullptr);
	TestTrue(TEXT("GetMission(1) is valid"), First != nullptr);
	TestTrue(TEXT("GetMission(6) is valid"), Last != nullptr);
	TestTrue(TEXT("GetMission(7) is null"), Config->GetMission(7) == nullptr);

	//# 첫/마지막 엔트리가 기획 순서대로 놓였는지 — §6-5 조건 1 (순서가 곧 교착 방지)
	TestTrue(TEXT("First mission is the kill mission"), (First != nullptr) && (First->MatchTag == SpyGameplayTags::Event_Mission_Kill));
	TestTrue(TEXT("Last mission is the grapple mission"), (Last != nullptr) && (Last->MatchTag == SpyGameplayTags::Skill_Move_GrappleHook));

	return true;
}

//# ─────────────────────────────────────────────────────────────────────────────
//# 오브젝트 상호작용 미션(ESpyMissionType::Interact) 추가 — ResolveMissionProgress
//# 는 이번 변경에서 한 줄도 바뀌지 않았다(spec §2-3·§4-3, plan Task 4가 소비만 한다).
//# 이 두 케이스는 "MissionType 은 판정 함수의 관심사가 아니다"라는 성질이 Interact 값에도
//# 그대로 성립하는지 고정한다(spec §8 표 5번째 행) — Accumulate/Threshold/overshoot 조합을
//# 타입별로 다시 돌리지 않는다(이미 위에서 타입 무관으로 망라됨).
//# ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionInteractTypeMatchesTest,
	"SkillProject.System.Mission.InteractType.MatchCompletes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionInteractTypeMatchesTest::RunTest(const FString& Parameters)
{
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();

	FSpyMissionRow Interact;
	Interact.MissionId = 1;
	Interact.MissionType = ESpyMissionType::Interact;
	Interact.MatchTag = SpyGameplayTags::Event_Mission_Interact;
	Interact.Mode = ESpyMissionMode::Accumulate;
	Interact.TargetCount = 1;
	SpyMissionTests_AddMissionRow(Config, Interact);
	SpyMissionTests_AddReward(Config, 1, 10.f);

	//# 1회성 오브젝트가 발신하는 이벤트 그대로 — 1회 매칭으로 즉시 완료(§3-2 TargetCount=1)
	const FSpyMissionProgressResult Result = Config->ResolveMissionProgress(1, 0, SpyGameplayTags::Event_Mission_Interact, 1);

	TestTrue(TEXT("Completed on the first (and only) interaction"), Result.bCompletedNow);
	TestTrue(TEXT("Reported as all completed (single-mission config)"), Result.bAllCompleted);
	TestEqual(TEXT("Reward resolves like any other MissionRewardTable row"), Config->GetMissionReward(1), 10.f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionInteractTypeMismatchTest,
	"SkillProject.System.Mission.InteractType.TagMismatchIgnored",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionInteractTypeMismatchTest::RunTest(const FString& Parameters)
{
	//# MissionType 이 Interact 라고 해서 태그 매칭 규칙이 느슨해지지 않는다 —
	//# 다른 오브젝트/이벤트의 태그는 여전히 no-op 이다(§6 엣지케이스, 별도 방어 코드 불필요)
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();

	FSpyMissionRow Interact;
	Interact.MissionId = 1;
	Interact.MissionType = ESpyMissionType::Interact;
	Interact.MatchTag = SpyGameplayTags::Event_Mission_Interact;
	Interact.Mode = ESpyMissionMode::Accumulate;
	Interact.TargetCount = 1;
	SpyMissionTests_AddMissionRow(Config, Interact);

	const FSpyMissionProgressResult Result = Config->ResolveMissionProgress(1, 0, SpyGameplayTags::Event_Mission_Kill, 1);

	TestEqual(TEXT("Index unchanged"), Result.MissionIndex, 1);
	TestEqual(TEXT("Count unchanged"), Result.Count, 0);
	TestFalse(TEXT("Mismatched tag completes nothing"), Result.bCompletedNow);

	return true;
}

//# Mission_TargetLocation(DataTable 수동 좌표) 테스트는 design §0(2026-08-05) 개정으로
//# 전부 제거됐다 — 좌표 소스가 레벨 배치 액터 자동 추적(USpyMissionTargetRegistrySubsystem)
//# 으로 바뀌어 대응 프로덕션 API(FSpyMission_TargetLocationRow 등)가 삭제됐다(design §7-6).
//# 신규 테스트는 test-engineer 가 레지스트리 서브시스템 대상으로 다시 작성한다.

#endif //# WITH_DEV_AUTOMATION_TESTS
