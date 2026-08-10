#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Data/SpyMissionConfig.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"
#include "Interactable/SpyInteractableObject.h"
#include "ManagerComponent/SpyNavigationComponent.h"
#include "ManagerComponent/Tests/SpyNavigationComponentTestAccessor.h"
#include "NPC/SpyNPCCharacter.h"
#include "Navigation/SpyMissionTargetPoint.h"
#include "System/CommonInterface.System.h"
#include "System/SpyMissionComponent.h"
#include "System/SpyMissionTargetRegistrySubsystem.h"
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

//# design §0(2026-08-05) 개정으로 Mission_TargetLocation(DataTable) 셋업은 제거했다 —
//# 좌표는 이제 USpyMissionTargetRegistrySubsystem(레벨 배치 액터 자동 추적)에서 온다(§7-6).
static USpyMissionConfig* SpyNavigationComponentTests_MakeConfig()
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

//# code-reviewer 가 지적한 구조적 공백(BLOCKER 회귀 위험) — RootComponent 가 없는 액터의
//# GetActorLocation() 은 항상 원점을 반환하므로, 레지스트리가 좌표를 통째로 무시해도 검증
//# 못 하는 테스트가 됐었다. World 없이도 SetActorLocation 이 실제로 반영되는 최소 픽스처
//# (RootComponent 부착 + 비원점 좌표)를 공용 헬퍼로 둔다 — 이하 모든 좌표 검증 테스트가 이걸 쓴다.
static AActor* SpyNavigationComponentTests_MakeLocatedActor(const FVector& InLocation)
{
	AActor* Actor = NewObject<AActor>(GetTransientPackage());
	USceneComponent* Root = NewObject<USceneComponent>(Actor, TEXT("Root"));
	Actor->SetRootComponent(Root);
	Actor->SetActorLocation(InLocation);

	return Actor;
}

//# design 2026-08-10 §5 — 트리거 활성 마커를 만들고 즉시 레지스트리에 등록한다.
//# ASpyMissionTargetPoint 는 RootComponent 가 이미 있어 SetActorLocation 이 World 없이도 반영된다.
static ASpyMissionTargetPoint* SpyNavigationComponentTests_MakeHideTriggerTarget(
	USpyMissionTargetRegistrySubsystem* Registry, const FVector& InLocation, bool bEnableHideTrigger)
{
	ASpyMissionTargetPoint* Target = NewObject<ASpyMissionTargetPoint>(GetTransientPackage());
	Target->SetActorLocation(InLocation);

	FBoolProperty* Prop = FindFProperty<FBoolProperty>(ASpyMissionTargetPoint::StaticClass(), TEXT("bEnableHideTrigger"));
	check(Prop != nullptr);
	Prop->SetPropertyValue_InContainer(Target, bEnableHideTrigger);

	Registry->RegisterMissionTargetLocation(SpyGameplayTags::Skill_Move_Vault, Target);

	return Target;
}

//# design 2026-08-10 §5 — 트리거 활성 NPC 타겟을 만들고 즉시 레지스트리에 등록한다(NPCId 키 공간).
static ASpyNPCCharacter* SpyNavigationComponentTests_MakeHideTriggerNPC(
	USpyMissionTargetRegistrySubsystem* Registry, int32 NPCId, const FVector& InLocation, bool bEnableHideTrigger)
{
	ASpyNPCCharacter* NPC = NewObject<ASpyNPCCharacter>(GetTransientPackage());
	NPC->SetActorLocation(InLocation);

	FBoolProperty* Prop = FindFProperty<FBoolProperty>(ASpyNPCCharacter::StaticClass(), TEXT("bEnableHideTrigger"));
	check(Prop != nullptr);
	Prop->SetPropertyValue_InContainer(NPC, bEnableHideTrigger);

	Registry->RegisterNPCLocation(NPCId, NPC);

	return NPC;
}

//# 기존 3개 핵심 테스트가 공유하는 Vault 마커 위치 — 원점이 아닌 값을 써서 "등록/조회
//# 성공 여부"뿐 아니라 "그 좌표가 정확히 패스스루되는가"까지 이 3개 테스트에서 함께 검증한다.
static const FVector SpyNavigationComponentTests_VaultMarkerLocation(500.f, -250.f, 120.f);

static USpyMissionTargetRegistrySubsystem* SpyNavigationComponentTests_MakeRegistryWithVaultTarget()
{
	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	AActor* MarkerActor = SpyNavigationComponentTests_MakeLocatedActor(SpyNavigationComponentTests_VaultMarkerLocation);
	Registry->RegisterMissionTargetLocation(SpyGameplayTags::Skill_Move_Vault, MarkerActor);

	return Registry;
}

//# SpyMissionComponentTests.cpp 의 동일 리플렉션 기법을 이 파일 전용으로 다시 둔다 — OnRep_MissionState
//# 는 protected UFUNCTION 이라 실제 레플리케이션 없이 "원격 클라이언트가 새 스냅샷을 받았다"를
//# 시뮬레이션하는 유일한 방법이다.
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

