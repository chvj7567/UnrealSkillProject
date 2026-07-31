// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ManagerComponent/CommonInterface.h"
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

#endif // WITH_DEV_AUTOMATION_TESTS
