// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "AIController.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Character/SpyCharacter.h"
#include "Character/SpyCharacterMovementComponent.h"
#include "ManagerComponent/SpyTargetingManagerComponent.h"
#include "Util/DefineEnum.h"

//# ---------------------------------------------------------------------------
//# AI 180도 스핀 회귀 스위트
//#
//# 고친 버그: SpyCharacterAssetData 의 공용 컴포넌트 목록에 USpyTargetingManagerComponent 가
//# 들어 있어 AI 도 이 컴포넌트를 갖는다. 그래서 USpyCharacterMovementComponent::PhysicsRotation
//# 의 타겟팅 분기가 AI 에서도 돌았고, 타겟 없는 AI 에 매 프레임 bOrientRotationToMovement = true
//# 를 강제해 ASpyCharacter::PossessedBy(SpyCharacter.cpp:112-117) 의 AI 전용 설정을 무효화했다.
//# 공격 중(Lock.Input.Move) BT 가 회전을 놓는 순간 엔진이 경로 이동 방향으로 회전시켜 180도 스핀.
//#
//# 이 스위트가 박제하는 불변식:
//#   "PhysicsRotation 은 AI 폰의 bOrientRotationToMovement 를 어느 방향으로도 쓰지 않는다."
//# 플래그 소유권이 회귀의 본체이므로 Lock.Input.Move / BT / EQS 는 재현하지 않는다.
//#
//# 픽스처 근거 (월드 없이 NewObject 를 쓰는 이유):
//#   AController::Possess -> APawn::PossessedBy 는 AI 분기에서 AActor::CopyRemoteRoleFrom 을
//#   타고, 그 안에서 GetWorld()->AddNetworkActor(this) 를 무조건 호출한다
//#   (UE 5.7 Engine/Private/Actor.cpp). 월드 없는 액터에서는 널 역참조로 죽는다.
//#   그래서 possess 대신 public 멤버인 APawn::Controller 를 직접 대입하고,
//#   PossessedBy 가 세팅하는 두 값(bUseControllerRotationYaw = true /
//#   bOrientRotationToMovement = false)을 픽스처가 그대로 재현한다.
//#   PossessedBy 쪽이 바뀌면 이 픽스처도 같이 고쳐야 한다 — SpyCharacter.cpp:112-117 참조.
//#
//#   실제 월드(UWorld::CreateWorld) + Possess 통합 티어는 의도적으로 만들지 않았다.
//#   APlayerController::PostInitializeComponents(카메라 매니저 스폰) / OnPossess(ClientRestart ->
//#   Restart -> AutoManageActiveCameraTarget) / AAIController::OnPossess(내비·브레인·퍼셉션 초기화)
//#   가 게임 모드 없는 빈 월드에서 안전한지 실행으로 확인할 수단이 없다.
//# ---------------------------------------------------------------------------

namespace SpyAIRotationTests
{
	//# 월드 없이 만든 캐릭터. 생성자 서브오브젝트(캡슐/무브먼트/PawnExtension)는 전부 만들어지고
	//# UActorComponent 생성자가 OwnerPrivate = GetTypedOuter<AActor>() 를 채우므로
	//# GetOwner() / GetCharacterMovement() 는 유효하다.
	static ASpyCharacter* MakeCharacter()
	{
		return NewObject<ASpyCharacter>(GetTransientPackage());
	}

	static USpyCharacterMovementComponent* GetSpyMovement(ASpyCharacter* InCharacter)
	{
		return Cast<USpyCharacterMovementComponent>(InCharacter->GetCharacterMovement());
	}

	//# 런타임에 CharacterAssetData 로 붙는 컴포넌트를 흉내낸다.
	//# UActorComponent::PostInitProperties 가 CreationMethod != Instance 일 때
	//# OwnerPrivate->AddOwnedComponent(this) 를 호출하므로 FindComponentByClass 로 잡힌다.
	static USpyTargetingManagerComponent* AttachTargeting(ASpyCharacter* InCharacter)
	{
		return NewObject<USpyTargetingManagerComponent>(InCharacter);
	}

