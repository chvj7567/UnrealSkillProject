// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"
#include "GameplayTagContainer.h"
#include "Interactable/SpyInteractableObject.h"
#include "Interactable/CommonInterface.Interactable.h"

//# ─────────────────────────────────────────────────────────────────────────────
//# ASpyInteractableObject 는 HasAuthority()/실제 물리 오버랩이 필요한 RequestInteract·
//# IsPawnInRange(비-null 경로)·오버랩 콜백은 Automation 대상이 아니다(spec §8 "컴포넌트/
//# 액터 레벨은 Automation 커버 불가"). 아래는 그 예외 — NewObject 로 액터를 만들되 스폰/
//# BeginPlay 는 하지 않고 생성자 시점 기본값과 월드 없이도 안전한 가드 절만 검증한다
//# (Character/Tests/SpyRootFacadeTests.cpp:78 과 동일한 근거, cpp-style §13).
//# ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyInteractableObjectCppDefaultsTest,
	"SkillProject.Interactable.Object.CppDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyInteractableObjectCppDefaultsTest::RunTest(const FString& Parameters)
{
	ASpyInteractableObject* Interactable = NewObject<ASpyInteractableObject>();

	TestTrue(TEXT("ASpyInteractableObject implements ISpyInteractableRoot"),
			 Interactable->GetClass()->ImplementsInterface(USpyInteractableRoot::StaticClass()));

	//# §4-1 콘텐츠 결정 — 코드 기본값이 "조사하기"에서 벗어나면 이 테스트가 알려준다
	TestTrue(TEXT("Default InteractVerb is 조사하기"), Interactable->GetInteractVerb().ToString() == TEXT("조사하기"));

	//# §8 조건표 #1 — MissionEventTag 는 C++ 기본값이 없다(에디터에서 인스턴스별로 반드시 지정).
	//# protected 필드라 리플렉션으로 읽는다 — production 에 테스트 전용 getter 를 추가하지 않는다
	const FStructProperty* TagProperty = FindFProperty<FStructProperty>(ASpyInteractableObject::StaticClass(), TEXT("MissionEventTag"));
	if (TagProperty == nullptr)
	{
		AddError(TEXT("MissionEventTag property not found via reflection — field renamed?"));

		return false;
	}

	const FGameplayTag* TagValue = TagProperty->ContainerPtrToValuePtr<FGameplayTag>(Interactable);
	TestFalse(TEXT("MissionEventTag has no C++ default — must be set per-instance in the editor"), TagValue->IsValid());

	//# ISpyInteractableRoot 계약 — null 요청자는 월드/물리 없이도 안전하게 false (가드 절)
	TestFalse(TEXT("IsPawnInRange(nullptr) is a safe guard clause, no world needed"), Interactable->IsPawnInRange(nullptr));

	return true;
}

#endif //# WITH_DEV_AUTOMATION_TESTS
