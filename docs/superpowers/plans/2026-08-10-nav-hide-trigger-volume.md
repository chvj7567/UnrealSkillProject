# 미션 타겟 네비 숨김 트리거 영역 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 미션 타겟 액터(`ASpyMissionTargetPoint`/`ASpyNPCCharacter`/`ASpyInteractableObject`)에 선택적 박스 트리거 볼륨을 추가해, 디자이너가 배치한 영역에 플레이어가 들어오면 `USpyNavigationComponent`가 거리 계산 없이 즉시 네비 라인을 숨기고, 나가면 즉시 다시 표시하게 한다.

**Architecture:** `USpyMissionTargetRegistrySubsystem`에 타겟 액터 자체를 조회하는 API를 추가하고, 세 타겟 액터가 공용 인터페이스 `ISpyMissionTargetHideVolume`(트리거 컴포넌트를 노출)를 구현한다. `USpyNavigationComponent`는 타겟을 새로 확정할 때마다 이 인터페이스로 트리거 컴포넌트를 얻어 오버랩 델리게이트를 직접 구독하고, 오버랩 상태(`bInsideHideTrigger`)가 켜져 있는 동안은 기존 NavMesh 거리 히스테리시스 계산 자체를 건너뛴다. 트리거 미사용 타겟(기본값)은 인터페이스가 `nullptr`을 반환해 기존 로직으로 완전히 폴백한다.

**Tech Stack:** Unreal Engine 5.7, C++, Unreal Automation(SimpleAutomationTest). 이 프로젝트는 전용 CLI 테스트 러너가 없다 — 컴파일은 Visual Studio(Ctrl+Shift+B) 또는 에디터 Live Coding(Ctrl+Alt+F11), 테스트 실행은 에디터 Window > Test Automation(Session Frontend) > Automation 탭에서 필터 문자열로 검색 후 Start Tests.

## Global Constraints

- 참조 스펙: `docs/superpowers/specs/2026-08-10-nav-hide-trigger-volume-design.md`
- `.h`/`.cpp` 한 줄이라도 만지면 `.claude/rules/cpp-style.md` §1~7(문법, 예외 없음) 준수: 한 줄 주석은 `//#`만, `auto`/`!` 단항부정 금지, 가드 절은 중괄호 없이 개행, `TObjectPtr<>` 사용, UPROPERTY 지정자 명시.
- 헤더 include 순서: 자기 자신 → UE 헤더 → 프로젝트 헤더 → `.generated.h`(항상 마지막).
- 매직 넘버 금지(§15) — 새 수치 상수는 이름 있는 상수 또는 `EditAnywhere`/`EditDefaultsOnly`로 노출.
- 서버 권한 로직 아님 — `USpyNavigationComponent`와 신규 트리거 모두 로컬 클라이언트 전용 연출(레플리케이션 없음), `InteractionSphere`와 동일한 로컬 콜리전 판정 패턴을 따른다.
- 테스트 경로: `SkillProject/Source/SkillProject/**/Tests/`, 파일 전체 `#if WITH_DEV_AUTOMATION_TESTS`로 감쌈, 구조체명 `F<Domain><Case>Test`, 등록 문자열 `"SkillProject.도메인.기능.케이스"` (기존 `SpyNavigationComponentTests.cpp`/`SpyMissionTargetRegistrySubsystemTests.cpp` 그대로 따름).
- 커밋은 각 태스크 끝에서 `git add`로 스테이징까지만 하고 커밋 메시지(안)를 제시한다 — **`git commit` 직접 실행 금지** (git-conventions.md, 프로젝트 규칙). 아래 각 태스크의 "Step: Commit"은 `git add` + 메시지 제안으로 읽는다.
- `.claude/.active-sessions.md`, `docs/automation-guide.html` 등 이번 작업과 무관한 기존 변경분은 건드리지 않는다(스테이징 범위에서 제외).

---

### Task 1: `USpyMissionTargetRegistrySubsystem` — 타겟 액터 조회 API

**Files:**
- Modify: `SkillProject/Source/SkillProject/System/SpyMissionTargetRegistrySubsystem.h`
- Modify: `SkillProject/Source/SkillProject/System/SpyMissionTargetRegistrySubsystem.cpp`
- Test: `SkillProject/Source/SkillProject/System/Tests/SpyMissionTargetRegistrySubsystemTests.cpp`

**Interfaces:**
- Produces: `AActor* USpyMissionTargetRegistrySubsystem::FindNPCActor(int32 NPCId) const`, `AActor* USpyMissionTargetRegistrySubsystem::FindMissionTargetActor(FGameplayTag InTag) const` — Task 5(`USpyNavigationComponent`)가 소비한다.

- [ ] **Step 1: 실패하는 테스트 작성**