	//# ASpyCharacter::PossessedBy 가 AI 에 넣는 설정 그대로 (SpyCharacter.cpp:112-117).
	static void ApplyPossessedByAISetup(ASpyCharacter* InCharacter)
	{
		InCharacter->bUseControllerRotationYaw = true;
		InCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	}
}

//# ---------------------------------------------------------------------------
//# 회귀 핵심 — AI 가 PhysicsRotation 을 지나도 PossessedBy 설정이 살아 있어야 한다.
//# 수정 전에는 타겟 없는 AI 가 else 분기(SpyCharacterMovementComponent.cpp:106)로 빠져
//# bOrientRotationToMovement 가 true 로 뒤집혔다 — 그 경우 이 테스트는 실패한다.
//# ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyRotationAIKeepsControllerYawTest,
	"SkillProject.Character.Rotation.AIKeepsControllerYaw",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyRotationAIKeepsControllerYawTest::RunTest(const FString& Parameters)
{
	ASpyCharacter* Character = SpyAIRotationTests::MakeCharacter();
	USpyCharacterMovementComponent* MoveComp = SpyAIRotationTests::GetSpyMovement(Character);

	TestTrue(TEXT("Movement component is the Spy override"), MoveComp != nullptr);
	if (MoveComp == nullptr)
		return false;

	//# 클래스 기본값 확인 — 생성자는 플레이어 기준(true)으로 시작한다 (SpyCharacter.cpp:45)
	TestTrue(TEXT("Constructor default orients to movement"), (MoveComp->bOrientRotationToMovement != 0));

	//# AI 는 CharacterAssetData 공용 목록을 통해 타겟팅 컴포넌트를 갖는다 — 버그의 전제 조건
	USpyTargetingManagerComponent* TargetingComp = SpyAIRotationTests::AttachTargeting(Character);
	TestTrue(TEXT("Targeting component is discoverable on the pawn"),
			 Character->FindComponentByClass<USpyTargetingManagerComponent>() == TargetingComp);
	TestFalse(TEXT("AI starts without a target"), TargetingComp->GetTarget().IsValid());

	//# AI 컨트롤러 possess 상태 재현
	AAIController* AIController = NewObject<AAIController>(GetTransientPackage());
	Character->Controller = AIController;
	SpyAIRotationTests::ApplyPossessedByAISetup(Character);

	//# 기본 이동 모드(MOVE_Default == 0)여야 PhysicsRotation 의 default 분기로 들어간다
	MoveComp->CustomMovementMode = static_cast<uint8>(ECustomMovementMode::MOVE_Default);

	TestFalse(TEXT("Fixture starts in the PossessedBy AI state"), (MoveComp->bOrientRotationToMovement != 0));
	TestTrue(TEXT("Fixture starts with controller yaw enabled"), (Character->bUseControllerRotationYaw != 0));

	//# 공격 중 BT 가 회전을 놓은 여러 프레임을 흉내낸다 — 한 프레임이라도 뒤집히면 스핀이 난다
	for (int32 Frame = 0; Frame < 8; ++Frame)
	{
		MoveComp->PhysicsRotation(1.f / 60.f);
	}

	TestFalse(TEXT("AI never gets orient-to-movement forced on"), (MoveComp->bOrientRotationToMovement != 0));
	TestTrue(TEXT("AI keeps controller yaw ownership"), (Character->bUseControllerRotationYaw != 0));

	return true;
}