static void SpyNavigationComponentTests_SetMissionState(USpyMissionComponent* Component, const FSpyMissionState& NewState)
{
	FStructProperty* MissionStateProp = FindFProperty<FStructProperty>(USpyMissionComponent::StaticClass(), TEXT("MissionState"));
	check(MissionStateProp != nullptr);
	MissionStateProp->SetValue_InContainer(Component, &NewState);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentAcceptStartsPathTest,
	"SkillProject.Navigation.Component.AcceptStartsPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentAcceptStartsPathTest::RunTest(const FString& Parameters)
{
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);
	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, SpyNavigationComponentTests_MakeConfig());

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);
	NavComponent->SetMissionTargetRegistry(SpyNavigationComponentTests_MakeRegistryWithVaultTarget());

	TestFalse(TEXT("Path inactive before accept"), NavComponent->IsPathActive());

	MissionComponent->AcceptCurrentMission();

	TestTrue(TEXT("Path active after accept"), NavComponent->IsPathActive());
	TestEqual(TEXT("Target location matches the registered marker's non-zero location"), NavComponent->GetCurrentTargetLocation(), SpyNavigationComponentTests_VaultMarkerLocation);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentNoTargetLocationTest,
	"SkillProject.Navigation.Component.NoTargetLocation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentNoTargetLocationTest::RunTest(const FString& Parameters)
{
	//# 장소 무관 미션(Kill)은 어떤 액터도 그 MatchTag 로 등록하지 않으므로 조회가 항상
	//# 실패해야 한다 — 이것이 §5-1 원칙 3항("대상 없음")을 구현하는 메커니즘이다(design §5-5)
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
	NavComponent->SetMissionTargetRegistry(SpyNavigationComponentTests_MakeRegistryWithVaultTarget());

	MissionComponent->AcceptCurrentMission();

	TestFalse(TEXT("No matching registry entry -> path stays inactive"), NavComponent->IsPathActive());

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
	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, SpyNavigationComponentTests_MakeConfig());

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);
	NavComponent->SetMissionTargetRegistry(SpyNavigationComponentTests_MakeRegistryWithVaultTarget());

	MissionComponent->AcceptCurrentMission();
	TestTrue(TEXT("Path active after accept"), NavComponent->IsPathActive());

	//# 목표 3회 중 3회 채워 완료시킨다 (Vault 미션, Accumulate)
	MissionComponent->AddProgress(SpyGameplayTags::Skill_Move_Vault, 3);

	TestFalse(TEXT("Path inactive after mission completed"), NavComponent->IsPathActive());

	return true;
}