`SpyMissionTargetRegistrySubsystemTests.cpp` 맨 아래(`#endif` 바로 위)에 추가:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionTargetRegistryFindNPCActorSucceedsTest,
	"SkillProject.System.MissionTargetRegistry.FindNPCActorSucceeds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionTargetRegistryFindNPCActorSucceedsTest::RunTest(const FString& Parameters)
{
	AActor* NPCActor = SpyMissionTargetRegistrySubsystemTests_MakeLocatedActor(FVector(1.f, 2.f, 3.f));

	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	Registry->RegisterNPCLocation(7, NPCActor);

	TestEqual(TEXT("FindNPCActor returns the exact registered actor"), Registry->FindNPCActor(7), NPCActor);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionTargetRegistryFindNPCActorUnknownFailsTest,
	"SkillProject.System.MissionTargetRegistry.FindNPCActorUnknownFails",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionTargetRegistryFindNPCActorUnknownFailsTest::RunTest(const FString& Parameters)
{
	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();

	TestNull(TEXT("Unregistered NPCId returns nullptr"), Registry->FindNPCActor(9999));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionTargetRegistryFindMissionTargetActorSucceedsTest,
	"SkillProject.System.MissionTargetRegistry.FindMissionTargetActorSucceeds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionTargetRegistryFindMissionTargetActorSucceedsTest::RunTest(const FString& Parameters)
{
	AActor* MarkerActor = SpyMissionTargetRegistrySubsystemTests_MakeLocatedActor(FVector(4.f, 5.f, 6.f));

	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	Registry->RegisterMissionTargetLocation(SpyGameplayTags::Skill_Move_Vault, MarkerActor);

	TestEqual(TEXT("FindMissionTargetActor returns the exact registered actor"), Registry->FindMissionTargetActor(SpyGameplayTags::Skill_Move_Vault), MarkerActor);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionTargetRegistryFindMissionTargetActorUnknownFailsTest,
	"SkillProject.System.MissionTargetRegistry.FindMissionTargetActorUnknownFails",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionTargetRegistryFindMissionTargetActorUnknownFailsTest::RunTest(const FString& Parameters)
{
	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();

	TestNull(TEXT("Unregistered tag returns nullptr"), Registry->FindMissionTargetActor(SpyGameplayTags::Skill_Move_Vault));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionTargetRegistryFindMissionTargetActorStaleFailsTest,
	"SkillProject.System.MissionTargetRegistry.FindMissionTargetActorStaleFails",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionTargetRegistryFindMissionTargetActorStaleFailsTest::RunTest(const FString& Parameters)
{
	//# FindMissionTargetLocation 의 기존 StaleActorFailsFind 테스트와 동일 근거 — 액터 조회 API 도
	//# TWeakObjectPtr::IsValid() 로 동일하게 방어해야 한다
	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	AActor* MarkerActor = SpyMissionTargetRegistrySubsystemTests_MakeLocatedActor(FVector(5.f, 5.f, 5.f));
	Registry->RegisterMissionTargetLocation(SpyGameplayTags::Skill_Move_Vault, MarkerActor);

	MarkerActor->MarkAsGarbage();

	TestNull(TEXT("FindMissionTargetActor returns nullptr once the registered actor is garbage"), Registry->FindMissionTargetActor(SpyGameplayTags::Skill_Move_Vault));

	return true;
}
```

- [ ] **Step 2: 컴파일 실패 확인**

Run: Visual Studio에서 `SkillProject` 빌드(Ctrl+Shift+B).
Expected: `FindNPCActor`/`FindMissionTargetActor`가 `USpyMissionTargetRegistrySubsystem`에 없다는 컴파일 에러.

- [ ] **Step 3: 최소 구현**

`SpyMissionTargetRegistrySubsystem.h`의 `FindMissionTargetLocation` 선언 바로 아래(§`private:` 위)에 추가:

```cpp
	//# HideTriggerVolume 조회용(design 2026-08-10 §3) — 좌표가 아니라 액터 자체가 필요한
	//# 소비자(IMissionTargetHideVolume 캐스팅용, Task 5)를 위한 API.
	AActor* FindNPCActor(int32 NPCId) const;
	AActor* FindMissionTargetActor(FGameplayTag InTag) const;
```

`SpyMissionTargetRegistrySubsystem.cpp`의 `FindMissionTargetLocation` 구현부 아래에 추가:

```cpp
AActor* USpyMissionTargetRegistrySubsystem::FindNPCActor(int32 NPCId) const
{
	const TWeakObjectPtr<AActor>* Found = NPCLocations.Find(NPCId);
	if (Found == nullptr || Found->IsValid() == false)
		return nullptr;

	return Found->Get();
}

AActor* USpyMissionTargetRegistrySubsystem::FindMissionTargetActor(FGameplayTag InTag) const
{
	const TWeakObjectPtr<AActor>* Found = MissionTargetLocations.Find(InTag);
	if (Found == nullptr || Found->IsValid() == false)
		return nullptr;

	return Found->Get();
}
```

- [ ] **Step 4: 컴파일 + 테스트 통과 확인**

Run: Visual Studio 빌드 후 에디터 실행 → Window > Test Automation → Automation 탭에서 `SkillProject.System.MissionTargetRegistry.Find` 필터 검색 → 5개 신규 테스트 체크 → Start Tests.
Expected: 5개 모두 PASS, 기존 `MissionTargetRegistry.*` 테스트도 회귀 없이 PASS.

- [ ] **Step 5: Commit**

```bash
git add SkillProject/Source/SkillProject/System/SpyMissionTargetRegistrySubsystem.h SkillProject/Source/SkillProject/System/SpyMissionTargetRegistrySubsystem.cpp SkillProject/Source/SkillProject/System/Tests/SpyMissionTargetRegistrySubsystemTests.cpp
```
제안 메시지: `[Feature] USpyMissionTargetRegistrySubsystem — 타겟 액터 조회 API(FindNPCActor/FindMissionTargetActor) 추가`

---

### Task 2: `ISpyMissionTargetHideVolume` 인터페이스 + `ASpyMissionTargetPoint` 구현

**Files:**
- Create: `SkillProject/Source/SkillProject/System/CommonInterface.System.h`
- Modify: `SkillProject/Source/SkillProject/Navigation/SpyMissionTargetPoint.h`
- Modify: `SkillProject/Source/SkillProject/Navigation/SpyMissionTargetPoint.cpp`
- Create: `SkillProject/Source/SkillProject/Navigation/Tests/SpyMissionTargetPointTests.cpp`

**Interfaces:**
- Produces: `class ISpyMissionTargetHideVolume { virtual UPrimitiveComponent* GetHideTriggerComponent() const = 0; }` — Task 3, 4, 5가 소비한다.
- Produces: `ASpyMissionTargetPoint`가 `ISpyMissionTargetHideVolume` 구현, `bool bEnableHideTrigger`(기본 false), `UBoxComponent* HideTriggerVolume` 보유.

- [ ] **Step 1: 인터페이스 헤더 작성**

`SkillProject/Source/SkillProject/System/CommonInterface.System.h` 신규 작성:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "CommonInterface.System.generated.h"

class UPrimitiveComponent;

//# 미션 타겟 액터(ASpyMissionTargetPoint/ASpyNPCCharacter/ASpyInteractableObject)가 선택적으로
//# 노출하는 네비 숨김 트리거 볼륨(design 2026-08-10 §4). USpyNavigationComponent 는 이 인터페이스로만
//# 접근하고 구체 타겟 액터 타입을 알지 않는다(cpp-style §8·§10).
UINTERFACE(MinimalAPI)
class USpyMissionTargetHideVolume : public UInterface
{
	GENERATED_BODY()
};

class ISpyMissionTargetHideVolume
{
	GENERATED_BODY()

public:
	//# 트리거 비활성 인스턴스는 nullptr 반환 — 호출부는 이를 "거리 히스테리시스로 폴백" 신호로 해석한다.
	virtual UPrimitiveComponent* GetHideTriggerComponent() const = 0;
};
```

- [ ] **Step 2: 실패하는 테스트 작성**

`SkillProject/Source/SkillProject/Navigation/Tests/SpyMissionTargetPointTests.cpp` 신규 작성:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"
#include "Navigation/SpyMissionTargetPoint.h"
#include "System/CommonInterface.System.h"

//# ASpyMissionTargetPoint 는 InteractionSphere/오버랩 콜백이 없는 순수 마커 액터라
//# NewObject 생성자 시점 기본값만으로 인터페이스 계약을 안전하게 검증할 수 있다
//# (Interactable/Tests/SpyInteractableObjectTests.cpp:26 과 동일 근거).

static void SpyMissionTargetPointTests_SetHideTriggerEnabled(ASpyMissionTargetPoint* Target, bool bEnabled)
{
	FBoolProperty* Prop = FindFProperty<FBoolProperty>(ASpyMissionTargetPoint::StaticClass(), TEXT("bEnableHideTrigger"));
	check(Prop != nullptr);
	Prop->SetPropertyValue_InContainer(Target, bEnabled);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionTargetPointImplementsHideVolumeTest,
	"SkillProject.Navigation.MissionTargetPoint.ImplementsHideVolume",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionTargetPointImplementsHideVolumeTest::RunTest(const FString& Parameters)
{
	ASpyMissionTargetPoint* Target = NewObject<ASpyMissionTargetPoint>();

	TestTrue(TEXT("ASpyMissionTargetPoint implements ISpyMissionTargetHideVolume"),
			 Target->GetClass()->ImplementsInterface(USpyMissionTargetHideVolume::StaticClass()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionTargetPointHideTriggerDisabledByDefaultTest,
	"SkillProject.Navigation.MissionTargetPoint.HideTriggerDisabledByDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionTargetPointHideTriggerDisabledByDefaultTest::RunTest(const FString& Parameters)
{
	//# 하위 호환(design §5) — 기존에 배치된 모든 마커는 bEnableHideTrigger 기본값 false 라
	//# GetHideTriggerComponent() 가 nullptr 을 반환해 기존 거리 히스테리시스로 폴백해야 한다
	ASpyMissionTargetPoint* Target = NewObject<ASpyMissionTargetPoint>();
	ISpyMissionTargetHideVolume* HideVolume = Cast<ISpyMissionTargetHideVolume>(Target);

	TestNotNull(TEXT("Cast to the interface succeeds"), HideVolume);
	TestNull(TEXT("GetHideTriggerComponent() is nullptr when bEnableHideTrigger is false"), HideVolume->GetHideTriggerComponent());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionTargetPointHideTriggerEnabledReturnsComponentTest,
	"SkillProject.Navigation.MissionTargetPoint.HideTriggerEnabledReturnsComponent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionTargetPointHideTriggerEnabledReturnsComponentTest::RunTest(const FString& Parameters)
{
	ASpyMissionTargetPoint* Target = NewObject<ASpyMissionTargetPoint>();
	SpyMissionTargetPointTests_SetHideTriggerEnabled(Target, true);

	ISpyMissionTargetHideVolume* HideVolume = Cast<ISpyMissionTargetHideVolume>(Target);
	TestNotNull(TEXT("Cast to the interface succeeds"), HideVolume);
	TestNotNull(TEXT("GetHideTriggerComponent() returns a valid component when bEnableHideTrigger is true"), HideVolume->GetHideTriggerComponent());

	return true;
}

#endif //# WITH_DEV_AUTOMATION_TESTS
```

- [ ] **Step 3: 컴파일 실패 확인**

Run: Visual Studio 빌드.
Expected: `ASpyMissionTargetPoint`가 `ISpyMissionTargetHideVolume`를 구현하지 않아 `Cast<ISpyMissionTargetHideVolume>`이 항상 실패하거나(테스트 자체는 컴파일은 되지만 1·3번 테스트가 FAIL), `bEnableHideTrigger` 프로퍼티가 없어 `FindFProperty` 결과가 `nullptr`이라 `check()`가 assert.

- [ ] **Step 4: 최소 구현 — 헤더**

`SpyMissionTargetPoint.h` 전체를 아래로 교체:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "System/CommonInterface.System.h"

#include "SpyMissionTargetPoint.generated.h"

class USceneComponent;
class UBillboardComponent;
class UBoxComponent;
class UPrimitiveComponent;

//# Vault/Climb/GrappleHook 전용 경량 구역 마커(design §5-3·§5-4). 레벨 디자이너가 구역
//# 안쪽(NPC 위치가 아닌 지점)에 배치한다 — 특정 오브젝트가 아니라 진입 안내일 뿐이다(§5-3).
UCLASS()
class SKILLPROJECT_API ASpyMissionTargetPoint : public AActor, public ISpyMissionTargetHideVolume
{
	GENERATED_BODY()

public:
	ASpyMissionTargetPoint();

	//# ISpyMissionTargetHideVolume
	virtual UPrimitiveComponent* GetHideTriggerComponent() const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	//# 대응 미션의 FSpyMissionRow.MatchTag 와 정확히 일치시켜 배치한다(leaf 태그, §8)
	UPROPERTY(EditAnywhere, Category = "Navigation")
	FGameplayTag TargetMissionTag;

	//# design 2026-08-10 §5 — 기본 false: 기존 배치 액터는 거리 히스테리시스로 그대로 동작(하위 호환)
	UPROPERTY(EditAnywhere, Category = "Navigation")
	bool bEnableHideTrigger = false;

private:
	//# RootComponent 필수(code-reviewer BLOCKER) — 없으면 AActor::GetActorLocation() 이
	//# 항상 원점을 반환해 레벨 배치 좌표가 레지스트리에 전혀 반영되지 않는다.
	UPROPERTY(VisibleAnywhere, Category = "Navigation")
	TObjectPtr<USceneComponent> RootScene;

	//# 항상 생성하되 bEnableHideTrigger 로 콜리전 on/off — 디자이너가 에디터에서 Extent/회전을
	//# 직접 드래그해 영역을 그린다(design 2026-08-10 §5)
	UPROPERTY(VisibleAnywhere, Category = "Navigation")
	TObjectPtr<UBoxComponent> HideTriggerVolume;

#if WITH_EDITORONLY_DATA
	//# 에디터 뷰포트 가시성 전용(design §5-4 "권장" 항목) — 런타임 렌더링 없음, 자동으로 스트립된다.
	UPROPERTY()
	TObjectPtr<UBillboardComponent> EditorBillboard;
#endif
};
```

- [ ] **Step 5: 최소 구현 — cpp**

`SpyMissionTargetPoint.cpp` 전체를 아래로 교체:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "Navigation/SpyMissionTargetPoint.h"
#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "System/SpyMissionTargetRegistrySubsystem.h"

ASpyMissionTargetPoint::ASpyMissionTargetPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	HideTriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("HideTriggerVolume"));
	HideTriggerVolume->SetupAttachment(RootScene);
	HideTriggerVolume->SetBoxExtent(FVector(200.f, 200.f, 100.f));
	HideTriggerVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);

#if WITH_EDITORONLY_DATA
	//# 에디터 전용 서브오브젝트 — WITH_EDITOR 빌드에서만 생성되고 나머지 빌드에선 자동 스트립된다
	EditorBillboard = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("EditorBillboard"));
	if (EditorBillboard != nullptr)
		EditorBillboard->SetupAttachment(RootScene);
#endif
}

void ASpyMissionTargetPoint::BeginPlay()
{
	Super::BeginPlay();

	//# InteractionSphere(ASpyInteractableObject/ASpyNPCCharacter)와 동일한 콜리전 설정 —
	//# 오버랩 델리게이트 구독은 USpyNavigationComponent 가 직접 한다(Task 5)
	if (bEnableHideTrigger)
		HideTriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	if (TargetMissionTag.IsValid() == false)
		return;

	UWorld* World = GetWorld();
	if (World == nullptr)
		return;

	if (USpyMissionTargetRegistrySubsystem* Registry = World->GetSubsystem<USpyMissionTargetRegistrySubsystem>())
		Registry->RegisterMissionTargetLocation(TargetMissionTag, this);
}

void ASpyMissionTargetPoint::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (TargetMissionTag.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			if (USpyMissionTargetRegistrySubsystem* Registry = World->GetSubsystem<USpyMissionTargetRegistrySubsystem>())
				Registry->UnregisterMissionTargetLocation(TargetMissionTag, this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

UPrimitiveComponent* ASpyMissionTargetPoint::GetHideTriggerComponent() const
{
	return bEnableHideTrigger ? HideTriggerVolume : nullptr;
}
```

- [ ] **Step 6: 컴파일 + 테스트 통과 확인**

Run: Visual Studio 빌드 후 에디터 Automation 탭에서 `SkillProject.Navigation.MissionTargetPoint` 필터 검색 → 3개 테스트 Start Tests.
Expected: 3개 모두 PASS. 기존 `SkillProject.Navigation.Component.*`(`SpyNavigationComponentTests.cpp`) 테스트도 회귀 없이 PASS(이 태스크는 그 파일을 건드리지 않았으므로 그대로 통과해야 한다).

- [ ] **Step 7: Commit**

```bash
git add SkillProject/Source/SkillProject/System/CommonInterface.System.h SkillProject/Source/SkillProject/Navigation/SpyMissionTargetPoint.h SkillProject/Source/SkillProject/Navigation/SpyMissionTargetPoint.cpp SkillProject/Source/SkillProject/Navigation/Tests/SpyMissionTargetPointTests.cpp
```
제안 메시지: `[Feature] ASpyMissionTargetPoint — 네비 숨김 트리거 볼륨(ISpyMissionTargetHideVolume) 추가`

---

### Task 3: `ASpyNPCCharacter` — 트리거 볼륨 구현

**Files:**
- Modify: `SkillProject/Source/SkillProject/NPC/SpyNPCCharacter.h`
- Modify: `SkillProject/Source/SkillProject/NPC/SpyNPCCharacter.cpp`
- Create: `SkillProject/Source/SkillProject/NPC/Tests/SpyNPCCharacterHideVolumeTests.cpp`

**Interfaces:**
- Consumes: `ISpyMissionTargetHideVolume`(Task 2, `System/CommonInterface.System.h`).
- Produces: `ASpyNPCCharacter`가 `ISpyMissionTargetHideVolume` 구현.

- [ ] **Step 1: 실패하는 테스트 작성**

`SkillProject/Source/SkillProject/NPC/Tests/SpyNPCCharacterHideVolumeTests.cpp` 신규 작성:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"
#include "NPC/SpyNPCCharacter.h"
#include "System/CommonInterface.System.h"

//# InteractionSphere/오버랩 콜백처럼 물리·월드가 필요한 경로는 대상 밖이다
//# (Interactable/Tests/SpyInteractableObjectTests.cpp:12 와 동일 근거). 생성자 시점
//# 인터페이스 계약과 bEnableHideTrigger 토글만 검증한다.

static void SpyNPCCharacterHideVolumeTests_SetHideTriggerEnabled(ASpyNPCCharacter* NPC, bool bEnabled)
{
	FBoolProperty* Prop = FindFProperty<FBoolProperty>(ASpyNPCCharacter::StaticClass(), TEXT("bEnableHideTrigger"));
	check(Prop != nullptr);
	Prop->SetPropertyValue_InContainer(NPC, bEnabled);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNPCCharacterImplementsHideVolumeTest,
	"SkillProject.NPC.Character.ImplementsHideVolume",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNPCCharacterImplementsHideVolumeTest::RunTest(const FString& Parameters)
{
	ASpyNPCCharacter* NPC = NewObject<ASpyNPCCharacter>();

	TestTrue(TEXT("ASpyNPCCharacter implements ISpyMissionTargetHideVolume"),
			 NPC->GetClass()->ImplementsInterface(USpyMissionTargetHideVolume::StaticClass()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNPCCharacterHideTriggerDisabledByDefaultTest,
	"SkillProject.NPC.Character.HideTriggerDisabledByDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNPCCharacterHideTriggerDisabledByDefaultTest::RunTest(const FString& Parameters)
{
	ASpyNPCCharacter* NPC = NewObject<ASpyNPCCharacter>();
	ISpyMissionTargetHideVolume* HideVolume = Cast<ISpyMissionTargetHideVolume>(NPC);

	TestNotNull(TEXT("Cast to the interface succeeds"), HideVolume);
	TestNull(TEXT("GetHideTriggerComponent() is nullptr when bEnableHideTrigger is false"), HideVolume->GetHideTriggerComponent());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNPCCharacterHideTriggerEnabledReturnsComponentTest,
	"SkillProject.NPC.Character.HideTriggerEnabledReturnsComponent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNPCCharacterHideTriggerEnabledReturnsComponentTest::RunTest(const FString& Parameters)
{
	ASpyNPCCharacter* NPC = NewObject<ASpyNPCCharacter>();
	SpyNPCCharacterHideVolumeTests_SetHideTriggerEnabled(NPC, true);

	ISpyMissionTargetHideVolume* HideVolume = Cast<ISpyMissionTargetHideVolume>(NPC);
	TestNotNull(TEXT("Cast to the interface succeeds"), HideVolume);
	TestNotNull(TEXT("GetHideTriggerComponent() returns a valid component when bEnableHideTrigger is true"), HideVolume->GetHideTriggerComponent());

	return true;
}

#endif //# WITH_DEV_AUTOMATION_TESTS
```

- [ ] **Step 2: 컴파일 실패 확인**

Run: Visual Studio 빌드.
Expected: `ASpyNPCCharacter`가 `ISpyMissionTargetHideVolume`를 구현하지 않고 `bEnableHideTrigger` 프로퍼티가 없어 컴파일/assert 실패.

- [ ] **Step 3: 최소 구현 — 헤더**

`SpyNPCCharacter.h`에서 include 블록과 클래스 선언을 아래처럼 수정(`ISpyNPCRoot` 상속에 인터페이스 하나 추가, 신규 멤버 추가):

```cpp
#include "CoreMinimal.h"
#include "ModularCharacter.h"
#include "NPC/CommonInterface.NPC.h"
#include "System/CommonInterface.System.h"

#include "SpyNPCCharacter.generated.h"

class USphereComponent;
class UBoxComponent;
class UPrimitiveComponent;
class USpyNPCConfig;

//# NPC 도메인 루트. NPCId 하나로 3개 DataTable(USpyNPCConfig 경유)을 BeginPlay에 1회 스캔해
//# 자신의 Default/Offer/InProgress/Report 대사와 담당 MissionId(Offer/Report) 를 캐싱한다.
UCLASS()
class SKILLPROJECT_API ASpyNPCCharacter : public AModularCharacter, public ISpyNPCRoot, public ISpyMissionTargetHideVolume
{
	GENERATED_BODY()

public:
	ASpyNPCCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//# ISpyNPCRoot
	virtual FSpyNPCDialogueResult RequestInteract(APlayerController* Requester) override;
	virtual int32 GetNPCId() const override
	{
		return NPCId;
	}
	virtual bool IsPawnInRange(const AActor* RequesterPawn) const override;
	virtual bool GetDialogueLineAtIndex(int32 InDialogueId, int32 InPageIndex, FText& OutLine) const override;

	//# ISpyMissionTargetHideVolume
	virtual UPrimitiveComponent* GetHideTriggerComponent() const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
										 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
									   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	void CacheNPCData();

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> InteractionSphere;

	//# NPC 테이블 행 식별자이자 MissionCommunication.NPCId 매칭 키
	UPROPERTY(EditDefaultsOnly, Category = "Dialogue")
	int32 NPCId = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Dialogue")
	TObjectPtr<USpyNPCConfig> NPCConfig;

	//# design 2026-08-10 §5 — 기본 false: 기존 배치 NPC는 거리 히스테리시스로 그대로 동작(하위 호환)
	UPROPERTY(EditAnywhere, Category = "Navigation")
	bool bEnableHideTrigger = false;

	//# InteractionSphere(상호작용 판정)와 완전히 분리된 네비 숨김 전용 볼륨(design 2026-08-10 §5)
	UPROPERTY(VisibleAnywhere, Category = "Navigation")
	TObjectPtr<UBoxComponent> HideTriggerVolume;

	//# BeginPlay 1회 캐싱 (§8 — 매 프레임/매 상호작용 조회 금지)
	bool bDataCached = false;
	FText CachedNPCDisplayName;
	int32 CachedDefaultDialogueId = 0;
	FText CachedDefaultLine;
	int32 CachedOfferMissionId = INDEX_NONE;
	int32 CachedOfferDialogueId = 0;
	FText CachedOfferLine;
	int32 CachedInProgressDialogueId = 0;
	FText CachedInProgressLine;
	int32 CachedReportMissionId = INDEX_NONE;
	int32 CachedReportDialogueId = 0;
	FText CachedReportLine;
};
```

- [ ] **Step 4: 최소 구현 — cpp**

`SpyNPCCharacter.cpp` 상단 include에 `#include "Components/BoxComponent.h"` 추가, 생성자/`BeginPlay`/파일 하단을 아래처럼 수정:

```cpp
#include "NPC/SpyNPCCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Character/CommonInterface.Character.h"
#include "ManagerComponent/CommonInterface.Manager.h"
#include "Data/SpyMissionConfig.h"
#include "Data/SpyNPCDialogueRow.h"
#include "System/SpyMissionComponent.h"
#include "System/SpyMissionTargetRegistrySubsystem.h"
#include "Util/SpyGameplayTags.h"

ASpyNPCCharacter::ASpyNPCCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(GetRootComponent());
	InteractionSphere->SetSphereRadius(300.f);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	HideTriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("HideTriggerVolume"));
	HideTriggerVolume->SetupAttachment(GetRootComponent());
	HideTriggerVolume->SetBoxExtent(FVector(200.f, 200.f, 100.f));
	HideTriggerVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ASpyNPCCharacter::BeginPlay()
{
	Super::BeginPlay();

	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ASpyNPCCharacter::OnInteractionSphereBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &ASpyNPCCharacter::OnInteractionSphereEndOverlap);

	if (bEnableHideTrigger)
		HideTriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	CacheNPCData();

	//# 미션 목표 좌표 레지스트리 자기등록(design §5-2·§5-4) — Dialogue 미션은 이 NPC 위치를 그대로 목표로 쓴다
	if (UWorld* World = GetWorld())
	{
		if (USpyMissionTargetRegistrySubsystem* Registry = World->GetSubsystem<USpyMissionTargetRegistrySubsystem>())
			Registry->RegisterNPCLocation(NPCId, this);
	}
}
```

(`EndPlay`/`CacheNPCData`/오버랩 콜백/`IsPawnInRange`/`RequestInteract`/`GetDialogueLineAtIndex`는 변경 없음 — 그대로 둔다.) 파일 맨 끝에 추가:

```cpp
UPrimitiveComponent* ASpyNPCCharacter::GetHideTriggerComponent() const
{
	return bEnableHideTrigger ? HideTriggerVolume : nullptr;
}
```

- [ ] **Step 5: 컴파일 + 테스트 통과 확인**

Run: Visual Studio 빌드 후 Automation 탭에서 `SkillProject.NPC.Character.` 필터로 3개 신규 테스트 실행.
Expected: 3개 모두 PASS. 기존 NPC/Dialogue 관련 테스트도 회귀 없이 PASS.

- [ ] **Step 6: Commit**

```bash
git add SkillProject/Source/SkillProject/NPC/SpyNPCCharacter.h SkillProject/Source/SkillProject/NPC/SpyNPCCharacter.cpp SkillProject/Source/SkillProject/NPC/Tests/SpyNPCCharacterHideVolumeTests.cpp
```
제안 메시지: `[Feature] ASpyNPCCharacter — 네비 숨김 트리거 볼륨(ISpyMissionTargetHideVolume) 추가`

---

### Task 4: `ASpyInteractableObject` — 트리거 볼륨 구현

**Files:**
- Modify: `SkillProject/Source/SkillProject/Interactable/SpyInteractableObject.h`
- Modify: `SkillProject/Source/SkillProject/Interactable/SpyInteractableObject.cpp`
- Modify: `SkillProject/Source/SkillProject/Interactable/Tests/SpyInteractableObjectTests.cpp`

**Interfaces:**
- Consumes: `ISpyMissionTargetHideVolume`(Task 2).
- Produces: `ASpyInteractableObject`가 `ISpyMissionTargetHideVolume` 구현.

- [ ] **Step 1: 실패하는 테스트 추가**

`SpyInteractableObjectTests.cpp`의 `#include` 블록에 `#include "System/CommonInterface.System.h"` 추가, `#endif` 바로 위에 테스트 2개 추가:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyInteractableObjectImplementsHideVolumeTest,
	"SkillProject.Interactable.Object.ImplementsHideVolume",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyInteractableObjectImplementsHideVolumeTest::RunTest(const FString& Parameters)
{
	ASpyInteractableObject* Interactable = NewObject<ASpyInteractableObject>();

	TestTrue(TEXT("ASpyInteractableObject implements ISpyMissionTargetHideVolume"),
			 Interactable->GetClass()->ImplementsInterface(USpyMissionTargetHideVolume::StaticClass()));

	ISpyMissionTargetHideVolume* HideVolume = Cast<ISpyMissionTargetHideVolume>(Interactable);
	TestNotNull(TEXT("Cast to the interface succeeds"), HideVolume);
	TestNull(TEXT("GetHideTriggerComponent() is nullptr when bEnableHideTrigger is false (C++ default)"), HideVolume->GetHideTriggerComponent());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyInteractableObjectHideTriggerEnabledReturnsComponentTest,
	"SkillProject.Interactable.Object.HideTriggerEnabledReturnsComponent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyInteractableObjectHideTriggerEnabledReturnsComponentTest::RunTest(const FString& Parameters)
{
	ASpyInteractableObject* Interactable = NewObject<ASpyInteractableObject>();

	FBoolProperty* Prop = FindFProperty<FBoolProperty>(ASpyInteractableObject::StaticClass(), TEXT("bEnableHideTrigger"));
	if (Prop == nullptr)
	{
		AddError(TEXT("bEnableHideTrigger property not found via reflection — field renamed?"));

		return false;
	}
	Prop->SetPropertyValue_InContainer(Interactable, true);

	ISpyMissionTargetHideVolume* HideVolume = Cast<ISpyMissionTargetHideVolume>(Interactable);
	TestNotNull(TEXT("Cast to the interface succeeds"), HideVolume);
	TestNotNull(TEXT("GetHideTriggerComponent() returns a valid component when bEnableHideTrigger is true"), HideVolume->GetHideTriggerComponent());

	return true;
}
```

- [ ] **Step 2: 컴파일 실패 확인**

Run: Visual Studio 빌드.
Expected: `ASpyInteractableObject`가 인터페이스 미구현이라 컴파일 실패(`ISpyMissionTargetHideVolume`, `USpyMissionTargetHideVolume` 미선언 타입 에러는 없음 — Task 2에서 이미 헤더가 존재하므로, 여기서는 "does not implement interface" 류의 로직 실패로 1번 테스트가 FAIL).

- [ ] **Step 3: 최소 구현 — 헤더**

`SpyInteractableObject.h` 전체를 아래로 교체:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Interactable/CommonInterface.Interactable.h"
#include "System/CommonInterface.System.h"

#include "SpyInteractableObject.generated.h"

class USphereComponent;
class UBoxComponent;

//# 레벨 배치형 상호작용 오브젝트. F키로 1회 상호작용하면 지정된 미션 태그로
//# 진행도를 올리고 스스로 소진된다.
UCLASS()
class SKILLPROJECT_API ASpyInteractableObject : public AActor, public ISpyInteractableRoot, public ISpyMissionTargetHideVolume
{
	GENERATED_BODY()

public:
	ASpyInteractableObject();

	//# ISpyInteractableRoot
	virtual void RequestInteract(APlayerController* Requester) override;
	virtual bool IsPawnInRange(const AActor* RequesterPawn) const override;
	virtual FText GetInteractVerb() const override
	{
		return InteractVerb;
	}

	//# ISpyMissionTargetHideVolume
	virtual UPrimitiveComponent* GetHideTriggerComponent() const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
										 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
									   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void OnRep_Consumed();

	//# 소진 직후 이 오브젝트와 여전히 오버랩 중인 로컬 폰들에게 범위 이탈을 알린다 —
	//# 콜리전 비활성화가 EndOverlap을 유발하지만, 명시적으로도 호출해 두어 안전망을 이중화한다
	void NotifyLocalOverlapEnd();

	//# 연출 훅 — VFX/사운드는 이번 범위 밖
	UFUNCTION(BlueprintImplementableEvent, Category = "Interactable")
	void OnConsumed();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Interactable")
	TObjectPtr<USphereComponent> InteractionSphere;

	//# 레벨 배치 시 오브젝트마다 다르게 조정할 수 있어야 해 인스턴스 편집 가능하게 노출한다 (cpp-style §15)
	UPROPERTY(EditAnywhere, Category = "Interactable")
	float InteractionRadius = 300.f;

	//# 이 오브젝트가 상호작용 시 발신할 미션 진행 태그 (Event.Mission.Interact 계열)
	UPROPERTY(EditAnywhere, Category = "Interactable")
	FGameplayTag MissionEventTag;

	//# 근접 프롬프트에 표시할 동사
	UPROPERTY(EditAnywhere, Category = "Interactable")
	FText InteractVerb;

	//# design 2026-08-10 §5 — 기본 false: 기존 배치 오브젝트는 거리 히스테리시스로 그대로 동작(하위 호환)
	UPROPERTY(EditAnywhere, Category = "Navigation")
	bool bEnableHideTrigger = false;

	//# InteractionSphere(상호작용 판정)와 완전히 분리된 네비 숨김 전용 볼륨(design 2026-08-10 §5)
	UPROPERTY(VisibleAnywhere, Category = "Navigation")
	TObjectPtr<UBoxComponent> HideTriggerVolume;

	UPROPERTY(ReplicatedUsing = OnRep_Consumed)
	bool bConsumed = false;
};
```

- [ ] **Step 4: 최소 구현 — cpp**

`SpyInteractableObject.cpp` 상단 `#include "Components/SphereComponent.h"` 아래에 `#include "Components/BoxComponent.h"` 추가. 생성자와 파일 끝을 수정:

```cpp
ASpyInteractableObject::ASpyInteractableObject()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	SetRootComponent(InteractionSphere);
	InteractionSphere->SetSphereRadius(InteractionRadius);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	HideTriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("HideTriggerVolume"));
	HideTriggerVolume->SetupAttachment(InteractionSphere);
	HideTriggerVolume->SetBoxExtent(FVector(200.f, 200.f, 100.f));
	HideTriggerVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	InteractVerb = NSLOCTEXT("SpyInteractable", "DefaultInteractVerb", "조사하기");
}
```

`BeginPlay`에 콜리전 토글 추가(기존 순서 유지, `OnComponentBeginOverlap` 바인딩 바로 아래):

```cpp
void ASpyInteractableObject::BeginPlay()
{
	Super::BeginPlay();

	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ASpyInteractableObject::OnInteractionSphereBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &ASpyInteractableObject::OnInteractionSphereEndOverlap);

	if (bEnableHideTrigger)
		HideTriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	//# 미션 목표 좌표 레지스트리 자기등록(design §5-3·§5-4) — 소진(consume) 시 해제 여부는
	//# 이번 범위 밖(design §7-6, 현재 Interact 데이터 없음)
	if (MissionEventTag.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			if (USpyMissionTargetRegistrySubsystem* Registry = World->GetSubsystem<USpyMissionTargetRegistrySubsystem>())
				Registry->RegisterMissionTargetLocation(MissionEventTag, this);
		}
	}
}
```

파일 맨 끝(`NotifyLocalOverlapEnd` 아래)에 추가:

```cpp
UPrimitiveComponent* ASpyInteractableObject::GetHideTriggerComponent() const
{
	return bEnableHideTrigger ? HideTriggerVolume : nullptr;
}
```

(`EndPlay`/`OnConstruction`/`GetLifetimeReplicatedProps`/오버랩 콜백/`IsPawnInRange`/`RequestInteract`/`OnRep_Consumed`/`NotifyLocalOverlapEnd`는 변경 없음.)

- [ ] **Step 5: 컴파일 + 테스트 통과 확인**

Run: Visual Studio 빌드 후 Automation 탭에서 `SkillProject.Interactable.Object.` 필터로 신규 2개 + 기존 `CppDefaults` 테스트 실행.
Expected: 3개 모두 PASS.

- [ ] **Step 6: Commit**

```bash
git add SkillProject/Source/SkillProject/Interactable/SpyInteractableObject.h SkillProject/Source/SkillProject/Interactable/SpyInteractableObject.cpp SkillProject/Source/SkillProject/Interactable/Tests/SpyInteractableObjectTests.cpp
```
제안 메시지: `[Feature] ASpyInteractableObject — 네비 숨김 트리거 볼륨(ISpyMissionTargetHideVolume) 추가`

---

### Task 5: `USpyNavigationComponent` — 트리거 바인딩/오버랩 반응

**Files:**
- Modify: `SkillProject/Source/SkillProject/ManagerComponent/SpyNavigationComponent.h`
- Modify: `SkillProject/Source/SkillProject/ManagerComponent/SpyNavigationComponent.cpp`
- Modify: `SkillProject/Source/SkillProject/ManagerComponent/Tests/SpyNavigationComponentTests.cpp`

**Interfaces:**
- Consumes: `AActor* FindNPCActor(int32)`/`FindMissionTargetActor(FGameplayTag)`(Task 1), `ISpyMissionTargetHideVolume::GetHideTriggerComponent()`(Task 2~4).
- Produces: `bool USpyNavigationComponent::IsPathVisible() const`, `bool USpyNavigationComponent::IsHideTriggerBound() const`, `void NotifyHideTriggerEntered(AActor*)`, `void NotifyHideTriggerExited(AActor*)` — 테스트 및 향후 UI/디버그 훅에서 재사용 가능.

- [ ] **Step 1: 실패하는 테스트 작성**

`SpyNavigationComponentTests.cpp`의 helper 블록(맨 위, `SpyNavigationComponentTests_MakeLocatedActor` 정의 아래)에 헬퍼 추가:

```cpp
//# design 2026-08-10 §5 — 트리거 활성 마커를 만들고 즉시 레지스트리에 등록한다.
//# ASpyMissionTargetPoint 는 RootComponent 가 이미 있어 SetActorLocation 이 World 없이도 반영된다
//# (SpyNavigationComponentTests_MakeLocatedActor 와 달리 별도 Root 부착이 필요 없다).
static ASpyMissionTargetPoint* SpyNavigationComponentTests_MakeHideTriggerTarget(
	USpyMissionTargetRegistrySubsystem* Registry, const FVector& InLocation, bool bEnableHideTrigger)
{
	ASpyMissionTargetPoint* Target = NewObject<ASpyMissionTargetPoint>(GetTransientPackage());
	Target->SetActorLocation(InLocation);

	FBoolProperty* Prop = FindFProperty<FBoolProperty>(ASpyMissionTargetPoint::StaticClass(), TEXT("bEnableHideTrigger"));
	check(Prop != nullptr);
	Prop->SetPropertyValue_InContainer(Target, bEnableHideTrigger);

	Registry->RegisterMissionTargetLocation(SpyGameplayTags::Skill_Move_Vault, Target);

	return Target;
}
```

파일 끝(마지막 테스트 뒤, `#endif` 바로 위)에 테스트 6개 추가:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentHideTriggerBindsWhenEnabledTest,
	"SkillProject.Navigation.Component.HideTriggerBindsWhenEnabled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentHideTriggerBindsWhenEnabledTest::RunTest(const FString& Parameters)
{
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);
	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, SpyNavigationComponentTests_MakeConfig());

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);

	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	SpyNavigationComponentTests_MakeHideTriggerTarget(Registry, FVector(500.f, -250.f, 120.f), true);
	NavComponent->SetMissionTargetRegistry(Registry);

	MissionComponent->AcceptCurrentMission();

	TestTrue(TEXT("Path active after accept"), NavComponent->IsPathActive());
	TestTrue(TEXT("Hide trigger bound — target has bEnableHideTrigger=true"), NavComponent->IsHideTriggerBound());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentHideTriggerDisabledTargetDoesNotBindTest,
	"SkillProject.Navigation.Component.HideTriggerDisabledTargetDoesNotBind",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentHideTriggerDisabledTargetDoesNotBindTest::RunTest(const FString& Parameters)
{
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);
	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, SpyNavigationComponentTests_MakeConfig());

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);

	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	//# bEnableHideTrigger 기본값(false) 그대로 둔 타겟 — 기존 레벨과 동일 조건(하위 호환)
	SpyNavigationComponentTests_MakeHideTriggerTarget(Registry, FVector(500.f, -250.f, 120.f), false);
	NavComponent->SetMissionTargetRegistry(Registry);

	MissionComponent->AcceptCurrentMission();

	TestTrue(TEXT("Path still starts normally"), NavComponent->IsPathActive());
	TestFalse(TEXT("No trigger bound — falls back to distance hysteresis"), NavComponent->IsHideTriggerBound());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentHideTriggerEnterHidesPathTest,
	"SkillProject.Navigation.Component.HideTriggerEnterHidesPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentHideTriggerEnterHidesPathTest::RunTest(const FString& Parameters)
{
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);
	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, SpyNavigationComponentTests_MakeConfig());

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);

	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	SpyNavigationComponentTests_MakeHideTriggerTarget(Registry, FVector(500.f, -250.f, 120.f), true);
	NavComponent->SetMissionTargetRegistry(Registry);

	MissionComponent->AcceptCurrentMission();
	TestTrue(TEXT("Path visible immediately after accept (seeded true in StartPathTo)"), NavComponent->IsPathVisible());

	NavComponent->NotifyHideTriggerEntered(Owner);

	TestFalse(TEXT("Path hides immediately on trigger enter, no distance check needed"), NavComponent->IsPathVisible());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentHideTriggerIgnoresOtherActorTest,
	"SkillProject.Navigation.Component.HideTriggerIgnoresOtherActor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentHideTriggerIgnoresOtherActorTest::RunTest(const FString& Parameters)
{
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);
	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, SpyNavigationComponentTests_MakeConfig());

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);

	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	SpyNavigationComponentTests_MakeHideTriggerTarget(Registry, FVector(500.f, -250.f, 120.f), true);
	NavComponent->SetMissionTargetRegistry(Registry);

	MissionComponent->AcceptCurrentMission();

	AActor* UnrelatedActor = NewObject<AActor>(GetTransientPackage());
	NavComponent->NotifyHideTriggerEntered(UnrelatedActor);

	TestTrue(TEXT("A different local pawn overlapping the same trigger does not affect this owner's nav"), NavComponent->IsPathVisible());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentHideTriggerExitReshowsImmediatelyTest,
	"SkillProject.Navigation.Component.HideTriggerExitReshowsImmediately",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentHideTriggerExitReshowsImmediatelyTest::RunTest(const FString& Parameters)
{
	//# World 없이(RecomputePath 조기반환 환경에서도) bPathVisible 시딩이 동기 반영되는지만 검증한다.
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);
	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, SpyNavigationComponentTests_MakeConfig());

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);

	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	SpyNavigationComponentTests_MakeHideTriggerTarget(Registry, FVector(500.f, -250.f, 120.f), true);
	NavComponent->SetMissionTargetRegistry(Registry);

	MissionComponent->AcceptCurrentMission();

	NavComponent->NotifyHideTriggerEntered(Owner);
	TestFalse(TEXT("Path hidden after entering the trigger"), NavComponent->IsPathVisible());

	NavComponent->NotifyHideTriggerExited(Owner);
	TestTrue(TEXT("Path visible again immediately on exit — no distance hysteresis dead zone"), NavComponent->IsPathVisible());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentHideTriggerUnbindsOnStopPathTest,
	"SkillProject.Navigation.Component.HideTriggerUnbindsOnStopPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentHideTriggerUnbindsOnStopPathTest::RunTest(const FString& Parameters)
{
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);
	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, SpyNavigationComponentTests_MakeConfig());

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);

	USpyMissionTargetRegistrySubsystem* Registry = NewObject<USpyMissionTargetRegistrySubsystem>();
	SpyNavigationComponentTests_MakeHideTriggerTarget(Registry, FVector(500.f, -250.f, 120.f), true);
	NavComponent->SetMissionTargetRegistry(Registry);

	MissionComponent->AcceptCurrentMission();
	TestTrue(TEXT("Hide trigger bound after accept"), NavComponent->IsHideTriggerBound());

	//# 목표 3회 중 3회 채워 완료시킨다 (Vault 미션, Accumulate) — FSpyNavigationComponentCompleteStopsPathTest 와 동일 패턴
	MissionComponent->AddProgress(SpyGameplayTags::Skill_Move_Vault, 3);

	TestFalse(TEXT("Path inactive after mission completed"), NavComponent->IsPathActive());
	TestFalse(TEXT("Hide trigger unbound once the path stops"), NavComponent->IsHideTriggerBound());

	return true;
}
```

- [ ] **Step 2: 컴파일 실패 확인**

Run: Visual Studio 빌드.
Expected: `IsPathVisible`/`IsHideTriggerBound`/`NotifyHideTriggerEntered`/`NotifyHideTriggerExited`가 `USpyNavigationComponent`에 없다는 컴파일 에러.

- [ ] **Step 3: 최소 구현 — 헤더**

`SpyNavigationComponent.h`에서 forward declare 블록에 `class UPrimitiveComponent;` 추가, `public:` 블록에 getter/알림 메서드 추가, `protected:` 블록에 델리게이트 핸들러/바인딩 메서드 추가, 멤버 추가:

```cpp
class UMaterialInstanceDynamic;
class USpyMissionComponent;
class USpyMissionTargetRegistrySubsystem;
class USplineComponent;
class USplineMeshComponent;
class UStaticMesh;
class UPrimitiveComponent;
```

```cpp
public:
	USpyNavigationComponent();

	UFUNCTION(BlueprintPure)
	bool IsPathActive() const
	{
		return bPathActive;
	}

	//# design 2026-08-10 §6 테스트/디버그 훅 — 현재 라인이 실제로 그려지고 있는지
	UFUNCTION(BlueprintPure)
	bool IsPathVisible() const
	{
		return bPathVisible;
	}

	//# design 2026-08-10 §6 테스트 훅 — 현재 타겟이 트리거 볼륨을 노출해 구독 중인지
	UFUNCTION(BlueprintPure)
	bool IsHideTriggerBound() const
	{
		return BoundHideTrigger.IsValid();
	}

	UFUNCTION(BlueprintPure)
	FVector GetCurrentTargetLocation() const
	{
		return CurrentTargetLocation;
	}

	//# design 2026-08-10 §6 테스트 주입 지점(§5-7 과 동일 목적) — 실제 트리거 오버랩
	//# 델리게이트(HandleHideTriggerBeginOverlap/EndOverlap)가 호출하는 것과 동일한 진입점이라,
	//# 테스트는 물리 오버랩 없이 이 함수를 직접 호출해 검증한다. OtherActor 가 소유 폰이 아니면 무시한다.
	void NotifyHideTriggerEntered(AActor* OtherActor);
	void NotifyHideTriggerExited(AActor* OtherActor);

	void BindMissionComponent(USpyMissionComponent* InMissionComponent);
	void UnbindMissionComponent();

	void SetMissionTargetRegistry(USpyMissionTargetRegistrySubsystem* InRegistry) { OverrideTargetRegistry = InRegistry; }
