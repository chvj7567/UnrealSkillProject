#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "System/SpyNavPathMath.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavPathMathBuildSegmentsTest,
	"SkillProject.Navigation.Math.BuildSplineSegments",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavPathMathBuildSegmentsTest::RunTest(const FString& Parameters)
{
	TArray<FVector> PathPoints;
	PathPoints.Add(FVector(0.f, 0.f, 0.f));
	PathPoints.Add(FVector(100.f, 0.f, 0.f));
	PathPoints.Add(FVector(100.f, 100.f, 0.f));

	const TArray<TPair<FVector, FVector>> Segments = SpyNavPathMath::BuildSplineSegments(PathPoints);

	TestEqual(TEXT("3 points produce 2 segments"), Segments.Num(), 2);

	if (Segments.Num() == 2)
	{
		TestEqual(TEXT("Segment 0 start"), Segments[0].Key, FVector(0.f, 0.f, 0.f));
		TestEqual(TEXT("Segment 0 end"), Segments[0].Value, FVector(100.f, 0.f, 0.f));
		TestEqual(TEXT("Segment 1 start"), Segments[1].Key, FVector(100.f, 0.f, 0.f));
		TestEqual(TEXT("Segment 1 end"), Segments[1].Value, FVector(100.f, 100.f, 0.f));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavPathMathBuildSegmentsTooFewPointsTest,
	"SkillProject.Navigation.Math.BuildSplineSegmentsTooFewPoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavPathMathBuildSegmentsTooFewPointsTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Empty input"), SpyNavPathMath::BuildSplineSegments(TArray<FVector>()).Num(), 0);

	TArray<FVector> SinglePoint;
	SinglePoint.Add(FVector::ZeroVector);
	TestEqual(TEXT("Single point produces no segments"), SpyNavPathMath::BuildSplineSegments(SinglePoint).Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavPathMathTrimLeadingDistanceTest,
	"SkillProject.Navigation.Math.TrimLeadingDistance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavPathMathTrimLeadingDistanceTest::RunTest(const FString& Parameters)
{
	TArray<FVector> PathPoints;
	PathPoints.Add(FVector(0.f, 0.f, 0.f));
	PathPoints.Add(FVector(200.f, 0.f, 0.f));

	const TArray<FVector> Trimmed = SpyNavPathMath::TrimLeadingDistance(PathPoints, 100.f);

	TestEqual(TEXT("Trimmed keeps 2 points"), Trimmed.Num(), 2);
	if (Trimmed.Num() == 2)
	{
		TestEqual(TEXT("Start moved forward by 100cm"), Trimmed[0], FVector(100.f, 0.f, 0.f));
		TestEqual(TEXT("End unchanged"), Trimmed[1], FVector(200.f, 0.f, 0.f));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavPathMathTrimLeadingDistanceExceedsPathTest,
	"SkillProject.Navigation.Math.TrimLeadingDistanceExceedsPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavPathMathTrimLeadingDistanceExceedsPathTest::RunTest(const FString& Parameters)
{
	//# 트림 거리(500)가 전체 경로 길이(200)보다 길면 마지막 점 하나만 남는다(도착 직전)
	TArray<FVector> PathPoints;
	PathPoints.Add(FVector(0.f, 0.f, 0.f));
	PathPoints.Add(FVector(200.f, 0.f, 0.f));

	const TArray<FVector> Trimmed = SpyNavPathMath::TrimLeadingDistance(PathPoints, 500.f);

	TestEqual(TEXT("Only the last point remains"), Trimmed.Num(), 1);
	if (Trimmed.Num() == 1)
		TestEqual(TEXT("Last point is the target"), Trimmed[0], FVector(200.f, 0.f, 0.f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavPathMathComputePathLengthTest,
	"SkillProject.Navigation.Math.ComputePathLength",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavPathMathComputePathLengthTest::RunTest(const FString& Parameters)
{
	TArray<FVector> PathPoints;
	PathPoints.Add(FVector(0.f, 0.f, 0.f));
	PathPoints.Add(FVector(100.f, 0.f, 0.f));
	PathPoints.Add(FVector(100.f, 100.f, 0.f));

	TestEqual(TEXT("Length is 100+100=200"), SpyNavPathMath::ComputePathLength(PathPoints), 200.f);
	TestEqual(TEXT("Empty path has 0 length"), SpyNavPathMath::ComputePathLength(TArray<FVector>()), 0.f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavPathMathHysteresisVisibilityTest,
	"SkillProject.Navigation.Math.HysteresisVisibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavPathMathHysteresisVisibilityTest::RunTest(const FString& Parameters)
{
	//# design §4-3 확정값: Hide=300, Reshow=400
	TestTrue(TEXT("Far away + was visible -> stays visible"), SpyNavPathMath::EvaluateHysteresisVisibility(1000.f, 300.f, 400.f, true));
	TestFalse(TEXT("Close + was visible -> hides"), SpyNavPathMath::EvaluateHysteresisVisibility(200.f, 300.f, 400.f, true));

	//# 밴드(300~400) 안에서는 "이전 상태 유지" — 보이던 상태였으면 계속 보임
	TestTrue(TEXT("Inside band + was visible -> stays visible"), SpyNavPathMath::EvaluateHysteresisVisibility(350.f, 300.f, 400.f, true));
	//# 밴드 안에서 숨어 있었으면 계속 숨음
	TestFalse(TEXT("Inside band + was hidden -> stays hidden"), SpyNavPathMath::EvaluateHysteresisVisibility(350.f, 300.f, 400.f, false));

	TestTrue(TEXT("Far away + was hidden -> reshows"), SpyNavPathMath::EvaluateHysteresisVisibility(1000.f, 300.f, 400.f, false));
	TestFalse(TEXT("Close + was hidden -> stays hidden"), SpyNavPathMath::EvaluateHysteresisVisibility(200.f, 300.f, 400.f, false));

	return true;
}

//# ─────────────────────────────────────────────────────────────────────────────
//# 이하 test-engineer 확장 — 중복 좌표(0 길이 세그먼트) · 다량의 점(정확성만) 엣지 케이스
//# ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavPathMathComputePathLengthDuplicateConsecutivePointsTest,
	"SkillProject.Navigation.Math.ComputePathLengthDuplicateConsecutivePoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavPathMathComputePathLengthDuplicateConsecutivePointsTest::RunTest(const FString& Parameters)
{
	//# 연속 중복 좌표(0 길이 세그먼트)는 길이에 기여하지 않을 뿐 루프를 깨지 않는다
	TArray<FVector> PathPoints;
	PathPoints.Add(FVector(0.f, 0.f, 0.f));
	PathPoints.Add(FVector(0.f, 0.f, 0.f));
	PathPoints.Add(FVector(100.f, 0.f, 0.f));

	TestEqual(TEXT("Duplicate leading segment contributes 0"), SpyNavPathMath::ComputePathLength(PathPoints), 100.f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavPathMathTrimLeadingDistanceSkipsZeroLengthSegmentsTest,
	"SkillProject.Navigation.Math.TrimLeadingDistanceSkipsZeroLengthSegments",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavPathMathTrimLeadingDistanceSkipsZeroLengthSegmentsTest::RunTest(const FString& Parameters)
{
	//# 중간에 중복 좌표(0 길이 세그먼트)가 있어도 트리밍이 실제 거리 기준으로 정확히 동작한다
	TArray<FVector> PathPoints;
	PathPoints.Add(FVector(0.f, 0.f, 0.f));
	PathPoints.Add(FVector(100.f, 0.f, 0.f));
	PathPoints.Add(FVector(100.f, 0.f, 0.f)); //# 중복
	PathPoints.Add(FVector(300.f, 0.f, 0.f));

	const TArray<FVector> Trimmed = SpyNavPathMath::TrimLeadingDistance(PathPoints, 150.f);

	TestEqual(TEXT("Trimmed keeps 2 points"), Trimmed.Num(), 2);
	if (Trimmed.Num() == 2)
	{
		TestEqual(TEXT("Interpolated correctly past the zero-length segment"), Trimmed[0], FVector(150.f, 0.f, 0.f));
		TestEqual(TEXT("End unchanged"), Trimmed[1], FVector(300.f, 0.f, 0.f));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavPathMathTrimLeadingDistanceAllDuplicatePointsTest,
	"SkillProject.Navigation.Math.TrimLeadingDistanceAllDuplicatePoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavPathMathTrimLeadingDistanceAllDuplicatePointsTest::RunTest(const FString& Parameters)
{
	//# 경로 전체가 동일 좌표(총 길이 0)면 어떤 트림 거리로도 세그먼트를 찾지 못한 채
	//# 루프가 끝나고, 마지막 점 하나로 안전하게 접힌다 — 크래시도 무한루프도 없다
	TArray<FVector> PathPoints;
	PathPoints.Add(FVector(50.f, 50.f, 0.f));
	PathPoints.Add(FVector(50.f, 50.f, 0.f));

	TestEqual(TEXT("Zero-length path has 0 length"), SpyNavPathMath::ComputePathLength(PathPoints), 0.f);

	const TArray<FVector> Trimmed = SpyNavPathMath::TrimLeadingDistance(PathPoints, 10.f);

	TestEqual(TEXT("Collapses to a single point"), Trimmed.Num(), 1);
	if (Trimmed.Num() == 1)
	{
		TestEqual(TEXT("The single point is the shared coordinate"), Trimmed[0], FVector(50.f, 50.f, 0.f));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavPathMathManyPointsAccuracyTest,
	"SkillProject.Navigation.Math.ManyPointsAccuracy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavPathMathManyPointsAccuracyTest::RunTest(const FString& Parameters)
{
	//# 성능이 아니라 정확성만 본다 — 점 100개(직선, 10cm 간격, 총 990cm)에서 길이 합산과
	//# 트리밍 보간이 세그먼트 수가 많아져도 오차 없이 정확한지 확인한다
	TArray<FVector> PathPoints;
	for (int32 Index = 0; Index < 100; ++Index)
	{
		PathPoints.Add(FVector(Index * 10.f, 0.f, 0.f));
	}

	TestEqual(TEXT("Total length across 99 segments"), SpyNavPathMath::ComputePathLength(PathPoints), 990.f);

	const TArray<FVector> Trimmed = SpyNavPathMath::TrimLeadingDistance(PathPoints, 505.f);

	TestEqual(TEXT("50 points remain after trimming 505cm off 990cm"), Trimmed.Num(), 50);
	if (Trimmed.Num() == 50)
	{
		TestEqual(TEXT("Interpolated start point"), Trimmed[0], FVector(505.f, 0.f, 0.f));
		TestEqual(TEXT("Last point untouched"), Trimmed.Last(), FVector(990.f, 0.f, 0.f));
	}

	return true;
}

#endif //# WITH_DEV_AUTOMATION_TESTS
