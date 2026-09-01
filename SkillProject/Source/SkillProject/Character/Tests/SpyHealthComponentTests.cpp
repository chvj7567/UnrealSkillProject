// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/CapsuleComponent.h"
#include "Misc/AutomationTest.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "Character/SpyCharacter.h"
#include "Character/SpyCharacterAttributeSet.h"
#include "Character/SpyHealthComponent.h"
#include "Character/SpyPawnExtensionComponent.h"
#include "Character/Tests/SpyHealthComponentTestListener.h"
#include "Util/SpyGameplayTags.h"

//# ---------------------------------------------------------------------------
//# 사망 클램프 / bDead 래치 / 재활용 리스폰 태그 리셋 스위트.
//#
//# 범위 노트 — Health 클램프(SKAttributeSet::PostGameplayEffectExecute, §9 검증 항목 1)는
//# 이 스위트에 없다: 그 클램프는 protected 함수 안, 실제 GameplayEffect 적용 경로에서만
//# 도달하고, 그 경로는 USKAttributeSet::GetWorld()(Outer->GetWorld()) 를 거쳐
//# UAISense_Damage::ReportDamageEvent 를 부르며 EffectCauser->GetActorLocation() 를
//# 널가드 없이 역참조한다(SKAttributeSet.cpp:55-63). 월드 없는 이 티어에서 억지로
//# 재현하면 사용자 에디터가 죽을 위험이 실측 없이 크다 — 시도하지 않는다.
//# 실제 월드 + Possess 통합 티어를 의도적으로 만들지 않는다는 이 저장소의 기존 결정
//# (SpyCharacterAIRotationTests.cpp:36-39)을 그대로 따른 것이다. gameplay-programmer 에게
//# 보고하는 production 코드 요청 사항으로 남긴다 (본 보고서 "production 코드 추가 요청" 참조).
//#
//# 픽스처 근거 — InitializeAbilitySystem 을 실제로 호출해 프로덕션과 동일한 배선을 만든다:
//#   USpyPawnExtensionComponent::InitializeAbilitySystem(ASC, Owner) 는
//#   ASpyCharacter::OnAbilitySystemInitialized() 를 그대로 트리거해
//#   SpyHealthComponent->InitializeByAbilitySystem 과 태그 이벤트 구독
//#   (HandleCharacterDeathTagChanged) 을 실제 프로덕션 순서대로 배선한다.
//#   이 경로 전체를 추적한 결과 GetWorld() 를 건드리는 지점이 없어 월드 없이도 안전하다.
//# ---------------------------------------------------------------------------

namespace SpyHealthComponentTests
{
	//# ASpyCharacter + ASC + AttributeSet 를 실제 InitializeAbilitySystem 경로로 완전히 배선한다.
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

		if (Character->GetSpyAbilitySystemComponent()->GetSet<USpyCharacterAttributeSet>() == nullptr)
		{
			Test.AddError(TEXT("AttributeSet 이 ASC 에 등록되지 않았다"));
			return nullptr;
		}

		if (Character->GetSpyHealthComponent() == nullptr)
		{
			Test.AddError(TEXT("SpyHealthComponent 를 찾지 못했다"));
			return nullptr;
		}

		return Character;
	}

	//# HealthSet->OnHealthChanged 는 public + mutable 델리게이트라 리플렉션 없이 직접 발화할 수 있다
	//# (USpyHealthComponent::InitializeByAbilitySystem 이 이 델리게이트에 HandleHealthChanged 를 바인딩).
	//# DamageEffectSpec = nullptr 로 넘기면 HandleHealthChanged 가 카메라 쉐이크 등 컨트롤러/RPC
	//# 의존 분기에 도달하기 전에 조기 반환한다(SpyHealthComponent.cpp:93) — 이 스위트가 검증하는
	//# 것은 bDead 래치/OnDeath 발화 그 자체이지 카메라 연출이 아니다.
	static void BroadcastHealthReachesZero(ASpyCharacter* Character)
	{
		const USpyCharacterAttributeSet* AttributeSet =
			Character->GetSpyAbilitySystemComponent()->GetSet<USpyCharacterAttributeSet>();

		AttributeSet->OnHealthChanged.Broadcast(nullptr, nullptr, nullptr, -100.f, 100.f, 0.f);
	}
}

