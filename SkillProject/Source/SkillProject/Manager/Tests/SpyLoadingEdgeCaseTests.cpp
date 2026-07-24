// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Data/SpyLoadingConfig.h"
#include "Manager/SpyLoadingSubsystem.h"

//# 확정값 — MinDisplaySeconds=2.5(loading-scene.md §2), AssetPhaseWeight=0.99(데모 99/1 재배치). 시뮬레이션 입력 고정
static const float SpyLoadingTest_DesignWeight = 0.99f;
static const float SpyLoadingTest_DesignMinDisplaySeconds = 2.5f;
static const float SpyLoadingTest_FrameDelta = 1.f / 60.f;

//# 진행률 콜백이 중복 도착해 Loaded > Total 이 되어도 1.0 을 넘지 않는다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingCombineLoadedExceedsTotalTest,
	"SkillProject.Manager.Loading.CombineLoadedExceedsTotal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingCombineLoadedExceedsTotalTest::RunTest(const FString& Parameters)
{
	//# 1단계 초과 카운트 + 2단계 완료 — 합이 1.05 여도 최종 클램프가 잡는다
	TestEqual(TEXT("Overshoot clamped to one"), USpyLoadingSubsystem::CombineProgress(12, 10, 100.f, 0.25f), 1.f, KINDA_SMALL_NUMBER);

	//# 가중치 전부를 1단계가 먹는 경우의 초과도 마찬가지
	TestEqual(TEXT("Weight one overshoot clamped"), USpyLoadingSubsystem::CombineProgress(20, 10, -1.f, 1.f), 1.f, KINDA_SMALL_NUMBER);

	//# 2단계 미시작 상태에서의 초과는 가중치 몫만큼만 새어 나온다 (0.3 > 0.25) — 상한 안이므로 클램프되지 않는다
	const float Leak = USpyLoadingSubsystem::CombineProgress(12, 10, -1.f, 0.25f);
	TestEqual(TEXT("Overshoot scaled by weight"), Leak, 0.3f, KINDA_SMALL_NUMBER);
	TestTrue(TEXT("Still within range"), Leak <= 1.f);

	return true;
}

//# 대용량 카운트 — 정수 나눗셈·오버플로 없이 비율이 유지된다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingCombineLargeCountsTest,
	"SkillProject.Manager.Loading.CombineLargeCounts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingCombineLargeCountsTest::RunTest(const FString& Parameters)
{
	//# 정수 나눗셈이었다면 0 또는 1 로 뭉개진다
	TestEqual(TEXT("Half of a million"), USpyLoadingSubsystem::CombineProgress(1000000, 2000000, -1.f, 1.f), 0.5f, KINDA_SMALL_NUMBER);

	//# int32 상한 — 곱셈 오버플로 없이 완료로 계산된다
	TestEqual(TEXT("Max int total complete"), USpyLoadingSubsystem::CombineProgress(MAX_int32, MAX_int32, 100.f, 0.25f), 1.f, KINDA_SMALL_NUMBER);

	//# 상한의 절반 — float 변환 오차가 있어도 0.5 근방을 유지한다
	TestEqual(TEXT("Max int half"), USpyLoadingSubsystem::CombineProgress(MAX_int32 / 2, MAX_int32, -1.f, 1.f), 0.5f, 1.e-3f);

	return true;
}

//# 기획 확정 가중치(0.99)에서 1단계 완료·2단계 미시작 구간의 정체 높이가 정확히 W 다 (기획서 §3-3)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingCombineDesignWeightPlateauTest,
	"SkillProject.Manager.Loading.CombineDesignWeightPlateau",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingCombineDesignWeightPlateauTest::RunTest(const FString& Parameters)
{
	//# 2단계 미시작(-1)
	TestEqual(TEXT("Plateau equals weight"), USpyLoadingSubsystem::CombineProgress(8, 8, -1.f, SpyLoadingTest_DesignWeight), 0.99f, KINDA_SMALL_NUMBER);

	//# 2단계 시작 직후 0%
	TestEqual(TEXT("Plateau holds at zero percent"), USpyLoadingSubsystem::CombineProgress(8, 8, 0.f, SpyLoadingTest_DesignWeight), 0.99f, KINDA_SMALL_NUMBER);

	//# 2단계가 절반이면 0.99 + 0.005
	TestEqual(TEXT("Half map percent"), USpyLoadingSubsystem::CombineProgress(8, 8, 50.f, SpyLoadingTest_DesignWeight), 0.995f, KINDA_SMALL_NUMBER);

	return true;
}

