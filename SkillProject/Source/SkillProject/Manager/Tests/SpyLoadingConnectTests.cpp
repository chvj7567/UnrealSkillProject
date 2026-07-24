// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Manager/SpyLoadingSubsystem.h"

//# ServerAddress 가 비어 있지 않으면 접속 모드
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingShouldConnectTest,
	"SkillProject.Manager.Loading.ShouldConnect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingShouldConnectTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Non-empty address connects"), USpyLoadingSubsystem::ShouldConnectToServer(TEXT("127.0.0.1:7777")));
	TestFalse(TEXT("Empty address is offline"), USpyLoadingSubsystem::ShouldConnectToServer(TEXT("")));

	return true;
}

//# 도착 판정 — 로드된 맵이 로딩맵이면 무시(true), 다른 맵이면 도착(false)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingIsLoadingMapNameTest,
	"SkillProject.Manager.Loading.IsLoadingMapName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingIsLoadingMapNameTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Same name is loading map"), USpyLoadingSubsystem::IsLoadingMapName(FName("LoadingMap"), FName("LoadingMap")));
	TestFalse(TEXT("Gameplay map is not loading map"), USpyLoadingSubsystem::IsLoadingMapName(FName("DevMap"), FName("LoadingMap")));

	return true;
}

//# 타임아웃 — 경과 >= 타임아웃일 때만 true, 0 이하 타임아웃은 항상 false(무제한)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingConnectTimeoutTest,
	"SkillProject.Manager.Loading.ConnectTimeout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingConnectTimeoutTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("Before timeout"), USpyLoadingSubsystem::HasConnectTimedOut(14.9f, 15.f));
	TestTrue(TEXT("At timeout"), USpyLoadingSubsystem::HasConnectTimedOut(15.f, 15.f));
	TestFalse(TEXT("Zero timeout means unlimited"), USpyLoadingSubsystem::HasConnectTimedOut(1000.f, 0.f));
	TestFalse(TEXT("Negative timeout means unlimited"), USpyLoadingSubsystem::HasConnectTimedOut(1000.f, -1.f));

	return true;
}

//# 접속 단계 표시 진행률 — 프리로드 지점에서 시작해 도착 전 1.0 에 못 닿는다(상한 0.95)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingConnectPhaseDisplayedTest,
	"SkillProject.Manager.Loading.ConnectPhaseDisplayed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingConnectPhaseDisplayedTest::RunTest(const FString& Parameters)
{
	//# 접속 시작(경과 0) → 프리로드 가중치에서 시작
	TestEqual(TEXT("Starts at preload weight"), USpyLoadingSubsystem::ConnectPhaseDisplayed(0.9f, 0.f, 15.f), 0.9f, KINDA_SMALL_NUMBER);

	//# 페이싱 절반(7.5s) → 0.9 + 0.1*0.5*0.95 = 0.9475
	TestEqual(TEXT("Half pacing"), USpyLoadingSubsystem::ConnectPhaseDisplayed(0.9f, 7.5f, 15.f), 0.9475f, KINDA_SMALL_NUMBER);

	//# 페이싱 도달/초과 → 상한(0.9 + 0.1*0.95 = 0.995), 절대 1.0 미만
	TestEqual(TEXT("Caps below one"), USpyLoadingSubsystem::ConnectPhaseDisplayed(0.9f, 100.f, 15.f), 0.995f, KINDA_SMALL_NUMBER);
	TestTrue(TEXT("Never reaches one"), USpyLoadingSubsystem::ConnectPhaseDisplayed(0.9f, 1e9f, 15.f) < 1.f);

	//# 페이싱 0 이하 → 즉시 상한
	TestEqual(TEXT("Zero pacing jumps to cap"), USpyLoadingSubsystem::ConnectPhaseDisplayed(0.9f, 0.f, 0.f), 0.995f, KINDA_SMALL_NUMBER);

	return true;
}

#endif
