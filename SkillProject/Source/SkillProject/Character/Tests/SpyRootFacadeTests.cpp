// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Character/CommonInterface.Character.h"
#include "Character/SpyCharacter.h"
#include "ManagerComponent/CommonInterface.Manager.h"
#include "ManagerComponent/SpyParkourManagerComponent.h"
#include "ManagerComponent/SpyTargetingManagerComponent.h"
#include "ManagerComponent/SpyGrappleTargetingComponent.h"

//# ---------------------------------------------------------------------------
//# §13 루트 파사드 규약 스위트.
//# 하위 컴포넌트가 인터페이스를 구현하고, 루트가 조립 시점에 핸들을 채우는지 박제한다.
//# ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyRootFacadeComponentsImplementInterfacesTest,
	"SkillProject.Character.RootFacade.ComponentsImplementInterfaces",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyRootFacadeComponentsImplementInterfacesTest::RunTest(const FString& Parameters)
{
	USpyParkourManagerComponent* Parkour = NewObject<USpyParkourManagerComponent>();
	USpyTargetingManagerComponent* Targeting = NewObject<USpyTargetingManagerComponent>();
	USpyGrappleTargetingComponent* Grapple = NewObject<USpyGrappleTargetingComponent>();

	TestTrue(TEXT("Parkour 컴포넌트가 ISpyParkourHost 를 구현한다"),
		Parkour->GetClass()->ImplementsInterface(USpyParkourHost::StaticClass()));
	TestTrue(TEXT("Targeting 컴포넌트가 ISpyTargetProvider 를 구현한다"),
		Targeting->GetClass()->ImplementsInterface(USpyTargetProvider::StaticClass()));
	TestTrue(TEXT("Grapple 컴포넌트가 ISpyGrappleHost 를 구현한다"),
		Grapple->GetClass()->ImplementsInterface(USpyGrappleHost::StaticClass()));

	//# TScriptInterface 로 감싼 뒤에도 호출이 도달하는지 — 캐스팅 경로 검증
	TScriptInterface<ISpyTargetProvider> Provider(Targeting);
	TestNotNull(TEXT("TScriptInterface 가 인터페이스 포인터를 잡는다"), Provider.GetInterface());
	TestFalse(TEXT("타깃 미설정 시 IsTargetValid 는 false"), Provider->IsTargetValid());

	//# 델리게이트 접근자가 사본이 아니라 실제 멤버의 참조를 돌려주는지 박제 —
	//# 어긋나면 구독이 조용히 유실되고 컴파일은 통과한다.
	TScriptInterface<ISpyParkourHost> ParkourHost(Parkour);
	TestNotNull(TEXT("ParkourHost TScriptInterface 가 인터페이스 포인터를 잡는다"), ParkourHost.GetInterface());
	if (ParkourHost.GetInterface() == nullptr)
		return false;

	TestTrue(TEXT("OnVaultMotionWarping 접근자가 실제 멤버 참조를 반환한다"),
		static_cast<void*>(&ParkourHost->OnVaultMotionWarping()) == static_cast<void*>(&Parkour->OnVaultMotionWarpingData));
	TestTrue(TEXT("OnHangUpMotionWarping 접근자가 실제 멤버 참조를 반환한다"),
		static_cast<void*>(&ParkourHost->OnHangUpMotionWarping()) == static_cast<void*>(&Parkour->OnHangUpMotionWarpingData));
	TestTrue(TEXT("OnClimb 접근자가 실제 멤버 참조를 반환한다"),
		static_cast<void*>(&ParkourHost->OnClimb()) == static_cast<void*>(&Parkour->OnClimbData));

	TScriptInterface<ISpyGrappleHost> GrappleHost(Grapple);
	TestNotNull(TEXT("GrappleHost TScriptInterface 가 인터페이스 포인터를 잡는다"), GrappleHost.GetInterface());
	if (GrappleHost.GetInterface() == nullptr)
		return false;

	TestTrue(TEXT("OnGrappleTargetChanged 접근자가 실제 멤버 참조를 반환한다"),
		static_cast<void*>(&GrappleHost->OnGrappleTargetChanged()) == static_cast<void*>(&Grapple->OnGrappleTargetChangedDelegate));

	return true;
}

//# ---------------------------------------------------------------------------
//# 루트 조립 — AssembleComponents 가 하위 컴포넌트 핸들을 채우는지 박제한다.
//# 실제 InitState 흐름은 월드가 필요하므로 조립 함수의 계약만 검증한다.
//# ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyRootFacadeAssemblyFillsHandlesTest,
	"SkillProject.Character.RootFacade.AssemblyFillsHandles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyRootFacadeAssemblyFillsHandlesTest::RunTest(const FString& Parameters)
{
	ASpyCharacter* Character = NewObject<ASpyCharacter>();

	//# NewObject 로 만든 컴포넌트는 Outer 의 OwnedComponents 에 등록되므로
	//# FindComponentByClass 로 잡힌다 (SpyCharacterAIRotationTests.cpp:58 과 동일한 근거).
	USpyTargetingManagerComponent* Targeting = NewObject<USpyTargetingManagerComponent>(Character);
	USpyParkourManagerComponent* Parkour = NewObject<USpyParkourManagerComponent>(Character);

	TScriptInterface<ISpyCharacterRoot> Root(Character);
	TestNotNull(TEXT("ASpyCharacter 가 ISpyCharacterRoot 를 구현한다"), Root.GetInterface());

	//# 조립 전 — 핸들이 비어 있다
	TestNull(TEXT("조립 전 TargetProvider 핸들은 비어 있다"), Root->GetTargetProvider().GetInterface());

	Character->AssembleComponents();

	//# 포인터 비교는 TestTrue 로 — TestEqual 은 값 타입에 문자열 변환을 요구하며
	//# UObject* 에는 없다 (SpyCharacterAIRotationTests.cpp:96-97, Task 1 static_cast 비교와 동일 패턴).
	TestTrue(TEXT("조립 후 TargetProvider 핸들이 실제 컴포넌트를 가리킨다"),
		Root->GetTargetProvider().GetObject() == Cast<UObject>(Targeting));
	TestTrue(TEXT("조립 후 ParkourHost 핸들이 실제 컴포넌트를 가리킨다"),
		Root->GetParkourHost().GetObject() == Cast<UObject>(Parkour));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