//# 음수 입력 방어 — NaN·무한대 없이 유한값을 돌려주고 Raw 상한을 넘지 않는다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingClampNegativeInputsTest,
	"SkillProject.Manager.Loading.ClampNegativeInputs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingClampNegativeInputsTest::RunTest(const FString& Parameters)
{
	//# Raw 가 음수면 0 으로 바닥을 잡는다
	TestEqual(TEXT("Negative raw floored"), USpyLoadingSubsystem::ClampDisplayed(-0.5f, 1.f, 2.5f), 0.f, KINDA_SMALL_NUMBER);

	//# Elapsed 가 음수(일시정지·타임스케일 조작)여도 크래시·NaN 이 없어야 한다.
	//# 하한 클램프는 없으므로 음수가 그대로 나올 수 있다 — Tick 의 "커질 때만 반영" 게이트가 실사용에서 이를 막는다
	const float NegativeElapsed = USpyLoadingSubsystem::ClampDisplayed(0.5f, -1.f, 2.5f);
	TestFalse(TEXT("Not NaN"), FMath::IsNaN(NegativeElapsed));
	TestTrue(TEXT("Finite"), FMath::IsFinite(NegativeElapsed));
	TestTrue(TEXT("Never above raw"), NegativeElapsed <= 0.5f);
	TestTrue(TEXT("Never above one"), NegativeElapsed <= 1.f);

	//# 둘 다 음수인 최악의 조합
	const float BothNegative = USpyLoadingSubsystem::ClampDisplayed(-1.f, -1.f, 2.5f);
	TestFalse(TEXT("Not NaN with both negative"), FMath::IsNaN(BothNegative));
	TestTrue(TEXT("Finite with both negative"), FMath::IsFinite(BothNegative));
	TestTrue(TEXT("Never above one with both negative"), BothNegative <= 1.f);

	return true;
}

//# Elapsed == 0 — 첫 프레임에는 아무것도 채워지지 않는다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingClampZeroElapsedTest,
	"SkillProject.Manager.Loading.ClampZeroElapsed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingClampZeroElapsedTest::RunTest(const FString& Parameters)
{
	//# 로드가 이미 끝나 있어도 경과 시간이 0 이면 0 이다
	TestEqual(TEXT("Zero elapsed shows nothing"), USpyLoadingSubsystem::ClampDisplayed(1.f, 0.f, 2.5f), 0.f, KINDA_SMALL_NUMBER);

	//# Raw 도 0 인 정상 출발점
	TestEqual(TEXT("Zero raw zero elapsed"), USpyLoadingSubsystem::ClampDisplayed(0.f, 0.f, 2.5f), 0.f, KINDA_SMALL_NUMBER);

	//# 최소 표시 시간이 아주 짧아도 0 초에는 0
	TestEqual(TEXT("Zero elapsed with tiny min"), USpyLoadingSubsystem::ClampDisplayed(0.3f, 0.f, 0.001f), 0.f, KINDA_SMALL_NUMBER);

	return true;
}

//# 아주 작은 MinDisplaySeconds — 0 나눗셈 없이 급격한 램프가 성립한다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingClampTinyMinDisplayTest,
	"SkillProject.Manager.Loading.ClampTinyMinDisplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingClampTinyMinDisplayTest::RunTest(const FString& Parameters)
{
	//# 램프의 절반 지점
	TestEqual(TEXT("Half of tiny ramp"), USpyLoadingSubsystem::ClampDisplayed(1.f, 0.0005f, 0.001f), 0.5f, KINDA_SMALL_NUMBER);

	//# 램프 끝
	TestEqual(TEXT("End of tiny ramp"), USpyLoadingSubsystem::ClampDisplayed(1.f, 0.001f, 0.001f), 1.f, KINDA_SMALL_NUMBER);

	//# 램프를 한참 넘겨도 1.0 상한
	const float Overshoot = USpyLoadingSubsystem::ClampDisplayed(1.f, 10.f, 0.001f);
	TestEqual(TEXT("Past tiny ramp"), Overshoot, 1.f, KINDA_SMALL_NUMBER);
	TestTrue(TEXT("Finite past tiny ramp"), FMath::IsFinite(Overshoot));

	return true;
}

