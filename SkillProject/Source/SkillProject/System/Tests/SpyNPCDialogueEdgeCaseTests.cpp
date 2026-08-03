// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Data/SpyNPCDialogueRow.h"
#include "Data/SpyMissionConfig.h"
#include "Engine/DataTable.h"
#include "Util/SpyGameplayTags.h"

//# ─────────────────────────────────────────────────────────────────────────────
//# test-engineer 확장 — SpyNPCDialogueTests.cpp(7개, 4상태 판정 + DialogueText 기본
//# 케이스)와 SpyMissionTests.cpp(GetMissionReward 기본 케이스 포함)의 순수 함수 커버리지는
//# 건드리지 않는다. 이 파일은 그 위의 엣지 케이스 · 회귀 · §5-2-1 레이스 컨디션 결정
//# 성질을 다룬다.
//# ─────────────────────────────────────────────────────────────────────────────

//# ═══════════════════════════════════════════════════════════════════════════
//# ResolveNPCDialogueState — 추가 엣지 (비정상 데이터 조합)
//# ═══════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNPCDialogueOfferEqualsReportTest,
	"SkillProject.System.NPCDialogue.State.OfferEqualsReportMissionId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNPCDialogueOfferEqualsReportTest::RunTest(const FString& Parameters)
{
	//# 에디터 데이터 오류 — 한 NPC의 MissionCommunication Offer/Report 행이 같은 MissionId를
	//# 가리키는 경우. Offer 분기가 먼저 검사되므로(spec §6 판정 순서) Report 분기는 이 NPC에게
	//# 영원히 도달 불가능해진다 — "보고를 절대 완료할 수 없는" 소프트락 형태를 회귀로 고정한다.
	//# (프로덕션 수정 대상 아님 — 데이터 검증 강화가 필요하면 별도로 보고한다)
	const ESpyNPCDialogueState Unaccepted = ResolveNPCDialogueState(4, false, 4, 4);
	TestTrue(TEXT("Offer wins over Report when both ids are equal"), Unaccepted == ESpyNPCDialogueState::Offer);

	const ESpyNPCDialogueState Accepted = ResolveNPCDialogueState(4, true, 4, 4);
	TestTrue(TEXT("InProgress — Report branch is unreachable for this NPC"), Accepted == ESpyNPCDialogueState::InProgress);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNPCDialogueUncachedDefaultTest,
	"SkillProject.System.NPCDialogue.State.UncachedNPCAlwaysDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNPCDialogueUncachedDefaultTest::RunTest(const FString& Parameters)
{
	//# ASpyNPCCharacter::CacheNPCData()가 Offer/Report 행을 정확히 1개씩 찾지 못하면
	//# CachedOfferMissionId/CachedReportMissionId는 INDEX_NONE(-1)에 머문다(spec §9).
	//# 실제 진행 중인 미션 인덱스(항상 0 이상, ResolveMissionProgress의 클램프)는 -1과 결코
	//# 같아질 수 없으므로, 어떤 CurrentMissionId가 오든 이 NPC는 항상 Default로만 응답해야 한다
	const int32 UncachedOffer = INDEX_NONE;
	const int32 UncachedReport = INDEX_NONE;

	TestTrue(TEXT("Default at mission 0"), ResolveNPCDialogueState(0, false, UncachedOffer, UncachedReport) == ESpyNPCDialogueState::Default);
	TestTrue(TEXT("Default at mission 1, accepted"), ResolveNPCDialogueState(1, true, UncachedOffer, UncachedReport) == ESpyNPCDialogueState::Default);
	TestTrue(TEXT("Default deep into the chain"), ResolveNPCDialogueState(11, false, UncachedOffer, UncachedReport) == ESpyNPCDialogueState::Default);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNPCDialogueNegativeIndexLatentRiskTest,
	"SkillProject.System.NPCDialogue.State.NegativeCurrentIdCollidesWithUncachedSentinel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNPCDialogueNegativeIndexLatentRiskTest::RunTest(const FString& Parameters)
{
	//# ResolveNPCDialogueState 자체는 CurrentMissionId가 0 이상이라는 것을 보장하지 않는다 —
	//# 그 보장은 호출부(USpyMissionConfig::ResolveMissionProgress의 FMath::Max(0, InIndex))가 진다.
	//# 만약 그 클램프가 훗날 제거되면, "미캐싱 NPC"의 sentinel(INDEX_NONE=-1)과 우연히 충돌해
	//# 엉뚱하게 Offer 카드가 뜰 수 있다는 것을 이 테스트가 고정한다. 지금은 재현되지 않는 잠재
	//# 위험이지 현재 버그가 아니다(호출부 클램프가 살아있는 한 CurrentMissionId는 -1이 될 수 없음)
	const ESpyNPCDialogueState State = ResolveNPCDialogueState(INDEX_NONE, false, INDEX_NONE, INDEX_NONE);

	TestTrue(TEXT("Documents the sentinel collision if the caller's clamp is ever removed"), State == ESpyNPCDialogueState::Offer);

	return true;
}

//# ═══════════════════════════════════════════════════════════════════════════
//# TryGetDialogueLineAtIndex — 추가 엣지
//# ═══════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyDialogueTextNullTableTest,
	"SkillProject.System.NPCDialogue.DialogueText.NullTable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyDialogueTextNullTableTest::RunTest(const FString& Parameters)
{
	FText Result;
	const bool bFound = TryGetDialogueLineAtIndex(nullptr, 0, 0, Result);

	TestFalse(TEXT("Null table is not found, no crash"), bFound);
	TestTrue(TEXT("Null table returns empty text, no crash"), Result.IsEmpty());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyDialogueTextNegativeIdTest,
	"SkillProject.System.NPCDialogue.DialogueText.NegativeId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyDialogueTextNegativeIdTest::RunTest(const FString& Parameters)
{
	UDataTable* Table = NewObject<UDataTable>();
	Table->RowStruct = FSpyDialogueRow::StaticStruct();

	FSpyDialogueRow Row;
	Row.DialogueId = -5;
	Row.DialogueIndex = 0;
	Row.Text = FText::FromString(TEXT("음수 ID 대사"));
	Table->AddRow(TEXT("Row0"), Row);

	FText Found;
	TestTrue(TEXT("Negative DialogueId resolves like any other id"), TryGetDialogueLineAtIndex(Table, -5, 0, Found));
	TestTrue(TEXT("Negative DialogueId text matches"), Found.EqualTo(FText::FromString(TEXT("음수 ID 대사"))));

	FText NotFound;
	TestFalse(TEXT("Positive counterpart is not confused with the negative id"), TryGetDialogueLineAtIndex(Table, 5, 0, NotFound));
	TestTrue(TEXT("Positive counterpart returns empty text"), NotFound.IsEmpty());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyDialogueTextTiedIndexTest,
	"SkillProject.System.NPCDialogue.DialogueText.TiedIndex",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyDialogueTextTiedIndexTest::RunTest(const FString& Parameters)
{
	UDataTable* Table = NewObject<UDataTable>();
	Table->RowStruct = FSpyDialogueRow::StaticStruct();

	//# 에디터 데이터 실수 — 같은 DialogueIndex가 중복 입력된 경우. TArray::Sort는 불안정
	//# 정렬이라 동률(tie) 사이의 최종 순서를 보장하지 않는다 — 순서를 단정하지 않고 페이지 0에
	//# 둘 중 하나가, 페이지 1에 나머지 하나가 오는지만 확인한다(둘 다 유실되지 않음)
	FSpyDialogueRow RowA;
	RowA.DialogueId = 11;
	RowA.DialogueIndex = 0;
	RowA.Text = FText::FromString(TEXT("조각A"));
	Table->AddRow(TEXT("RowA"), RowA);

	FSpyDialogueRow RowB;
	RowB.DialogueId = 11;
	RowB.DialogueIndex = 0;
	RowB.Text = FText::FromString(TEXT("조각B"));
	Table->AddRow(TEXT("RowB"), RowB);

	FText Page0;
	TestTrue(TEXT("Page 0 resolves despite tied DialogueIndex"), TryGetDialogueLineAtIndex(Table, 11, 0, Page0));

	FText Page1;
	TestTrue(TEXT("Page 1 resolves despite tied DialogueIndex"), TryGetDialogueLineAtIndex(Table, 11, 1, Page1));

	const bool bBothFragmentsPresent =
		(Page0.ToString() == TEXT("조각A") || Page0.ToString() == TEXT("조각B")) &&
		(Page1.ToString() == TEXT("조각A") || Page1.ToString() == TEXT("조각B")) &&
		(Page0.ToString() != Page1.ToString());
	TestTrue(TEXT("Both fragments are present across the two pages, neither lost"), bBothFragmentsPresent);

	FText Page2;
	TestFalse(TEXT("Page 2 does not exist — only 2 rows tied at index 0"), TryGetDialogueLineAtIndex(Table, 11, 2, Page2));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyDialogueTextRowStructMismatchTest,
	"SkillProject.System.NPCDialogue.DialogueText.RowStructMismatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyDialogueTextRowStructMismatchTest::RunTest(const FString& Parameters)
{
	//# 테이블은 존재하지만 RowStruct가 FSpyDialogueRow가 아닌 경우(에디터에서 잘못된
	//# DataTable을 연결한 실수). GetAllRows<FSpyDialogueRow>()는 타입 불일치를 로그로 남기고
	//# 빈 배열을 반환한다 — 크래시 없이 빈 텍스트로 안전하게 귀결되는지 확인한다.
	//# ⚠ 최초 실행 시 이 줄이 실패하면 두 경우를 구분한다: (a) "RowStruct" 등을 언급하는
	//# 미매칭 Error 로그가 함께 실패로 잡히면 → 아래 문자열을 실제 로그로 교체.
	//# (b) "expected error not matched"만 실패하면(다른 Error 로그 없음) → 이 줄을 통째로
	//# 지운다(엔진이 Error 레벨로 남기지 않는다는 뜻). 어느 쪽이든 아래 IsEmpty 단언은 유지한다
	AddExpectedErrorPlain(TEXT("TryGetDialogueLineAtIndex"), EAutomationExpectedErrorFlags::Contains, 1);

	UDataTable* MismatchedTable = NewObject<UDataTable>();
	MismatchedTable->RowStruct = FSpyMissionRewardRow::StaticStruct();

	FText Result;
	const bool bFound = TryGetDialogueLineAtIndex(MismatchedTable, 0, 0, Result);

	TestFalse(TEXT("Mismatched RowStruct is not found, no crash"), bFound);
	TestTrue(TEXT("Mismatched RowStruct returns empty text, no crash"), Result.IsEmpty());

	return true;
}

//# ═══════════════════════════════════════════════════════════════════════════
//# USpyMissionConfig::GetMissionReward — 추가 엣지
//# ═══════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionRewardDuplicateIdTest,
	"SkillProject.System.Mission.Reward.DuplicateMissionId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionRewardDuplicateIdTest::RunTest(const FString& Parameters)
{
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();
	UDataTable* Table = NewObject<UDataTable>();
	Table->RowStruct = FSpyMissionRewardRow::StaticStruct();
	Config->MissionRewardTable = Table;

	//# 같은 MissionId가 중복 등록된 데이터 오류 시나리오. GetAllRows는 TMap(RowMap)을
	//# 순회하므로 등록 순서가 조회 순서를 보장하지 않는다 — 두 값을 동일하게 둬 어느 쪽이
	//# 먼저 잡혀도 결과가 결정적이도록 픽스처를 구성한다(순서 의존 단정 금지)
	FSpyMissionRewardRow RowA;
	RowA.MissionId = 7;
	RowA.ExperienceReward = 15.f;
	Table->AddRow(TEXT("RowA"), RowA);

	FSpyMissionRewardRow RowB;
	RowB.MissionId = 7;
	RowB.ExperienceReward = 15.f;
	Table->AddRow(TEXT("RowB"), RowB);

	TestEqual(TEXT("Duplicate rows with the same value resolve deterministically"), Config->GetMissionReward(7), 15.f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionRewardNegativeIdTest,
	"SkillProject.System.Mission.Reward.NegativeMissionId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionRewardNegativeIdTest::RunTest(const FString& Parameters)
{
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();
	UDataTable* Table = NewObject<UDataTable>();
	Table->RowStruct = FSpyMissionRewardRow::StaticStruct();
	Config->MissionRewardTable = Table;

	FSpyMissionRewardRow Row;
	Row.MissionId = -3;
	Row.ExperienceReward = 42.f;
	Table->AddRow(TEXT("Row0"), Row);

	TestEqual(TEXT("Negative MissionId resolves its own reward"), Config->GetMissionReward(-3), 42.f);
	TestEqual(TEXT("Positive counterpart is not confused with the negative row"), Config->GetMissionReward(3), 0.f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionRewardZeroAmbiguityTest,
	"SkillProject.System.Mission.Reward.ZeroValueIndistinguishableFromMissing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionRewardZeroAmbiguityTest::RunTest(const FString& Parameters)
{
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();
	UDataTable* Table = NewObject<UDataTable>();
	Table->RowStruct = FSpyMissionRewardRow::StaticStruct();
	Config->MissionRewardTable = Table;

	//# 행이 존재하지만 값이 0인 경우와, 행이 아예 없는 경우가 GetMissionReward() 만으로는
	//# 구분되지 않는다 — 이 구분은 GrantReward()가 MissionType을 함께 확인해야만 가능하다
	//# (spec §4-3). 이 테스트는 그 모호성 자체를 회귀로 고정한다(GrantReward는 컴포넌트
	//# 레벨이라 Automation 대상 아님 — PIE 확인 필요)
	FSpyMissionRewardRow ZeroRow;
	ZeroRow.MissionId = 9;
	ZeroRow.ExperienceReward = 0.f;
	Table->AddRow(TEXT("Row0"), ZeroRow);

	TestEqual(TEXT("Explicit zero-reward row"), Config->GetMissionReward(9), 0.f);
	TestEqual(TEXT("Missing row for an unrelated MissionId"), Config->GetMissionReward(123), 0.f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionRewardLargeTableTest,
	"SkillProject.System.Mission.Reward.LargeTableScan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionRewardLargeTableTest::RunTest(const FString& Parameters)
{
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();
	UDataTable* Table = NewObject<UDataTable>();
	Table->RowStruct = FSpyMissionRewardRow::StaticStruct();
	Config->MissionRewardTable = Table;

	const int32 RowCount = 50;
	for (int32 Index = 0; Index < RowCount; ++Index)
	{
		FSpyMissionRewardRow Row;
		Row.MissionId = Index;
		Row.ExperienceReward = (float)(Index + 1) * 2.f;
		Table->AddRow(FName(*FString::Printf(TEXT("Row_%d"), Index)), Row);
	}

	TestEqual(TEXT("Finds the first row"), Config->GetMissionReward(0), 2.f);
	TestEqual(TEXT("Finds a row in the middle of a large table"), Config->GetMissionReward(27), 56.f);
	TestEqual(TEXT("Finds the last row"), Config->GetMissionReward(RowCount - 1), 100.f);
	TestEqual(TEXT("An id past the table returns 0"), Config->GetMissionReward(RowCount), 0.f);

	return true;
}

//# ═══════════════════════════════════════════════════════════════════════════
//# spec §5-2-1 레이스 컨디션 회귀 — 미러 모델
//#
//# ⚠ USpyMissionComponent::AddProgress/ProcessProgress는 protected MissionConfig +
//# HasAuthority()(액터 컨텍스트 필요)에 의존해 실제 컴포넌트를 액터 없이 호출할 수
//# 없다(설계 문서 §11 "컴포넌트 레벨은 Automation 커버 불가"와 동일한 제약 — production
//# 코드에 테스트용 setter/friend를 추가하는 것은 test-engineer 범위 밖이라 하지 않는다).
//#
//# 아래는 그 실제 컴포넌트를 대체 검증하지 않는다. 대신 §5-2-1이 고치는 "결정 성질"
//# 자체 — 드레인 루프가 매 반복 bAccepted를 재검증하는가 — 를 진짜 순수 함수인
//# USpyMissionConfig::ResolveMissionProgress로 구동해 pin한다. AddProgress/ProcessProgress의
//# 재진입 큐잉 구조를 그대로 미러링하되, GrantReward의 GE 적용이 유발하는 재진입 레벨업은
//# "Dialogue 미션이 방금 완료되면 레벨 신호를 재호출한다"로 단순화해 흉내낸다.
//# 실제 컴포넌트(AddProgress/GrantReward/SpyLevelComponent 연동)의 동작은 PIE로 확인한다.
//#
//# ⚠ 이 미러는 사양(§5-2-1이 요구하는 "매 반복 재검증" 성질)을 고정하지, 프로덕션
//# 구현을 고정하지 않는다 — System/SpyMissionComponent.cpp::AddProgress 의
//# "if (MissionState.bAccepted) ProcessProgress(...)" 재검증을 실수로 지워도 이 두
//# 테스트는 계속 PASS 한다. 그 함수를 수정하는 사람은 이 주석을 참고해 실제 회귀 여부를
//# PIE로 재확인해야 한다.
//# ═══════════════════════════════════════════════════════════════════════════

struct FSpyMissionRaceMirrorState
{
	int32 MissionIndex = 0;
	int32 Count = 0;
	bool bAccepted = false;
	bool bProcessing = false;

	//# true = spec §5-2-1 수정본(드레인 루프가 매 반복 bAccepted 재검증), false = 수정 전 버그 재현
	bool bRecheckOnDrain = true;

	TArray<TPair<FGameplayTag, int32>> PendingEvents;
};

static void SpyMissionRaceMirror_ProcessProgress(const USpyMissionConfig* Config, FSpyMissionRaceMirrorState& State, FGameplayTag InEventTag, int32 InAmount);

static void SpyMissionRaceMirror_AddProgress(const USpyMissionConfig* Config, FSpyMissionRaceMirrorState& State, FGameplayTag InEventTag, int32 InAmount)
{
	if (State.bProcessing)
	{
		State.PendingEvents.Add(TPair<FGameplayTag, int32>(InEventTag, InAmount));

		return;
	}

	State.bProcessing = true;

	if (State.bAccepted)
		SpyMissionRaceMirror_ProcessProgress(Config, State, InEventTag, InAmount);

	while (State.PendingEvents.Num() > 0)
	{
		const TPair<FGameplayTag, int32> Next = State.PendingEvents[0];
		State.PendingEvents.RemoveAt(0);

		//# 버그 재현 모드(bRecheckOnDrain == false)는 큐잉 시점 판정을 그대로 신뢰한다 —
		//# 수정 전 코드가 "지금" 상태를 다시 보지 않고 무조건 처리하던 것을 그대로 흉내낸다
		if (State.bRecheckOnDrain == false || State.bAccepted)
			SpyMissionRaceMirror_ProcessProgress(Config, State, Next.Key, Next.Value);
	}

	State.bProcessing = false;
}

static void SpyMissionRaceMirror_ProcessProgress(const USpyMissionConfig* Config, FSpyMissionRaceMirrorState& State, FGameplayTag InEventTag, int32 InAmount)
{
	const FSpyMissionProgressResult Result = Config->ResolveMissionProgress(State.MissionIndex, State.Count, InEventTag, InAmount);
	const bool bChanged = (Result.MissionIndex != State.MissionIndex) || (Result.Count != State.Count);

	if (Result.bCompletedNow)
	{
		const FSpyMissionEntry* CompletedEntry = Config->GetMission(State.MissionIndex);
		if (CompletedEntry != nullptr && CompletedEntry->MissionType == ESpyMissionType::Dialogue)
		{
			//# GrantReward의 GE 적용 → SpyLevelComponent::TryLevelUp → AddProgress 재진입
			//# 경로를 흉내낸다. 인덱스가 아직 전진하지 않은 이 시점에 재호출되는 것이
			//# 실제 버그의 핵심 타이밍이다 (spec §5-2-1)
			SpyMissionRaceMirror_AddProgress(Config, State, SpyGameplayTags::Event_Mission_Level, 3);
		}
	}

	if (bChanged == false)
		return;

	State.MissionIndex = Result.MissionIndex;
	State.Count = Result.Count;

	const FSpyMissionEntry* NewEntry = Config->GetMission(State.MissionIndex);
	State.bAccepted = (NewEntry != nullptr && NewEntry->MissionType == ESpyMissionType::Dialogue);
}

//# 픽스처 — [0] Gameplay(Kill) → [1] Dialogue(Report, 자동 수락) → [2] Gameplay(Level, Threshold 3)
static USpyMissionConfig* SpyMissionRaceMirror_MakeConfig()
{
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();

	FSpyMissionEntry Kill;
	Kill.MissionType = ESpyMissionType::Gameplay;
	Kill.MatchTag = SpyGameplayTags::Event_Mission_Kill;
	Kill.Mode = ESpyMissionMode::Accumulate;
	Kill.TargetCount = 1;
	Config->Missions.Add(Kill);

	FSpyMissionEntry Report;
	Report.MissionType = ESpyMissionType::Dialogue;
	Report.MatchTag = SpyGameplayTags::Event_Mission_Report;
	Report.Mode = ESpyMissionMode::Accumulate;
	Report.TargetCount = 1;
	Config->Missions.Add(Report);

	FSpyMissionEntry Level;
	Level.MissionType = ESpyMissionType::Gameplay;
	Level.MatchTag = SpyGameplayTags::Event_Mission_Level;
	Level.Mode = ESpyMissionMode::Threshold;
	Level.TargetCount = 3;
	Config->Missions.Add(Level);

	return Config;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionRaceWithoutRecheckTest,
	"SkillProject.System.NPCDialogue.Race.WithoutRecheckCompletesUnaccepted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionRaceWithoutRecheckTest::RunTest(const FString& Parameters)
{
	USpyMissionConfig* Config = SpyMissionRaceMirror_MakeConfig();

	FSpyMissionRaceMirrorState State;
	State.bRecheckOnDrain = false; //# 수정 전 코드 재현 — 드레인 루프가 bAccepted를 재검증하지 않음
	State.bAccepted = true; //# 처치 미션(0)을 이미 NPC Offer로 수락한 상태에서 시작

	//# 처치 완료 → 보고 미션(1, Dialogue)으로 자동 전진 + 자동 수락
	SpyMissionRaceMirror_AddProgress(Config, State, SpyGameplayTags::Event_Mission_Kill, 1);
	TestEqual(TEXT("Advanced to the report mission"), State.MissionIndex, 1);
	TestTrue(TEXT("Report mission auto-accepted"), State.bAccepted);

	//# 보고 완료 시 보상이 우연히 레벨 3 신호를 재진입시킨다(§5-2-1). 수정 전 코드는 이
	//# 신호를 큐에서 무조건 처리해 아직 미수락인 레벨 미션(2)을 조용히 완료시켜 버린다
	SpyMissionRaceMirror_AddProgress(Config, State, SpyGameplayTags::Event_Mission_Report, 1);

	TestEqual(TEXT("BUG: level mission completed without ever being accepted"), State.MissionIndex, 3);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionRaceWithRecheckTest,
	"SkillProject.System.NPCDialogue.Race.WithRecheckDropsUnaccepted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionRaceWithRecheckTest::RunTest(const FString& Parameters)
{
	USpyMissionConfig* Config = SpyMissionRaceMirror_MakeConfig();

	FSpyMissionRaceMirrorState State;
	State.bRecheckOnDrain = true; //# spec §5-2-1 수정 — 드레인 루프도 매 반복 bAccepted를 재검증
	State.bAccepted = true;

	SpyMissionRaceMirror_AddProgress(Config, State, SpyGameplayTags::Event_Mission_Kill, 1);
	TestEqual(TEXT("Advanced to the report mission"), State.MissionIndex, 1);

	SpyMissionRaceMirror_AddProgress(Config, State, SpyGameplayTags::Event_Mission_Report, 1);

	//# 재진입 레벨 신호는 아직 미수락인 레벨 미션에 적용되지 못하고 조용히 버려진다
	TestEqual(TEXT("FIX: still on the level mission, not accepted"), State.MissionIndex, 2);
	TestFalse(TEXT("FIX: level mission not auto-accepted"), State.bAccepted);
	TestEqual(TEXT("FIX: level progress untouched by the discarded signal"), State.Count, 0);

	return true;
}

#endif //# WITH_DEV_AUTOMATION_TESTS