//# ─────────────────────────────────────────────────────────────────────────────
//# 이하 test-engineer 확장 (design §5-7·§7-6, 2026-08-05 개정 기준 재작성)
//# ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentTargetLocationPassesThroughTest,
	"SkillProject.Navigation.Component.TargetLocationPassesThrough",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentTargetLocationPassesThroughTest::RunTest(const FString& Parameters)
{
	//# code-reviewer BLOCKER 회귀 고정 — 실제 ASpyMissionTargetPoint(마커 액터)를 원점이 아닌
	//# 좌표로 옮겨서, 레지스트리→NavComponent 경로가 그 좌표를 정확히 패스스루하는지 검증한다.
	//# RootComponent 가 없었다면(수정 전) 이 좌표는 항상 (0,0,0)으로 관측됐을 것이다.
	const FVector ExpectedLocation(1234.f, -567.f, 89.f);

	ASpyMissionTargetPoint* Marker = NewObject<ASpyMissionTargetPoint>(GetTransientPackage());
	Marker->SetActorLocation(ExpectedLocation);

	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	Registry->RegisterMissionTargetLocation(SpyGameplayTags::Skill_Move_Vault, Marker);

	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);
	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, SpyNavigationComponentTests_MakeConfig());

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);
	NavComponent->SetMissionTargetRegistry(Registry);

	MissionComponent->AcceptCurrentMission();

	TestTrue(TEXT("Path active after accept"), NavComponent->IsPathActive());
	TestEqual(TEXT("Target location exactly matches ASpyMissionTargetPoint's non-zero world location"), NavComponent->GetCurrentTargetLocation(), ExpectedLocation);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentDialogueTargetPassesThroughTest,
	"SkillProject.Navigation.Component.DialogueTargetPassesThrough",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentDialogueTargetPassesThroughTest::RunTest(const FString& Parameters)
{
	//# Dialogue 분기(NPCId 키 공간)의 동일 회귀 고정 — 실제 ASpyNPCCharacter 를 원점이 아닌
	//# 좌표로 옮겨 등록한다(design §5-1 원칙 1항).
	const FVector ExpectedLocation(-42.f, 815.f, 16.f);
	const int32 NPCId = 7;

	ASpyNPCCharacter* NPC = NewObject<ASpyNPCCharacter>(GetTransientPackage());
	NPC->SetActorLocation(ExpectedLocation);

	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	Registry->RegisterNPCLocation(NPCId, NPC);

	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();
	UDataTable* MissionTable = NewObject<UDataTable>();
	MissionTable->RowStruct = FSpyMissionRow::StaticStruct();

	FSpyMissionRow Row;
	Row.MissionId = 1;
	Row.MissionType = ESpyMissionType::Dialogue;
	Row.MatchTag = SpyGameplayTags::Event_Mission_Report;
	Row.NPCId = NPCId;
	MissionTable->AddRow(TEXT("Mission_1"), Row);
	Config->MissionTable = MissionTable;

	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);
	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, Config);

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);
	NavComponent->SetMissionTargetRegistry(Registry);

	MissionComponent->AcceptCurrentMission();

	TestTrue(TEXT("Path active after accept (Dialogue auto-resolves NPCId branch)"), NavComponent->IsPathActive());
	TestEqual(TEXT("Target location exactly matches ASpyNPCCharacter's non-zero world location"), NavComponent->GetCurrentTargetLocation(), ExpectedLocation);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentInteractTargetUsesMatchTagBranchTest,
	"SkillProject.Navigation.Component.InteractTargetUsesMatchTagBranch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentInteractTargetUsesMatchTagBranchTest::RunTest(const FString& Parameters)
{
	//# design §5-3 마지막 문단 — Interact 타입은 NPCId 가 없으므로(ASpyInteractableObject 실측)
	//# Dialogue 분기가 아니라 Gameplay 와 동일한 MatchTag 분기를 타야 한다. NPCId 는 일부러
	//# NoNPCId 로 남겨 둔다 — 그래도 조회가 성공해야 이 분기가 MatchTag 로 갔다는 증거가 된다.
	const FVector ExpectedLocation(300.f, 300.f, 0.f);
	const FGameplayTag InteractTag = SpyGameplayTags::Event_Mission_Interact;

	ASpyInteractableObject* Interactable = NewObject<ASpyInteractableObject>(GetTransientPackage());
	Interactable->SetActorLocation(ExpectedLocation);

	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	Registry->RegisterMissionTargetLocation(InteractTag, Interactable);

	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();
	UDataTable* MissionTable = NewObject<UDataTable>();
	MissionTable->RowStruct = FSpyMissionRow::StaticStruct();

	FSpyMissionRow Row;
	Row.MissionId = 1;
	Row.MissionType = ESpyMissionType::Interact;
	Row.MatchTag = InteractTag;
	MissionTable->AddRow(TEXT("Mission_1"), Row);
	Config->MissionTable = MissionTable;

	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);
	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, Config);

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);
	NavComponent->SetMissionTargetRegistry(Registry);

	MissionComponent->AcceptCurrentMission();

	TestTrue(TEXT("Path active — Interact resolved via the MatchTag branch, not NPCId"), NavComponent->IsPathActive());
	TestEqual(TEXT("Target location matches the interactable object's location"), NavComponent->GetCurrentTargetLocation(), ExpectedLocation);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentDialogueMissingNPCIdSkipsLookupTest,
	"SkillProject.Navigation.Component.DialogueMissingNPCIdSkipsLookup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentDialogueMissingNPCIdSkipsLookupTest::RunTest(const FString& Parameters)
{
	//# design §5-2 회귀 고정 — Dialogue 인데 NPCId == NoNPCId(9999) 이면 레지스트리 조회
	//# 자체를 시도하지 않아야 한다. 레지스트리에 "우연히" 9999 키로 등록된 액터를 심어 둬서,
	//# 정말로 게이트가 조회를 막았는지(그저 아무것도 없어서 실패한 게 아닌지) 구분한다.
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);

	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();
	UDataTable* MissionTable = NewObject<UDataTable>();
	MissionTable->RowStruct = FSpyMissionRow::StaticStruct();

	FSpyMissionRow Row;
	Row.MissionId = 1;
	Row.MissionType = ESpyMissionType::Dialogue;
	Row.MatchTag = SpyGameplayTags::Event_Mission_Report;
	Row.NPCId = FSpyMissionRow::NoNPCId; //# 명시적으로 담당 NPC 없음
	MissionTable->AddRow(TEXT("Mission_1"), Row);
	Config->MissionTable = MissionTable;

	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, Config);

	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	Registry->RegisterNPCLocation(FSpyMissionRow::NoNPCId, SpyNavigationComponentTests_MakeLocatedActor(FVector(1.f, 2.f, 3.f)));

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);
	NavComponent->SetMissionTargetRegistry(Registry);

	MissionComponent->AcceptCurrentMission();

	TestFalse(TEXT("NPCId == NoNPCId gate blocks the lookup even though the sentinel key resolves"), NavComponent->IsPathActive());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentGameplayInvalidMatchTagSkipsLookupTest,
	"SkillProject.Navigation.Component.GameplayInvalidMatchTagSkipsLookup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentGameplayInvalidMatchTagSkipsLookupTest::RunTest(const FString& Parameters)
{
	//# design §5-2 회귀 고정 — Gameplay/Interact 인데 MatchTag 가 invalid 이면 조회를 시도하지
	//# 않아야 한다(에디터 데이터 실수 방어). 레지스트리에는 관계없는 유효한 태그 항목을 하나
	//# 남겨 둬서, "아무것도 없어서"가 아니라 게이트가 실제로 막았음을 구분한다.
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);

	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();
	UDataTable* MissionTable = NewObject<UDataTable>();
	MissionTable->RowStruct = FSpyMissionRow::StaticStruct();

	FSpyMissionRow Row;
	Row.MissionId = 1;
	Row.MissionType = ESpyMissionType::Gameplay;
	//# Row.MatchTag 를 의도적으로 설정하지 않는다 — invalid 상태 그대로 (에디터 데이터 누락 시뮬레이션)
	MissionTable->AddRow(TEXT("Mission_1"), Row);
	Config->MissionTable = MissionTable;

	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, Config);

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);
	NavComponent->SetMissionTargetRegistry(SpyNavigationComponentTests_MakeRegistryWithVaultTarget());

	MissionComponent->AcceptCurrentMission();

	TestFalse(TEXT("Invalid MatchTag gate blocks the lookup"), NavComponent->IsPathActive());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentUnbindStopsListeningTest,
	"SkillProject.Navigation.Component.UnbindStopsListening",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentUnbindStopsListeningTest::RunTest(const FString& Parameters)
{
	//# 구독 해제 엣지케이스 — UnbindMissionComponent() 이후에는 그 컴포넌트가 계속 진행돼도
	//# NavComponent 가 더 이상 반응하지 않아야 한다(델리게이트가 실제로 RemoveDynamic 됐는가).
	USpyMissionConfig* Config = SpyNavigationComponentTests_MakeConfig();

	UDataTable* MissionTable = Config->MissionTable;
	FSpyMissionRow DialogueRow;
	DialogueRow.MissionId = 2;
	DialogueRow.MissionType = ESpyMissionType::Dialogue;
	DialogueRow.MatchTag = SpyGameplayTags::Event_Mission_Report;
	MissionTable->AddRow(TEXT("Mission_2"), DialogueRow);

	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);
	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, Config);

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);
	NavComponent->SetMissionTargetRegistry(SpyNavigationComponentTests_MakeRegistryWithVaultTarget());

	MissionComponent->AcceptCurrentMission();
	TestTrue(TEXT("Path active after accept, before unbind"), NavComponent->IsPathActive());
	TestEqual(TEXT("Target is the vault marker before unbind"), NavComponent->GetCurrentTargetLocation(), SpyNavigationComponentTests_VaultMarkerLocation);

	NavComponent->UnbindMissionComponent();

	//# 미션 1을 완료시켜 미션 2(Dialogue, 자동 수락)로 전이 — Unbind 전이었다면 StopPath() 후
	//# 재조회가 일어났겠지만, Unbind 이후에는 어떤 반응도 없어야 한다.
	MissionComponent->AddProgress(SpyGameplayTags::Skill_Move_Vault, 3);

	TestTrue(TEXT("Path state unchanged after unbind — no longer reacting to completion"), NavComponent->IsPathActive());
	TestEqual(TEXT("Target location unchanged after unbind — no retarget attempted"), NavComponent->GetCurrentTargetLocation(), SpyNavigationComponentTests_VaultMarkerLocation);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentSequentialMissionsRetargetTest,
	"SkillProject.Navigation.Component.SequentialMissionsRetarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentSequentialMissionsRetargetTest::RunTest(const FString& Parameters)
{
	//# 정상 순차 전이 — 미션1(Gameplay, Vault) 완료 -> 미션2(Dialogue, 자동 수락) 진입 시
	//# 내비게이션이 미션1의 낡은 목표를 지우고 미션2의 새 목표(NPC 위치)로 정확히 재타겟팅해야 한다.
	const FVector VaultLocation(100.f, 0.f, 0.f);
	const FVector NPCLocation(0.f, 100.f, 0.f);
	const int32 NPCId = 42;

	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();
	UDataTable* MissionTable = NewObject<UDataTable>();
	MissionTable->RowStruct = FSpyMissionRow::StaticStruct();

	FSpyMissionRow VaultRow;
	VaultRow.MissionId = 1;
	VaultRow.MissionType = ESpyMissionType::Gameplay;
	VaultRow.MatchTag = SpyGameplayTags::Skill_Move_Vault;
	VaultRow.Mode = ESpyMissionMode::Accumulate;
	VaultRow.TargetCount = 1;
	MissionTable->AddRow(TEXT("Mission_1"), VaultRow);

	FSpyMissionRow DialogueRow;
	DialogueRow.MissionId = 2;
	DialogueRow.MissionType = ESpyMissionType::Dialogue;
	DialogueRow.MatchTag = SpyGameplayTags::Event_Mission_Report;
	DialogueRow.NPCId = NPCId;
	MissionTable->AddRow(TEXT("Mission_2"), DialogueRow);

	Config->MissionTable = MissionTable;

	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	Registry->RegisterMissionTargetLocation(SpyGameplayTags::Skill_Move_Vault, SpyNavigationComponentTests_MakeLocatedActor(VaultLocation));
	Registry->RegisterNPCLocation(NPCId, SpyNavigationComponentTests_MakeLocatedActor(NPCLocation));

	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);
	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, Config);

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);
	NavComponent->SetMissionTargetRegistry(Registry);

	MissionComponent->AcceptCurrentMission();
	TestEqual(TEXT("Target starts at the vault marker"), NavComponent->GetCurrentTargetLocation(), VaultLocation);

	MissionComponent->AddProgress(SpyGameplayTags::Skill_Move_Vault, 1);

	TestTrue(TEXT("Path still active — mission 2 auto-accepted and retargeted"), NavComponent->IsPathActive());
	TestEqual(TEXT("Target retargets to mission 2's NPC location, not the stale vault location"), NavComponent->GetCurrentTargetLocation(), NPCLocation);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentMultiStepSkipRetargetsTest,
	"SkillProject.Navigation.Component.MultiStepSkipRetargets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentMultiStepSkipRetargetsTest::RunTest(const FString& Parameters)
{
	//# design §2-3 "매 트랜지션과 1:1 대응하지 않는다, 그래도 내비게이션엔 무해"를 고정한다 —
	//# 원격 클라이언트가 미션1(완료)->미션2(스킵, Dialogue)->미션3(Dialogue) 스냅샷을 한 번의
	//# OnRep 으로 받으면, 내비게이션은 미션2 를 전혀 조회하지 않고 최종 미션3 로 곧바로
	//# 재타겟팅해야 한다(SpyMissionComponentTests.cpp RemoteClientMultiStepSkipTest 와 짝).
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();
	UDataTable* MissionTable = NewObject<UDataTable>();
	MissionTable->RowStruct = FSpyMissionRow::StaticStruct();

	FSpyMissionRow Row1;
	Row1.MissionId = 1;
	Row1.MissionType = ESpyMissionType::Gameplay;
	Row1.MatchTag = SpyGameplayTags::Skill_Move_Vault;
	MissionTable->AddRow(TEXT("Mission_1"), Row1);

	FSpyMissionRow Row2; //# 스킵되는 중간 미션 — 목표를 등록해 두어 "조회조차 안 됐다"를 검증 가능하게 한다
	Row2.MissionId = 2;
	Row2.MissionType = ESpyMissionType::Dialogue;
	Row2.MatchTag = SpyGameplayTags::Event_Mission_Report;
	Row2.NPCId = 20;
	MissionTable->AddRow(TEXT("Mission_2"), Row2);

	FSpyMissionRow Row3;
	Row3.MissionId = 3;
	Row3.MissionType = ESpyMissionType::Dialogue;
	Row3.MatchTag = SpyGameplayTags::Event_Mission_Report;
	Row3.NPCId = 30;
	MissionTable->AddRow(TEXT("Mission_3"), Row3);

	Config->MissionTable = MissionTable;

	const FVector SkippedLocation(999.f, 999.f, 999.f);
	const FVector FinalLocation(50.f, 60.f, 70.f);

	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	Registry->RegisterNPCLocation(20, SpyNavigationComponentTests_MakeLocatedActor(SkippedLocation));
	Registry->RegisterNPCLocation(30, SpyNavigationComponentTests_MakeLocatedActor(FinalLocation));

	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);
	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, Config);

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);
	NavComponent->SetMissionTargetRegistry(Registry);

	FSpyMissionState OldState;
	OldState.MissionIndex = 1;
	OldState.Count = 0;
	OldState.bAccepted = true;

	FSpyMissionState NewState;
	NewState.MissionIndex = 3;
	NewState.Count = 0;
	NewState.bAccepted = true;
	SpyNavigationComponentTests_SetMissionState(MissionComponent, NewState);

	SpyNavigationComponentTests_SimulateReplication(MissionComponent, OldState);

	TestTrue(TEXT("Path active after skip-retarget to mission 3"), NavComponent->IsPathActive());
	TestEqual(TEXT("Target retargets directly to mission 3's NPC location, skipping mission 2 entirely"), NavComponent->GetCurrentTargetLocation(), FinalLocation);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentFirstResolveAttemptFailsGracefullyTest,
	"SkillProject.Navigation.Component.FirstResolveAttemptFailsGracefully",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentFirstResolveAttemptFailsGracefullyTest::RunTest(const FString& Parameters)
{
	//# design §5-6 재시도 타이머 — 실제 0.2초 간격 최대 약 10회 반복은 World 의 타이머 틱이
	//# 필요해 이 SimpleAutomationTest 하네스(World 없음)로는 자동화할 수 없다(보고서 "자동화
	//# 한계" 참조). TargetRetryCount 는 UPROPERTY 가 아니라 리플렉션으로도 순수하게 들여다볼
	//# 수 없다 — production 코드 추가 요청으로 별도 보고한다. 여기서는 관측 가능한 범위에서
	//# "빈 레지스트리로 최초 조회가 실패해도 크래시 없이 path-inactive 로 안전하게 끝난다"만 고정한다.
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);

	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();
	UDataTable* MissionTable = NewObject<UDataTable>();
	MissionTable->RowStruct = FSpyMissionRow::StaticStruct();

	FSpyMissionRow Row;
	Row.MissionId = 1;
	Row.MissionType = ESpyMissionType::Gameplay;
	Row.MatchTag = SpyGameplayTags::Skill_Move_Vault;
	MissionTable->AddRow(TEXT("Mission_1"), Row);
	Config->MissionTable = MissionTable;

	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, Config);

	//# 빈 레지스트리 — Vault 를 등록한 액터가 없어 최초 조회가 반드시 실패한다
	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);
	NavComponent->SetMissionTargetRegistry(NewObject<USpyMissionTargetRegistrySubsystem>());

	MissionComponent->AcceptCurrentMission();

	TestFalse(TEXT("Path stays inactive — first resolve attempt failed, no crash"), NavComponent->IsPathActive());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentFixtureRealClassesSelfCheckTest,
	"SkillProject.Navigation.Component.FixtureRealClassesSelfCheck",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentFixtureRealClassesSelfCheckTest::RunTest(const FString& Parameters)
{
	//# 자가진단 — TargetLocationPassesThrough/DialogueTargetPassesThrough/InteractTargetUsesMatchTagBranch
	//# 는 전부 "World 없이 NewObject 로 만든 ASpyMissionTargetPoint/ASpyNPCCharacter 도
	//# SetActorLocation 이 RootComponent 를 통해 실제로 반영된다"는 가정에 기댄다. 이 가정이
	//# 틀리면 그 테스트들은 production 회귀와 똑같은 실패 모양(기대 좌표 대신 원점 관측)으로
	//# 보인다 — 이 테스트가 먼저 깨지면 원인이 픽스처임을 바로 구분할 수 있다.
	const FVector ExpectedLocation(111.f, 222.f, 333.f);

	ASpyMissionTargetPoint* Marker = NewObject<ASpyMissionTargetPoint>(GetTransientPackage());
	Marker->SetActorLocation(ExpectedLocation);
	TestEqual(TEXT("ASpyMissionTargetPoint.SetActorLocation takes effect without a World"), Marker->GetActorLocation(), ExpectedLocation);

	ASpyNPCCharacter* NPC = NewObject<ASpyNPCCharacter>(GetTransientPackage());
	NPC->SetActorLocation(ExpectedLocation);
	TestEqual(TEXT("ASpyNPCCharacter.SetActorLocation takes effect without a World"), NPC->GetActorLocation(), ExpectedLocation);

	return true;
}

