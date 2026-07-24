// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Data/SpyLoadingConfig.h"
#include "Manager/SpyLoadingSubsystem.h"

//# CombineProgress 경계값 — Total == 0 이면 1단계는 완료로 본다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingCombineEmptyTotalTest,
	"SkillProject.Manager.Loading.CombineEmptyTotal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingCombineEmptyTotalTest::RunTest(const FString& Parameters)
{
	//# 1단계 대상 0개 + 2단계 미시작(-1) + 가중치 0.5 → 0.5
	const float Raw = USpyLoadingSubsystem::CombineProgress(0, 0, -1.f, 0.5f);

	TestEqual(TEXT("Phase1 counts as complete"), Raw, 0.5f, KINDA_SMALL_NUMBER);

	return true;
}

//# Loaded == 0 이면 1단계 기여는 0
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingCombineZeroLoadedTest,
	"SkillProject.Manager.Loading.CombineZeroLoaded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingCombineZeroLoadedTest::RunTest(const FString& Parameters)
{
	const float Raw = USpyLoadingSubsystem::CombineProgress(0, 10, -1.f, 0.5f);

	TestEqual(TEXT("Nothing loaded yet"), Raw, 0.f, KINDA_SMALL_NUMBER);

	return true;
}

//# 가중치 경계 — W == 0 이면 2단계만, W == 1 이면 1단계만 반영
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingCombineWeightBoundsTest,
	"SkillProject.Manager.Loading.CombineWeightBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingCombineWeightBoundsTest::RunTest(const FString& Parameters)
{
	//# W == 0 → 1단계 100% 여도 2단계(50%)만 반영
	TestEqual(TEXT("W == 0 uses map only"), USpyLoadingSubsystem::CombineProgress(10, 10, 50.f, 0.f), 0.5f, KINDA_SMALL_NUMBER);

	//# W == 1 → 2단계 100% 여도 1단계(50%)만 반영
	TestEqual(TEXT("W == 1 uses assets only"), USpyLoadingSubsystem::CombineProgress(5, 10, 100.f, 1.f), 0.5f, KINDA_SMALL_NUMBER);

	//# 범위 밖 가중치는 클램프된다
	TestEqual(TEXT("Weight clamped high"), USpyLoadingSubsystem::CombineProgress(10, 10, 0.f, 5.f), 1.f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Weight clamped low"), USpyLoadingSubsystem::CombineProgress(0, 10, 100.f, -5.f), 1.f, KINDA_SMALL_NUMBER);

	return true;
}

//# 2단계 미시작(-1)은 0 으로 바닥을 잡는다 — 음수가 진행률을 끌어내리면 안 된다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingCombineNegativeMapPercentTest,
	"SkillProject.Manager.Loading.CombineNegativeMapPercent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingCombineNegativeMapPercentTest::RunTest(const FString& Parameters)
{
	const float Raw = USpyLoadingSubsystem::CombineProgress(10, 10, -1.f, 0.5f);

	TestEqual(TEXT("Negative map percent floored to 0"), Raw, 0.5f, KINDA_SMALL_NUMBER);
	TestTrue(TEXT("Never below zero"), Raw >= 0.f);

	return true;
}

//# Displayed 는 시간 클램프를 받고 1.0 을 넘지 않는다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingClampDisplayedTest,
	"SkillProject.Manager.Loading.ClampDisplayed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingClampDisplayedTest::RunTest(const FString& Parameters)
{
	//# 로드는 끝났지만 1초만 지났으면 절반까지만 보여준다
	TestEqual(TEXT("Time clamped"), USpyLoadingSubsystem::ClampDisplayed(1.f, 1.f, 2.f), 0.5f, KINDA_SMALL_NUMBER);

	//# 시간이 충분해도 Raw 를 넘지 않는다
	TestEqual(TEXT("Raw is the ceiling"), USpyLoadingSubsystem::ClampDisplayed(0.3f, 10.f, 2.f), 0.3f, KINDA_SMALL_NUMBER);

	//# 1.0 초과 금지
	TestEqual(TEXT("Never exceeds one"), USpyLoadingSubsystem::ClampDisplayed(1.f, 10.f, 2.f), 1.f, KINDA_SMALL_NUMBER);

	//# MinDisplaySeconds <= 0 이면 시간 클램프를 건너뛴다
	TestEqual(TEXT("No time clamp when min is zero"), USpyLoadingSubsystem::ClampDisplayed(1.f, 0.f, 0.f), 1.f, KINDA_SMALL_NUMBER);

	return true;
}

//# Displayed 는 시간이 흐를수록 단조 증가한다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingDisplayedMonotonicTest,
	"SkillProject.Manager.Loading.DisplayedMonotonic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingDisplayedMonotonicTest::RunTest(const FString& Parameters)
{
	float Previous = 0.f;

	for (int32 Step = 0; Step <= 40; ++Step)
	{
		const float Elapsed = (float)Step * 0.1f;
		const float Displayed = USpyLoadingSubsystem::ClampDisplayed(1.f, Elapsed, 2.f);

		TestTrue(TEXT("Monotonic increase"), Displayed >= Previous);
		TestTrue(TEXT("Never exceeds one"), Displayed <= 1.f + KINDA_SMALL_NUMBER);

		Previous = Displayed;
	}

	return true;
}

//# 최소 표시 시간 전에는 전환하지 않는다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingTransitionBeforeMinTimeTest,
	"SkillProject.Manager.Loading.TransitionBeforeMinTime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingTransitionBeforeMinTimeTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("Not yet at min display time"), USpyLoadingSubsystem::ShouldTransition(1.f, 1.9f, 2.f));
	TestTrue(TEXT("At min display time"), USpyLoadingSubsystem::ShouldTransition(1.f, 2.f, 2.f));

	return true;
}

//# Raw 가 1.0 미만이면 시간이 아무리 지나도 전환하지 않는다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingTransitionIncompleteRawTest,
	"SkillProject.Manager.Loading.TransitionIncompleteRaw",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingTransitionIncompleteRawTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("Raw below one blocks transition"), USpyLoadingSubsystem::ShouldTransition(0.99f, 1000.f, 2.f));

	return true;
}

//# Config 누락 — 크래시 없이 false 를 반환하고 전환을 시작하지 않는다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingMissingConfigTest,
	"SkillProject.Manager.Loading.MissingConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingMissingConfigTest::RunTest(const FString& Parameters)
{
	//# ApplyConfig 실패 경로는 Error 로그를 2회 남긴다 — 등록하지 않으면 자동화 프레임워크가 이를 테스트 실패로 집계한다
	AddExpectedErrorPlain(TEXT("[SpyLoadingSubsystem] LoadingConfig"), EAutomationExpectedErrorFlags::Contains, 2);

	//# GameInstanceSubsystem 은 ClassWithin = UGameInstance 이므로 반드시 GameInstance 아우터로 생성한다 (라이프사이클은 돌리지 않음)
	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	USpyLoadingSubsystem* Subsystem = NewObject<USpyLoadingSubsystem>(GameInstance);

	//# Config 자체가 없음
	TestFalse(TEXT("Null config rejected"), Subsystem->ApplyConfig(nullptr));

	//# Config 는 있으나 GameplayMap 미설정
	USpyLoadingConfig* Config = NewObject<USpyLoadingConfig>();
	TestFalse(TEXT("Unset gameplay map rejected"), Subsystem->ApplyConfig(Config));

	TestEqual(TEXT("Progress untouched"), Subsystem->GetDisplayedProgress(), 0.f, KINDA_SMALL_NUMBER);

	return true;
}

#endif