//# ---------------------------------------------------------------------------
//# 엣지 — AI 에 대해서는 이 함수가 플래그를 "읽기만" 해야 한다. 어느 방향으로도 쓰지 않는다.
//# 타겟이 있는 AI 도 마찬가지다 — 수정 전에는 타겟 분기(cpp:98)가 false 로 강제 대입했다.
//# ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyRotationAINeverWritesFlagTest,
	"SkillProject.Character.Rotation.AINeverWritesFlag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyRotationAINeverWritesFlagTest::RunTest(const FString& Parameters)
{
	//# (1) 타겟 없음 + 플래그가 true 로 들어온 상태 — 강제로 false 로 내리지도 않는다.
	//#     이 입력 상태는 PossessedBy 로는 만들어지지 않는다(그게 바로 버그 상태였다).
	//#     "플래그 소유권이 PhysicsRotation 에 없다"는 것을 양방향으로 못박기 위한 케이스다.
	{
		ASpyCharacter* Character = SpyAIRotationTests::MakeCharacter();
		USpyCharacterMovementComponent* MoveComp = SpyAIRotationTests::GetSpyMovement(Character);
		if (MoveComp == nullptr)
			return false;

		SpyAIRotationTests::AttachTargeting(Character);

		AAIController* AIController = NewObject<AAIController>(GetTransientPackage());
		Character->Controller = AIController;
		MoveComp->CustomMovementMode = static_cast<uint8>(ECustomMovementMode::MOVE_Default);
		MoveComp->bOrientRotationToMovement = true;

		MoveComp->PhysicsRotation(1.f / 60.f);

		TestTrue(TEXT("Untargeted AI keeps an inbound true value"), (MoveComp->bOrientRotationToMovement != 0));
	}

	//# (2) 타겟 있음 + 플래그 true — 수정 전 타겟 분기는 여기서 false 로 덮어썼다.
	//#     이제 AI 는 분기 자체에 들어가지 않으므로 값이 그대로여야 한다.
	{
		ASpyCharacter* Character = SpyAIRotationTests::MakeCharacter();
		USpyCharacterMovementComponent* MoveComp = SpyAIRotationTests::GetSpyMovement(Character);
		if (MoveComp == nullptr)
			return false;

		USpyTargetingManagerComponent* TargetingComp = SpyAIRotationTests::AttachTargeting(Character);

		//# 평범한 AActor 를 타겟으로 준다 — ASpyCharacter 가 아니면 SetCurrentTarget 이
		//# 사망 델리게이트/ASC 정리를 건너뛰고 참조만 저장한다.
		AActor* TargetActor = NewObject<AActor>(GetTransientPackage());
		TargetingComp->SetCurrentTarget(TargetActor);
		TestTrue(TEXT("Target is registered"), TargetingComp->GetTarget().IsValid());

		AAIController* AIController = NewObject<AAIController>(GetTransientPackage());
		Character->Controller = AIController;
		MoveComp->CustomMovementMode = static_cast<uint8>(ECustomMovementMode::MOVE_Default);
		MoveComp->bOrientRotationToMovement = true;

		MoveComp->PhysicsRotation(1.f / 60.f);

		TestTrue(TEXT("Targeted AI is not pushed into the player targeting branch"),
				 (MoveComp->bOrientRotationToMovement != 0));
	}

	//# (3) 타겟 있음 + 플래그 false — PossessedBy 상태 그대로 유지된다.
	//#     수정 전에도 false 였으므로 이 케이스만으로는 회귀를 잡지 못한다. 완결성용 고정이다.
	{
		ASpyCharacter* Character = SpyAIRotationTests::MakeCharacter();
		USpyCharacterMovementComponent* MoveComp = SpyAIRotationTests::GetSpyMovement(Character);
		if (MoveComp == nullptr)
			return false;

		USpyTargetingManagerComponent* TargetingComp = SpyAIRotationTests::AttachTargeting(Character);
		TargetingComp->SetCurrentTarget(NewObject<AActor>(GetTransientPackage()));

		AAIController* AIController = NewObject<AAIController>(GetTransientPackage());
		Character->Controller = AIController;
		SpyAIRotationTests::ApplyPossessedByAISetup(Character);
		MoveComp->CustomMovementMode = static_cast<uint8>(ECustomMovementMode::MOVE_Default);

		MoveComp->PhysicsRotation(1.f / 60.f);

		TestFalse(TEXT("Targeted AI keeps the PossessedBy value"), (MoveComp->bOrientRotationToMovement != 0));
		TestTrue(TEXT("Targeted AI keeps controller yaw ownership"), (Character->bUseControllerRotationYaw != 0));
	}

	return true;
}

