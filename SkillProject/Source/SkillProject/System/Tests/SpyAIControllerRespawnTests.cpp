// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Perception/AIPerceptionComponent.h"
#include "UObject/UnrealType.h"
#include "Data/SpyAIConfig.h"
#include "System/SpyAIController.h"

//# ---------------------------------------------------------------------------
//# ASpyAIController — 디졸브 시간 게터 + AI 로직 재시작 API 스위트 (plan Task 4, §9 항목 5/6).
//#
//# 범위 노트 — RestartAILogic() 이 실제로 BT 를 재개시키는지(GetBrainComponent()->IsRunning())는
//# 엔진 RunBehaviorTree() 가 내부에서 컴포넌트 생성/등록을 하기 때문에 월드 없이 안전을
//# 확신할 수 없어 재현하지 않는다. BehaviorTreeAsset 이 비어 있을 때 크래시 없이 완료되는지만
//# 고정한다(안전 가드용 최소 회귀).
//# ---------------------------------------------------------------------------

namespace SpyAIControllerRespawnTests {
//# AIConfig 는 private UPROPERTY 라 프로덕션에 테스트 전용 setter 를 추가하지 않기 위해
//# 리플렉션으로 직접 주입한다 (System/Tests/SpyMissionComponentTests.cpp 의 기존 패턴과 동일).
static void SetAIConfig(ASpyAIController* Controller, USpyAIConfig* Config)
{
	FObjectProperty* Prop = FindFProperty<FObjectProperty>(ASpyAIController::StaticClass(), TEXT("AIConfig"));
	check(Prop != nullptr);
	Prop->SetObjectPropertyValue_InContainer(Controller, Config);
}
} //namespace SpyAIControllerRespawnTests

//# ---------------------------------------------------------------------------
//# 회귀 — AIConfig 미지정 컨트롤러는 기획서 §2-1 확정값(2.0초)과 동일한 코드 기본값으로 폴백한다.
//# ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyAIControllerDissolveDurationDefaultTest,
	"SkillProject.System.AIController.DissolveDurationDefaultsToDesignValue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyAIControllerDissolveDurationDefaultTest::RunTest(const FString& Parameters)
{
	ASpyAIController* Controller = NewObject<ASpyAIController>(GetTransientPackage());

	TestEqual(TEXT("Unconfigured controller falls back to the design default"),
			  Controller->GetDissolveDurationSeconds(), 2.0f);

	return true;
}

//# ---------------------------------------------------------------------------
//# 정상 — AIConfig 가 있으면 그 값을 읽는다. USpyAIConfig 자체의 코드 기본값도 함께 고정한다.
//# ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyAIControllerDissolveDurationFromConfigTest,
	"SkillProject.System.AIController.DissolveDurationReadsConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyAIControllerDissolveDurationFromConfigTest::RunTest(const FString& Parameters)
{
	ASpyAIController* Controller = NewObject<ASpyAIController>(GetTransientPackage());

	USpyAIConfig* Config = NewObject<USpyAIConfig>();
	TestEqual(TEXT("USpyAIConfig code default matches the design value (§2-1)"), Config->DissolveDurationSeconds, 2.0f);

	Config->DissolveDurationSeconds = 5.5f;
	SpyAIControllerRespawnTests::SetAIConfig(Controller, Config);

	TestEqual(TEXT("Configured controller reads AIConfig's value instead of the fallback"),
			  Controller->GetDissolveDurationSeconds(), 5.5f);

	return true;
}

//# ---------------------------------------------------------------------------
//# 엣지 — BehaviorTreeAsset 미설정 상태에서 RestartAILogic 은 RunBehaviorTree 분기를
//# 건너뛰고 크래시 없이 완료된다. 이 테스트가 끝난다는 사실 자체가 가드 동작의 증거다
//# (Character/Tests/SpyLevelTests.cpp의 FSpyLevelZeroInCurveTest 와 동일한 패턴).
//# ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyAIControllerRestartAILogicWithoutBehaviorTreeTest,
	"SkillProject.System.AIController.RestartAILogicSafeWithoutBehaviorTree",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyAIControllerRestartAILogicWithoutBehaviorTreeTest::RunTest(const FString& Parameters)
{
	ASpyAIController* Controller = NewObject<ASpyAIController>(GetTransientPackage());

	TestTrue(TEXT("Fixture has a perception component before the call"),
			 IsValid(Controller->GetAIPerceptionComponent()));

	Controller->RestartAILogic();

	//# RequestStimuliListenerUpdate 호출 이후에도 퍼셉션 컴포넌트가 살아있는지 — 완료 자체가
	//# RunBehaviorTree 분기를 건드리지 않았다는 증거이지만, 최소한 관찰 가능한 상태를 남긴다.
	TestTrue(TEXT("Perception component survives RestartAILogic without a behavior tree"),
			 IsValid(Controller->GetAIPerceptionComponent()));

	return true;
}

#endif //# WITH_DEV_AUTOMATION_TESTS