```

```cpp
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool AutoDiscoverAndBindMissionComponent();

	UFUNCTION()
	void HandleMissionProgressChanged(USpyMissionComponent* MissionComponent, int32 MissionIndex, int32 Count, int32 TargetCount);

	void StartPathTo(const FVector& InTargetLocation, AActor* InTargetActor);
	void StopPath();

	void RecomputePath();
	void ApplyPathPoints(const TArray<FVector>& InPathPoints);
	void EnsureSegmentPoolSize(int32 InRequiredCount);
	void HideVisual();

	//# design 2026-08-10 §6 — 타겟이 ISpyMissionTargetHideVolume 를 구현하고 트리거를 노출하면
	//# 구독하고, 그렇지 않으면 아무 것도 하지 않는다(거리 히스테리시스 폴백).
	void BindHideTrigger(AActor* TargetActor);
	void UnbindHideTrigger();

	UFUNCTION()
	void HandleHideTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
										UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleHideTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
									  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	USpyMissionTargetRegistrySubsystem* GetMissionTargetRegistry();
	void TryResolveTarget();
```

`bPathActive` 선언 바로 아래에 트리거 상태 멤버 추가:

```cpp
	FVector CurrentTargetLocation = FVector::ZeroVector;
	bool bPathActive = false;

	//# design 2026-08-10 §6 — 현재 타겟이 노출한 트리거 컴포넌트(없으면 invalid). 타겟 전환/StopPath 시 해제된다.
	UPROPERTY(Transient)
	TWeakObjectPtr<UPrimitiveComponent> BoundHideTrigger;

	bool bInsideHideTrigger = false;
