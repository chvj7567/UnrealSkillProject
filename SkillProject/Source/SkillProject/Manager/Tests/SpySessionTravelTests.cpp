// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Manager/SpyLoadingSubsystem.h"

//# 주소 우선순위 — 조인이 넘긴 override 가 config 보다 우선한다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyResolveTravelAddressTest,
	"SkillProject.Manager.Session.ResolveTravelAddress",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyResolveTravelAddressTest::RunTest(const FString& Parameters)
{
	//# 조인 경로 — override 가 이긴다
	TestEqual(TEXT("Override wins over config"),
			  USpyLoadingSubsystem::ResolveTravelAddress(TEXT("10.0.0.5:7777"), TEXT("127.0.0.1:7777")),
			  FString(TEXT("10.0.0.5:7777")));

	//# 기존 자동 접속 경로 — override 가 없으면 config 를 쓴다(회귀 방지)
	TestEqual(TEXT("Config used when no override"),
			  USpyLoadingSubsystem::ResolveTravelAddress(TEXT(""), TEXT("127.0.0.1:7777")),
			  FString(TEXT("127.0.0.1:7777")));

	//# 둘 다 비면 오프라인 폴백 판정으로 넘어가야 하므로 빈 문자열
	TestEqual(TEXT("Empty when both empty"),
			  USpyLoadingSubsystem::ResolveTravelAddress(TEXT(""), TEXT("")),
			  FString(TEXT("")));

	//# override 가 있으면 config 가 비어 있어도 접속한다
	TestEqual(TEXT("Override alone is enough"),
			  USpyLoadingSubsystem::ResolveTravelAddress(TEXT("10.0.0.5:7777"), TEXT("")),
			  FString(TEXT("10.0.0.5:7777")));

	return true;
}

//# 리슨 서버 URL — 맵 패키지명에 ?listen 옵션을 붙인다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMakeListenTravelURLTest,
	"SkillProject.Manager.Session.MakeListenTravelURL",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMakeListenTravelURLTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Appends listen option"),
			  USpyLoadingSubsystem::MakeListenTravelURL(TEXT("/Game/Spy/Maps/DevMap")),
			  FString(TEXT("/Game/Spy/Maps/DevMap?listen")));

	//# 이미 옵션이 붙어 있으면 중복해서 붙이지 않는다
	TestEqual(TEXT("Does not duplicate listen option"),
			  USpyLoadingSubsystem::MakeListenTravelURL(TEXT("/Game/Spy/Maps/DevMap?listen")),
			  FString(TEXT("/Game/Spy/Maps/DevMap?listen")));

	//# 빈 맵 이름은 빈 문자열 — 호출부가 트래블을 중단해야 한다
	TestEqual(TEXT("Empty map yields empty url"),
			  USpyLoadingSubsystem::MakeListenTravelURL(TEXT("")),
			  FString(TEXT("")));

	return true;
}

//# 방 목록 표시 판정 — config 주소가 비어 있을 때만 방 목록을 띄운다(D6 이중 경로)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyShouldShowSessionBrowserTest,
	"SkillProject.Manager.Session.ShouldShowSessionBrowser",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyShouldShowSessionBrowserTest::RunTest(const FString& Parameters)
{
	//# 기본 경로 — 주소가 비었으므로 방 목록
	TestTrue(TEXT("Empty config address shows browser"),
			 USpyLoadingSubsystem::ShouldShowSessionBrowser(TEXT("")));

	//# 데디서버·CI 경로 — 주소가 있으면 기존 자동 접속을 유지한다
	TestFalse(TEXT("Configured address keeps auto connect"),
			  USpyLoadingSubsystem::ShouldShowSessionBrowser(TEXT("127.0.0.1:7777")));

	return true;
}

#endif
