// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/SpyCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Misc/AutomationTest.h"

//# 월드 없이 생성한 캐릭터 픽스처 — 억제 API 는 CameraBoom(생성자 서브오브젝트)만 건드리므로
//# 스폰 없이 검증할 수 있다.
static ASpyCharacter* SpyCameraTests_MakeCharacter()
{
	return NewObject<ASpyCharacter>(GetTransientPackage());
}

//# 정상 케이스 — 파쿠르 1건이 억제를 걸었다가 풀면 원래 값으로 돌아온다.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyCameraCollisionSuppressSingleTest,
	"SkillProject.Character.Camera.CollisionSuppressSingle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyCameraCollisionSuppressSingleTest::RunTest(const FString& Parameters)
{
	ASpyCharacter* Character = SpyCameraTests_MakeCharacter();
	USpringArmComponent* CameraBoom = Character->GetCameraBoom();

	TestTrue(TEXT("CameraBoom exists"), IsValid(CameraBoom));
	TestTrue(TEXT("Collision test is on by default"), (CameraBoom->bDoCollisionTest != 0));

	Character->PushCameraCollisionSuppress();
	TestFalse(TEXT("Collision test suppressed while active"), (CameraBoom->bDoCollisionTest != 0));
	TestEqual(TEXT("Suppress count is 1"), Character->GetCameraCollisionSuppressCount(), 1);

	Character->PopCameraCollisionSuppress();
	TestTrue(TEXT("Collision test restored after release"), (CameraBoom->bDoCollisionTest != 0));
	TestEqual(TEXT("Suppress count is 0"), Character->GetCameraCollisionSuppressCount(), 0);

	return true;
}

//# 엣지 케이스 — 파쿠르 체이닝(Vault -> HangUp)에서 먼저 끝난 쪽이 억제를 풀면 안 되고,
//# 짝이 맞지 않는 해제로 카운트가 음수가 되어도 안 된다.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyCameraCollisionSuppressNestedTest,
	"SkillProject.Character.Camera.CollisionSuppressNested",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyCameraCollisionSuppressNestedTest::RunTest(const FString& Parameters)
{
	ASpyCharacter* Character = SpyCameraTests_MakeCharacter();
	USpringArmComponent* CameraBoom = Character->GetCameraBoom();

	Character->PushCameraCollisionSuppress();
	Character->PushCameraCollisionSuppress();
	TestEqual(TEXT("Suppress count is 2"), Character->GetCameraCollisionSuppressCount(), 2);

	//# 먼저 끝난 액션 — 아직 진행 중인 액션이 있으므로 억제가 유지돼야 한다.
	Character->PopCameraCollisionSuppress();
	TestFalse(TEXT("Still suppressed while one action remains"), (CameraBoom->bDoCollisionTest != 0));

	Character->PopCameraCollisionSuppress();
	TestTrue(TEXT("Restored after last action released"), (CameraBoom->bDoCollisionTest != 0));

	//# 짝이 맞지 않는 여분 해제 — 카운트가 음수로 내려가지 않고 값도 그대로여야 한다.
	Character->PopCameraCollisionSuppress();
	TestEqual(TEXT("Suppress count never goes negative"), Character->GetCameraCollisionSuppressCount(), 0);
	TestTrue(TEXT("Collision test unchanged by extra release"), (CameraBoom->bDoCollisionTest != 0));

	return true;
}

//# 엣지 케이스 — BP 에서 bDoCollisionTest 를 false 로 세팅한 캐릭터는
//# 해제 시 하드코딩 true 가 아니라 원래 값(false)으로 돌아와야 한다.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyCameraCollisionSuppressRestoreOriginalTest,
	"SkillProject.Character.Camera.CollisionSuppressRestoreOriginal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyCameraCollisionSuppressRestoreOriginalTest::RunTest(const FString& Parameters)
{
	ASpyCharacter* Character = SpyCameraTests_MakeCharacter();
	USpringArmComponent* CameraBoom = Character->GetCameraBoom();

	CameraBoom->bDoCollisionTest = false;

	Character->PushCameraCollisionSuppress();
	Character->PopCameraCollisionSuppress();

	TestFalse(TEXT("Original false value restored"), (CameraBoom->bDoCollisionTest != 0));

	return true;
}

#endif
