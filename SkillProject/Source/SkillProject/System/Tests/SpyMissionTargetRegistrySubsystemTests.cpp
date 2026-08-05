// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "System/SpyMissionTargetRegistrySubsystem.h"
#include "Util/SpyGameplayTags.h"

//# design §5-4 — 레지스트리는 액터를 약참조로만 저장하고 조회 시점에 GetActorLocation() 을
//# 읽는다. World 없이 만든 액터는 RootComponent 가 없으면 GetActorLocation() 이 항상
//# 원점을 반환하므로(code-reviewer BLOCKER), 좌표를 실제로 검증하는 테스트는 반드시
//# RootComponent 를 붙이고 SetActorLocation 으로 원점이 아닌 좌표를 준다.
static AActor* SpyMissionTargetRegistrySubsystemTests_MakeLocatedActor(const FVector& InLocation)
{
	AActor* Actor = NewObject<AActor>(GetTransientPackage());
	USceneComponent* Root = NewObject<USceneComponent>(Actor, TEXT("Root"));
	Actor->SetRootComponent(Root);
	Actor->SetActorLocation(InLocation);

	return Actor;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionTargetRegistryRegisterNPCFindsExactLocationTest,
	"SkillProject.System.MissionTargetRegistry.RegisterNPCFindsExactLocation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionTargetRegistryRegisterNPCFindsExactLocationTest::RunTest(const FString& Parameters)
{
	const FVector ExpectedLocation(100.f, 200.f, 300.f);
	AActor* NPCActor = SpyMissionTargetRegistrySubsystemTests_MakeLocatedActor(ExpectedLocation);

	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	Registry->RegisterNPCLocation(7, NPCActor);

	FVector OutLocation = FVector::ZeroVector;
	const bool bFound = Registry->FindNPCLocation(7, OutLocation);

	TestTrue(TEXT("FindNPCLocation succeeds for a registered key"), bFound);
	TestEqual(TEXT("Location exactly matches the actor's world location"), OutLocation, ExpectedLocation);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionTargetRegistryRegisterMissionTargetFindsExactLocationTest,
	"SkillProject.System.MissionTargetRegistry.RegisterMissionTargetFindsExactLocation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionTargetRegistryRegisterMissionTargetFindsExactLocationTest::RunTest(const FString& Parameters)
{
	const FVector ExpectedLocation(-450.f, 12.5f, 0.f);
	AActor* MarkerActor = SpyMissionTargetRegistrySubsystemTests_MakeLocatedActor(ExpectedLocation);

	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	Registry->RegisterMissionTargetLocation(SpyGameplayTags::Skill_Move_Vault, MarkerActor);

	FVector OutLocation = FVector::ZeroVector;
	const bool bFound = Registry->FindMissionTargetLocation(SpyGameplayTags::Skill_Move_Vault, OutLocation);

	TestTrue(TEXT("FindMissionTargetLocation succeeds for a registered tag"), bFound);
	TestEqual(TEXT("Location exactly matches the actor's world location"), OutLocation, ExpectedLocation);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionTargetRegistryFindUnknownNPCIdFailsTest,
	"SkillProject.System.MissionTargetRegistry.FindUnknownNPCIdFails",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionTargetRegistryFindUnknownNPCIdFailsTest::RunTest(const FString& Parameters)
{
	//# §5-1 원칙 3항의 메커니즘 — 등록된 액터가 없는 키는 조용히 조회 실패해야 한다
	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();

	FVector OutLocation = FVector::ZeroVector;
	const bool bFound = Registry->FindNPCLocation(9999, OutLocation);

	TestFalse(TEXT("Unregistered NPCId never resolves"), bFound);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionTargetRegistryFindUnknownTagFailsTest,
	"SkillProject.System.MissionTargetRegistry.FindUnknownTagFails",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionTargetRegistryFindUnknownTagFailsTest::RunTest(const FString& Parameters)
{
	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	Registry->RegisterMissionTargetLocation(SpyGameplayTags::Skill_Move_Vault, SpyMissionTargetRegistrySubsystemTests_MakeLocatedActor(FVector(1.f, 1.f, 1.f)));

	FVector OutLocation = FVector::ZeroVector;
	//# Kill/Level/Combo 처럼 어떤 액터도 등록하지 않은 태그는 다른 태그가 등록돼 있어도 실패해야 한다
	const bool bFound = Registry->FindMissionTargetLocation(SpyGameplayTags::Event_Mission_Kill, OutLocation);

	TestFalse(TEXT("An unrelated registered tag does not leak into an unregistered tag's lookup"), bFound);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionTargetRegistryFindInvalidTagFailsTest,
	"SkillProject.System.MissionTargetRegistry.FindInvalidTagFails",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionTargetRegistryFindInvalidTagFailsTest::RunTest(const FString& Parameters)
{
	//# design §5-2 게이트의 최하단 방어선 — 호출부가 유효성 체크를 빠뜨려도 레지스트리 자체가 안전하다
	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();

	FVector OutLocation = FVector::ZeroVector;
	const bool bFound = Registry->FindMissionTargetLocation(FGameplayTag(), OutLocation);

	TestFalse(TEXT("An invalid (default) tag never resolves"), bFound);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionTargetRegistryRegisterNullActorIsNoOpTest,
	"SkillProject.System.MissionTargetRegistry.RegisterNullActorIsNoOp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionTargetRegistryRegisterNullActorIsNoOpTest::RunTest(const FString& Parameters)
{
	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	Registry->RegisterNPCLocation(1, nullptr);
	Registry->RegisterMissionTargetLocation(SpyGameplayTags::Skill_Move_Vault, nullptr);

	FVector OutLocation = FVector::ZeroVector;
	TestFalse(TEXT("RegisterNPCLocation(nullptr) does not create an entry"), Registry->FindNPCLocation(1, OutLocation));
	TestFalse(TEXT("RegisterMissionTargetLocation(nullptr) does not create an entry"), Registry->FindMissionTargetLocation(SpyGameplayTags::Skill_Move_Vault, OutLocation));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionTargetRegistryRegisterInvalidTagIsNoOpTest,
	"SkillProject.System.MissionTargetRegistry.RegisterInvalidTagIsNoOp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionTargetRegistryRegisterInvalidTagIsNoOpTest::RunTest(const FString& Parameters)
{
	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	AActor* MarkerActor = SpyMissionTargetRegistrySubsystemTests_MakeLocatedActor(FVector(10.f, 20.f, 30.f));

	Registry->RegisterMissionTargetLocation(FGameplayTag(), MarkerActor);

	FVector OutLocation = FVector::ZeroVector;
	TestFalse(TEXT("RegisterMissionTargetLocation with an invalid tag does not create an entry"), Registry->FindMissionTargetLocation(FGameplayTag(), OutLocation));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionTargetRegistryUnregisterSameActorRemovesEntryTest,
	"SkillProject.System.MissionTargetRegistry.UnregisterSameActorRemovesEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionTargetRegistryUnregisterSameActorRemovesEntryTest::RunTest(const FString& Parameters)
{
	//# 실제 EndPlay 경로(NPCId 키 공간) — 등록한 바로 그 액터가 Unregister 하면 반드시 지워져야
	//# 한다. 아래 재등록 가드 테스트들은 전부 "지우지 않아야 하는" 케이스만 다루므로, 이 정상
	//# 해제 경로를 별도로 고정하지 않으면 Unregister 를 완전 no-op 으로 구현해도 스위트가 통과한다.
	AActor* NPCActor = SpyMissionTargetRegistrySubsystemTests_MakeLocatedActor(FVector(1.f, 2.f, 3.f));

	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	Registry->RegisterNPCLocation(11, NPCActor);

	FVector OutLocation = FVector::ZeroVector;
	TestTrue(TEXT("Find succeeds right after registration"), Registry->FindNPCLocation(11, OutLocation));

	Registry->UnregisterNPCLocation(11, NPCActor);

	TestFalse(TEXT("Find fails after the same actor unregisters"), Registry->FindNPCLocation(11, OutLocation));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionTargetRegistryUnregisterSameActorRemovesEntryMissionTagTest,
	"SkillProject.System.MissionTargetRegistry.UnregisterSameActorRemovesEntryMissionTag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionTargetRegistryUnregisterSameActorRemovesEntryMissionTagTest::RunTest(const FString& Parameters)
{
	//# 실제 EndPlay 경로(MatchTag 키 공간) — ASpyMissionTargetPoint/ASpyInteractableObject 가
	//# 이 정확한 경로를 탄다. 두 Unregister 함수는 서로 다른 TMap 을 다루는 별도 구현이라
	//# NPCId 키 공간과 별개로 고정해야 한다.
	AActor* MarkerActor = SpyMissionTargetRegistrySubsystemTests_MakeLocatedActor(FVector(4.f, 5.f, 6.f));

	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	Registry->RegisterMissionTargetLocation(SpyGameplayTags::Skill_Move_HangUp, MarkerActor);

	FVector OutLocation = FVector::ZeroVector;
	TestTrue(TEXT("Find succeeds right after registration"), Registry->FindMissionTargetLocation(SpyGameplayTags::Skill_Move_HangUp, OutLocation));

	Registry->UnregisterMissionTargetLocation(SpyGameplayTags::Skill_Move_HangUp, MarkerActor);

	TestFalse(TEXT("Find fails after the same actor unregisters"), Registry->FindMissionTargetLocation(SpyGameplayTags::Skill_Move_HangUp, OutLocation));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionTargetRegistryFixtureLocatedActorSelfCheckTest,
	"SkillProject.System.MissionTargetRegistry.FixtureLocatedActorSelfCheck",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionTargetRegistryFixtureLocatedActorSelfCheckTest::RunTest(const FString& Parameters)
{
	//# 자가진단 — 이 파일의 나머지 테스트는 전부 "World 없이 만든 액터도 RootComponent 를
	//# 붙이면 SetActorLocation 이 실제로 반영된다"는 가정에 기댄다. 이 가정이 틀리면 나머지
	//# 테스트들은 전부 "(777,-888,999) 기대, (0,0,0) 관측"처럼 레지스트리가 좌표를 무시하는
	//# 것과 똑같은 실패 모양으로 보인다 — 이 테스트가 먼저 깨지면 원인이 픽스처임을 바로 안다.
	const FVector ExpectedLocation(777.f, -888.f, 999.f);
	AActor* Actor = SpyMissionTargetRegistrySubsystemTests_MakeLocatedActor(ExpectedLocation);

	TestEqual(TEXT("SetActorLocation on a NewObject'd actor with a RootComponent takes effect without a World"), Actor->GetActorLocation(), ExpectedLocation);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionTargetRegistryReRegisterSameKeyLatestWinsTest,
	"SkillProject.System.MissionTargetRegistry.ReRegisterSameKeyLatestWins",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionTargetRegistryReRegisterSameKeyLatestWinsTest::RunTest(const FString& Parameters)
{
	//# 같은 키로 두 번 등록 — TMap::Add 의미대로 나중 등록이 앞선 등록을 덮어써야 한다
	const FVector FirstLocation(1.f, 1.f, 1.f);
	const FVector SecondLocation(2.f, 2.f, 2.f);

	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	Registry->RegisterNPCLocation(5, SpyMissionTargetRegistrySubsystemTests_MakeLocatedActor(FirstLocation));
	Registry->RegisterNPCLocation(5, SpyMissionTargetRegistrySubsystemTests_MakeLocatedActor(SecondLocation));

	FVector OutLocation = FVector::ZeroVector;
	const bool bFound = Registry->FindNPCLocation(5, OutLocation);

	TestTrue(TEXT("Find succeeds after re-registration"), bFound);
	TestEqual(TEXT("The second registration wins"), OutLocation, SecondLocation);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionTargetRegistryUnregisterMismatchedActorIsNoOpTest,
	"SkillProject.System.MissionTargetRegistry.UnregisterMismatchedActorIsNoOp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionTargetRegistryUnregisterMismatchedActorIsNoOpTest::RunTest(const FString& Parameters)
{
	//# code-reviewer 확인 회귀 고정 — "재등록 가드". A가 등록한 뒤 B가 같은 키로 재등록하고,
	//# (늦게 실행되는) A의 EndPlay가 그제서야 Unregister(key, A) 를 호출해도 B의 등록을 지우면 안 된다.
	const FVector LocationA(10.f, 0.f, 0.f);
	const FVector LocationB(20.f, 0.f, 0.f);

	AActor* ActorA = SpyMissionTargetRegistrySubsystemTests_MakeLocatedActor(LocationA);
	AActor* ActorB = SpyMissionTargetRegistrySubsystemTests_MakeLocatedActor(LocationB);

	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	Registry->RegisterNPCLocation(3, ActorA);
	Registry->RegisterNPCLocation(3, ActorB); //# B가 같은 키로 재등록 (A의 EndPlay보다 먼저 실행됐다고 가정)

	Registry->UnregisterNPCLocation(3, ActorA); //# 뒤늦게 실행된 A의 EndPlay — B의 등록을 건드리면 안 된다

	FVector OutLocation = FVector::ZeroVector;
	const bool bFound = Registry->FindNPCLocation(3, OutLocation);

	TestTrue(TEXT("B's registration survives A's stale Unregister call"), bFound);
	TestEqual(TEXT("Location is still B's"), OutLocation, LocationB);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionTargetRegistryUnregisterMismatchedActorIsNoOpMissionTagTest,
	"SkillProject.System.MissionTargetRegistry.UnregisterMismatchedActorIsNoOpMissionTag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionTargetRegistryUnregisterMismatchedActorIsNoOpMissionTagTest::RunTest(const FString& Parameters)
{
	//# 위와 동일한 재등록 가드를 MatchTag 키 공간(UnregisterMissionTargetLocation)에서도 고정한다 —
	//# 두 Unregister 함수는 서로 다른 TMap 을 다루는 별도 구현이라 각각 회귀 테스트가 필요하다
	const FVector LocationA(0.f, 10.f, 0.f);
	const FVector LocationB(0.f, 20.f, 0.f);

	AActor* ActorA = SpyMissionTargetRegistrySubsystemTests_MakeLocatedActor(LocationA);
	AActor* ActorB = SpyMissionTargetRegistrySubsystemTests_MakeLocatedActor(LocationB);

	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	Registry->RegisterMissionTargetLocation(SpyGameplayTags::Skill_Move_Climb, ActorA);
	Registry->RegisterMissionTargetLocation(SpyGameplayTags::Skill_Move_Climb, ActorB);

	Registry->UnregisterMissionTargetLocation(SpyGameplayTags::Skill_Move_Climb, ActorA);

	FVector OutLocation = FVector::ZeroVector;
	const bool bFound = Registry->FindMissionTargetLocation(SpyGameplayTags::Skill_Move_Climb, OutLocation);

	TestTrue(TEXT("B's registration survives A's stale Unregister call"), bFound);
	TestEqual(TEXT("Location is still B's"), OutLocation, LocationB);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionTargetRegistryUnregisterNeverRegisteredIsSafeTest,
	"SkillProject.System.MissionTargetRegistry.UnregisterNeverRegisteredIsSafe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionTargetRegistryUnregisterNeverRegisteredIsSafeTest::RunTest(const FString& Parameters)
{
	//# 등록된 적 없는 키를 Unregister 해도 크래시·예외 없이 안전해야 한다 (EndPlay 가 BeginPlay
	//# 등록 실패 뒤에도 항상 호출되는 액터 라이프사이클과 맞물린다)
	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	AActor* NeverRegisteredActor = SpyMissionTargetRegistrySubsystemTests_MakeLocatedActor(FVector::ZeroVector);

	Registry->UnregisterNPCLocation(42, NeverRegisteredActor);
	Registry->UnregisterMissionTargetLocation(SpyGameplayTags::Skill_Move_GrappleHook, NeverRegisteredActor);

	FVector OutLocation = FVector::ZeroVector;
	TestFalse(TEXT("Still no entry for the NPCId key"), Registry->FindNPCLocation(42, OutLocation));
	TestFalse(TEXT("Still no entry for the tag key"), Registry->FindMissionTargetLocation(SpyGameplayTags::Skill_Move_GrappleHook, OutLocation));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionTargetRegistryStaleActorFailsFindTest,
	"SkillProject.System.MissionTargetRegistry.StaleActorFailsFind",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionTargetRegistryStaleActorFailsFindTest::RunTest(const FString& Parameters)
{
	//# 리소스 재사용 후 상태 잔존 엣지케이스 — 액터가 파괴되면(약참조가 무효화되면) 등록을
	//# 명시적으로 해제하지 않아도 조회가 안전하게 실패해야 한다(TWeakObjectPtr::IsValid())
	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	AActor* MarkerActor = SpyMissionTargetRegistrySubsystemTests_MakeLocatedActor(FVector(5.f, 5.f, 5.f));
	Registry->RegisterMissionTargetLocation(SpyGameplayTags::Skill_Move_Vault, MarkerActor);

	FVector PreDestroyLocation = FVector::ZeroVector;
	TestTrue(TEXT("Find succeeds while the actor is alive"), Registry->FindMissionTargetLocation(SpyGameplayTags::Skill_Move_Vault, PreDestroyLocation));

	MarkerActor->MarkAsGarbage();

	FVector OutLocation = FVector::ZeroVector;
	const bool bFound = Registry->FindMissionTargetLocation(SpyGameplayTags::Skill_Move_Vault, OutLocation);

	TestFalse(TEXT("Find fails once the registered actor is garbage — no dangling read"), bFound);

	return true;
}

#endif //# WITH_DEV_AUTOMATION_TESTS
