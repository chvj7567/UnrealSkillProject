// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Data/SpyMissionConfig.h"
#include "Engine/DataTable.h"
#include "System/SpyMissionComponent.h"
#include "System/Tests/SpyMissionComponentTestListener.h"
#include "UObject/UnrealType.h"
#include "Util/SpyGameplayTags.h"

//# MissionConfig 는 protected UPROPERTY 라 프로덕션 코드에 테스트 전용 setter 를 추가하지
//# 않기 위해 리플렉션으로 직접 주입한다 (production API 오염 방지, cpp-style §15 의 "코드
//# 전용" 취지와 동일하게 테스트 접근도 프로덕션 표면을 늘리지 않는다)
static void SpyMissionComponentTests_SetMissionConfig(USpyMissionComponent* Component, USpyMissionConfig* Config)
{
	FObjectProperty* Prop = FindFProperty<FObjectProperty>(USpyMissionComponent::StaticClass(), TEXT("MissionConfig"));
	check(Prop != nullptr);
	Prop->SetObjectPropertyValue_InContainer(Component, Config);
}

//# design §0(2026-08-05) 개정으로 Mission_TargetLocation(DataTable) 셋업은 제거했다 —
//# 좌표는 이제 USpyMissionTargetRegistrySubsystem(레벨 배치 액터 자동 추적)에서 온다(§7-6).
//# 이 헬퍼는 순수 MissionTable 픽스처(Vault 미션 1개)만 담당한다.
static USpyMissionConfig* SpyMissionComponentTests_MakeConfig()
{
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();

	UDataTable* MissionTable = NewObject<UDataTable>();
	MissionTable->RowStruct = FSpyMissionRow::StaticStruct();

	FSpyMissionRow Row;
	Row.MissionId = 1;
	Row.MissionType = ESpyMissionType::Gameplay;
	Row.MatchTag = SpyGameplayTags::Skill_Move_Vault;
	Row.Mode = ESpyMissionMode::Accumulate;
	Row.TargetCount = 3;
	MissionTable->AddRow(TEXT("Mission_1"), Row);
	Config->MissionTable = MissionTable;

	return Config;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionComponentAcceptIdempotentTest,
	"SkillProject.System.MissionComponent.AcceptIdempotent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionComponentAcceptIdempotentTest::RunTest(const FString& Parameters)
{
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* Component = NewObject<USpyMissionComponent>(Owner);
	SpyMissionComponentTests_SetMissionConfig(Component, SpyMissionComponentTests_MakeConfig());

	TestFalse(TEXT("Not accepted initially"), Component->IsCurrentAccepted());

	const bool bFirstAccept = Component->AcceptCurrentMission();
	TestTrue(TEXT("First accept succeeds"), bFirstAccept);
	TestTrue(TEXT("Now accepted"), Component->IsCurrentAccepted());

	const bool bSecondAccept = Component->AcceptCurrentMission();
	TestTrue(TEXT("Second accept is idempotent (still true)"), bSecondAccept);

	return true;
}

//# OnRep_MissionState 는 protected UFUNCTION 이라 리플렉션으로 직접 호출한다 — 실제 네트워크
//# 레플리케이션 없이 "원격 클라이언트가 새 MissionState 를 받았다"를 시뮬레이션하는 유일한 방법.
//# UE 표준 RepNotify 파라미터 마샬링과 동일한 파라미터 구조체 레이아웃을 쓴다.
static void SpyMissionComponentTests_SimulateReplication(USpyMissionComponent* Component, const FSpyMissionState& OldState)
{
	UFunction* Func = USpyMissionComponent::StaticClass()->FindFunctionByName(TEXT("OnRep_MissionState"));
	check(Func != nullptr);

	struct FOnRepMissionStateParms
	{
		FSpyMissionState OldMissionState;
	};

	FOnRepMissionStateParms Parms;
	Parms.OldMissionState = OldState;

	Component->ProcessEvent(Func, &Parms);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionComponentRemoteClientAcceptedTest,
	"SkillProject.System.MissionComponent.RemoteClientAccepted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionComponentRemoteClientAcceptedTest::RunTest(const FString& Parameters)
{
	//# 원격 클라이언트 시뮬레이션 — AcceptCurrentMission() 을 절대 호출하지 않는다(서버 전용
	//# 경로). OnRep_MissionState 만으로 OnMissionAccepted 가 발화해야 한다(design §2-3)
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* Component = NewObject<USpyMissionComponent>(Owner);
	SpyMissionComponentTests_SetMissionConfig(Component, SpyMissionComponentTests_MakeConfig());

	USpyMissionComponentTestListener* Listener = NewObject<USpyMissionComponentTestListener>();
	Component->OnMissionAccepted.AddDynamic(Listener, &USpyMissionComponentTestListener::HandleMissionAccepted);

	const FSpyMissionState OldState; //# 기본값 — MissionIndex=1, bAccepted=false (서버가 레플리케이트하기 전 초기 상태)

	//# "서버가 방금 미션 1을 수락 상태로 레플리케이트했다"를 시뮬레이션 — 리플렉션으로
	//# MissionState 를 먼저 갱신한 뒤(실제 네트워크는 이 갱신을 RepNotify 호출 전에 끝낸다)
	FStructProperty* MissionStateProp = FindFProperty<FStructProperty>(USpyMissionComponent::StaticClass(), TEXT("MissionState"));
	check(MissionStateProp != nullptr);

	FSpyMissionState NewState;
	NewState.MissionIndex = 1;
	NewState.Count = 0;
	NewState.bAccepted = true;
	MissionStateProp->SetValue_InContainer(Component, &NewState);

	SpyMissionComponentTests_SimulateReplication(Component, OldState);

	TestEqual(TEXT("OnMissionAccepted fired exactly once via OnRep"), Listener->AcceptedCallCount, 1);
	TestEqual(TEXT("Accepted index matches"), Listener->LastAcceptedIndex, 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionComponentRemoteClientCompletedTest,
	"SkillProject.System.MissionComponent.RemoteClientCompleted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionComponentRemoteClientCompletedTest::RunTest(const FString& Parameters)
{
	//# 미션 1(수락됨) -> 미션 2 로 인덱스가 넘어간 스냅샷을 시뮬레이션 — 완료된 인덱스(1)로
	//# OnMissionCompleted 가 발화해야 한다(design §2-3 "완료된 인덱스는 이전 값이다")
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* Component = NewObject<USpyMissionComponent>(Owner);
	SpyMissionComponentTests_SetMissionConfig(Component, SpyMissionComponentTests_MakeConfig());

	USpyMissionComponentTestListener* Listener = NewObject<USpyMissionComponentTestListener>();
	Component->OnMissionCompleted.AddDynamic(Listener, &USpyMissionComponentTestListener::HandleMissionCompleted);

	FSpyMissionState OldState;
	OldState.MissionIndex = 1;
	OldState.Count = 3;
	OldState.bAccepted = true;

	FStructProperty* MissionStateProp = FindFProperty<FStructProperty>(USpyMissionComponent::StaticClass(), TEXT("MissionState"));
	check(MissionStateProp != nullptr);

	FSpyMissionState NewState;
	NewState.MissionIndex = 2;
	NewState.Count = 0;
	NewState.bAccepted = false;
	MissionStateProp->SetValue_InContainer(Component, &NewState);

	SpyMissionComponentTests_SimulateReplication(Component, OldState);

	TestEqual(TEXT("OnMissionCompleted fired exactly once via OnRep"), Listener->CompletedCallCount, 1);
	TestEqual(TEXT("Completed index is the OLD index (1), not the new one"), Listener->LastCompletedIndex, 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionComponentRemoteClientOrderTest,
	"SkillProject.System.MissionComponent.RemoteClientCompletedBeforeAccepted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionComponentRemoteClientOrderTest::RunTest(const FString& Parameters)
{
	//# design §2-3(3차 개정) 회귀 고정 — 완료(1)와 자동수락(2, Dialogue 타입)이 같은 OnRep
	//# 호출 한 번에 동시에 일어날 때, Completed 가 Accepted 보다 반드시 먼저 발화해야 한다.
	//# 뒤집히면 USpyNavigationComponent::HandleMissionCompleted(무조건 StopPath) 가 같은
	//# 프레임에 막 시작된 새 경로를 지워버린다(design-reviewer 1차검토 BLOCKER)
	USpyMissionConfig* Config = SpyMissionComponentTests_MakeConfig();

	UDataTable* SecondMissionTable = Config->MissionTable;
	FSpyMissionRow DialogueRow;
	DialogueRow.MissionId = 2;
	DialogueRow.MissionType = ESpyMissionType::Dialogue;
	DialogueRow.MatchTag = SpyGameplayTags::Event_Mission_Report;
	SecondMissionTable->AddRow(TEXT("Mission_2"), DialogueRow);

	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* Component = NewObject<USpyMissionComponent>(Owner);
	SpyMissionComponentTests_SetMissionConfig(Component, Config);

	USpyMissionComponentTestListener* Listener = NewObject<USpyMissionComponentTestListener>();
	Component->OnMissionAccepted.AddDynamic(Listener, &USpyMissionComponentTestListener::HandleMissionAccepted);
	Component->OnMissionCompleted.AddDynamic(Listener, &USpyMissionComponentTestListener::HandleMissionCompleted);

	//# 미션 1(수락됨, 완료 직전) -> 미션 2(Dialogue, 진입과 동시에 자동수락) 로 한 번에 전이
	FSpyMissionState OldState;
	OldState.MissionIndex = 1;
	OldState.Count = 3;
	OldState.bAccepted = true;

	FStructProperty* MissionStateProp = FindFProperty<FStructProperty>(USpyMissionComponent::StaticClass(), TEXT("MissionState"));
	check(MissionStateProp != nullptr);

	FSpyMissionState NewState;
	NewState.MissionIndex = 2;
	NewState.Count = 0;
	NewState.bAccepted = true;
	MissionStateProp->SetValue_InContainer(Component, &NewState);

	SpyMissionComponentTests_SimulateReplication(Component, OldState);

	TestEqual(TEXT("Both fired exactly once"), Listener->CompletedCallCount, 1);
	TestEqual(TEXT("Both fired exactly once (accepted)"), Listener->AcceptedCallCount, 1);
	TestTrue(TEXT("Completed fired strictly before Accepted"), Listener->CompletedOrder < Listener->AcceptedOrder);

	return true;
}

//# ─────────────────────────────────────────────────────────────────────────────
//# 이하 test-engineer 확장 — PendingEvents 드레인 루프처럼 한 틱에 미션이 2단계 이상
//# 전진하는(중간 미션을 건너뛰는) 레이스 시나리오. design §2-3 이 인정한 "매 트랜지션과
//# 1:1 대응하지 않는다, 손실 있지만 내비게이션엔 무해"는 주장을 실제로 검증한다.
//# ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionComponentRemoteClientMultiStepSkipTest,
	"SkillProject.System.MissionComponent.RemoteClientMultiStepSkip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionComponentRemoteClientMultiStepSkipTest::RunTest(const FString& Parameters)
{
	//# 서버가 한 레플리케이션 윈도우 안에서 1(완료)->2(자동수락+즉시완료, 예: 보상 XP 재진입)
	//# ->3(자동수락) 을 전부 처리 — 클라이언트는 중간 상태(2)를 전혀 못 보고 최종
	//# 스냅샷(1->3)만 받는다(design §2-3 "PendingEvents 드레인 루프" 시나리오)
	USpyMissionConfig* Config = SpyMissionComponentTests_MakeConfig();

	UDataTable* MissionTable = Config->MissionTable;
	FSpyMissionRow SkippedRow;
	SkippedRow.MissionId = 2;
	SkippedRow.MissionType = ESpyMissionType::Dialogue;
	SkippedRow.MatchTag = SpyGameplayTags::Event_Mission_Report;
	MissionTable->AddRow(TEXT("Mission_2"), SkippedRow);

	FSpyMissionRow FinalRow;
	FinalRow.MissionId = 3;
	FinalRow.MissionType = ESpyMissionType::Dialogue;
	FinalRow.MatchTag = SpyGameplayTags::Event_Mission_Report;
	MissionTable->AddRow(TEXT("Mission_3"), FinalRow);

	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* Component = NewObject<USpyMissionComponent>(Owner);
	SpyMissionComponentTests_SetMissionConfig(Component, Config);

	USpyMissionComponentTestListener* Listener = NewObject<USpyMissionComponentTestListener>();
	Component->OnMissionAccepted.AddDynamic(Listener, &USpyMissionComponentTestListener::HandleMissionAccepted);
	Component->OnMissionCompleted.AddDynamic(Listener, &USpyMissionComponentTestListener::HandleMissionCompleted);

	FSpyMissionState OldState;
	OldState.MissionIndex = 1;
	OldState.Count = 3;
	OldState.bAccepted = true;

	FStructProperty* MissionStateProp = FindFProperty<FStructProperty>(USpyMissionComponent::StaticClass(), TEXT("MissionState"));
	check(MissionStateProp != nullptr);

	FSpyMissionState NewState;
	NewState.MissionIndex = 3;
	NewState.Count = 0;
	NewState.bAccepted = true;
	MissionStateProp->SetValue_InContainer(Component, &NewState);

	SpyMissionComponentTests_SimulateReplication(Component, OldState);

	//# design §2-3 이 인정한 손실 — 미션 2의 개별 완료/수락 이벤트는 관측되지 않는다.
	//# 완료는 "이전 인덱스(1)"로 정확히 1회, 수락은 "최종 인덱스(3)"로 정확히 1회만 발화한다
	TestEqual(TEXT("Completed fired exactly once despite skipping mission 2"), Listener->CompletedCallCount, 1);
	TestEqual(TEXT("Completed carries the OLD index (1), not the skipped mission 2"), Listener->LastCompletedIndex, 1);
	TestEqual(TEXT("Accepted fired exactly once"), Listener->AcceptedCallCount, 1);
	TestEqual(TEXT("Accepted carries the FINAL index (3) — the loss is harmless for navigation"), Listener->LastAcceptedIndex, 3);
	TestTrue(TEXT("Completed still fires before Accepted even with a multi-step skip"), Listener->CompletedOrder < Listener->AcceptedOrder);

	//# 내비게이션 목적에 무해하다는 design 주장의 근거 — 최종 인덱스(3)의 엔트리가 정확히 조회된다.
	//# 좌표 자체(design §0 개정 후 USpyMissionTargetRegistrySubsystem 소관)는 여기서 검증하지 않는다.
	const FSpyMissionRow* FinalEntry = Component->GetMissionEntry(3);
	if (FinalEntry == nullptr)
	{
		AddError(TEXT("Expected mission entry 3 to resolve after the skip"));

		return false;
	}

	TestTrue(TEXT("Final mission entry is the Dialogue mission we added"), FinalEntry->MissionType == ESpyMissionType::Dialogue);

	return true;
}

//# design npc-mission-dialogue.md §6-2 개정(2026-08-05) — ASC-init 시점 자동수락 특례는
//# mission-ground-navigation.md §5-2-1(미수락 상태에서도 네비게이션이 NPC를 직접 가리킴)로
//# 대체되어 제거됐다. 위 특례를 검증하던 회귀 테스트(ASCInit.* 4건)도 함께 제거한다 —
//# test-engineer 가 §5-2-1 기준으로 새 회귀를 작성한다.

#endif //# WITH_DEV_AUTOMATION_TESTS