//# ─────────────────────────────────────────────────────────────────────────────
//# test-engineer 확장 (design npc-mission-dialogue.md §6-2(a) 결함 B) —
//# 바인드 시점 pull 회귀. 픽스처는 추상적인 MissionId 값만 쓰므로 실제 미션 체인 행수와
//# 무관하게 유효하다.
//# ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentBindAfterAcceptPullsImmediatelyTest,
	"SkillProject.Navigation.Component.BindAfterAcceptPullsImmediately",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentBindAfterAcceptPullsImmediatelyTest::RunTest(const FString& Parameters)
{
	//# design npc-mission-dialogue.md §6-2(a) 결함 B 회귀 — ASC-init 처럼 바인드보다 먼저
	//# 수락이 끝나 있는 세션 시작 경합을 재현한다. 이미 수락된 상태로 바인드하면 새 브로드캐스트를
	//# 기다리지 않고 바인드 직후 즉시 1회 pull 로 경로를 시작해야 한다
	//# (USpyNavigationComponent::BindMissionComponent, SpyMainHUD::RefreshMission()과 동일 패턴).
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);
	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, SpyNavigationComponentTests_MakeConfig());

	//# 수락을 바인드보다 먼저 완료시킨다 — 바인드 시점에는 이미 OnMissionAccepted 가 지나간 뒤다
	MissionComponent->AcceptCurrentMission();

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->SetMissionTargetRegistry(SpyNavigationComponentTests_MakeRegistryWithVaultTarget());

	TestFalse(TEXT("Path inactive before bind (no subscriber yet)"), NavComponent->IsPathActive());

	NavComponent->BindMissionComponent(MissionComponent);

	TestTrue(TEXT("Bind pulls the already-accepted mission and starts the path immediately"), NavComponent->IsPathActive());
	TestEqual(TEXT("Target resolves to the vault marker without waiting for a new broadcast"), NavComponent->GetCurrentTargetLocation(), SpyNavigationComponentTests_VaultMarkerLocation);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentUnacceptedMissingNPCIdSkipsLookupTest,
	"SkillProject.Navigation.Component.UnacceptedMissingNPCIdSkipsLookup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentUnacceptedMissingNPCIdSkipsLookupTest::RunTest(const FString& Parameters)
{
	//# code-reviewer 지적 회귀 — 이전 이름("BindWithoutPriorAcceptDoesNotPull")은 mission-ground-
	//# navigation.md §5-2-1 신규 규칙(미수락이면 MissionType 무관 담당 NPC로 안내)과 정반대로
	//# 읽힌다. 실제로 이 테스트가 통과하는 이유는 "미수락이라서"가 아니라 픽스처의 NPCId 가
	//# NoNPCId(9999) sentinel 로 남아 있어 TryResolveTarget 의 sentinel 게이트(§5-6)에 걸리기
	//# 때문이다 — 진짜 의도대로 이름·픽스처를 명시화해 재작성한다(§5-2-1 자체의 반례가 아님).
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);

	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();
	UDataTable* MissionTable = NewObject<UDataTable>();
	MissionTable->RowStruct = FSpyMissionRow::StaticStruct();

	FSpyMissionRow Row;
	Row.MissionId = 1;
	Row.MissionType = ESpyMissionType::Gameplay;
	Row.MatchTag = SpyGameplayTags::Skill_Move_Vault;
	Row.NPCId = FSpyMissionRow::NoNPCId; //# 명시적으로 담당 NPC 없음 — §5-2-1 미수락 분기라도 조회할 대상이 없다
	MissionTable->AddRow(TEXT("Mission_1"), Row);
	Config->MissionTable = MissionTable;

	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, Config);

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->SetMissionTargetRegistry(SpyNavigationComponentTests_MakeRegistryWithVaultTarget());

	//# AcceptCurrentMission() 을 호출하지 않는다 — 미수락 상태에서 바인드해도 NPCId 가 없으면
	//# (§5-2-1 분기 자체는 타지만) sentinel 게이트가 조회를 막아야 한다
	NavComponent->BindMissionComponent(MissionComponent);

	TestFalse(TEXT("Unaccepted + NoNPCId sentinel — the gate blocks the lookup, not the unaccepted state itself"), NavComponent->IsPathActive());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentUnacceptedGameplayGuidesToNPCTest,
	"SkillProject.Navigation.Component.UnacceptedGameplayGuidesToNPC",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentUnacceptedGameplayGuidesToNPCTest::RunTest(const FString& Parameters)
{
	//# design npc-mission-dialogue.md §6-2·§6-5(조건8) / mission-ground-navigation.md §5-2-1 —
	//# 부트스트랩(Greeting) 없이도 세션 시작부터 곧장 담당 NPC로 안내되는 신규 핵심 동작.
	//# 미수락 상태는 MissionType 과 무관하게 NPCId 키 공간을 조회한다 — 여기서는 일부러
	//# MissionType 을 Dialogue 가 아니라 Gameplay(카드 수락이 필요한 처치 미션 등)로 두고
	//# AcceptCurrentMission() 을 호출하지 않는다. MatchTag(Event_Mission_Kill)는 레지스트리에
	//# 등록하지 않아, 조회가 성공한다면 그것이 반드시 NPCId 경로를 탔다는 증거가 되게 한다.
	const FVector NPCLocation(700.f, -300.f, 50.f);
	const int32 NPCId = 5;

	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	Registry->RegisterNPCLocation(NPCId, SpyNavigationComponentTests_MakeLocatedActor(NPCLocation));

	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();
	UDataTable* MissionTable = NewObject<UDataTable>();
	MissionTable->RowStruct = FSpyMissionRow::StaticStruct();

	FSpyMissionRow Row;
	Row.MissionId = 1;
	Row.MissionType = ESpyMissionType::Gameplay; //# Dialogue 가 아님 — NPCId 조회가 타입과 무관함을 증명하는 핵심
	Row.MatchTag = SpyGameplayTags::Event_Mission_Kill; //# 레지스트리에 등록하지 않는다 — MatchTag 분기였다면 실패했어야 한다
	Row.Mode = ESpyMissionMode::Accumulate;
	Row.TargetCount = 1;
	Row.NPCId = NPCId;
	MissionTable->AddRow(TEXT("Mission_1"), Row);
	Config->MissionTable = MissionTable;

	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);
	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, Config);

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->SetMissionTargetRegistry(Registry);

	//# AcceptCurrentMission() 을 의도적으로 호출하지 않는다 — 미수락 상태를 유지한 채 바인드한다.
	//# BindMissionComponent 는 구독 직후 현재 상태로 HandleMissionProgressChanged 를 1회
	//# 동기 호출하므로, 레지스트리를 먼저 세팅해 둬야 이 1회 호출 안에서 바로 해석된다.
	NavComponent->BindMissionComponent(MissionComponent);

	TestTrue(TEXT("Unaccepted Gameplay mission still guides to its NPC — MissionType is not the gate while unaccepted"), NavComponent->IsPathActive());
	TestEqual(TEXT("Target resolves via NPCId, not MatchTag, while unaccepted"), NavComponent->GetCurrentTargetLocation(), NPCLocation);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentHideTriggerBindsWhenEnabledTest,
	"SkillProject.Navigation.Component.HideTriggerBindsWhenEnabled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentHideTriggerBindsWhenEnabledTest::RunTest(const FString& Parameters)
{
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);
	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, SpyNavigationComponentTests_MakeConfig());

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);

	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	SpyNavigationComponentTests_MakeHideTriggerTarget(Registry, FVector(500.f, -250.f, 120.f), true);
	NavComponent->SetMissionTargetRegistry(Registry);

	MissionComponent->AcceptCurrentMission();

	TestTrue(TEXT("Path active after accept"), NavComponent->IsPathActive());
	TestTrue(TEXT("Hide trigger bound — target has bEnableHideTrigger=true"), NavComponent->IsHideTriggerBound());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentHideTriggerDisabledTargetDoesNotBindTest,
	"SkillProject.Navigation.Component.HideTriggerDisabledTargetDoesNotBind",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentHideTriggerDisabledTargetDoesNotBindTest::RunTest(const FString& Parameters)
{
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);
	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, SpyNavigationComponentTests_MakeConfig());

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);

	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	//# 명시적으로 false로 끈 타겟(opt-out) — 기존 레벨과 동일 조건
	SpyNavigationComponentTests_MakeHideTriggerTarget(Registry, FVector(500.f, -250.f, 120.f), false);
	NavComponent->SetMissionTargetRegistry(Registry);

	MissionComponent->AcceptCurrentMission();

	TestTrue(TEXT("Path still starts normally"), NavComponent->IsPathActive());
	TestFalse(TEXT("No trigger bound — falls back to distance hysteresis"), NavComponent->IsHideTriggerBound());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentHideTriggerEnterHidesPathTest,
	"SkillProject.Navigation.Component.HideTriggerEnterHidesPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentHideTriggerEnterHidesPathTest::RunTest(const FString& Parameters)
{
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);
	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, SpyNavigationComponentTests_MakeConfig());

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);

	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	SpyNavigationComponentTests_MakeHideTriggerTarget(Registry, FVector(500.f, -250.f, 120.f), true);
	NavComponent->SetMissionTargetRegistry(Registry);

	MissionComponent->AcceptCurrentMission();
	TestTrue(TEXT("Path visible immediately after accept (seeded true in StartPathTo)"), NavComponent->IsPathVisible());

	NavComponent->NotifyHideTriggerEntered(Owner);

	TestFalse(TEXT("Path hides immediately on trigger enter, no distance check needed"), NavComponent->IsPathVisible());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentHideTriggerIgnoresOtherActorTest,
	"SkillProject.Navigation.Component.HideTriggerIgnoresOtherActor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentHideTriggerIgnoresOtherActorTest::RunTest(const FString& Parameters)
{
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);
	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, SpyNavigationComponentTests_MakeConfig());

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);

	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	SpyNavigationComponentTests_MakeHideTriggerTarget(Registry, FVector(500.f, -250.f, 120.f), true);
	NavComponent->SetMissionTargetRegistry(Registry);

	MissionComponent->AcceptCurrentMission();

	AActor* UnrelatedActor = NewObject<AActor>(GetTransientPackage());
	NavComponent->NotifyHideTriggerEntered(UnrelatedActor);

	TestTrue(TEXT("A different local pawn overlapping the same trigger does not affect this owner's nav"), NavComponent->IsPathVisible());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentHideTriggerExitReshowsImmediatelyTest,
	"SkillProject.Navigation.Component.HideTriggerExitReshowsImmediately",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentHideTriggerExitReshowsImmediatelyTest::RunTest(const FString& Parameters)
{
	//# design-reviewer BLOCKER 회귀 고정 — World/NavSystem 없이도(RecomputePath 조기반환 환경) 즉시
	//# 재표시되는지 검증한다. bPathVisible 시딩은 RecomputePath 성공 여부와 무관하게 동기 반영돼야 한다.
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);
	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, SpyNavigationComponentTests_MakeConfig());

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);

	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	SpyNavigationComponentTests_MakeHideTriggerTarget(Registry, FVector(500.f, -250.f, 120.f), true);
	NavComponent->SetMissionTargetRegistry(Registry);

	MissionComponent->AcceptCurrentMission();

	NavComponent->NotifyHideTriggerEntered(Owner);
	TestFalse(TEXT("Path hidden after entering the trigger"), NavComponent->IsPathVisible());

	NavComponent->NotifyHideTriggerExited(Owner);
	TestTrue(TEXT("Path visible again immediately on exit — no distance hysteresis dead zone"), NavComponent->IsPathVisible());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentHideTriggerUnbindsOnStopPathTest,
	"SkillProject.Navigation.Component.HideTriggerUnbindsOnStopPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentHideTriggerUnbindsOnStopPathTest::RunTest(const FString& Parameters)
{
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);
	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, SpyNavigationComponentTests_MakeConfig());

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);

	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	SpyNavigationComponentTests_MakeHideTriggerTarget(Registry, FVector(500.f, -250.f, 120.f), true);
	NavComponent->SetMissionTargetRegistry(Registry);

	MissionComponent->AcceptCurrentMission();
	TestTrue(TEXT("Hide trigger bound after accept"), NavComponent->IsHideTriggerBound());

	//# 목표 3회 중 3회 채워 완료시킨다 (Vault 미션, Accumulate) — FSpyNavigationComponentCompleteStopsPathTest 와 동일 패턴
	MissionComponent->AddProgress(SpyGameplayTags::Skill_Move_Vault, 3);

	TestFalse(TEXT("Path inactive after mission completed"), NavComponent->IsPathActive());
	TestFalse(TEXT("Hide trigger unbound once the path stops"), NavComponent->IsHideTriggerBound());

	return true;
}