```

- [ ] **Step 4: 최소 구현 — cpp**

`SpyNavigationComponent.cpp` 상단 include에 추가:

```cpp
#include "Components/PrimitiveComponent.h"
```

```cpp
#include "System/SpyMissionTargetRegistrySubsystem.h"
#include "System/CommonInterface.System.h"
#include "System/SpyNavPathMath.h"
```

`StartPathTo`를 아래로 교체:

```cpp
void USpyNavigationComponent::StartPathTo(const FVector& InTargetLocation, AActor* InTargetActor)
{
	CurrentTargetLocation = InTargetLocation;
	bPathActive = true;

	//# 콜드 스타트를 "이전에 보이고 있었다"로 시드한다 — design §4-3 콜드 스타트 조항 참조.
	bPathVisible = true;

	BindHideTrigger(InTargetActor);

	RecomputePath();

	if (UWorld* World = GetWorld())
		World->GetTimerManager().SetTimer(RepathTimerHandle, this, &USpyNavigationComponent::RecomputePath, UpdateIntervalSeconds, true);
}
```

`StopPath`에 `UnbindHideTrigger()` 호출 추가:

```cpp
void USpyNavigationComponent::StopPath()
{
	bPathActive = false;
	CurrentTargetLocation = FVector::ZeroVector;

	UnbindHideTrigger();

	//# design §5-6 — 대기 중인 좌표 재시도 타이머도 함께 정리한다. 완료→수락이 같은 OnRep 안에서
	//# 연달아 일어날 때(§2-3), 낡은 재시도가 새 미션에 잘못된 좌표를 주입하는 경합을 막는다.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RepathTimerHandle);
		World->GetTimerManager().ClearTimer(TargetRetryTimerHandle);
	}

	if (PathSpline != nullptr)
		PathSpline->ClearSplinePoints(true);

	HideVisual();
}
```

`TryResolveTarget`의 `if (bFound)` 블록을 아래로 교체:

```cpp
	if (bFound)
	{
		if (UWorld* World = GetWorld())
			World->GetTimerManager().ClearTimer(TargetRetryTimerHandle);

		AActor* TargetActor = bPendingIsDialogue
			? Registry->FindNPCActor(PendingNPCId)
			: Registry->FindMissionTargetActor(PendingMatchTag);

		StartPathTo(TargetLocation, TargetActor);

		return;
	}
