// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"
#include "NPC/SpyNPCCharacter.h"
#include "System/CommonInterface.System.h"

//# 물리·월드가 필요한 오버랩 경로는 대상 밖이다(SpyInteractableObjectTests.cpp:12 와 동일 근거) —
//# 생성자 시점 인터페이스 계약과 bEnableHideTrigger 토글만 검증한다.

static void SpyNPCCharacterHideVolumeTests_SetHideTriggerEnabled(ASpyNPCCharacter* NPC, bool bEnabled)
{
	FBoolProperty* Prop = FindFProperty<FBoolProperty>(ASpyNPCCharacter::StaticClass(), TEXT("bEnableHideTrigger"));
	check(Prop != nullptr);
	Prop->SetPropertyValue_InContainer(NPC, bEnabled);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNPCCharacterImplementsHideVolumeTest,
	"SkillProject.NPC.Character.ImplementsHideVolume",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNPCCharacterImplementsHideVolumeTest::RunTest(const FString& Parameters)
{
	ASpyNPCCharacter* NPC = NewObject<ASpyNPCCharacter>();

	TestTrue(TEXT("ASpyNPCCharacter implements ISpyMissionTargetHideVolume"),
			 NPC->GetClass()->ImplementsInterface(USpyMissionTargetHideVolume::StaticClass()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNPCCharacterHideTriggerEnabledByDefaultTest,
	"SkillProject.NPC.Character.HideTriggerEnabledByDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNPCCharacterHideTriggerEnabledByDefaultTest::RunTest(const FString& Parameters)
{
	//# 사용자 요청(2026-08-10) — bEnableHideTrigger 기본값이 true 로 바뀌어 모든 신규 NPC가
	//# 별도 설정 없이도 GetHideTriggerComponent() 가 유효 컴포넌트를 반환해야 한다
	ASpyNPCCharacter* NPC = NewObject<ASpyNPCCharacter>();
	ISpyMissionTargetHideVolume* HideVolume = Cast<ISpyMissionTargetHideVolume>(NPC);

	TestNotNull(TEXT("Cast to the interface succeeds"), HideVolume);
	TestNotNull(TEXT("GetHideTriggerComponent() returns a valid component by default (bEnableHideTrigger defaults to true)"), HideVolume->GetHideTriggerComponent());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNPCCharacterHideTriggerEnabledReturnsComponentTest,
	"SkillProject.NPC.Character.HideTriggerEnabledReturnsComponent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNPCCharacterHideTriggerEnabledReturnsComponentTest::RunTest(const FString& Parameters)
{
	ASpyNPCCharacter* NPC = NewObject<ASpyNPCCharacter>();
	SpyNPCCharacterHideVolumeTests_SetHideTriggerEnabled(NPC, true);

	ISpyMissionTargetHideVolume* HideVolume = Cast<ISpyMissionTargetHideVolume>(NPC);
	TestNotNull(TEXT("Cast to the interface succeeds"), HideVolume);
	TestNotNull(TEXT("GetHideTriggerComponent() returns a valid component when bEnableHideTrigger is true"), HideVolume->GetHideTriggerComponent());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNPCCharacterHideTriggerExplicitlyDisabledReturnsNullTest,
	"SkillProject.NPC.Character.HideTriggerExplicitlyDisabledReturnsNull",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNPCCharacterHideTriggerExplicitlyDisabledReturnsNullTest::RunTest(const FString& Parameters)
{
	//# 기본값이 true 로 바뀐 뒤에도 디자이너가 인스턴스별로 false 를 켜면 여전히 거리
	//# 히스테리시스 폴백 경로(nullptr)를 타야 한다 — opt-out 경로 자체가 죽지 않았는지 고정
	ASpyNPCCharacter* NPC = NewObject<ASpyNPCCharacter>();
	SpyNPCCharacterHideVolumeTests_SetHideTriggerEnabled(NPC, false);

	ISpyMissionTargetHideVolume* HideVolume = Cast<ISpyMissionTargetHideVolume>(NPC);
	TestNotNull(TEXT("Cast to the interface succeeds"), HideVolume);
	TestNull(TEXT("GetHideTriggerComponent() is nullptr when bEnableHideTrigger is explicitly set to false"), HideVolume->GetHideTriggerComponent());

	return true;
}

#endif //# WITH_DEV_AUTOMATION_TESTS