//# ─────────────────────────────────────────────────────────────────────────────
//# code-reviewer 확인 갭 — HideTriggerExitReshowsImmediatelyTest 는 World 없어 RecomputePath 가
//# 조기반환해 ApplyPathPoints()의 BoundHideTrigger.IsValid() 바이패스를 위양성으로 통과시켰다.
//# ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentApplyPathPointsBypassesHysteresisWhenTriggerBoundTest,
	"SkillProject.Navigation.Component.ApplyPathPointsBypassesHysteresisWhenTriggerBound",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentApplyPathPointsBypassesHysteresisWhenTriggerBoundTest::RunTest(const FString& Parameters)
{
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyNavigationComponentTestAccessor* NavComponent = NewObject<USpyNavigationComponentTestAccessor>(Owner);

	USplineComponent* Spline = NewObject<USplineComponent>(GetTransientPackage());
	NavComponent->Test_SetPathSpline(Spline);

	UBoxComponent* FakeTrigger = NewObject<UBoxComponent>(GetTransientPackage());
	NavComponent->Test_SetBoundHideTrigger(FakeTrigger);

	//# exit 직후 시딩된 "이전엔 보였다" 상태를 재현한다(NotifyHideTriggerExited 의 bPathVisible=true 시드와 동일)
	NavComponent->Test_SetPathVisible(true);

	//# half-extent 가 ArrivalHideDistanceCm(300cm) 보다 작은 트리거에서 exit 한 상황을 흉내낸다 —
	//# 단일 좌표라 ComputePathLength == 0, 히스테리시스만 있었다면 즉시 재은닉됐어야 한다.
	TArray<FVector> ShortPath;
	ShortPath.Add(FVector(10.f, 20.f, 30.f));
	NavComponent->Test_ApplyPathPoints(ShortPath);

	TestTrue(TEXT("BoundHideTrigger valid — bypass branch taken"), NavComponent->IsHideTriggerBound());
	TestTrue(TEXT("ApplyPathPoints keeps the path visible despite RemainingPathLength(0cm) being far below ArrivalHideDistanceCm(300cm)"), NavComponent->IsPathVisible());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentApplyPathPointsAppliesHysteresisWhenTriggerNotBoundTest,
	"SkillProject.Navigation.Component.ApplyPathPointsAppliesHysteresisWhenTriggerNotBound",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentApplyPathPointsAppliesHysteresisWhenTriggerNotBoundTest::RunTest(const FString& Parameters)
{
	//# 대조군 — 위 바이패스 테스트가 "항상 true"가 아니라 BoundHideTrigger 유무에 실제로
	//# 좌우됨을 증명한다. 트리거 미바인딩이면 동일한 짧은 경로가 정상 히스테리시스로 숨어야 한다.
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyNavigationComponentTestAccessor* NavComponent = NewObject<USpyNavigationComponentTestAccessor>(Owner);

	USplineComponent* Spline = NewObject<USplineComponent>(GetTransientPackage());
	NavComponent->Test_SetPathSpline(Spline);
	NavComponent->Test_SetPathVisible(true);

	TArray<FVector> ShortPath;
	ShortPath.Add(FVector(10.f, 20.f, 30.f));
	NavComponent->Test_ApplyPathPoints(ShortPath);

	TestFalse(TEXT("No trigger bound"), NavComponent->IsHideTriggerBound());
	TestFalse(TEXT("Without a bound trigger, the same short path correctly hides via the normal hysteresis"), NavComponent->IsPathVisible());

	return true;
}

//# ─────────────────────────────────────────────────────────────────────────────
//# 요청 갭 — StopPath 없이 다음 목표로 전환될 때(BindHideTrigger 가 항상 먼저 호출하는
//# UnbindHideTrigger, cpp:493) 이전 트리거 구독이 델리게이트 IsBound() 로 실제로 끊기는지 확인.
//# ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentSequentialHideTriggerTargetsRebindTest,
	"SkillProject.Navigation.Component.SequentialHideTriggerTargetsRebind",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentSequentialHideTriggerTargetsRebindTest::RunTest(const FString& Parameters)
{
	const int32 NPCId = 88;
	const FVector VaultLocation(500.f, -250.f, 120.f);
	const FVector NPCLocation(0.f, 300.f, 0.f);

	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();
	UDataTable* MissionTable = NewObject<UDataTable>();
	MissionTable->RowStruct = FSpyMissionRow::StaticStruct();

	FSpyMissionRow VaultRow;
	VaultRow.MissionId = 1;
	VaultRow.MissionType = ESpyMissionType::Gameplay;
	VaultRow.MatchTag = SpyGameplayTags::Skill_Move_Vault;
	VaultRow.Mode = ESpyMissionMode::Accumulate;
	VaultRow.TargetCount = 1;
	MissionTable->AddRow(TEXT("Mission_1"), VaultRow);

	FSpyMissionRow DialogueRow;
	DialogueRow.MissionId = 2;
	DialogueRow.MissionType = ESpyMissionType::Dialogue;
	DialogueRow.MatchTag = SpyGameplayTags::Event_Mission_Report;
	DialogueRow.NPCId = NPCId;
	MissionTable->AddRow(TEXT("Mission_2"), DialogueRow);

	Config->MissionTable = MissionTable;

	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	ASpyMissionTargetPoint* TargetA = SpyNavigationComponentTests_MakeHideTriggerTarget(Registry, VaultLocation, true);
	ASpyNPCCharacter* TargetB = SpyNavigationComponentTests_MakeHideTriggerNPC(Registry, NPCId, NPCLocation, true);

	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);
	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, Config);

	USpyNavigationComponentTestAccessor* NavComponent = NewObject<USpyNavigationComponentTestAccessor>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);
	NavComponent->SetMissionTargetRegistry(Registry);

	MissionComponent->AcceptCurrentMission();
	TestTrue(TEXT("Hide trigger bound to target A after accept"), NavComponent->IsHideTriggerBound());

	ISpyMissionTargetHideVolume* HideVolumeA = Cast<ISpyMissionTargetHideVolume>(TargetA);
	TestNotNull(TEXT("Target A casts to ISpyMissionTargetHideVolume"), HideVolumeA);
	if (HideVolumeA == nullptr)
		return false;

	UPrimitiveComponent* TriggerA = HideVolumeA->GetHideTriggerComponent();
	TestTrue(TEXT("Target A's trigger component listens for overlap events after bind"), NavComponent->Test_IsAnyHideTriggerListenerBound(TriggerA));

	//# 목표 1회 채워 완료 -> 미션2(Dialogue, 자동 수락)로 곧바로 전환. StopPath 를 거치지 않는다.
	MissionComponent->AddProgress(SpyGameplayTags::Skill_Move_Vault, 1);

	TestFalse(TEXT("Target A's trigger listener is removed once the target switches — no stale subscription"), NavComponent->Test_IsAnyHideTriggerListenerBound(TriggerA));
	TestTrue(TEXT("Hide trigger re-bound to target B"), NavComponent->IsHideTriggerBound());

	ISpyMissionTargetHideVolume* HideVolumeB = Cast<ISpyMissionTargetHideVolume>(TargetB);
	TestNotNull(TEXT("Target B casts to ISpyMissionTargetHideVolume"), HideVolumeB);
	if (HideVolumeB == nullptr)
		return false;

	UPrimitiveComponent* TriggerB = HideVolumeB->GetHideTriggerComponent();
	TestTrue(TEXT("Target B's trigger component now listens for overlap events"), NavComponent->Test_IsAnyHideTriggerListenerBound(TriggerB));

	return true;
}

#endif //# WITH_DEV_AUTOMATION_TESTS