```

`RecomputePath` 맨 위(`if (bPathActive == false) return;` 바로 아래)에 트리거 가드 추가:

```cpp
void USpyNavigationComponent::RecomputePath()
{
	if (bPathActive == false)
		return;

	//# design 2026-08-10 §6 — 트리거 활성 타겟은 NavMesh 쿼리 자체를 생략한다(거리 계산 불필요)
	if (bInsideHideTrigger)
	{
		HideVisual();

		return;
	}

	AActor* Owner = GetOwner();
	if (Owner == nullptr)
		return;

	UWorld* World = GetWorld();
	if (World == nullptr)
		return;
	...
```

(이후 `RecomputePath` 나머지는 변경 없음.) `ApplyPathPoints` 맨 위(`if (PathSpline == nullptr) return;` 바로 아래)를 아래로 교체 — **code-reviewer 회귀 고정**: 위 `RecomputePath` 가드는 트리거 "안"만 우회할 뿐, "밖"으로 나온 프레임은 그대로 `EvaluateHysteresisVisibility`를 타므로 트리거 half-extent가 `ArrivalHideDistanceCm`(300cm) 미만인 타겟(§3-4가 승인한 75cm/300cm 축소 케이스 포함)에서 나가자마자 `RemainingPathLength`가 300cm 미만이라 같은 프레임에 다시 숨는 결함이 있었다. 트리거가 바인딩된 동안은 히스테리시스 자체를 완전히 끈다:

```cpp
void USpyNavigationComponent::ApplyPathPoints(const TArray<FVector>& InPathPoints)
{
	if (PathSpline == nullptr)
		return;

	if (BoundHideTrigger.IsValid())
	{
		//# 트리거 바인딩 타겟은 히스테리시스 자체를 쓰지 않는다 — RecomputePath 가드가
		//# "안"을 이미 걸러내므로 여기 도달한 것 자체가 "밖"이라는 뜻이다(design 2026-08-10 §6 개정).
		bPathVisible = true;
	}
	else
	{
		//# 트리밍 전 "원본" 경로 길이로 히스테리시스를 먼저 판정한다 — design §4-3 규칙 순서 참조.
		const float RemainingPathLength = SpyNavPathMath::ComputePathLength(InPathPoints);
		bPathVisible = SpyNavPathMath::EvaluateHysteresisVisibility(RemainingPathLength, ArrivalHideDistanceCm, ArrivalReshowDistanceCm, bPathVisible);
	}

	if (bPathVisible == false)
	{
		HideVisual();

		return;
	}

	...
```

(`if (bPathVisible == false) { HideVisual(); return; }` 이후 트리밍·오프셋·스플라인 세그먼트 렌더링 로직은 전부 변경 없음. `EnsureSegmentPoolSize`도 변경 없음.) 파일 끝(`EnsureSegmentPoolSize` 아래)에 추가:

```cpp
void USpyNavigationComponent::BindHideTrigger(AActor* TargetActor)
{
	UnbindHideTrigger();

	//# TScriptInterface(RawPtr) 생성자는 인터페이스 미구현이어도 ObjectPointer 를 그대로 저장한다 —
	//# GetObject() 널체크로는 구현 여부를 걸러낼 수 없다. Cast<Interface> 로 먼저 판정한다
	//# (Interactable/SpyInteractableObject.cpp 의 동일 근거 주석 참조).
	ISpyMissionTargetHideVolume* HideVolumeHost = Cast<ISpyMissionTargetHideVolume>(TargetActor);
	if (HideVolumeHost == nullptr)
		return;

	UPrimitiveComponent* TriggerComponent = HideVolumeHost->GetHideTriggerComponent();
	if (TriggerComponent == nullptr)
		return;

	BoundHideTrigger = TriggerComponent;
	TriggerComponent->OnComponentBeginOverlap.AddDynamic(this, &USpyNavigationComponent::HandleHideTriggerBeginOverlap);
	TriggerComponent->OnComponentEndOverlap.AddDynamic(this, &USpyNavigationComponent::HandleHideTriggerEndOverlap);

	//# 구독 시점에 이미 트리거 안에 서 있는 경우(예: 트리거 안에서 미션을 수락) 대비 —
	//# BeginOverlap 이벤트는 발화하지 않으므로 현재 상태를 직접 조회해 동기화한다.
	AActor* Owner = GetOwner();
	bInsideHideTrigger = (Owner != nullptr) && TriggerComponent->IsOverlappingActor(Owner);
}

void USpyNavigationComponent::UnbindHideTrigger()
{
	if (UPrimitiveComponent* TriggerComponent = BoundHideTrigger.Get())
	{
		TriggerComponent->OnComponentBeginOverlap.RemoveDynamic(this, &USpyNavigationComponent::HandleHideTriggerBeginOverlap);
		TriggerComponent->OnComponentEndOverlap.RemoveDynamic(this, &USpyNavigationComponent::HandleHideTriggerEndOverlap);
	}

	BoundHideTrigger = nullptr;
	bInsideHideTrigger = false;
}

void USpyNavigationComponent::HandleHideTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
															  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	NotifyHideTriggerEntered(OtherActor);
}

void USpyNavigationComponent::HandleHideTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
														   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	NotifyHideTriggerExited(OtherActor);
}