//# ---------------------------------------------------------------------------
//# 플레이어 무회귀 — 타겟이 없으면 기존대로 이동 방향 회전을 되살려야 한다.
//# 이 대입이 사라지면 타겟 해제 후 플레이어가 회전하지 않는다 (cpp:104-107 주석).
//# ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyRotationPlayerWithoutTargetTest,
	"SkillProject.Character.Rotation.PlayerWithoutTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyRotationPlayerWithoutTargetTest::RunTest(const FString& Parameters)
{
	ASpyCharacter* Character = SpyAIRotationTests::MakeCharacter();
	USpyCharacterMovementComponent* MoveComp = SpyAIRotationTests::GetSpyMovement(Character);
	if (MoveComp == nullptr)
		return false;

	USpyTargetingManagerComponent* TargetingComp = SpyAIRotationTests::AttachTargeting(Character);
	TestFalse(TEXT("Player starts without a target"), TargetingComp->GetTarget().IsValid());

	APlayerController* PlayerController = NewObject<APlayerController>(GetTransientPackage());
	TestTrue(TEXT("Fixture controller reports as a player controller"), PlayerController->IsPlayerController());

	Character->Controller = PlayerController;
	MoveComp->CustomMovementMode = static_cast<uint8>(ECustomMovementMode::MOVE_Default);

	//# 벽타기 종료 직후처럼 false 로 내려간 상태에서 시작한다
	MoveComp->bOrientRotationToMovement = false;

	MoveComp->PhysicsRotation(1.f / 60.f);

	TestTrue(TEXT("Player without a target regains orient-to-movement"), (MoveComp->bOrientRotationToMovement != 0));

	//# 이 경로는 폰의 컨트롤러 yaw 설정을 건드리지 않는다
	TestFalse(TEXT("Player controller yaw is untouched"), (Character->bUseControllerRotationYaw != 0));

	return true;
}

//# ---------------------------------------------------------------------------
//# 엣지 — 스폰 직후 possess 전 프레임. 크래시하지 않고 플래그도 건드리지 않아야 한다.
//# false / true 양쪽에서 확인해야 "안 건드림" 과 "false 로 강제" 를 구분할 수 있다.
//# ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyRotationNoControllerTest,
	"SkillProject.Character.Rotation.NoController",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyRotationNoControllerTest::RunTest(const FString& Parameters)
{
	ASpyCharacter* Character = SpyAIRotationTests::MakeCharacter();
	USpyCharacterMovementComponent* MoveComp = SpyAIRotationTests::GetSpyMovement(Character);
	if (MoveComp == nullptr)
		return false;

	SpyAIRotationTests::AttachTargeting(Character);
	MoveComp->CustomMovementMode = static_cast<uint8>(ECustomMovementMode::MOVE_Default);

	TestTrue(TEXT("Fixture has no controller yet"), Character->GetController() == nullptr);

	MoveComp->bOrientRotationToMovement = false;
	MoveComp->PhysicsRotation(1.f / 60.f);
	TestFalse(TEXT("Unpossessed pawn keeps a false value"), (MoveComp->bOrientRotationToMovement != 0));

	MoveComp->bOrientRotationToMovement = true;
	MoveComp->PhysicsRotation(1.f / 60.f);
	TestTrue(TEXT("Unpossessed pawn keeps a true value"), (MoveComp->bOrientRotationToMovement != 0));

	//# 타겟팅 컴포넌트가 아직 붙기 전 프레임도 같은 결과여야 한다
	ASpyCharacter* BareCharacter = SpyAIRotationTests::MakeCharacter();
	USpyCharacterMovementComponent* BareMoveComp = SpyAIRotationTests::GetSpyMovement(BareCharacter);
	if (BareMoveComp == nullptr)
		return false;

	BareMoveComp->CustomMovementMode = static_cast<uint8>(ECustomMovementMode::MOVE_Default);
	BareMoveComp->bOrientRotationToMovement = false;
	BareMoveComp->PhysicsRotation(1.f / 60.f);
	TestFalse(TEXT("Pawn without a targeting component is untouched"), (BareMoveComp->bOrientRotationToMovement != 0));

	return true;
}

