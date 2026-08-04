// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "System/SpyNavPathMath.h"

//# USpyNavigationComponent::ApplyPathPoints(ManagerComponent/SpyNavigationComponent.cpp)는
//# protected non-UFUNCTION 멤버라 월드 없이 단위 테스트할 수 없다(PathSpline/USplineMeshComponent
//# 는 BeginPlay 없이 생성되지 않는다). 대신 ApplyPathPoints 가 실제로 조합하는 순서
//# (ComputePathLength -> EvaluateHysteresisVisibility -> 조건부 TrimLeadingDistance) 를
//# 테스트가 그대로 재현해 이 "조합" 자체를 회귀 고정한다 — production 에 새 함수를 추가하는
//# 게 아니라 테스트가 ApplyPathPoints 의 로직을 미러링하는 것이다. 이 조합부는 개발 중
//# 두 번 회귀했다(plan 4차 개정 트림 순서 버그 / code-reviewer 1차검토 콜드 스타트 미표시
//# 버그) — 개별 순수 함수는 각각 정상인데 호출부 조합만 틀리는 계열의 결함 재발 위험이 가장
//# 큰 지점이다.

struct FSpyNavPathVisibilityMirrorResult
{
	bool bVisible = false;
	TArray<FVector> TrimmedPoints;
};

//# ApplyPathPoints(SpyNavigationComponent.cpp) 를 순서 그대로 미러링한다:
//# 1) 원본(트림 전) 길이로 히스테리시스 판정 2) 안 보이면 조기 반환 3) 보이면 조건부 트리밍
static FSpyNavPathVisibilityMirrorResult SpyNavPathVisibilityIntegrationTests_ApplyPathPointsMirror(
	const TArray<FVector>& InPathPoints,
	float HideThresholdCm,
	float ReshowThresholdCm,
	float StartOffsetDistanceCm,
	bool bPreviouslyVisible)
{
	FSpyNavPathVisibilityMirrorResult Result;

	const float RemainingPathLength = SpyNavPathMath::ComputePathLength(InPathPoints);
	Result.bVisible = SpyNavPathMath::EvaluateHysteresisVisibility(RemainingPathLength, HideThresholdCm, ReshowThresholdCm, bPreviouslyVisible);

	if (Result.bVisible == false)
		return Result;

	const float TrimDistanceCm = (RemainingPathLength >= ReshowThresholdCm) ? StartOffsetDistanceCm : 0.f;
	Result.TrimmedPoints = SpyNavPathMath::TrimLeadingDistance(InPathPoints, TrimDistanceCm);

	return Result;
}

