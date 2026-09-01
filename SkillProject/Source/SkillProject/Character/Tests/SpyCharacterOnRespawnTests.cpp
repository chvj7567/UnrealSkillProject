// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "Character/SpyCharacter.h"
#include "Character/SpyCharacterAttributeSet.h"
#include "Character/SpyHealthComponent.h"
#include "Character/SpyPawnExtensionComponent.h"

//# ---------------------------------------------------------------------------
//# ASpyCharacter::OnRespawn 단일 진입점 스위트 — 텔레포트 + Health 복구를 직접 검증한다.
//#
//# 범위 노트 — 이 스위트는 "OnRespawn 자체가 무엇을 하는가"만 겨눈다.
//#   - 재활용 봇이 자기 원래 SpawnTransform 으로 복귀하는지(§9 검증 항목 7)는
//#     USpySpawnBotManagerComponent::RespawnBot 이 FSpySpawnedBotInfo 에서 좌표를 꺼내
//#     OnRespawn 에 넘기는 조합까지 봐야 하는데, 그 조합은 SpawnOneBot(GetWorld()->SpawnActor,
//#     GetAuthGameMode() 필수)과 private HandleBotDeath/RespawnBot(GetWorld()->GetTimerManager()
//#     필수, UFUNCTION 도 아니라 리플렉션 경로도 없음)에 묶여 있어 이 월드 없는 티어에서
//#     재현할 수 없다 — "production 코드 추가 요청" 참조.
//#   - AI 로직 재개(RestartAILogic 호출 여부, §9 항목 6 후반)는 GetController() 가 비어 있는
//#     이 픽스처에서 분기 자체가 스킵된다 — ASpyAIController::RestartAILogic 단독 동작은
//#     System/Tests/SpyAIControllerRespawnTests.cpp 가 별도로 다룬다.
//#   - HasAuthority() == false 분기(§9 항목 8)는 AActor 의 net Role 을 월드/NetDriver 없이
//#     안전하게 뒤집을 공개 수단이 없어 재현하지 않는다 — 기본(Authority) 분기만 고정한다.
//#
//# 픽스처는 Character/Tests/SpyHealthComponentTests.cpp 의 MakeWiredCharacter 와 동일한
//# InitializeAbilitySystem 경로를 쓴다(월드 안전성 근거도 동일) — 파일마다 독립된 static
//# 헬퍼를 두는 이 저장소의 기존 관례(SpyLevelTests_MakeConfig 등)를 그대로 따른다.
//# ---------------------------------------------------------------------------

namespace SpyCharacterOnRespawnTests
{
	static ASpyCharacter* MakeWiredCharacter(FAutomationTestBase& Test, float InHealth, float InMaxHealth)
	{
		ASpyCharacter* Character = NewObject<ASpyCharacter>(GetTransientPackage());

		USpyAbilitySystemComponent* ASC = NewObject<USpyAbilitySystemComponent>(Character);
		USpyCharacterAttributeSet* AttributeSet = NewObject<USpyCharacterAttributeSet>(ASC->GetOwner());
		AttributeSet->InitMaxHealth(InMaxHealth);
		AttributeSet->InitHealth(InHealth);
		ASC->AddAttributeSetSubobject(AttributeSet);

		USpyPawnExtensionComponent* Ext = USpyPawnExtensionComponent::FindPawnExtensionComponent(Character);
		if (Ext == nullptr)
		{
			Test.AddError(TEXT("SpyPawnExtensionComponent 를 찾지 못했다"));
			return nullptr;
		}

		Ext->InitializeAbilitySystem(ASC, Character);

		if (Character->GetSpyAbilitySystemComponent() == nullptr)
		{
			Test.AddError(TEXT("InitializeAbilitySystem 이후에도 ASC 를 찾지 못했다"));
			return nullptr;
		}

		if (Character->GetSpyHealthComponent() == nullptr)
		{
			Test.AddError(TEXT("SpyHealthComponent 를 찾지 못했다"));
			return nullptr;
		}

		return Character;
	}
}

