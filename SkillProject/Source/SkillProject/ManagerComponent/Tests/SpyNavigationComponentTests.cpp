#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Data/SpyMissionConfig.h"
#include "Engine/DataTable.h"
#include "ManagerComponent/SpyNavigationComponent.h"
#include "System/SpyMissionComponent.h"
#include "UObject/UnrealType.h"
#include "Util/SpyGameplayTags.h"

//# System/Tests/SpyMissionComponentTests.cpp 의 동일 헬퍼를 이 파일 전용으로 다시 둔다 —
//# 기존 테스트 파일들도 픽스처 헬퍼를 파일마다 static 으로 독립 소유한다(SpyMissionTests.cpp 선례)
static void SpyNavigationComponentTests_SetMissionConfig(USpyMissionComponent* Component, USpyMissionConfig* Config)
{
	FObjectProperty* Prop = FindFProperty<FObjectProperty>(USpyMissionComponent::StaticClass(), TEXT("MissionConfig"));
	check(Prop != nullptr);
	Prop->SetObjectPropertyValue_InContainer(Component, Config);
}

static USpyMissionConfig* SpyNavigationComponentTests_MakeConfigWithTargetLocation()
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

	UDataTable* TargetLocationTable = NewObject<UDataTable>();
	TargetLocationTable->RowStruct = FSpyMission_TargetLocationRow::StaticStruct();

	FSpyMission_TargetLocationRow TargetRow;
	TargetRow.MissionId = 1;
	TargetRow.TargetLocation = FVector(500.f, 0.f, 0.f);
	TargetLocationTable->AddRow(TEXT("TargetLocation_1"), TargetRow);
	Config->MissionTargetLocationTable = TargetLocationTable;

	return Config;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentAcceptStartsPathTest,
	"SkillProject.Navigation.Component.AcceptStartsPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentAcceptStartsPathTest::RunTest(const FString& Parameters)
{
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);
	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, SpyNavigationComponentTests_MakeConfigWithTargetLocation());

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);

	TestFalse(TEXT("Path inactive before accept"), NavComponent->IsPathActive());

	MissionComponent->AcceptCurrentMission();

	TestTrue(TEXT("Path active after accept"), NavComponent->IsPathActive());
	TestEqual(TEXT("Target location matches Mission_TargetLocation row"), NavComponent->GetCurrentTargetLocation(), FVector(500.f, 0.f, 0.f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentNoTargetLocationTest,
	"SkillProject.Navigation.Component.NoTargetLocation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentNoTargetLocationTest::RunTest(const FString& Parameters)
{
	//# 목표 좌표가 정의되지 않은 미션은 수락해도 경로가 활성화되지 않는다 (spec 범위 §2 "제외")
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);

	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();
	UDataTable* MissionTable = NewObject<UDataTable>();
	MissionTable->RowStruct = FSpyMissionRow::StaticStruct();

	FSpyMissionRow Row;
	Row.MissionId = 1;
	Row.MissionType = ESpyMissionType::Gameplay;
	Row.MatchTag = SpyGameplayTags::Event_Mission_Kill;
	Row.Mode = ESpyMissionMode::Accumulate;
	Row.TargetCount = 5;
	MissionTable->AddRow(TEXT("Mission_1"), Row);
	Config->MissionTable = MissionTable;

	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, Config);

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);

	MissionComponent->AcceptCurrentMission();

	TestFalse(TEXT("No target location row -> path stays inactive"), NavComponent->IsPathActive());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentCompleteStopsPathTest,
	"SkillProject.Navigation.Component.CompleteStopsPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentCompleteStopsPathTest::RunTest(const FString& Parameters)
{
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);
	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, SpyNavigationComponentTests_MakeConfigWithTargetLocation());

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);

	MissionComponent->AcceptCurrentMission();
	TestTrue(TEXT("Path active after accept"), NavComponent->IsPathActive());

	//# 목표 3회 중 3회 채워 완료시킨다 (Vault 미션, Accumulate)
	MissionComponent->AddProgress(SpyGameplayTags::Skill_Move_Vault, 3);

	TestFalse(TEXT("Path inactive after mission completed"), NavComponent->IsPathActive());

	return true;
}

//# ─────────────────────────────────────────────────────────────────────────────
//# 이하 test-engineer 확장 — 재진입(구독 해제 후 무반응)·상태 잔존(연속 미션 재사용)·
//# 레이스(OnRep 다단계 스킵) 엣지 케이스. 기존 3개 테스트는 RecomputePath 의
//# GetWorld()==nullptr 조기 반환 경로만 타므로(bPathActive 플래그만 검증), 여기서는
//# HandleMissionAccepted/HandleMissionCompleted 상태 머신 그 자체의 조합을 확장한다.
//# ─────────────────────────────────────────────────────────────────────────────