//# design §4-3 확정값 그대로 (SpyNavigationComponent.h EditDefaultsOnly 기본값과 동일)
static constexpr float SpyNavPathVisibilityIntegrationTests_HideThresholdCm = 300.f;
static constexpr float SpyNavPathVisibilityIntegrationTests_ReshowThresholdCm = 400.f;
static constexpr float SpyNavPathVisibilityIntegrationTests_StartOffsetDistanceCm = 100.f;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavPathVisibilityColdStartBandTest,
	"SkillProject.Navigation.Math.ApplyPathPointsColdStartBand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavPathVisibilityColdStartBandTest::RunTest(const FString& Parameters)
{
	//# 원본 390cm(300~400 밴드 안, 콜드 스타트=이전에 보이던 상태로 시드) — §4-3 규칙1
	//# 두번째 문장(보이던 상태 유지)이 적용돼 visible=true, 트림량 0(규칙2: 400 미만).
	//# code-reviewer 1차검토가 발견한 콜드 스타트 버그의 정확한 재현 시나리오.
	TArray<FVector> PathPoints;
	PathPoints.Add(FVector::ZeroVector);
	PathPoints.Add(FVector(390.f, 0.f, 0.f));

	const FSpyNavPathVisibilityMirrorResult Result = SpyNavPathVisibilityIntegrationTests_ApplyPathPointsMirror(
		PathPoints,
		SpyNavPathVisibilityIntegrationTests_HideThresholdCm,
		SpyNavPathVisibilityIntegrationTests_ReshowThresholdCm,
		SpyNavPathVisibilityIntegrationTests_StartOffsetDistanceCm,
		/*bPreviouslyVisible=*/true);

	TestTrue(TEXT("390cm + cold-start-seeded visible stays visible (in-band)"), Result.bVisible);
	TestEqual(TEXT("No trim applied (remaining < ReshowThreshold)"), Result.TrimmedPoints.Num(), 2);
	if (Result.TrimmedPoints.Num() == 2)
	{
		TestEqual(TEXT("Start point untouched — trim distance is 0"), Result.TrimmedPoints[0], FVector::ZeroVector);
		TestEqual(TEXT("End point untouched"), Result.TrimmedPoints[1], FVector(390.f, 0.f, 0.f));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavPathVisibilityAboveReshowTrimsOffsetTest,
	"SkillProject.Navigation.Math.ApplyPathPointsAboveReshowTrimsOffset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavPathVisibilityAboveReshowTrimsOffsetTest::RunTest(const FString& Parameters)
{
	//# 원본 450cm(>=400) — visible=true, 트림량 100(§4-3 규칙2 정상 오프셋 트리밍 구간)
	TArray<FVector> PathPoints;
	PathPoints.Add(FVector::ZeroVector);
	PathPoints.Add(FVector(450.f, 0.f, 0.f));

	const FSpyNavPathVisibilityMirrorResult Result = SpyNavPathVisibilityIntegrationTests_ApplyPathPointsMirror(
		PathPoints,
		SpyNavPathVisibilityIntegrationTests_HideThresholdCm,
		SpyNavPathVisibilityIntegrationTests_ReshowThresholdCm,
		SpyNavPathVisibilityIntegrationTests_StartOffsetDistanceCm,
		/*bPreviouslyVisible=*/true);

	TestTrue(TEXT("450cm stays visible"), Result.bVisible);
	TestEqual(TEXT("Trimmed keeps 2 points"), Result.TrimmedPoints.Num(), 2);
	if (Result.TrimmedPoints.Num() == 2)
	{
		TestEqual(TEXT("Start trimmed forward by 100cm"), Result.TrimmedPoints[0], FVector(100.f, 0.f, 0.f));
		TestEqual(TEXT("End unchanged"), Result.TrimmedPoints[1], FVector(450.f, 0.f, 0.f));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavPathVisibilityBelowHideHidesTest,
	"SkillProject.Navigation.Math.ApplyPathPointsBelowHideHides",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavPathVisibilityBelowHideHidesTest::RunTest(const FString& Parameters)
{
	//# 원본 250cm(<300) — 콜드 스타트로 시드돼도 히스테리시스 하한 아래면 그대로 숨겨진다
	TArray<FVector> PathPoints;
	PathPoints.Add(FVector::ZeroVector);
	PathPoints.Add(FVector(250.f, 0.f, 0.f));

	const FSpyNavPathVisibilityMirrorResult Result = SpyNavPathVisibilityIntegrationTests_ApplyPathPointsMirror(
		PathPoints,
		SpyNavPathVisibilityIntegrationTests_HideThresholdCm,
		SpyNavPathVisibilityIntegrationTests_ReshowThresholdCm,
		SpyNavPathVisibilityIntegrationTests_StartOffsetDistanceCm,
		/*bPreviouslyVisible=*/true);

	TestFalse(TEXT("250cm hides even with cold-start seed"), Result.bVisible);
	TestEqual(TEXT("No trimmed points computed when hidden"), Result.TrimmedPoints.Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavPathVisibilityColdStartSeedRegressionTest,
	"SkillProject.Navigation.Math.ApplyPathPointsColdStartSeedRegression",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavPathVisibilityColdStartSeedRegressionTest::RunTest(const FString& Parameters)
{
	//# design §4-3 콜드 스타트 조항 자체를 명시 검증한다 — StartPathTo() 는 bPathVisible 을
	//# true 로 시드한다(SpyNavigationComponent.cpp). 시드하지 않았다면(구버전 버그, 기본값
	//# false) 목표<400cm 인 채로 미션이 시작될 때 그 미션 동안 라인이 한 번도 안 보인다 —
	//# 시드 유무에 따른 첫 프레임 결과 차이를 대조해 그 결함을 회귀로 고정한다.
	TArray<FVector> PathPoints;
	PathPoints.Add(FVector::ZeroVector);
	PathPoints.Add(FVector(350.f, 0.f, 0.f)); //# 밴드 안(300~400) — 시드 유무가 결과를 가르는 값

	const FSpyNavPathVisibilityMirrorResult WithColdStartSeed = SpyNavPathVisibilityIntegrationTests_ApplyPathPointsMirror(
		PathPoints,
		SpyNavPathVisibilityIntegrationTests_HideThresholdCm,
		SpyNavPathVisibilityIntegrationTests_ReshowThresholdCm,
		SpyNavPathVisibilityIntegrationTests_StartOffsetDistanceCm,
		/*bPreviouslyVisible=*/true);
	TestTrue(TEXT("StartPathTo's bPathVisible=true seed -> first frame visible"), WithColdStartSeed.bVisible);

	const FSpyNavPathVisibilityMirrorResult WithoutColdStartSeed = SpyNavPathVisibilityIntegrationTests_ApplyPathPointsMirror(
		PathPoints,
		SpyNavPathVisibilityIntegrationTests_HideThresholdCm,
		SpyNavPathVisibilityIntegrationTests_ReshowThresholdCm,
		SpyNavPathVisibilityIntegrationTests_StartOffsetDistanceCm,
		/*bPreviouslyVisible=*/false);
	TestFalse(TEXT("Regression proof — default-false seed reproduces the fixed bug (never shows)"), WithoutColdStartSeed.bVisible);

	return true;
}

//# ─────────────────────────────────────────────────────────────────────────────
//# 이하 self-review 보강(advisor 지적) — (1) 경계값 300/400 자체를 정확히 찍는 케이스
//# (기존 케이스는 250/350/390/450 전부 밴드 내부값이라 >=/> 오타를 못 잡는다), (2) 여러
//# 프레임에 걸쳐 bVisible 을 되먹이는 상태 스레딩(단일 프레임 판정만으로는 히스테리시스가
//# 왜 필요한지 — 진동 방지 — 를 검증하지 못한다)
//# ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavPathVisibilityExactBoundaryValuesTest,
	"SkillProject.Navigation.Math.ApplyPathPointsExactBoundaryValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavPathVisibilityExactBoundaryValuesTest::RunTest(const FString& Parameters)
{
	//# 이전 두 회귀는 모두 "300/400 중 어느 쪽에 걸치는가" 문제였다(plan 4차 개정 트림 순서
	//# 버그, code-reviewer 1차검토 콜드 스타트 버그) — 250/350/390/450 같은 밴드 내부값만으로는
	//# >=/> 오타(경계 포함 방향이 뒤집힌 실수)를 못 잡는다. 정확히 300과 400을 찍어 확인한다.
	TArray<FVector> AtHideThreshold;
	AtHideThreshold.Add(FVector::ZeroVector);
	AtHideThreshold.Add(FVector(300.f, 0.f, 0.f));

	TArray<FVector> AtReshowThreshold;
	AtReshowThreshold.Add(FVector::ZeroVector);
	AtReshowThreshold.Add(FVector(400.f, 0.f, 0.f));

	//# 정확히 300 + 직전 보임 -> 그대로 보임 (규칙1: "< 300"일 때만 숨김, 300은 미만이 아니다)
	const FSpyNavPathVisibilityMirrorResult HideBoundaryFromVisible = SpyNavPathVisibilityIntegrationTests_ApplyPathPointsMirror(
		AtHideThreshold,
		SpyNavPathVisibilityIntegrationTests_HideThresholdCm,
		SpyNavPathVisibilityIntegrationTests_ReshowThresholdCm,
		SpyNavPathVisibilityIntegrationTests_StartOffsetDistanceCm,
		/*bPreviouslyVisible=*/true);
	TestTrue(TEXT("Exactly 300cm from visible stays visible (>= is inclusive)"), HideBoundaryFromVisible.bVisible);

	//# 정확히 300 + 직전 숨김 -> 재표시 조건(>=400)을 만족 못해 그대로 숨김
	const FSpyNavPathVisibilityMirrorResult HideBoundaryFromHidden = SpyNavPathVisibilityIntegrationTests_ApplyPathPointsMirror(
		AtHideThreshold,
		SpyNavPathVisibilityIntegrationTests_HideThresholdCm,
		SpyNavPathVisibilityIntegrationTests_ReshowThresholdCm,
		SpyNavPathVisibilityIntegrationTests_StartOffsetDistanceCm,
		/*bPreviouslyVisible=*/false);
	TestFalse(TEXT("Exactly 300cm from hidden stays hidden (needs >=400 to reshow)"), HideBoundaryFromHidden.bVisible);

	//# 정확히 400 + 직전 숨김 -> 재표시되고, 같은 프레임에 오프셋도 즉시 100cm로 트리밍된다
	//# (규칙2 경계가 규칙1의 재표시 경계와 동일값 400을 공유한다는 §4-3 설계 그 자체를 확인)
	const FSpyNavPathVisibilityMirrorResult ReshowBoundaryFromHidden = SpyNavPathVisibilityIntegrationTests_ApplyPathPointsMirror(
		AtReshowThreshold,
		SpyNavPathVisibilityIntegrationTests_HideThresholdCm,
		SpyNavPathVisibilityIntegrationTests_ReshowThresholdCm,
		SpyNavPathVisibilityIntegrationTests_StartOffsetDistanceCm,
		/*bPreviouslyVisible=*/false);
	TestTrue(TEXT("Exactly 400cm from hidden reshows (>= is inclusive)"), ReshowBoundaryFromHidden.bVisible);
	TestEqual(TEXT("Reshow at exactly 400cm keeps 2 points"), ReshowBoundaryFromHidden.TrimmedPoints.Num(), 2);
	if (ReshowBoundaryFromHidden.TrimmedPoints.Num() == 2)
	{
		TestEqual(TEXT("Offset trimmed to 100cm on the exact reshow boundary"), ReshowBoundaryFromHidden.TrimmedPoints[0], FVector(100.f, 0.f, 0.f));
	}

	//# 정확히 400 + 직전 보임 -> 계속 보이고, 오프셋 트리밍도 동일하게 100cm(규칙2 경계도 400)
	const FSpyNavPathVisibilityMirrorResult ReshowBoundaryFromVisible = SpyNavPathVisibilityIntegrationTests_ApplyPathPointsMirror(
		AtReshowThreshold,
		SpyNavPathVisibilityIntegrationTests_HideThresholdCm,
		SpyNavPathVisibilityIntegrationTests_ReshowThresholdCm,
		SpyNavPathVisibilityIntegrationTests_StartOffsetDistanceCm,
		/*bPreviouslyVisible=*/true);
	TestTrue(TEXT("Exactly 400cm from visible stays visible"), ReshowBoundaryFromVisible.bVisible);
	TestEqual(TEXT("Trim also snaps to 100cm at exactly 400cm"), ReshowBoundaryFromVisible.TrimmedPoints.Num(), 2);
	if (ReshowBoundaryFromVisible.TrimmedPoints.Num() == 2)
	{
		TestEqual(TEXT("Offset trimmed to 100cm"), ReshowBoundaryFromVisible.TrimmedPoints[0], FVector(100.f, 0.f, 0.f));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavPathVisibilityStateThreadedAcrossFramesTest,
	"SkillProject.Navigation.Math.ApplyPathPointsStateThreadedAcrossFrames",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavPathVisibilityStateThreadedAcrossFramesTest::RunTest(const FString& Parameters)
{
	//# 단일 프레임 판정만으로는 히스테리시스가 왜 필요한지(진동 방지)를 검증하지 못한다.
	//# ApplyPathPoints 는 매 RecomputePath 호출마다 이 프레임에서 계산한 bPathVisible 을
	//# 다음 프레임의 입력으로 그대로 되먹인다(SpyNavigationComponent.cpp) — 그 되먹임을
	//# 여러 프레임에 걸쳐 재현해, 같은 350cm 가 "직전 상태가 무엇이었는가"에 따라 서로
	//# 다른 결과를 내는지(히스테리시스의 핵심 성질) 확인한다.
	struct FSpyNavPathVisibilityFrame
	{
		float LengthCm = 0.f;
		bool bExpectedVisible = false;
	};

	const FSpyNavPathVisibilityFrame Frames[] = {
		{ 1000.f, true }, //# 멀리 — 계속 보임
		{ 350.f, true }, //# 밴드 진입, 직전 "보임" 상태 유지
		{ 250.f, false }, //# 하한 아래 — 숨김으로 전환
		{ 350.f, false }, //# 같은 350cm 인데 직전이 "숨김"이라 그대로 숨김(히스테리시스 핵심 대조)
		{ 450.f, true }, //# 재표시 상한 초과 — 다시 보임
	};

	bool bPreviouslyVisible = true; //# 임의의 안정된 초기 상태(멀리서 시작)

	for (const FSpyNavPathVisibilityFrame& Frame : Frames)
	{
		TArray<FVector> FramePoints;
		FramePoints.Add(FVector::ZeroVector);
		FramePoints.Add(FVector(Frame.LengthCm, 0.f, 0.f));

		const FSpyNavPathVisibilityMirrorResult Result = SpyNavPathVisibilityIntegrationTests_ApplyPathPointsMirror(
			FramePoints,
			SpyNavPathVisibilityIntegrationTests_HideThresholdCm,
			SpyNavPathVisibilityIntegrationTests_ReshowThresholdCm,
			SpyNavPathVisibilityIntegrationTests_StartOffsetDistanceCm,
			bPreviouslyVisible);

		const FString Description = FString::Printf(TEXT("Frame at %.0fcm (prev visible=%s)"), Frame.LengthCm, bPreviouslyVisible ? TEXT("true") : TEXT("false"));
		if (Frame.bExpectedVisible)
		{
			TestTrue(Description, Result.bVisible);
		}
		else
		{
			TestFalse(Description, Result.bVisible);
		}

		bPreviouslyVisible = Result.bVisible; //# 다음 프레임으로 되먹임 — ApplyPathPoints가 bPathVisible을 갱신하는 것과 동일
	}

	//# 마지막 프레임(450cm, 방금 재표시)은 §4-3 한계 2항 그대로 오프셋이 즉시 100cm로 스냅된다
	TArray<FVector> FinalFramePoints;
	FinalFramePoints.Add(FVector::ZeroVector);
	FinalFramePoints.Add(FVector(450.f, 0.f, 0.f));

	const FSpyNavPathVisibilityMirrorResult FinalResult = SpyNavPathVisibilityIntegrationTests_ApplyPathPointsMirror(
		FinalFramePoints,
		SpyNavPathVisibilityIntegrationTests_HideThresholdCm,
		SpyNavPathVisibilityIntegrationTests_ReshowThresholdCm,
		SpyNavPathVisibilityIntegrationTests_StartOffsetDistanceCm,
		/*bPreviouslyVisible=*/false);

	TestTrue(TEXT("Reshow frame is visible"), FinalResult.bVisible);
	TestEqual(TEXT("Reshow frame keeps 2 trimmed points"), FinalResult.TrimmedPoints.Num(), 2);
	if (FinalResult.TrimmedPoints.Num() == 2)
	{
		TestEqual(TEXT("Offset snapped to 100cm in the same tick as the reshow (§4-3 한계 2항)"), FinalResult.TrimmedPoints[0], FVector(100.f, 0.f, 0.f));
	}

	return true;
}

#endif //# WITH_DEV_AUTOMATION_TESTS
