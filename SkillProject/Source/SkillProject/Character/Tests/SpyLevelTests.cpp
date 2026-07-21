// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Data/SpyLevelConfig.h"
#include "Misc/AutomationTest.h"

//# 테스트 전용 커브 {100, 200, 300} — 최대 레벨 4
//# (DA_SpyLevelConfig 의 실제 확정값과 무관한 픽스처다)
static USpyLevelConfig* SpyLevelTests_MakeConfig()
{
	USpyLevelConfig* Config = NewObject<USpyLevelConfig>();
	Config->ExperienceToNextLevel = {100.f, 200.f, 300.f};
	Config->MaxHealthPerLevel = 10.f;
	Config->MaxManaPerLevel = 5.f;
	Config->ExperienceRewardPerLevel = 20.f;

	return Config;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLevelBelowThresholdTest,
	"SkillProject.Character.Level.BelowThreshold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLevelBelowThresholdTest::RunTest(const FString& Parameters)
{
	const USpyLevelConfig* Config = SpyLevelTests_MakeConfig();

	const FSpyLevelUpResult Result = Config->ResolveLevelUp(1, 50.f);

	TestEqual(TEXT("Level stays 1"), Result.Level, 1);
	TestEqual(TEXT("Experience kept"), Result.Experience, 50.f);
	TestEqual(TEXT("MaxExperience is first curve entry"), Result.MaxExperience, 100.f);
	TestEqual(TEXT("No level gained"), Result.LevelsGained, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLevelExactThresholdTest,
	"SkillProject.Character.Level.ExactThreshold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLevelExactThresholdTest::RunTest(const FString& Parameters)
{
	const USpyLevelConfig* Config = SpyLevelTests_MakeConfig();

	const FSpyLevelUpResult Result = Config->ResolveLevelUp(1, 100.f);

	TestEqual(TEXT("Level up to 2"), Result.Level, 2);
	TestEqual(TEXT("Experience consumed exactly"), Result.Experience, 0.f);
	TestEqual(TEXT("MaxExperience advances"), Result.MaxExperience, 200.f);
	TestEqual(TEXT("One level gained"), Result.LevelsGained, 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLevelMultiLevelTest,
	"SkillProject.Character.Level.MultiLevel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLevelMultiLevelTest::RunTest(const FString& Parameters)
{
	const USpyLevelConfig* Config = SpyLevelTests_MakeConfig();

	//# 350 = 100(→Lv2) + 200(→Lv3) + 잔여 50
	const FSpyLevelUpResult Result = Config->ResolveLevelUp(1, 350.f);

	TestEqual(TEXT("Level up to 3"), Result.Level, 3);
	TestEqual(TEXT("Remainder carried over"), Result.Experience, 50.f);
	TestEqual(TEXT("MaxExperience is third curve entry"), Result.MaxExperience, 300.f);
	TestEqual(TEXT("Two levels gained"), Result.LevelsGained, 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLevelMaxClampTest,
	"SkillProject.Character.Level.MaxClamp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLevelMaxClampTest::RunTest(const FString& Parameters)
{
	const USpyLevelConfig* Config = SpyLevelTests_MakeConfig();

	TestEqual(TEXT("Max level is curve size + 1"), Config->GetMaxLevel(), 4);

	const FSpyLevelUpResult Result = Config->ResolveLevelUp(4, 9999.f);

	TestEqual(TEXT("Level stays at max"), Result.Level, 4);
	TestEqual(TEXT("Experience clamped to MaxExperience"), Result.Experience, 300.f);
	TestEqual(TEXT("No level gained"), Result.LevelsGained, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLevelEmptyCurveTest,
	"SkillProject.Character.Level.EmptyCurve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLevelEmptyCurveTest::RunTest(const FString& Parameters)
{
	USpyLevelConfig* Config = NewObject<USpyLevelConfig>();
	Config->ExperienceToNextLevel.Empty();

	const FSpyLevelUpResult Result = Config->ResolveLevelUp(1, 500.f);

	TestEqual(TEXT("Level stays 1 without a curve"), Result.Level, 1);
	TestEqual(TEXT("MaxExperience is zero"), Result.MaxExperience, 0.f);
	TestEqual(TEXT("No level gained"), Result.LevelsGained, 0);

	return true;
}

//# ---------------------------------------------------------------------------
//# 확장 스위트 — 기획 확정 커브 회귀 / 경계값 / 방어 로직
//# 위쪽 5개 케이스(픽스처 {100,200,300})는 그대로 두고 아래에 쌓는다.
//# ---------------------------------------------------------------------------

//# 기획 확정 커브 {20, 40, 60} — docs/design/experience-level.md §9-1 (최대 레벨 4)
//# 위 픽스처와 달리 이쪽은 DA_SpyLevelConfig 에 실제로 입력되는 값이다.
static USpyLevelConfig* SpyLevelTests_MakeDesignConfig()
{
	USpyLevelConfig* Config = NewObject<USpyLevelConfig>();
	Config->ExperienceToNextLevel = {20.f, 40.f, 60.f};

	return Config;
}

//# 봇(Lv1) 1킬 보상 — 기획서 §3
static constexpr float SpyLevelTests_KillReward = 20.f;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLevelDesignCurveProgressionTest,
	"SkillProject.Character.Level.DesignCurveProgression",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLevelDesignCurveProgressionTest::RunTest(const FString& Parameters)
{
	const USpyLevelConfig* Config = SpyLevelTests_MakeDesignConfig();

	TestEqual(TEXT("Design curve max level is 4"), Config->GetMaxLevel(), 4);

	//# 기획서 §4-3 "킬별 진행 추적" 표 그대로. 킬마다 +20 을 누적 적용한다.
	struct FExpectedStep
	{
		int32 Level;
		float Experience;
		float MaxExperience;
		int32 LevelsGained;
	};

	const FExpectedStep Expected[] =
		{
			{2, 0.f, 40.f, 1},  //# 1킬 — 20 >= 20 → 승급
			{2, 20.f, 40.f, 0}, //# 2킬 — 20 < 40
			{3, 0.f, 60.f, 1},  //# 3킬 — 40 >= 40 → 승급
			{3, 20.f, 60.f, 0}, //# 4킬
			{3, 40.f, 60.f, 0}, //# 5킬
			{4, 0.f, 60.f, 1},  //# 6킬 = 맵 전멸 → 최대 레벨. 잔여 0 이라 진행도 바는 비어 있다(§7)
		};

	const int32 KillCount = static_cast<int32>(UE_ARRAY_COUNT(Expected));

	int32 CurrentLevel = 1;
	float CurrentExperience = 0.f;
	int32 TotalLevelsGained = 0;

	for (int32 KillIndex = 0; KillIndex < KillCount; ++KillIndex)
	{
		//# 이월된 잔여 경험치에 킬 보상을 더해 다시 판정한다
		CurrentExperience += SpyLevelTests_KillReward;

		const FSpyLevelUpResult Result = Config->ResolveLevelUp(CurrentLevel, CurrentExperience);

		CurrentLevel = Result.Level;
		CurrentExperience = Result.Experience;
		TotalLevelsGained += Result.LevelsGained;

		const FExpectedStep& Step = Expected[KillIndex];
		const int32 KillNumber = KillIndex + 1;

		TestEqual(*FString::Printf(TEXT("Kill %d level"), KillNumber), Result.Level, Step.Level);
		TestEqual(*FString::Printf(TEXT("Kill %d experience"), KillNumber), Result.Experience, Step.Experience);
		TestEqual(*FString::Printf(TEXT("Kill %d max experience"), KillNumber), Result.MaxExperience, Step.MaxExperience);
		TestEqual(*FString::Printf(TEXT("Kill %d levels gained"), KillNumber), Result.LevelsGained, Step.LevelsGained);
	}

	//# 검산 — 6킬 동안 레벨업은 정확히 3회, 커브 총합 120 을 남김없이 소진한다(§4-3)
	TestEqual(TEXT("Three level ups across six kills"), TotalLevelsGained, 3);
	TestEqual(TEXT("No experience left at max level"), CurrentExperience, 0.f);

	//# MaxHealth 성장분 — Config 가 정하는 것은 델타뿐이다(기본 100 은 GE_InitStat 소관)
	TestEqual(TEXT("MaxHealth growth delta over the session"),
			  Config->MaxHealthPerLevel * TotalLevelsGained, 30.f);
	TestEqual(TEXT("MaxMana growth delta over the session"),
			  Config->MaxManaPerLevel * TotalLevelsGained, 15.f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLevelDesignCurveDefaultsTest,
	"SkillProject.Character.Level.DesignCurveDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLevelDesignCurveDefaultsTest::RunTest(const FString& Parameters)
{
	//# 필드를 건드리지 않은 순수 기본값 — 기획서 §9-1 이 "그대로 유지" 로 확정한 4개
	const USpyLevelConfig* Config = NewObject<USpyLevelConfig>();

	TestEqual(TEXT("MaxHealthPerLevel default"), Config->MaxHealthPerLevel, 10.f);
	TestEqual(TEXT("MaxManaPerLevel default"), Config->MaxManaPerLevel, 5.f);
	TestEqual(TEXT("ExperienceRewardPerLevel default"), Config->ExperienceRewardPerLevel, 20.f);
	TestTrue(TEXT("bFullHealOnLevelUp default is true"), Config->bFullHealOnLevelUp);

	//# 처치 보상 공식 = 처치당한 대상 Level x 계수 (§3). 봇은 Lv1 이라 항상 20 이다.
	TestEqual(TEXT("Level 1 kill reward"), 1 * Config->ExperienceRewardPerLevel, 20.f);
	TestEqual(TEXT("Level 3 kill reward"), 3 * Config->ExperienceRewardPerLevel, 60.f);

	//# 커브 미입력 상태의 기본값은 빈 배열 — DA 미설정 시 최대 레벨 1
	TestEqual(TEXT("Curve has no code default"), Config->ExperienceToNextLevel.Num(), 0);
	TestEqual(TEXT("Max level without a curve"), Config->GetMaxLevel(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLevelDesignCurveOverflowTest,
	"SkillProject.Character.Level.DesignCurveOverflow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLevelDesignCurveOverflowTest::RunTest(const FString& Parameters)
{
	const USpyLevelConfig* Config = SpyLevelTests_MakeDesignConfig();

	//# 기획서 §7 — 최대 레벨에서 60(=3킬 분)을 더 얻으면 Min(60,60) 이라 버려지는 초과분이 없다
	const FSpyLevelUpResult Exact = Config->ResolveLevelUp(4, 60.f);
	TestEqual(TEXT("Level stays at max"), Exact.Level, 4);
	TestEqual(TEXT("Bar fills without discarding"), Exact.Experience, 60.f);
	TestEqual(TEXT("MaxExperience holds last curve entry"), Exact.MaxExperience, 60.f);
	TestEqual(TEXT("No level gained at max"), Exact.LevelsGained, 0);

	//# 4킬 분(80)부터 실제로 초과분이 폐기된다 (§7)
	const FSpyLevelUpResult Discarded = Config->ResolveLevelUp(4, 80.f);
	TestEqual(TEXT("Overflow discarded above max"), Discarded.Experience, 60.f);
	TestEqual(TEXT("Level unchanged on overflow"), Discarded.Level, 4);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLevelThresholdBoundaryTest,
	"SkillProject.Character.Level.ThresholdBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLevelThresholdBoundaryTest::RunTest(const FString& Parameters)
{
	const USpyLevelConfig* Config = SpyLevelTests_MakeConfig();

	//# 임계치 바로 아래 — 승급하지 않는다
	const FSpyLevelUpResult Below = Config->ResolveLevelUp(1, 99.99f);
	TestEqual(TEXT("Just below threshold does not level up"), Below.Level, 1);
	TestNearlyEqual(TEXT("Just below threshold keeps experience"), (double)Below.Experience, 99.99, 0.001);
	TestEqual(TEXT("Just below threshold gains nothing"), Below.LevelsGained, 0);

	//# 임계치 바로 위 — 승급하고 초과분이 이월된다
	const FSpyLevelUpResult Above = Config->ResolveLevelUp(1, 100.01f);
	TestEqual(TEXT("Just above threshold levels up"), Above.Level, 2);
	TestNearlyEqual(TEXT("Just above threshold carries remainder"), (double)Above.Experience, 0.01, 0.001);
	TestEqual(TEXT("Just above threshold advances max experience"), Above.MaxExperience, 200.f);
	TestEqual(TEXT("Just above threshold gains one level"), Above.LevelsGained, 1);

	//# 2단 임계치 경계 — 300(=100+200) 직전/직후
	const FSpyLevelUpResult TwoStepBelow = Config->ResolveLevelUp(1, 299.99f);
	TestEqual(TEXT("Below second threshold stops at level 2"), TwoStepBelow.Level, 2);
	TestEqual(TEXT("Below second threshold gains one level"), TwoStepBelow.LevelsGained, 1);

	const FSpyLevelUpResult TwoStepExact = Config->ResolveLevelUp(1, 300.f);
	TestEqual(TEXT("Exact second threshold reaches level 3"), TwoStepExact.Level, 3);
	TestEqual(TEXT("Exact second threshold consumes everything"), TwoStepExact.Experience, 0.f);
	TestEqual(TEXT("Exact second threshold gains two levels"), TwoStepExact.LevelsGained, 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLevelNegativeExperienceTest,
	"SkillProject.Character.Level.NegativeExperience",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLevelNegativeExperienceTest::RunTest(const FString& Parameters)
{
	const USpyLevelConfig* Config = SpyLevelTests_MakeConfig();

	//# 음수 경험치는 0 으로 클램프된다 — 음수 잔량이 HUD 진행도로 새어나가지 않는다
	const FSpyLevelUpResult Result = Config->ResolveLevelUp(2, -50.f);

	TestEqual(TEXT("Negative experience keeps level"), Result.Level, 2);
	TestEqual(TEXT("Negative experience clamped to zero"), Result.Experience, 0.f);
	TestEqual(TEXT("Negative experience keeps max experience"), Result.MaxExperience, 200.f);
	TestEqual(TEXT("Negative experience gains nothing"), Result.LevelsGained, 0);

	//# 정확히 0 도 같은 결과여야 한다
	const FSpyLevelUpResult Zero = Config->ResolveLevelUp(1, 0.f);
	TestEqual(TEXT("Zero experience keeps level"), Zero.Level, 1);
	TestEqual(TEXT("Zero experience stays zero"), Zero.Experience, 0.f);
	TestEqual(TEXT("Zero experience gains nothing"), Zero.LevelsGained, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLevelInvalidLevelInputTest,
	"SkillProject.Character.Level.InvalidLevelInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLevelInvalidLevelInputTest::RunTest(const FString& Parameters)
{
	const USpyLevelConfig* Config = SpyLevelTests_MakeConfig();

	//# InLevel 0 — FMath::Max(1, InLevel) 로 1 로 끌어올려 계산한다.
	//# LevelConfig 미지정으로 Level 어트리뷰트가 0 에 머무는 경우(기획서 §9-6)의 방어선이다.
	const FSpyLevelUpResult FromZero = Config->ResolveLevelUp(0, 100.f);
	TestEqual(TEXT("Level 0 clamped up then levels once"), FromZero.Level, 2);
	TestEqual(TEXT("Level 0 consumes first threshold"), FromZero.Experience, 0.f);
	TestEqual(TEXT("Level 0 gains one level"), FromZero.LevelsGained, 1);

	//# 음수 레벨도 동일하게 1 로 클램프된다
	const FSpyLevelUpResult FromNegative = Config->ResolveLevelUp(-5, 50.f);
	TestEqual(TEXT("Negative level clamped to one"), FromNegative.Level, 1);
	TestEqual(TEXT("Negative level keeps experience"), FromNegative.Experience, 50.f);
	TestEqual(TEXT("Negative level uses first curve entry"), FromNegative.MaxExperience, 100.f);

	//# 최대 레벨 초과 입력 — 현재 구현은 레벨을 아래로 끌어내리지 않고 그대로 통과시킨다.
	//# 정상 흐름에서는 발생하지 않는 입력이며, 여기서는 "현재 동작"을 고정해 둔다.
	const FSpyLevelUpResult AboveMax = Config->ResolveLevelUp(10, 500.f);
	TestEqual(TEXT("Level above max passes through unchanged"), AboveMax.Level, 10);
	TestEqual(TEXT("Level above max uses last curve entry"), AboveMax.MaxExperience, 300.f);
	TestEqual(TEXT("Level above max clamps experience"), AboveMax.Experience, 300.f);
	TestEqual(TEXT("Level above max gains nothing"), AboveMax.LevelsGained, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLevelZeroInCurveTest,
	"SkillProject.Character.Level.ZeroInCurve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLevelZeroInCurveTest::RunTest(const FString& Parameters)
{
	//# 커브 첫 칸이 0 — Required > 0.f 가드가 없으면 무한 루프에 빠지는 입력이다.
	//# 이 테스트가 완료된다는 사실 자체가 가드 동작의 증거다.
	{
		USpyLevelConfig* Config = NewObject<USpyLevelConfig>();
		Config->ExperienceToNextLevel = {0.f, 40.f};

		const FSpyLevelUpResult Result = Config->ResolveLevelUp(1, 100.f);

		TestEqual(TEXT("Leading zero blocks level up"), Result.Level, 1);
		TestEqual(TEXT("Leading zero keeps experience"), Result.Experience, 100.f);
		TestEqual(TEXT("Leading zero yields zero max experience"), Result.MaxExperience, 0.f);
		TestEqual(TEXT("Leading zero gains nothing"), Result.LevelsGained, 0);
	}

	//# 커브 중간이 0 — 0 칸에 도달한 시점에서 승급이 멈춘다
	{
		USpyLevelConfig* Config = NewObject<USpyLevelConfig>();
		Config->ExperienceToNextLevel = {20.f, 0.f, 60.f};

		const FSpyLevelUpResult Result = Config->ResolveLevelUp(1, 1000.f);

		TestEqual(TEXT("Mid zero stops at level 2"), Result.Level, 2);
		TestEqual(TEXT("Mid zero keeps remaining experience"), Result.Experience, 980.f);
		TestEqual(TEXT("Mid zero yields zero max experience"), Result.MaxExperience, 0.f);
		TestEqual(TEXT("Mid zero gains one level"), Result.LevelsGained, 1);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLevelSingleEntryCurveTest,
	"SkillProject.Character.Level.SingleEntryCurve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLevelSingleEntryCurveTest::RunTest(const FString& Parameters)
{
	USpyLevelConfig* Config = NewObject<USpyLevelConfig>();
	Config->ExperienceToNextLevel = {50.f};

	TestEqual(TEXT("Single entry curve max level is 2"), Config->GetMaxLevel(), 2);

	//# 정확히 임계치 — 한 번에 만렙 도달, 잔여 0
	const FSpyLevelUpResult Exact = Config->ResolveLevelUp(1, 50.f);
	TestEqual(TEXT("Single entry reaches max level"), Exact.Level, 2);
	TestEqual(TEXT("Single entry consumes threshold"), Exact.Experience, 0.f);
	TestEqual(TEXT("Single entry max experience holds last entry"), Exact.MaxExperience, 50.f);
	TestEqual(TEXT("Single entry gains one level"), Exact.LevelsGained, 1);

	//# 초과 투입 — 만렙 도달 후 잔여가 MaxExperience 로 클램프된다
	const FSpyLevelUpResult Overflow = Config->ResolveLevelUp(1, 500.f);
	TestEqual(TEXT("Single entry overflow stays at max level"), Overflow.Level, 2);
	TestEqual(TEXT("Single entry overflow clamped"), Overflow.Experience, 50.f);
	TestEqual(TEXT("Single entry overflow gains one level"), Overflow.LevelsGained, 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLevelHugeExperienceTest,
	"SkillProject.Character.Level.HugeExperience",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLevelHugeExperienceTest::RunTest(const FString& Parameters)
{
	const USpyLevelConfig* Config = SpyLevelTests_MakeConfig();

	//# 한 번에 커브 총합을 크게 넘는 경험치 — 최대 레벨에서 멈추고 초과분은 폐기된다
	const FSpyLevelUpResult Result = Config->ResolveLevelUp(1, 100000.f);

	TestEqual(TEXT("Huge experience stops at max level"), Result.Level, 4);
	TestEqual(TEXT("Huge experience clamped to max experience"), Result.Experience, 300.f);
	TestEqual(TEXT("Huge experience max experience is last entry"), Result.MaxExperience, 300.f);
	TestEqual(TEXT("Huge experience gains three levels"), Result.LevelsGained, 3);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLevelExperienceToNextLevelBoundsTest,
	"SkillProject.Character.Level.ExperienceToNextLevelBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLevelExperienceToNextLevelBoundsTest::RunTest(const FString& Parameters)
{
	const USpyLevelConfig* Config = SpyLevelTests_MakeConfig();

	//# 유효 인덱스 구간
	TestEqual(TEXT("Level 1 requirement"), Config->GetExperienceToNextLevel(1), 100.f);
	TestEqual(TEXT("Level 2 requirement"), Config->GetExperienceToNextLevel(2), 200.f);
	TestEqual(TEXT("Level 3 requirement"), Config->GetExperienceToNextLevel(3), 300.f);

	//# 최대 레벨 이상 — 마지막 커브값을 유지해 진행도 바 분모를 고정한다
	TestEqual(TEXT("Max level falls back to last entry"), Config->GetExperienceToNextLevel(4), 300.f);
	TestEqual(TEXT("Far above max falls back to last entry"), Config->GetExperienceToNextLevel(99), 300.f);

	//# InLevel 0 은 인덱스 -1 이라 같은 폴백(Last)에 걸린다.
	//# ResolveLevelUp 이 레벨을 1 이상으로 클램프한 뒤 호출하므로 정상 흐름에서는 도달하지 않는다 — 현재 동작 고정.
	TestEqual(TEXT("Level 0 falls back to last entry"), Config->GetExperienceToNextLevel(0), 300.f);

	//# 빈 커브는 어떤 레벨을 넣어도 0
	USpyLevelConfig* EmptyConfig = NewObject<USpyLevelConfig>();
	EmptyConfig->ExperienceToNextLevel.Empty();

	TestEqual(TEXT("Empty curve requirement at level 1"), EmptyConfig->GetExperienceToNextLevel(1), 0.f);
	TestEqual(TEXT("Empty curve requirement at level 5"), EmptyConfig->GetExperienceToNextLevel(5), 0.f);
	TestEqual(TEXT("Empty curve max level is one"), EmptyConfig->GetMaxLevel(), 1);

	return true;
}

#endif //# WITH_DEV_AUTOMATION_TESTS