//# ---------------------------------------------------------------------------
//# 정상 — OnRespawn 은 주어진 트랜스폼으로 텔레포트한다 (§9 항목 7의 부분집합: 좌표 인자
//# 자체를 정확히 반영하는지. "자기 원래 스폰 지점" 조합은 위 범위 노트 참고).
//# ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyCharacterOnRespawnTeleportsTest,
	"SkillProject.Character.OnRespawn.TeleportsToTransform",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyCharacterOnRespawnTeleportsTest::RunTest(const FString& Parameters)
{
	using namespace SpyCharacterOnRespawnTests;

	ASpyCharacter* Character = MakeWiredCharacter(*this, 1.f, 100.f);
	if (Character == nullptr)
		return false;

	//# 이 픽스처는 컨트롤러가 아직 빙의되지 않은 상태다 — OnRespawn 의
	//# Cast<AAIController>(GetController()) 분기가 안전하게 스킵되는 전제를 여기서 함께 고정한다.
	TestNull(TEXT("Fixture starts without a controller"), Character->GetController());

	const FVector SpawnLocation(500.f, -200.f, 50.f);
	const FRotator SpawnRotation(0.f, 90.f, 0.f);
	const FTransform SpawnTransform(SpawnRotation, SpawnLocation);

	Character->OnRespawn(SpawnTransform);

	TestEqual(TEXT("OnRespawn teleports to the given location"), Character->GetActorLocation(), SpawnLocation);
	TestNearlyEqual(TEXT("OnRespawn teleports to the given yaw"),
		(double)Character->GetActorRotation().Yaw, (double)SpawnRotation.Yaw, 0.01);

	return true;
}

//# ---------------------------------------------------------------------------
//# 정상 — OnRespawn 은 Health 를 MaxHealth 로 복구한다 (§9 항목 6 전반).
//# ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyCharacterOnRespawnRestoresHealthTest,
	"SkillProject.Character.OnRespawn.RestoresHealthToMax",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyCharacterOnRespawnRestoresHealthTest::RunTest(const FString& Parameters)
{
	using namespace SpyCharacterOnRespawnTests;

	ASpyCharacter* Character = MakeWiredCharacter(*this, 1.f, 250.f);
	if (Character == nullptr)
		return false;

	TestEqual(TEXT("Fixture starts near death"), Character->GetSpyHealthComponent()->GetHealth(), 1.f);

	Character->OnRespawn(FTransform::Identity);

	TestEqual(TEXT("OnRespawn restores Health to MaxHealth"), Character->GetSpyHealthComponent()->GetHealth(), 250.f);

	//# Base 값도 함께 확인 — SetNumericAttributeBase 가 실패하면 Current/Base 중 어느 쪽이
	//# 안 맞는지 실패 메시지로 바로 구분된다(현재값만 보면 fixture 아티팩트와 production 버그를
	//# 혼동할 수 있다).
	USpyAbilitySystemComponent* ASC = Character->GetSpyAbilitySystemComponent();
	TestEqual(TEXT("OnRespawn restores the base Health value too"),
		ASC->GetNumericAttributeBase(USpyCharacterAttributeSet::GetHealthAttribute()), 250.f);

	return true;
}

//# ---------------------------------------------------------------------------
//# 회귀 — 재활용된 봇이 여러 생애주기를 거쳐도 OnRespawn 이 매번 안전하게 Health 복구 +
//# 새 트랜스폼 반영을 반복한다 (§9 항목 7 의 "같은 인스턴스가 반복 재사용된다" 전제와 맞닿음).
//# 두 번째 사이클의 "죽기 직전" 상태는 실제 전투 GE 대신 SetNumericAttributeBase 로 직접
//# 만든다 — 이 스위트가 겨누는 것은 OnRespawn 자체의 반복 안전성이지 데미지 파이프라인이 아니다.
//# ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyCharacterOnRespawnRepeatedCyclesTest,
	"SkillProject.Character.OnRespawn.RestoresAcrossRepeatedCycles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyCharacterOnRespawnRepeatedCyclesTest::RunTest(const FString& Parameters)
{
	using namespace SpyCharacterOnRespawnTests;

	ASpyCharacter* Character = MakeWiredCharacter(*this, 1.f, 80.f);
	if (Character == nullptr)
		return false;

	const FTransform FirstSpawn(FRotator::ZeroRotator, FVector(100.f, 0.f, 0.f));
	Character->OnRespawn(FirstSpawn);
	TestEqual(TEXT("Cycle 1 restores Health"), Character->GetSpyHealthComponent()->GetHealth(), 80.f);
	TestEqual(TEXT("Cycle 1 teleports to the first spawn point"), Character->GetActorLocation(), FirstSpawn.GetLocation());

	USpyAbilitySystemComponent* ASC = Character->GetSpyAbilitySystemComponent();
	ASC->SetNumericAttributeBase(USpyCharacterAttributeSet::GetHealthAttribute(), 5.f);
	TestEqual(TEXT("Health drops before the second cycle"), Character->GetSpyHealthComponent()->GetHealth(), 5.f);

	const FTransform SecondSpawn(FRotator(0.f, 180.f, 0.f), FVector(-300.f, 150.f, 0.f));
	Character->OnRespawn(SecondSpawn);
	TestEqual(TEXT("Cycle 2 restores Health again"), Character->GetSpyHealthComponent()->GetHealth(), 80.f);
	TestEqual(TEXT("Cycle 2 teleports to the new spawn point"), Character->GetActorLocation(), SecondSpawn.GetLocation());

	return true;
}

#endif //# WITH_DEV_AUTOMATION_TESTS
