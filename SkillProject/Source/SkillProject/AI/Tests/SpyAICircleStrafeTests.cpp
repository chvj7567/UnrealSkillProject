#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ManagerComponent/SpyAnimManagerComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSpyStrafeDirVectorTest,
    "SkillProject.AI.CircleStrafe.StrafeDirVector",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyStrafeDirVectorTest::RunTest(const FString& Parameters)
{
    // Querier=(0,0,0), Target=(100,0,0) → Forward=(1,0,0)
    // RightVec = Cross(Up, Forward) = (0,1,0) = +Y
    FVector QuerierLoc(0.0, 0.0, 0.0);
    FVector TargetLoc(100.0, 0.0, 0.0);
    FVector Forward = (TargetLoc - QuerierLoc).GetSafeNormal2D();
    FVector RightVec = FVector::CrossProduct(FVector::UpVector, Forward);

    TestNearlyEqual(TEXT("Strafe right X"), RightVec.X, 0.0, 0.01);
    TestNearlyEqual(TEXT("Strafe right Y"), RightVec.Y, 1.0, 0.01);
    TestNearlyEqual(TEXT("Strafe right Z"), RightVec.Z, 0.0, 0.01);

    FVector LeftVec = -RightVec;
    TestNearlyEqual(TEXT("Strafe left Y"), LeftVec.Y, -1.0, 0.01);

    FVector Target45 = FVector(100.0, 100.0, 0.0);
    FVector Forward45 = (Target45 - QuerierLoc).GetSafeNormal2D();
    FVector Right45 = FVector::CrossProduct(FVector::UpVector, Forward45);
    TestNearlyEqual(TEXT("45deg strafe length"), Right45.Size(), 1.0, 0.01);
    TestNearlyEqual(TEXT("45deg perp"), FVector::DotProduct(Forward45, Right45), 0.0, 0.01);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSpyCalcDirectionTest,
    "SkillProject.AI.CircleStrafe.CalcDirection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyCalcDirectionTest::RunTest(const FString& Parameters)
{
    // 전방 이동 (Velocity=+X, Rotation=0) → 0도
    float Dir = USpyAnimManagerComponent::CalcDirectionFromVelocity(
        FVector(100.0, 0.0, 0.0), FRotator::ZeroRotator);
    TestNearlyEqual(TEXT("Forward = 0 deg"), (double)Dir, 0.0, 0.5);

    // 우측 이동 (Velocity=+Y) → +90도
    Dir = USpyAnimManagerComponent::CalcDirectionFromVelocity(
        FVector(0.0, 100.0, 0.0), FRotator::ZeroRotator);
    TestNearlyEqual(TEXT("Right = +90 deg"), (double)Dir, 90.0, 0.5);

    // 좌측 이동 (Velocity=-Y) → -90도
    Dir = USpyAnimManagerComponent::CalcDirectionFromVelocity(
        FVector(0.0, -100.0, 0.0), FRotator::ZeroRotator);
    TestNearlyEqual(TEXT("Left = -90 deg"), (double)Dir, -90.0, 0.5);

    // 후방 이동 (Velocity=-X) → ±180도 근사
    Dir = USpyAnimManagerComponent::CalcDirectionFromVelocity(
        FVector(-100.0, 0.0, 0.0), FRotator::ZeroRotator);
    TestTrue(TEXT("Backward near ±180"), FMath::Abs(Dir) > 170.0f);

    // 액터가 90도 회전된 상태에서 World +X 이동 → 로컬로는 -Y → -90도
    Dir = USpyAnimManagerComponent::CalcDirectionFromVelocity(
        FVector(100.0, 0.0, 0.0), FRotator(0.0, 90.0, 0.0));
    TestNearlyEqual(TEXT("Rotated actor right = -90 deg"), (double)Dir, -90.0, 0.5);

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