//# ---------------------------------------------------------------------------
//# 정상 + 엣지 — 사망 후 추가 피해가 들어와도 OnDeath 는 1회만 발화한다 (§2-3 bDead 래치 계약).
//# ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyHealthComponentDeathLatchOnceTest,
	"SkillProject.Character.HealthComponent.DeathLatchFiresOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyHealthComponentDeathLatchOnceTest::RunTest(const FString& Parameters)
{
	using namespace SpyHealthComponentTests;

	ASpyCharacter* Character = MakeWiredCharacter(*this, 100.f, 100.f);
	if (Character == nullptr)
		return false;

	USpyHealthComponentTestListener* Listener = NewObject<USpyHealthComponentTestListener>();
	Character->GetSpyHealthComponent()->OnDeath.AddDynamic(Listener, &USpyHealthComponentTestListener::HandleDeath);

	UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	TestTrue(TEXT("Fixture starts with a blocking capsule"),
		Capsule->GetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn) == ECollisionResponse::ECR_Block);

	BroadcastHealthReachesZero(Character);
	TestEqual(TEXT("First death broadcasts OnDeath once"), Listener->DeathCallCount, 1);
	TestTrue(TEXT("OnDeath side effect ignores the pawn collision channel"),
		Capsule->GetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn) == ECollisionResponse::ECR_Ignore);

	//# 엣지 — 사망 후 추가 피해(리소스 재사용 중 상태 잔존 방어). 두 번 더 반복해도 늘지 않는다.
	BroadcastHealthReachesZero(Character);
	BroadcastHealthReachesZero(Character);
	TestEqual(TEXT("Repeated damage after death does not refire OnDeath"), Listener->DeathCallCount, 1);

	return true;
}

