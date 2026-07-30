// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "SKOnlineSettings.h"

//# 기본 프로필 = OSS Null(LAN). Steam 전환은 ini 로만 이뤄져야 한다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSKOnlineSettingsNullDefaultsTest,
	"SKOnline.Session.Settings.NullDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSKOnlineSettingsNullDefaultsTest::RunTest(const FString& Parameters)
{
	const USKOnlineSettings* Settings = GetDefault<USKOnlineSettings>();
	if (Settings == nullptr)
	{
		AddError(TEXT("USKOnlineSettings CDO is null"));
		return false;
	}

	//# Null 백엔드는 LAN 브로드캐스트로만 방을 찾는다
	TestTrue(TEXT("LAN match on by default"), Settings->bIsLanMatch);

	//# presence/lobby 는 Steam 전용 — Null 에서 켜면 검색이 깨진다
	TestFalse(TEXT("Presence off by default"), Settings->bUsesPresence);
	TestFalse(TEXT("Lobbies off by default"), Settings->bUseLobbiesIfAvailable);

	//# 광고를 끄면 방이 목록에 뜨지 않는다
	TestTrue(TEXT("Advertise on by default"), Settings->bShouldAdvertise);

	return true;
}

//# 인원/검색 수는 1 이상이어야 세션이 성립한다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSKOnlineSettingsPositiveCountsTest,
	"SKOnline.Session.Settings.PositiveCounts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSKOnlineSettingsPositiveCountsTest::RunTest(const FString& Parameters)
{
	const USKOnlineSettings* Settings = GetDefault<USKOnlineSettings>();
	if (Settings == nullptr)
	{
		AddError(TEXT("USKOnlineSettings CDO is null"));
		return false;
	}

	TestTrue(TEXT("MaxPlayers is at least two"), Settings->MaxPlayers >= 2);
	TestTrue(TEXT("MaxSearchResults is positive"), Settings->MaxSearchResults > 0);
	TestFalse(TEXT("Room name format is not empty"), Settings->DefaultRoomNameFormat.IsEmpty());

	return true;
}

#endif