//# ---------------------------------------------------------------------------
//# 엣지 — 플레이어인데 타겟팅 컴포넌트가 없는 캐릭터(에셋 목록 미포함).
//# 기존 가드(cpp:81-82)가 살아 있는지 고정한다.
//# ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyRotationPlayerWithoutTargetingComponentTest,
	"SkillProject.Character.Rotation.PlayerWithoutTargetingComponent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyRotationPlayerWithoutTargetingComponentTest::RunTest(const FString& Parameters)
{
	ASpyCharacter* Character = SpyAIRotationTests::MakeCharacter();
	USpyCharacterMovementComponent* MoveComp = SpyAIRotationTests::GetSpyMovement(Character);
	if (MoveComp == nullptr)
		return false;

	TestTrue(TEXT("No targeting component attached"),
			 Character->FindComponentByClass<USpyTargetingManagerComponent>() == nullptr);

	APlayerController* PlayerController = NewObject<APlayerController>(GetTransientPackage());
	Character->Controller = PlayerController;
	MoveComp->CustomMovementMode = static_cast<uint8>(ECustomMovementMode::MOVE_Default);

	MoveComp->bOrientRotationToMovement = false;
	MoveComp->PhysicsRotation(1.f / 60.f);
	TestFalse(TEXT("Missing targeting component leaves false untouched"), (MoveComp->bOrientRotationToMovement != 0));

	MoveComp->bOrientRotationToMovement = true;
	MoveComp->PhysicsRotation(1.f / 60.f);
	TestTrue(TEXT("Missing targeting component leaves true untouched"), (MoveComp->bOrientRotationToMovement != 0));

	return true;
}

//# ---------------------------------------------------------------------------
//# 엣지 — 벽타기 커스텀 모드는 회전 처리 전체를 조기 반환한다.
//# 이 조기 반환이 AI 게이트보다 앞에 있으므로 AI/플레이어 모두 플래그가 그대로여야 한다.
//# ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyRotationWallClimbEarlyOutTest,
	"SkillProject.Character.Rotation.WallClimbEarlyOut",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyRotationWallClimbEarlyOutTest::RunTest(const FString& Parameters)
{
	//# 플레이어 — 타겟이 없어도 등반 중에는 orient-to-movement 를 되살리지 않는다
	{
		ASpyCharacter* Character = SpyAIRotationTests::MakeCharacter();
		USpyCharacterMovementComponent* MoveComp = SpyAIRotationTests::GetSpyMovement(Character);
		if (MoveComp == nullptr)
			return false;

		SpyAIRotationTests::AttachTargeting(Character);

		APlayerController* PlayerController = NewObject<APlayerController>(GetTransientPackage());
		Character->Controller = PlayerController;

		MoveComp->CustomMovementMode = static_cast<uint8>(ECustomMovementMode::MOVE_WallClimb);
		MoveComp->bOrientRotationToMovement = false;

		MoveComp->PhysicsRotation(1.f / 60.f);

		TestFalse(TEXT("Wall climb blocks the player targeting branch"), (MoveComp->bOrientRotationToMovement != 0));
	}

	//# AI — 등반 중에도 PossessedBy 설정이 유지된다
	{
		ASpyCharacter* Character = SpyAIRotationTests::MakeCharacter();
		USpyCharacterMovementComponent* MoveComp = SpyAIRotationTests::GetSpyMovement(Character);
		if (MoveComp == nullptr)
			return false;

		SpyAIRotationTests::AttachTargeting(Character);

		AAIController* AIController = NewObject<AAIController>(GetTransientPackage());
		Character->Controller = AIController;
		SpyAIRotationTests::ApplyPossessedByAISetup(Character);

		MoveComp->CustomMovementMode = static_cast<uint8>(ECustomMovementMode::MOVE_WallClimb);

		MoveComp->PhysicsRotation(1.f / 60.f);

		TestFalse(TEXT("Wall climbing AI keeps orient-to-movement off"), (MoveComp->bOrientRotationToMovement != 0));
		TestTrue(TEXT("Wall climbing AI keeps controller yaw ownership"), (Character->bUseControllerRotationYaw != 0));
	}

	return true;
}

#endif //# WITH_DEV_AUTOMATION_TESTS
