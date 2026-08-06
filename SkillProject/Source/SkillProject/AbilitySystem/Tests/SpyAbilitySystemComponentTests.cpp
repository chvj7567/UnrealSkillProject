// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"

//# 콤보 실행(윈도우) 하나당 미션 진행 카운트를 정확히 1회로 묶는 서버 전용 플래그.
//# 링크가 몇 번 이어지든 실행 경계(ResetComboMissionCounted) 전까지는 재카운트되지 않는다.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyAbilitySystemComponentComboMissionCountTest,
	"SkillProject.AbilitySystem.SpyAbilitySystemComponent.ComboMissionCountOncePerExecution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyAbilitySystemComponentComboMissionCountTest::RunTest(const FString& Parameters)
{
	USpyAbilitySystemComponent* ASC = NewObject<USpyAbilitySystemComponent>(GetTransientPackage());

	//# 정상 케이스 — 같은 실행 안에서는 최초 1회만 카운트를 허용한다 (링크 2번째 이후는 차단)
	const bool bFirstLink = ASC->TryMarkComboMissionCounted();
	TestTrue(TEXT("First link in an execution should count"), bFirstLink);

	const bool bSecondLink = ASC->TryMarkComboMissionCounted();
	TestFalse(TEXT("Second link in the same execution should not count again"), bSecondLink);

	const bool bThirdLink = ASC->TryMarkComboMissionCounted();
	TestFalse(TEXT("Third link in the same execution should not count again"), bThirdLink);

	//# 엣지 케이스 — 실행 경계(리셋) 이후에는 새 실행으로 취급해 다시 1회 카운트를 허용한다
	ASC->ResetComboMissionCounted();

	const bool bNextExecutionFirstLink = ASC->TryMarkComboMissionCounted();
	TestTrue(TEXT("First link of a new execution after reset should count"), bNextExecutionFirstLink);

	return true;
}

#endif //WITH_DEV_AUTOMATION_TESTS
