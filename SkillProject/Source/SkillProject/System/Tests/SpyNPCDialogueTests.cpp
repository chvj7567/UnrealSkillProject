// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Data/SpyNPCDialogueRow.h"
#include "Engine/DataTable.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNPCDialogueOfferTest,
	"SkillProject.System.NPCDialogue.Offer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNPCDialogueOfferTest::RunTest(const FString& Parameters)
{
	//# 현재 미션이 이 NPC의 Offer 미션, 미수락
	const ESpyNPCDialogueState State = ResolveNPCDialogueState(0, false, 0, 1);

	TestTrue(TEXT("Offer when current mission is this NPC's offer mission and not accepted"), State == ESpyNPCDialogueState::Offer);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNPCDialogueInProgressTest,
	"SkillProject.System.NPCDialogue.InProgress",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNPCDialogueInProgressTest::RunTest(const FString& Parameters)
{
	//# 현재 미션이 이 NPC의 Offer 미션, 수락됨
	const ESpyNPCDialogueState State = ResolveNPCDialogueState(0, true, 0, 1);

	TestTrue(TEXT("InProgress when accepted"), State == ESpyNPCDialogueState::InProgress);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNPCDialogueReportTest,
	"SkillProject.System.NPCDialogue.Report",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNPCDialogueReportTest::RunTest(const FString& Parameters)
{
	//# 현재 미션이 이 NPC의 Report 미션
	const ESpyNPCDialogueState State = ResolveNPCDialogueState(1, true, 0, 1);

	TestTrue(TEXT("Report when current mission is this NPC's report mission"), State == ESpyNPCDialogueState::Report);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNPCDialogueDefaultTest,
	"SkillProject.System.NPCDialogue.Default",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNPCDialogueDefaultTest::RunTest(const FString& Parameters)
{
	//# 현재 미션이 이 NPC의 Offer/Report 둘 다 아님 — 아직 차례 아님과 이미 끝남 양쪽 모두 여기로 온다
	TestTrue(TEXT("Default before this NPC's turn"), ResolveNPCDialogueState(2, false, 4, 5) == ESpyNPCDialogueState::Default);
	TestTrue(TEXT("Default after this NPC's turn"), ResolveNPCDialogueState(6, false, 0, 1) == ESpyNPCDialogueState::Default);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyDialogueTextResolveSingleTest,
	"SkillProject.System.NPCDialogue.DialogueText.Single",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyDialogueTextResolveSingleTest::RunTest(const FString& Parameters)
{
	UDataTable* Table = NewObject<UDataTable>();
	Table->RowStruct = FSpyDialogueRow::StaticStruct();

	FSpyDialogueRow Row;
	Row.DialogueId = 3;
	Row.DialogueIndex = 0;
	Row.Text = FText::FromString(TEXT("한 줄"));
	Table->AddRow(TEXT("Row0"), Row);

	FText Result;
	const bool bFound = TryGetDialogueLineAtIndex(Table, 3, 0, Result);

	TestTrue(TEXT("Page 0 of a single-line group is found"), bFound);
	TestTrue(TEXT("Single-line group returns that line"), Result.EqualTo(FText::FromString(TEXT("한 줄"))));

	FText Overflow;
	TestFalse(TEXT("Page 1 of a single-line group does not exist — dialogue ends"), TryGetDialogueLineAtIndex(Table, 3, 1, Overflow));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyDialogueTextResolveMultiTest,
	"SkillProject.System.NPCDialogue.DialogueText.Multi",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyDialogueTextResolveMultiTest::RunTest(const FString& Parameters)
{
	UDataTable* Table = NewObject<UDataTable>();
	Table->RowStruct = FSpyDialogueRow::StaticStruct();

	FSpyDialogueRow Row1;
	Row1.DialogueId = 7;
	Row1.DialogueIndex = 1;
	Row1.Text = FText::FromString(TEXT("두번째"));
	Table->AddRow(TEXT("Row1"), Row1);

	FSpyDialogueRow Row0;
	Row0.DialogueId = 7;
	Row0.DialogueIndex = 0;
	Row0.Text = FText::FromString(TEXT("첫번째"));
	Table->AddRow(TEXT("Row0"), Row0);

	//# DialogueIndex 오름차순으로 페이지가 매겨져야 한다 — 로우 추가 순서와 무관하게
	FText Page0;
	TestTrue(TEXT("Page 0 resolves"), TryGetDialogueLineAtIndex(Table, 7, 0, Page0));
	TestTrue(TEXT("Page 0 is the lowest DialogueIndex"), Page0.EqualTo(FText::FromString(TEXT("첫번째"))));

	FText Page1;
	TestTrue(TEXT("Page 1 resolves"), TryGetDialogueLineAtIndex(Table, 7, 1, Page1));
	TestTrue(TEXT("Page 1 is the next DialogueIndex"), Page1.EqualTo(FText::FromString(TEXT("두번째"))));

	FText Page2;
	TestFalse(TEXT("Page 2 does not exist — dialogue ends after the last page"), TryGetDialogueLineAtIndex(Table, 7, 2, Page2));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyDialogueTextResolveMissingTest,
	"SkillProject.System.NPCDialogue.DialogueText.Missing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyDialogueTextResolveMissingTest::RunTest(const FString& Parameters)
{
	UDataTable* Table = NewObject<UDataTable>();
	Table->RowStruct = FSpyDialogueRow::StaticStruct();

	FText Result;
	const bool bFound = TryGetDialogueLineAtIndex(Table, 99, 0, Result);

	TestFalse(TEXT("Missing DialogueId is not found"), bFound);
	TestTrue(TEXT("Missing DialogueId returns empty text"), Result.IsEmpty());

	return true;
}

#endif //# WITH_DEV_AUTOMATION_TESTS