//# MinDisplaySeconds 가 0 이하(음수 포함)면 시간 클램프를 건너뛰고 즉시 전환이 성립한다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingNonPositiveMinDisplayTest,
	"SkillProject.Manager.Loading.NonPositiveMinDisplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingNonPositiveMinDisplayTest::RunTest(const FString& Parameters)
{
	//# 음수 최소 표시 시간 — 0 과 동일하게 Raw 를 그대로 통과시킨다
	TestEqual(TEXT("Negative min passes raw through"), USpyLoadingSubsystem::ClampDisplayed(0.42f, 0.f, -1.f), 0.42f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Negative min still clamps to one"), USpyLoadingSubsystem::ClampDisplayed(2.f, 0.f, -0.001f), 1.f, KINDA_SMALL_NUMBER);

	//# ClampDisplayed 와 조합 — 로드가 끝나 있으면 0 초에 즉시 전환된다
	TestTrue(TEXT("Zero min transitions immediately"), USpyLoadingSubsystem::ShouldTransition(1.f, 0.f, 0.f));
	TestTrue(TEXT("Negative min transitions immediately"), USpyLoadingSubsystem::ShouldTransition(1.f, 0.f, -1.f));

	//# 다만 로드가 안 끝났으면 여전히 전환하지 않는다
	TestFalse(TEXT("Incomplete raw still blocks"), USpyLoadingSubsystem::ShouldTransition(0.99f, 0.f, -1.f));

	return true;
}

//# ApplyConfig 재호출 — 성공 후 nullptr 로 다시 불려도 기존 상태를 오염시키지 않는다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingConfigApplyThenNullTest,
	"SkillProject.Manager.Loading.ConfigApplyThenNull",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingConfigApplyThenNullTest::RunTest(const FString& Parameters)
{
	//# 두 번째 호출이 Error 로그를 남긴다 — 자동화 프레임워크가 이를 실패로 집계하지 않게 미리 등록한다
	AddExpectedErrorPlain(TEXT("[SpyLoadingSubsystem] LoadingConfig"), EAutomationExpectedErrorFlags::Contains, 1);

	//# GameInstanceSubsystem 은 ClassWithin = UGameInstance 이므로 반드시 GameInstance 아우터로 생성한다 (라이프사이클은 돌리지 않음)
	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	USpyLoadingSubsystem* Subsystem = NewObject<USpyLoadingSubsystem>(GameInstance);
	USpyLoadingConfig* Config = NewObject<USpyLoadingConfig>();

	//# 실제 에셋을 로드하지 않는다 — 경로 문자열의 유효성만 사용한다
	Config->GameplayMap = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Spy/Maps/DevMap.DevMap")));

	TestTrue(TEXT("Valid config accepted"), Subsystem->ApplyConfig(Config));

	//# 재호출 실패는 조기 반환이므로 이전 성공 상태를 덮어쓰지 않아야 한다
	TestFalse(TEXT("Null config rejected on re-apply"), Subsystem->ApplyConfig(nullptr));

	//# 두 번째 유효 호출이 여전히 받아들여진다 — 계약이 1회성으로 굳지 않는다
	TestTrue(TEXT("Valid config accepted again"), Subsystem->ApplyConfig(Config));

	TestEqual(TEXT("Progress untouched"), Subsystem->GetDisplayedProgress(), 0.f, KINDA_SMALL_NUMBER);

	return true;
}

//# C++ 기본값 회귀 고정 — DataAsset 이 유실돼도 기획 확정 페이싱이 나와야 한다 (기획서 §9-4)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingConfigDesignDefaultsTest,
	"SkillProject.Manager.Loading.ConfigDesignDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingConfigDesignDefaultsTest::RunTest(const FString& Parameters)
{
	const USpyLoadingConfig* Defaults = GetDefault<USpyLoadingConfig>();

	TestNotNull(TEXT("CDO exists"), Defaults);
	if (Defaults == nullptr)
	{
		return false;
	}

	TestEqual(TEXT("MinDisplaySeconds default"), Defaults->MinDisplaySeconds, SpyLoadingTest_DesignMinDisplaySeconds, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("AssetPhaseWeight default"), Defaults->AssetPhaseWeight, SpyLoadingTest_DesignWeight, KINDA_SMALL_NUMBER);

	return true;
}

//# 파이프라인 시뮬레이션 — 1단계 0→100% 후 2단계 0→100%, 기획 확정값(W=0.99 / Min=2.5) 기준
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingPipelineSequenceTest,
	"SkillProject.Manager.Loading.PipelineSequence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingPipelineSequenceTest::RunTest(const FString& Parameters)
{
	const int32 AssetTotal = 8;
	const int32 Phase1EndStep = 32;
	const int32 Phase2StepCount = 60;
	const int32 TotalSteps = 240;

	//# Tick 을 그대로 흉내낸다 — 값이 커질 때만 반영하는 게이트 포함
	float Elapsed = 0.f;
	float Displayed = 0.f;
	float PreviousCandidate = 0.f;

	bool bTransitioned = false;
	bool bTransitionEverReverted = false;
	float TransitionElapsed = 0.f;
	float TransitionDisplayed = 0.f;

	for (int32 Step = 0; Step < TotalSteps; ++Step)
	{
		Elapsed += SpyLoadingTest_FrameDelta;

		const int32 Loaded = FMath::Min(AssetTotal, Step / 4);

		//# 1단계가 끝나기 전에는 2단계가 시작되지 않았으므로 -1 (GetAsyncLoadPercentage 미시작 값)
		const float MapPercent = (Step < Phase1EndStep)
									 ? -1.f
									 : FMath::Min(100.f, (float)(Step - Phase1EndStep) * (100.f / (float)Phase2StepCount));

		const float Raw = USpyLoadingSubsystem::CombineProgress(Loaded, AssetTotal, MapPercent, SpyLoadingTest_DesignWeight);
		const float Candidate = USpyLoadingSubsystem::ClampDisplayed(Raw, Elapsed, SpyLoadingTest_DesignMinDisplaySeconds);

		//# 게이트 이전 단계에서 이미 단조 증가여야 한다 — 가중치 로직이 뒤집히면 여기서 잡힌다
		TestTrue(TEXT("Candidate never decreases"), Candidate >= PreviousCandidate - KINDA_SMALL_NUMBER);
		TestTrue(TEXT("Candidate never exceeds one"), Candidate <= 1.f + KINDA_SMALL_NUMBER);
		PreviousCandidate = Candidate;

		if (Candidate > Displayed)
		{
			Displayed = Candidate;
		}

		//# 최소 표시 시간 전에는 시간 램프가 항상 상한이다
		if (Elapsed < SpyLoadingTest_DesignMinDisplaySeconds)
		{
			TestTrue(TEXT("Time ramp caps display"), Displayed <= (Elapsed / SpyLoadingTest_DesignMinDisplaySeconds) + KINDA_SMALL_NUMBER);
			TestTrue(TEXT("Not full before min display time"), Displayed < 1.f);
		}

		const bool bShouldTransition = USpyLoadingSubsystem::ShouldTransition(Raw, Elapsed, SpyLoadingTest_DesignMinDisplaySeconds);

		if (bShouldTransition && bTransitioned == false)
		{
			bTransitioned = true;
			TransitionElapsed = Elapsed;
			TransitionDisplayed = Displayed;
		}

		//# 한 번 참이 된 뒤 다시 거짓으로 떨어지면 안 된다
		if (bTransitioned && bShouldTransition == false)
		{
			bTransitionEverReverted = true;
		}
	}

	TestTrue(TEXT("Transition happened"), bTransitioned);
	TestFalse(TEXT("Transition never reverted"), bTransitionEverReverted);
	TestTrue(TEXT("Transition respected min display time"), TransitionElapsed >= SpyLoadingTest_DesignMinDisplaySeconds - KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Bar full at transition"), TransitionDisplayed, 1.f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Bar full at the end"), Displayed, 1.f, KINDA_SMALL_NUMBER);

	return true;
}