//# ---------------------------------------------------------------------------
//# 핵심 — 재활용된 봇의 2회차 이상 사망에서도 OnDeath 가 정확히 1회씩 다시 발화하는지
//# (code-reviewer 2차 검토 지적). Character.State.Death 태그가 0으로 돌아오는 시점에
//# 캡슐 콜리전이 ECR_Block 으로 복구되는지도 함께 고정한다(code-reviewer 2차 검토 지적).
//#
//# 태그 전이는 SetLooseGameplayTagCount 로 직접 시뮬레이션한다 — 실제 프로덕션에서는
//# GA_Death 의 ActivationOwnedTags/CancelAbilities(OnRespawn)가 이 전이를 일으키지만,
//# 그 GA 활성/취소 경로 자체는 GAS 엔진 표준 동작이라 이 스위트가 재검증하지 않는다.
//# 검증 대상은 "태그 카운트가 0으로 돌아오면 HandleCharacterDeathTagChanged 가 콜리전
//# 복구 + bDead 래치 해제를 실제로 수행하는가"이다.
//# ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyHealthComponentDeathLatchResetsOnTagClearedTest,
	"SkillProject.Character.HealthComponent.DeathLatchResetsOnTagCleared",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyHealthComponentDeathLatchResetsOnTagClearedTest::RunTest(const FString& Parameters)
{
	using namespace SpyHealthComponentTests;

	ASpyCharacter* Character = MakeWiredCharacter(*this, 100.f, 100.f);
	if (Character == nullptr)
		return false;

	USpyHealthComponentTestListener* Listener = NewObject<USpyHealthComponentTestListener>();
	Character->GetSpyHealthComponent()->OnDeath.AddDynamic(Listener, &USpyHealthComponentTestListener::HandleDeath);

	USpyAbilitySystemComponent* ASC = Character->GetSpyAbilitySystemComponent();
	UCapsuleComponent* Capsule = Character->GetCapsuleComponent();

	//# 1회차 사망
	BroadcastHealthReachesZero(Character);
	TestEqual(TEXT("Cycle 1 death count"), Listener->DeathCallCount, 1);
	TestTrue(TEXT("Cycle 1 collision ignores the pawn channel"),
		Capsule->GetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn) == ECollisionResponse::ECR_Ignore);

	//# GA_Death 활성 상태(ActivationOwnedTags)를 흉내낸다 — 태그가 남아있는 동안은 복구되지 않는다.
	ASC->SetLooseGameplayTagCount(SKGameplayTags::Character_State_Death, 1);
	TestTrue(TEXT("Tag still set — collision stays ignored"),
		Capsule->GetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn) == ECollisionResponse::ECR_Ignore);

	//# 태그가 0으로 돌아오는 순간(OnRespawn 의 CancelAbilities 가 GA_Death 를 종료시키는 것과
	//# 동일한 신호) — 콜리전 복구 + bDead 래치 해제가 함께 일어난다.
	ASC->SetLooseGameplayTagCount(SKGameplayTags::Character_State_Death, 0);
	TestTrue(TEXT("Tag cleared — collision restores to block"),
		Capsule->GetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn) == ECollisionResponse::ECR_Block);

	//# 재활용된 봇의 2회차 사망 — 래치가 리셋됐으므로 OnDeath 가 다시 발화해야 한다.
	BroadcastHealthReachesZero(Character);
	TestEqual(TEXT("Cycle 2 death fires OnDeath again"), Listener->DeathCallCount, 2);
	TestTrue(TEXT("Cycle 2 collision ignores the pawn channel again"),
		Capsule->GetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn) == ECollisionResponse::ECR_Ignore);

	//# 3회차까지 반복해 "1회 리셋 우연"이 아니라 매 사이클 성립함을 고정한다.
	ASC->SetLooseGameplayTagCount(SKGameplayTags::Character_State_Death, 1);
	ASC->SetLooseGameplayTagCount(SKGameplayTags::Character_State_Death, 0);
	BroadcastHealthReachesZero(Character);
	TestEqual(TEXT("Cycle 3 death fires OnDeath a third time"), Listener->DeathCallCount, 3);

	return true;
}

//# ---------------------------------------------------------------------------
//# 엣지 — 동시 발생. 두 소스가 동시에 Character.State.Death 태그를 걸었다고 가정하면
//# 카운트가 2에서 1로 내려가도(하나만 해제) 아직 남은 소스가 있으므로 복구되면 안 된다.
//# HandleCharacterDeathTagChanged 의 "NewCount > 0 이면 조기 반환" 분기를 정확히 겨눈다.
//# ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyHealthComponentCollisionStaysBlockedWhileTagCountAboveOneTest,
	"SkillProject.Character.HealthComponent.CollisionStaysBlockedWhileTagCountAboveOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyHealthComponentCollisionStaysBlockedWhileTagCountAboveOneTest::RunTest(const FString& Parameters)
{
	using namespace SpyHealthComponentTests;

	ASpyCharacter* Character = MakeWiredCharacter(*this, 100.f, 100.f);
	if (Character == nullptr)
		return false;

	USpyAbilitySystemComponent* ASC = Character->GetSpyAbilitySystemComponent();
	UCapsuleComponent* Capsule = Character->GetCapsuleComponent();

	BroadcastHealthReachesZero(Character);

	ASC->SetLooseGameplayTagCount(SKGameplayTags::Character_State_Death, 2);
	ASC->SetLooseGameplayTagCount(SKGameplayTags::Character_State_Death, 1);
	TestTrue(TEXT("One remaining tag source still blocks the collision restore"),
		Capsule->GetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn) == ECollisionResponse::ECR_Ignore);

	ASC->SetLooseGameplayTagCount(SKGameplayTags::Character_State_Death, 0);
	TestTrue(TEXT("Last tag source cleared — collision restores"),
		Capsule->GetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn) == ECollisionResponse::ECR_Block);

	return true;
}

#endif //# WITH_DEV_AUTOMATION_TESTS