void USpyNavigationComponent::NotifyHideTriggerEntered(AActor* OtherActor)
{
	if (OtherActor != GetOwner())
		return;

	bInsideHideTrigger = true;
	HideVisual();
}

void USpyNavigationComponent::NotifyHideTriggerExited(AActor* OtherActor)
{
	if (OtherActor != GetOwner())
		return;

	bInsideHideTrigger = false;

	//# World/NavSystem 없어 RecomputePath 가 조기반환해도 "밖" 상태를 동기 반영(design §6).
	bPathVisible = true;
	RecomputePath();
}
```

- [ ] **Step 5: 컴파일 + 테스트 통과 확인**

Run: Visual Studio 빌드 후 Automation 탭에서 `SkillProject.Navigation.Component.HideTrigger` 필터로 6개 신규 테스트 + `SkillProject.Navigation.Component.`(전체, 기존 테스트 포함) 실행.
Expected: 6개 신규 테스트 PASS, 기존 `SpyNavigationComponentTests.cpp`의 모든 테스트도 회귀 없이 PASS(`StartPathTo` 시그니처가 바뀌었으므로 이 파일 자체가 컴파일되는 것 자체가 1차 검증).

- [ ] **Step 6: Commit**

```bash
git add SkillProject/Source/SkillProject/ManagerComponent/SpyNavigationComponent.h SkillProject/Source/SkillProject/ManagerComponent/SpyNavigationComponent.cpp SkillProject/Source/SkillProject/ManagerComponent/Tests/SpyNavigationComponentTests.cpp
```
제안 메시지: `[Feature] USpyNavigationComponent — 미션 타겟 트리거 볼륨 기반 네비 숨김/표시`

---

### Task 6: 전체 회귀 확인

**Files:** 없음(신규/수정 없음) — 검증 전용 태스크.

- [ ] **Step 1: 전체 Navigation/System/NPC/Interactable 테스트 스위트 실행**

Run: 에디터 Automation 탭에서 `SkillProject.Navigation.`, `SkillProject.System.MissionTargetRegistry.`, `SkillProject.NPC.`, `SkillProject.Interactable.` 4개 필터를 각각 전체 선택해 Start Tests.
Expected: Task 1~5에서 추가한 19개 신규 테스트(레지스트리 5 + MissionTargetPoint 3 + NPCCharacter 3 + InteractableObject 2 + NavigationComponent 6) + 기존 전체 테스트가 모두 PASS. 하나라도 FAIL이면 해당 태스크로 돌아가 원인 수정.

- [ ] **Step 2: DevMap 등 기존 레벨의 배치 액터 수동 확인 (선택)**

Run: 에디터에서 `SkillProject/Content/Spy/Maps/DevMap.umap` 로드 → 기존에 배치된 `ASpyMissionTargetPoint`/NPC/Interactable 액터 중 하나를 선택 → Details 패널에서 `Hide Trigger Volume`(Box, 콜리전 None) 컴포넌트가 보이고 `Enable Hide Trigger`가 체크 해제(false) 상태인지 확인.
Expected: 기존 배치 전부 `bEnableHideTrigger = false` — 하위 호환 확인. 디자이너가 특정 타겟에서 트리거를 쓰고 싶으면 이 시점 이후 수동으로 체크 + Box Extent 조정.

- [ ] **Step 3: Commit**

이 태스크는 코드 변경이 없으므로 커밋 대상 없음 — Task 1~5의 커밋만 존재.

---

## Self-Review 결과

- **스펙 커버리지**: §3(레지스트리)=Task 1, §4(인터페이스)=Task 2, §5(3개 액터)=Task 2·3·4, §6(NavigationComponent 바인딩/즉시 반응/가드)=Task 5, §7(테스트 가능 범위)=각 태스크의 테스트 스텝. §8(콜리전 프로파일 열린 질문)은 `InteractionSphere`와 동일하게 `QueryOnly`만 설정하는 것으로 확정(별도 프로파일 신설 안 함) — 열린 질문 해소.
- **플레이스홀더 스캔**: 없음 — 전 스텝 실제 코드/실행 방법 명시.
- **타입 일관성**: `ISpyMissionTargetHideVolume::GetHideTriggerComponent()` 반환 타입(`UPrimitiveComponent*`)이 Task 2~4의 구현체와 Task 5의 소비 코드에서 동일. `StartPathTo(const FVector&, AActor*)` 시그니처가 Task 5의 선언·정의·호출부(`TryResolveTarget`)에서 일치.
- **개정 1차(design-reviewer BLOCKER 반영)**: `NotifyHideTriggerExited`가 `RecomputePath()`만 호출하면 `ApplyPathPoints()`의 거리 히스테리시스(`bPreviouslyVisible==false` 분기, `ArrivalReshowDistanceCm=400cm` 문턱)가 그대로 걸려 고착이 발생 — `RecomputePath()` 호출 전에 `bPathVisible = true` 선시드 추가. 회귀 테스트 `FSpyNavigationComponentHideTriggerExitReshowsImmediatelyTest` 추가(Task 5).
- **개정 2차(code-reviewer BLOCKER 반영, 구현 완료 후 검토)**: 1차 수정은 "안"만 우회했을 뿐 "밖"으로 나온 프레임은 여전히 `EvaluateHysteresisVisibility`(`bPreviouslyVisible==true` 분기, `ArrivalHideDistanceCm=300cm` 문턱)를 탔다 — 트리거 half-extent가 300cm 미만인 타겟(§3-4가 승인한 75cm 케이스)에서 나가자마자 `RemainingPathLength`<300cm라 같은 프레임에 재차 숨는 재발 결함. `ApplyPathPoints()`가 `BoundHideTrigger.IsValid()`면 히스테리시스 계산 자체를 건너뛰고 무조건 `bPathVisible = true`로 확정하도록 수정 — 트리거 바인딩 타겟은 히스테리시스가 완전히 개입하지 않는다(spec §6 동기화 완료). 코드/테스트 파일 내 3줄 이상 주석 6곳(code-reviewer MINOR)은 2줄 이내로 압축 필요 — plan 내 대응 텍스트도 함께 정리.