//# System/Tests/SpyMissionComponentTests.cpp 의 동일 헬퍼를 이 파일 전용으로 다시 둔다 —
//# OnRep_MissionState 는 protected UFUNCTION 이라 리플렉션으로 직접 호출한다.
static void SpyNavigationComponentTests_SimulateReplication(USpyMissionComponent* Component, const FSpyMissionState& OldState)
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
	FSpyNavigationComponentUnbindStopsReactingTest,
	"SkillProject.Navigation.Component.UnbindStopsReacting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentUnbindStopsReactingTest::RunTest(const FString& Parameters)
{
	//# 구독 해제 누락 회귀 방지 — UnbindMissionComponent 이후에는 그 컴포넌트의 어떤
	//# 이벤트에도 더 이상 반응하지 않아야 한다(델리게이트 구독 해제 누락 엣지케이스)
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);
	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, SpyNavigationComponentTests_MakeConfigWithTargetLocation());

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);
	NavComponent->UnbindMissionComponent();

	MissionComponent->AcceptCurrentMission();

	TestFalse(TEXT("Path stays inactive — delegate was detached before the accept"), NavComponent->IsPathActive());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentSequentialMissionsNoStaleStateTest,
	"SkillProject.Navigation.Component.SequentialMissionsNoStaleState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentSequentialMissionsNoStaleStateTest::RunTest(const FString& Parameters)
{
	//# 리소스 재사용 후 상태 잔존 엣지케이스 — 같은 NavComponent 로 미션 1을 완료하고
	//# 미션 2를 재수락했을 때, 미션 1의 목표 좌표가 남아있지 않아야 한다
	USpyMissionConfig* Config = SpyNavigationComponentTests_MakeConfigWithTargetLocation();

	FSpyMissionRow SecondMission;
	SecondMission.MissionId = 2;
	SecondMission.MissionType = ESpyMissionType::Gameplay;
	SecondMission.MatchTag = SpyGameplayTags::Skill_Move_Climb;
	SecondMission.Mode = ESpyMissionMode::Accumulate;
	SecondMission.TargetCount = 1;
	Config->MissionTable->AddRow(TEXT("Mission_2"), SecondMission);

	FSpyMission_TargetLocationRow SecondTargetRow;
	SecondTargetRow.MissionId = 2;
	SecondTargetRow.TargetLocation = FVector(0.f, 777.f, 0.f);
	Config->MissionTargetLocationTable->AddRow(TEXT("TargetLocation_2"), SecondTargetRow);

	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);
	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, Config);

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);

	MissionComponent->AcceptCurrentMission();
	TestEqual(TEXT("Mission 1 target active"), NavComponent->GetCurrentTargetLocation(), FVector(500.f, 0.f, 0.f));

	MissionComponent->AddProgress(SpyGameplayTags::Skill_Move_Vault, 3);
	TestFalse(TEXT("Path inactive between missions — no stale target"), NavComponent->IsPathActive());
	TestEqual(TEXT("Target reset to zero, not left at mission 1's location"), NavComponent->GetCurrentTargetLocation(), FVector::ZeroVector);

	//# Gameplay 타입은 자동 수락되지 않는다 — 명시적으로 재수락한다
	MissionComponent->AcceptCurrentMission();

	TestTrue(TEXT("Path active again for mission 2"), NavComponent->IsPathActive());
	TestEqual(TEXT("Retargeted to mission 2, no leak from mission 1"), NavComponent->GetCurrentTargetLocation(), FVector(0.f, 777.f, 0.f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentRemoteClientMultiStepSkipRetargetsTest,
	"SkillProject.Navigation.Component.RemoteClientMultiStepSkipRetargets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentRemoteClientMultiStepSkipRetargetsTest::RunTest(const FString& Parameters)
{
	//# design §2-3 이 인정한 "손실 있지만 내비게이션엔 무해" 주장을 NavigationComponent
	//# 관점에서 직접 검증한다 — AcceptCurrentMission() 을 그 이후로는 호출하지 않고(원격
	//# 클라이언트 시뮬레이션) OnRep 한 번으로 1->3(중간 2 스킵) 전이를 재현했을 때,
	//# 최종적으로 미션 3의 목표로 정확히 리타겟되는지 확인한다.
	USpyMissionConfig* Config = SpyNavigationComponentTests_MakeConfigWithTargetLocation();

	FSpyMissionRow SkippedRow;
	SkippedRow.MissionId = 2;
	SkippedRow.MissionType = ESpyMissionType::Dialogue;
	SkippedRow.MatchTag = SpyGameplayTags::Event_Mission_Report;
	Config->MissionTable->AddRow(TEXT("Mission_2"), SkippedRow);

	FSpyMissionRow FinalRow;
	FinalRow.MissionId = 3;
	FinalRow.MissionType = ESpyMissionType::Dialogue;
	FinalRow.MatchTag = SpyGameplayTags::Event_Mission_Report;
	Config->MissionTable->AddRow(TEXT("Mission_3"), FinalRow);

	FSpyMission_TargetLocationRow FinalTargetRow;
	FinalTargetRow.MissionId = 3;
	FinalTargetRow.TargetLocation = FVector(999.f, 250.f, 0.f);
	Config->MissionTargetLocationTable->AddRow(TEXT("TargetLocation_3"), FinalTargetRow);

	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);
	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, Config);

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);

	//# 미션 1을 먼저 정상 수락해 경로가 활성 상태(구 목표 500,0,0)로 시작하게 만든다 —
	//# "리소스 재사용 후 상태 잔존" 여부를 함께 검증하기 위한 사전 조건
	MissionComponent->AcceptCurrentMission();
	TestEqual(TEXT("Starts on mission 1's target"), NavComponent->GetCurrentTargetLocation(), FVector(500.f, 0.f, 0.f));

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
	MissionStateProp->SetValue_InContainer(MissionComponent, &NewState);

	SpyNavigationComponentTests_SimulateReplication(MissionComponent, OldState);

	TestTrue(TEXT("Path still active after the multi-step transition"), NavComponent->IsPathActive());
	TestEqual(TEXT("Retargeted to mission 3's location, not a stale mission-1 target"), NavComponent->GetCurrentTargetLocation(), FVector(999.f, 250.f, 0.f));

	return true;
}

#endif //# WITH_DEV_AUTOMATION_TESTS
