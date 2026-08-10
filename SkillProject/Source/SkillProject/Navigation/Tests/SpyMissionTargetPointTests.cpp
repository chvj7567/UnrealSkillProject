// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"
#include "Navigation/SpyMissionTargetPoint.h"
#include "System/CommonInterface.System.h"

//# ASpyMissionTargetPoint 는 오버랩 콜백이 없는 순수 마커 액터라 NewObject 생성자 시점 기본값만으로
//# 인터페이스 계약을 안전하게 검증할 수 있다(SpyInteractableObjectTests.cpp:26 과 동일 근거).

static void SpyMissionTargetPointTests_SetHideTriggerEnabled(ASpyMissionTargetPoint* Target, bool bEnabled)
{
	FBoolProperty* Prop = FindFProperty<FBoolProperty>(ASpyMissionTargetPoint::StaticClass(), TEXT("bEnableHideTrigger"));
	check(Prop != nullptr);
	Prop->SetPropertyValue_InContainer(Target, bEnabled);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionTargetPointImplementsHideVolumeTest,
	"SkillProject.Navigation.MissionTargetPoint.ImplementsHideVolume",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionTargetPointImplementsHideVolumeTest::RunTest(const FString& Parameters)
{
	ASpyMissionTargetPoint* Target = NewObject<ASpyMissionTargetPoint>();

	TestTrue(TEXT("ASpyMissionTargetPoint implements ISpyMissionTargetHideVolume"),
			 Target->GetClass()->ImplementsInterface(USpyMissionTargetHideVolume::StaticClass()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionTargetPointHideTriggerEnabledByDefaultTest,
	"SkillProject.Navigation.MissionTargetPoint.HideTriggerEnabledByDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionTargetPointHideTriggerEnabledByDefaultTest::RunTest(const FString& Parameters)
{
	//# 사용자 요청(2026-08-10) — bEnableHideTrigger 기본값이 true 로 바뀌어 모든 신규 마커가
	//# 별도 설정 없이도 GetHideTriggerComponent() 가 유효 컴포넌트를 반환해야 한다
	ASpyMissionTargetPoint* Target = NewObject<ASpyMissionTargetPoint>();
	ISpyMissionTargetHideVolume* HideVolume = Cast<ISpyMissionTargetHideVolume>(Target);

	TestNotNull(TEXT("Cast to the interface succeeds"), HideVolume);
	TestNotNull(TEXT("GetHideTriggerComponent() returns a valid component by default (bEnableHideTrigger defaults to true)"), HideVolume->GetHideTriggerComponent());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionTargetPointHideTriggerEnabledReturnsComponentTest,
	"SkillProject.Navigation.MissionTargetPoint.HideTriggerEnabledReturnsComponent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionTargetPointHideTriggerEnabledReturnsComponentTest::RunTest(const FString& Parameters)
{
	ASpyMissionTargetPoint* Target = NewObject<ASpyMissionTargetPoint>();
	SpyMissionTargetPointTests_SetHideTriggerEnabled(Target, true);

	ISpyMissionTargetHideVolume* HideVolume = Cast<ISpyMissionTargetHideVolume>(Target);
	TestNotNull(TEXT("Cast to the interface succeeds"), HideVolume);
	TestNotNull(TEXT("GetHideTriggerComponent() returns a valid component when bEnableHideTrigger is true"), HideVolume->GetHideTriggerComponent());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionTargetPointHideTriggerExplicitlyDisabledReturnsNullTest,
	"SkillProject.Navigation.MissionTargetPoint.HideTriggerExplicitlyDisabledReturnsNull",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionTargetPointHideTriggerExplicitlyDisabledReturnsNullTest::RunTest(const FString& Parameters)
{
	//# 기본값이 true 로 바뀐 뒤에도 디자이너가 인스턴스별로 false 를 켜면 여전히 거리
	//# 히스테리시스 폴백 경로(nullptr)를 타야 한다 — opt-out 경로 자체가 죽지 않았는지 고정
	ASpyMissionTargetPoint* Target = NewObject<ASpyMissionTargetPoint>();
	SpyMissionTargetPointTests_SetHideTriggerEnabled(Target, false);

	ISpyMissionTargetHideVolume* HideVolume = Cast<ISpyMissionTargetHideVolume>(Target);
	TestNotNull(TEXT("Cast to the interface succeeds"), HideVolume);
	TestNull(TEXT("GetHideTriggerComponent() is nullptr when bEnableHideTrigger is explicitly set to false"), HideVolume->GetHideTriggerComponent());

	return true;
}

#endif //# WITH_DEV_AUTOMATION_TESTS