//# 진행률 콜백이 되돌아가거나 중복 도착해도 표시값이 역행하지 않는다 (Tick 게이트 회귀 고정)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingPipelineRegressingProgressTest,
	"SkillProject.Manager.Loading.PipelineRegressingProgress",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingPipelineRegressingProgressTest::RunTest(const FString& Parameters)
{
	const int32 AssetTotal = 8;

	//# (Loaded, MapPercent) — 3·6 번째가 역행 입력이다
	const int32 LoadedSequence[] = {4, 8, 2, 8, 8, 8, 8};
	const float MapSequence[] = {-1.f, -1.f, -1.f, -1.f, 50.f, 20.f, 100.f};
	const int32 StepCount = UE_ARRAY_COUNT(LoadedSequence);

	float Elapsed = 0.f;
	float Displayed = 0.f;
	int32 TransitionCount = 0;

	for (int32 Step = 0; Step < StepCount; ++Step)
	{
		//# 시간 램프가 마지막에 상한이 되지 않도록 넉넉히 진행시킨다
		Elapsed += 0.5f;

		const float Raw = USpyLoadingSubsystem::CombineProgress(LoadedSequence[Step], AssetTotal, MapSequence[Step], SpyLoadingTest_DesignWeight);
		const float Candidate = USpyLoadingSubsystem::ClampDisplayed(Raw, Elapsed, SpyLoadingTest_DesignMinDisplaySeconds);

		const float Before = Displayed;
		if (Candidate > Displayed)
		{
			Displayed = Candidate;
		}

		TestTrue(TEXT("Displayed never regresses"), Displayed >= Before);
		TestTrue(TEXT("Displayed never exceeds one"), Displayed <= 1.f + KINDA_SMALL_NUMBER);

		if (USpyLoadingSubsystem::ShouldTransition(Raw, Elapsed, SpyLoadingTest_DesignMinDisplaySeconds))
		{
			++TransitionCount;
		}
	}

	//# 마지막 스텝에서만 전환 조건이 성립한다
	TestEqual(TEXT("Transition only at the end"), TransitionCount, 1);
	TestEqual(TEXT("Bar full at the end"), Displayed, 1.f, KINDA_SMALL_NUMBER);

	return true;
}

#endif
