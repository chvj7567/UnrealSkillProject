// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "SKOnlineTypes.h"

//# 인원 계산 — 현재 인원 = 최대 − 남은 자리
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSKSessionInfoPlayerCountTest,
	"SKOnline.Session.SessionInfo.PlayerCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSKSessionInfoPlayerCountTest::RunTest(const FString& Parameters)
{
	const FSKSessionInfo Full = FSKSessionInfo::Make(TEXT("Room"), TEXT("Host"), 4, 0, 30, 0);
	TestEqual(TEXT("Full room reports max players"), Full.CurrentPlayers, 4);
	TestEqual(TEXT("Max players preserved"), Full.MaxPlayers, 4);

	const FSKSessionInfo Empty = FSKSessionInfo::Make(TEXT("Room"), TEXT("Host"), 4, 4, 30, 0);
	TestEqual(TEXT("Empty room reports zero"), Empty.CurrentPlayers, 0);

	const FSKSessionInfo Partial = FSKSessionInfo::Make(TEXT("Room"), TEXT("Host"), 4, 1, 30, 0);
	TestEqual(TEXT("Partial room counts occupied slots"), Partial.CurrentPlayers, 3);

	return true;
}

//# 방어 — 음수/역전 입력에도 표시 값이 깨지지 않는다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSKSessionInfoClampTest,
	"SKOnline.Session.SessionInfo.Clamp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSKSessionInfoClampTest::RunTest(const FString& Parameters)
{
	//# 남은 자리가 최대보다 크면(비정상 응답) 현재 인원이 음수가 되면 안 된다
	const FSKSessionInfo Weird = FSKSessionInfo::Make(TEXT("Room"), TEXT("Host"), 4, 9, 30, 0);
	TestEqual(TEXT("Current players never negative"), Weird.CurrentPlayers, 0);

	const FSKSessionInfo Negative = FSKSessionInfo::Make(TEXT("Room"), TEXT("Host"), -3, 0, -5, 0);
	TestEqual(TEXT("Max players never negative"), Negative.MaxPlayers, 0);
	TestEqual(TEXT("Ping never negative"), Negative.PingMs, 0);

	return true;
}

//# 방 이름이 비면 호스트명으로 대체한다 (목록에 빈 줄이 뜨지 않게)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSKSessionInfoRoomNameFallbackTest,
	"SKOnline.Session.SessionInfo.RoomNameFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSKSessionInfoRoomNameFallbackTest::RunTest(const FString& Parameters)
{
	const FSKSessionInfo NoName = FSKSessionInfo::Make(TEXT(""), TEXT("Tae"), 4, 3, 12, 2);
	TestEqual(TEXT("Empty room name falls back to host name"), NoName.RoomName, FString(TEXT("Tae")));
	TestEqual(TEXT("Host name preserved"), NoName.HostName, FString(TEXT("Tae")));
	TestEqual(TEXT("Search index preserved"), NoName.SearchResultIndex, 2);

	const FSKSessionInfo Named = FSKSessionInfo::Make(TEXT("Alpha"), TEXT("Tae"), 4, 3, 12, 2);
	TestEqual(TEXT("Explicit room name kept"), Named.RoomName, FString(TEXT("Alpha")));

	return true;
}

#endif
