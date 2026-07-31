# §13 루트 파사드 준수 리팩터링 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** `ASpyCharacter` 도메인의 외부 소비자·형제 컴포넌트가 하위 컴포넌트를 `FindComponentByClass` 로 직접 탐색하는 것을 없애고, 루트가 제공하는 인터페이스 핸들로 대체한다. 동작은 보존한다.

**Architecture:** 루트(`ASpyCharacter`)가 `ISpyCharacterRoot` 를 구현해 도메인의 유일한 외부 진입점이 된다. 하위 매니저 컴포넌트 3종은 각자 인터페이스(`ISpyParkourHost`/`ISpyTargetProvider`/`ISpyGrappleHost`)를 구현하고, 루트가 InitState `DataInitialized` 시점에 1회 캐싱한 뒤 형제 컴포넌트에 주입한다. 소비자는 구체 컴포넌트 타입을 몰라도 된다. 프로토콜(호출 순서·권한 분기)은 일절 바꾸지 않는다.

**Tech Stack:** Unreal Engine 5.7 / C++ / GAS(SKGAS) / ModularGameplayActors InitState / Unreal Automation Test

**참조 spec:** `docs/superpowers/specs/2026-07-31-root-facade-refactor-design.md`

## Global Constraints

- **코딩 룰**: `.claude/rules/cpp-style.md` 전문 준수. 특히 — 주석은 `//#` 만 · `!` 단항 부정 금지(`== false` / `== nullptr`) · `auto` 금지 · 가드 절은 중괄호 없이 개행 · 그 외 분기는 한 줄이어도 중괄호 · `TObjectPtr` 사용 · include 순서(자기 자신 → UE → 프로젝트 → `.generated.h`).
- **주석 분량 2줄 내외.** 긴 사유는 spec 으로.
- **커밋 금지.** 각 Task 는 `git add` 까지만 하고 커밋 메시지(안)를 제시한다. `git commit` 을 실행하지 않는다 (`.claude/rules/git-conventions.md`).
- **브랜치·워크트리 생성 금지.** 현재 브랜치(`main`)에서 그대로 작업한다.
- **인터페이스는 순수 가상.** `UFUNCTION(BlueprintNativeEvent)` + `Execute_` 패턴을 쓰지 않는다 — 델리게이트 참조 반환을 BP 로 표현할 수 없다.
- **UHT 이름 충돌 금지.** UHT 는 접두사를 떼고 등록하므로 `ASpyCharacter` 와 `USpyCharacter` 는 둘 다 `SpyCharacter` 가 되어 중복 에러. 루트 인터페이스는 `USpyCharacterRoot`/`ISpyCharacterRoot` 로 접미사를 유지한다.
- **동작 보존이 최우선.** 프로토콜 순서·`HasAuthority` 분기·`EndAbility` 호출 조건을 바꾸지 않는다. 리팩터링은 "무엇을 통해 접근하는가"만 바꾼다.
- **범위 밖 코드 손대지 않음.** `GetSpyWeapon`/`GetSpyHealthComponent` 등 public getter 6개, `SpyGA_GrappleHook.cpp` 의 `UE_LOG` 디버그 출력, 주변 코드의 기존 §6·§8 위반은 그대로 둔다 (cpp-style 「적용 범위」가 변경 hunk 로 한정).
- **테스트 실행 방법**: 이 프로젝트는 Unreal Automation 을 쓴다. 에디터에서 `Tools > Session Frontend > Automation` 탭에서 `SkillProject` 필터로 실행하거나, 커맨드라인으로:
  ```
  "<UE_ROOT>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "<PROJECT>\SkillProject.uproject" -ExecCmds="Automation RunTests SkillProject; Quit" -unattended -nopause -nullrhi -log
  ```
  테스트 파일은 전체를 `#if WITH_DEV_AUTOMATION_TESTS` 로 감싸고, 등록 문자열은 `"SkillProject.<도메인>.<기능>.<케이스>"`, 구조체는 `F<Domain><Case>Test` (`.claude/project.md` test_method_naming).
- **컴파일 검증**: C++ 클래스 파일을 새로 추가한 뒤에는 `SkillProject/Launch.bat` 으로 프로젝트 파일을 재생성해야 Visual Studio 에 반영된다.

---

## File Structure

**신규 (2)**

| 파일 | 책임 |
|---|---|
| `SkillProject/Source/SkillProject/ManagerComponent/CommonInterface.Manager.h` | 매니저 컴포넌트 도메인 공용 인터페이스 3종 — `ISpyParkourHost` · `ISpyTargetProvider` · `ISpyGrappleHost` |
| `SkillProject/Source/SkillProject/Character/CommonInterface.Character.h` | 캐릭터 도메인 루트 인터페이스 — `ISpyCharacterRoot` |

`Character/CommonInterface.Character.h` 가 `ManagerComponent/CommonInterface.Manager.h` 를 include 한다 (반환 타입 `TScriptInterface<ISpyParkourHost>` 등). 같은 모듈 내이며 역방향 의존은 없다.

**수정 (13)**

| 파일 | 변경 |
|---|---|
| `ManagerComponent/SpyParkourManagerComponent.h` | `ISpyParkourHost` implement, 델리게이트 접근자 추가 |
| `ManagerComponent/SpyTargetingManagerComponent.h` | `ISpyTargetProvider` implement |
| `ManagerComponent/SpyGrappleTargetingComponent.h` | `ISpyGrappleHost` implement, 델리게이트 접근자 추가 |
| `Character/SpyCharacter.h/.cpp` | `ISpyCharacterRoot` implement, 핸들 4개 캐싱 + 형제 주입 + `AddMotionWarpTarget` |
| `Character/SpyCharacterMovementComponent.h/.cpp` | 주입 수신 + `PhysicsRotation` 매 프레임 탐색 제거 |
| `ManagerComponent/SpyGrappleUIComponent.h/.cpp` | 주입 수신, `BeginPlay` 형제 탐색 제거 |
| `AbilitySystem/Skill/Move/SpyGA_SkillMove_Vault.cpp` | 탐색 4곳 → 루트 경유 |
| `AbilitySystem/Skill/Move/SpyGA_SkillMove_HangUp.cpp` | 탐색 4곳 → 루트 경유 |
| `AbilitySystem/Movement/SpyGA_WallClimb.cpp` | 탐색 2곳 → 루트 경유 |
| `AbilitySystem/Movement/SpyGA_GrappleHook.cpp` | 탐색 3곳 → 루트 경유 |
| `AbilitySystem/Skill/SpyGA_Targeting.cpp` · `SpyGA_Death.cpp` | 탐색 1곳씩 → 루트 경유 |
| `System/SpyPlayerController.cpp` | 탐색 2곳 → 루트 경유 |
| `Character/AnimInstance/SpyCharacterAnimInstance.cpp` | 탐색 1곳 → 루트 경유 |

**테스트 (2)**

| 파일 | 변경 |
|---|---|
| `Character/Tests/SpyRootFacadeTests.cpp` | 신규 — 인터페이스 구현·조립 검증 |
| `Character/Tests/SpyCharacterAIRotationTests.cpp` | 기존 유지 + 주입 상태별 케이스 추가 |

---

## Task 1: 매니저 컴포넌트 인터페이스 3종

**Files:**
- Create: `SkillProject/Source/SkillProject/ManagerComponent/CommonInterface.Manager.h`
- Modify: `SkillProject/Source/SkillProject/ManagerComponent/SpyParkourManagerComponent.h`
- Modify: `SkillProject/Source/SkillProject/ManagerComponent/SpyTargetingManagerComponent.h`
- Modify: `SkillProject/Source/SkillProject/ManagerComponent/SpyGrappleTargetingComponent.h`
- Test: `SkillProject/Source/SkillProject/Character/Tests/SpyRootFacadeTests.cpp`

**Interfaces:**
- Consumes: 없음 (첫 Task)
- Produces: `ISpyParkourHost` · `ISpyTargetProvider` · `ISpyGrappleHost` — 시그니처는 Step 1 코드 블록이 SoT. Task 2~7 이 이 타입들을 `TScriptInterface<>` 로 받는다.

- [ ] **Step 1: `ManagerComponent/CommonInterface.Manager.h` 작성 — 델리게이트·구조체 이동 포함**

**순환 include 를 반드시 먼저 끊는다.** 인터페이스는 `FSyncMotionWarpingDataDelegate` 등을 반환하고, 그 델리게이트는 지금 `SpyParkourManagerComponent.h` 에 있다. 컴포넌트 헤더가 인터페이스를 상속하려면 `CommonInterface.h` 를 include 해야 하므로 양방향이 된다.

**해결 (선택지 아님 — 이대로 한다)**: 아래 6개를 `SpyParkourManagerComponent.h` / `SpyGrappleTargetingComponent.h` 에서 **잘라내어** `CommonInterface.h` 로 옮기고, 의존 방향을 `컴포넌트 헤더 → CommonInterface.h` 단일 방향으로 만든다.

| 이동 대상 | 원래 위치 |
|---|---|
| `FMotionWarpingData` (USTRUCT) | `SpyParkourManagerComponent.h:199-228` |
| `FClimbData` (USTRUCT) | `SpyParkourManagerComponent.h:159-197` |
| `FClimbWallData` (USTRUCT) | `SpyParkourManagerComponent.h:136-157` |
| `FSyncMotionWarpingDataDelegate` | `SpyParkourManagerComponent.h:255` |
| `FSyncClilmbDataDelegate` | `SpyParkourManagerComponent.h:256` |
| `FOnGrappleTargetChanged` | `SpyGrappleTargetingComponent.h:12` |

나머지 구조체(`FParkourWallBaseData`·`FVaultData`·`FVaultWallData`·`FWallData`·`FHangUpData`)는 인터페이스가 쓰지 않으므로 **그대로 둔다.**

`FClimbData`/`FClimbWallData`/`FMotionWarpingData` 를 참조하는 기존 헤더 4개(`SpyCharacterMovementComponent.h`·`SpyGA_WallClimb.h`·`SpyGA_SkillMove_Vault.h`·`SpyGA_SkillMove_HangUp.h`)는 모두 `SpyParkourManagerComponent.h` 를 include 하고 있고, 그 헤더가 이제 `CommonInterface.h` 를 include 하므로 **수정 없이 계속 컴파일된다.**

메서드는 **현재 호출부가 실제로 쓰는 것만** 담는다. 컴포넌트의 public 메서드를 전부 옮기지 않는다.

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "CommonInterface.generated.h"

//# 아래 3개 USTRUCT 과 3개 델리게이트는 SpyParkourManagerComponent.h /
//# SpyGrappleTargetingComponent.h 에서 이동해 왔다 — 인터페이스가 참조하므로 순환을 피한다.

USTRUCT(BlueprintType)
struct FClimbWallData
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere)
	FVector NormalVector;
	UPROPERTY(VisibleAnywhere)
	FVector HitVector;

	FClimbWallData()
		: NormalVector(FVector::ZeroVector)
		, HitVector(FVector::ZeroVector)
	{
	}

	void Clear()
	{
		NormalVector = FVector::ZeroVector;
		HitVector = FVector::ZeroVector;
	}
};

//# FClimbData / FMotionWarpingData 는 SpyParkourManagerComponent.h 의 원본을
//# 필드·생성자·Clear() 까지 그대로 옮긴다. 내용을 바꾸지 않는다.

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSyncMotionWarpingDataDelegate, FMotionWarpingData, InVaultData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSyncClilmbDataDelegate, const FClimbData&, InClimbData, const FClimbWallData&, InClimbWallData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGrappleTargetChanged, AActor*, NewTarget);

//# 파쿠르 프로토콜 제공자 — USpyParkourManagerComponent 가 구현한다.
//# GA 는 이 인터페이스로만 접근하고 구체 컴포넌트 타입을 알지 않는다.
UINTERFACE(MinimalAPI)
class USpyParkourHost : public UInterface { GENERATED_BODY() };

class ISpyParkourHost
{
	GENERATED_BODY()

public:
	virtual bool CanVaultAction() = 0;
	virtual void SetVaultMotionWarpingData() = 0;
	virtual void SetHangUpMotionWarpingData(const FVector& HitVector) = 0;
	virtual bool TryToggleClimbAction() = 0;
	virtual void SetFreeMoveMode(bool bInFreeMoveMode) = 0;

	virtual FSyncMotionWarpingDataDelegate& OnVaultMotionWarping() = 0;
	virtual FSyncMotionWarpingDataDelegate& OnHangUpMotionWarping() = 0;
	virtual FSyncClilmbDataDelegate& OnClimb() = 0;
};

//# 타깃 제공자 — USpyTargetingManagerComponent 가 구현한다.
UINTERFACE(MinimalAPI)
class USpyTargetProvider : public UInterface { GENERATED_BODY() };

class ISpyTargetProvider
{
	GENERATED_BODY()

public:
	virtual TWeakObjectPtr<AActor> GetTarget() const = 0;
	virtual bool IsTargetValid() const = 0;
	virtual bool FindTarget(float Radius) = 0;
	virtual void SetCurrentTarget(AActor* NewTarget) = 0;
};

//# 그래플 타깃 제공자 — USpyGrappleTargetingComponent 가 구현한다.
UINTERFACE(MinimalAPI)
class USpyGrappleHost : public UInterface { GENERATED_BODY() };

class ISpyGrappleHost
{
	GENERATED_BODY()

public:
	virtual AActor* GetLocalCachedTarget() const = 0;
	virtual AActor* GetCurrentGrappleTarget() const = 0;
	virtual FOnGrappleTargetChanged& OnGrappleTargetChanged() = 0;
};
```

- [ ] **Step 2: `USpyParkourManagerComponent` 가 implement**

`SpyParkourManagerComponent.h` 를 수정한다. 델리게이트 멤버는 이름이 인터페이스 접근자와 겹치지 않으므로 그대로 두고, 접근자만 추가한다.

```cpp
//# include 에 추가 (기존 include 아래, .generated.h 위)
#include "ManagerComponent/CommonInterface.Manager.h"

//# 클래스 선언 변경
class SKILLPROJECT_API USpyParkourManagerComponent : public UActorComponent, public ISpyParkourHost

//# public: 구역에 추가 — 기존 멤버 함수는 이미 같은 시그니처라 override 만 붙인다
public:
	//# ISpyParkourHost
	virtual bool CanVaultAction() override;
	virtual void SetVaultMotionWarpingData() override;
	virtual void SetHangUpMotionWarpingData(const FVector& HitVector) override;
	virtual bool TryToggleClimbAction() override;
	virtual void SetFreeMoveMode(bool bInFreeMoveMode) override;

	virtual FSyncMotionWarpingDataDelegate& OnVaultMotionWarping() override { return OnVaultMotionWarpingData; }
	virtual FSyncMotionWarpingDataDelegate& OnHangUpMotionWarping() override { return OnHangUpMotionWarpingData; }
	virtual FSyncClilmbDataDelegate& OnClimb() override { return OnClimbData; }
```

> 기존 선언 `UFUNCTION(BlueprintCallable) bool TryToggleClimbAction();` 등에 `virtual`/`override` 를 붙인다. **`.cpp` 정의는 건드리지 않는다** — 시그니처가 동일하다. Step 1 에서 이 헤더의 델리게이트 3종·구조체 3종을 잘라냈으므로, 그 자리에 `#include "ManagerComponent/CommonInterface.Manager.h"` 만 남는다.

- [ ] **Step 3: `USpyTargetingManagerComponent` 가 implement**

```cpp
#include "ManagerComponent/CommonInterface.Manager.h"

class SKILLPROJECT_API USpyTargetingManagerComponent : public UActorComponent, public ISpyTargetProvider

public:
	//# ISpyTargetProvider
	virtual TWeakObjectPtr<AActor> GetTarget() const override { return CurrentTarget.Get(); }
	virtual bool IsTargetValid() const override;
	virtual bool FindTarget(float Radius) override;
	virtual void SetCurrentTarget(AActor* NewTarget) override;
```

기존 `GetTarget()`/`IsTargetValid()`/`FindTarget()`/`SetCurrentTarget()` 선언을 위 형태로 바꾼다. `.cpp` 정의는 그대로.

- [ ] **Step 4: `USpyGrappleTargetingComponent` 가 implement**

```cpp
#include "ManagerComponent/CommonInterface.Manager.h"

class SKILLPROJECT_API USpyGrappleTargetingComponent : public UActorComponent, public ISpyGrappleHost

public:
	//# ISpyGrappleHost
	virtual AActor* GetCurrentGrappleTarget() const override { return CurrentGrappleTarget; }
	virtual AActor* GetLocalCachedTarget() const override { return LocalCachedTarget.Get(); }
	virtual FOnGrappleTargetChanged& OnGrappleTargetChanged() override { return OnGrappleTargetChangedDelegate; }
```

> `OnGrappleTargetChanged` 는 현재 **멤버 변수 이름**이다. 접근자와 이름이 충돌하므로 멤버를 `OnGrappleTargetChangedDelegate` 로 리네임하고, 기존 참조부(`SpyGrappleTargetingComponent.cpp` 내부 Broadcast 호출, `SpyGrappleUIComponent.cpp:29`)를 함께 고친다. BP 에서 `BlueprintAssignable` 로 바인딩한 곳이 있으면 리네임이 끊기므로, 리네임 전에 `Content/` 에서 `OnGrappleTargetChanged` 문자열을 검색해 확인한다.

- [ ] **Step 5: 실패하는 테스트 작성 — `SpyRootFacadeTests.cpp` 신규**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ManagerComponent/CommonInterface.Manager.h"
#include "ManagerComponent/SpyParkourManagerComponent.h"
#include "ManagerComponent/SpyTargetingManagerComponent.h"
#include "ManagerComponent/SpyGrappleTargetingComponent.h"

//# ---------------------------------------------------------------------------
//# §13 루트 파사드 규약 스위트.
//# 하위 컴포넌트가 인터페이스를 구현하고, 루트가 조립 시점에 핸들을 채우는지 박제한다.
//# ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyRootFacadeComponentsImplementInterfacesTest,
	"SkillProject.Character.RootFacade.ComponentsImplementInterfaces",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyRootFacadeComponentsImplementInterfacesTest::RunTest(const FString& Parameters)
{
	USpyParkourManagerComponent* Parkour = NewObject<USpyParkourManagerComponent>();
	USpyTargetingManagerComponent* Targeting = NewObject<USpyTargetingManagerComponent>();
	USpyGrappleTargetingComponent* Grapple = NewObject<USpyGrappleTargetingComponent>();

	TestTrue(TEXT("Parkour 컴포넌트가 ISpyParkourHost 를 구현한다"),
		Parkour->GetClass()->ImplementsInterface(USpyParkourHost::StaticClass()));
	TestTrue(TEXT("Targeting 컴포넌트가 ISpyTargetProvider 를 구현한다"),
		Targeting->GetClass()->ImplementsInterface(USpyTargetProvider::StaticClass()));
	TestTrue(TEXT("Grapple 컴포넌트가 ISpyGrappleHost 를 구현한다"),
		Grapple->GetClass()->ImplementsInterface(USpyGrappleHost::StaticClass()));

	//# TScriptInterface 로 감싼 뒤에도 호출이 도달하는지 — 캐스팅 경로 검증
	TScriptInterface<ISpyTargetProvider> Provider(Targeting);
	TestNotNull(TEXT("TScriptInterface 가 인터페이스 포인터를 잡는다"), Provider.GetInterface());
	TestFalse(TEXT("타깃 미설정 시 IsTargetValid 는 false"), Provider->IsTargetValid());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
```

- [ ] **Step 6: 테스트가 실패하는지 확인**

Step 1~4 를 되돌린 상태에서는 컴파일 자체가 실패한다(`USpyParkourHost` 미정의). Step 1~4 적용 후 컴파일이 통과하고 테스트가 PASS 하는 것으로 검증을 대신한다. 컴파일 실패 → 통과 전환이 이 Task 의 red→green 이다.

- [ ] **Step 7: 빌드 & 테스트 실행**

```
SkillProject/Launch.bat
```
로 프로젝트 파일 재생성 후 에디터/VS 에서 빌드. 이어서:
```
Automation RunTests SkillProject.Character.RootFacade
```
Expected: `ComponentsImplementInterfaces` PASS.

- [ ] **Step 8: 스테이징 (커밋하지 않음)**

```bash
git add SkillProject/Source/SkillProject/ManagerComponent/CommonInterface.Manager.h \
        SkillProject/Source/SkillProject/ManagerComponent/SpyParkourManagerComponent.h \
        SkillProject/Source/SkillProject/ManagerComponent/SpyTargetingManagerComponent.h \
        SkillProject/Source/SkillProject/ManagerComponent/SpyGrappleTargetingComponent.h \
        SkillProject/Source/SkillProject/Character/Tests/SpyRootFacadeTests.cpp
```
커밋 메시지(안): `[Refactor] CommonInterface — 매니저 컴포넌트 인터페이스 3종 신설`

---

## Task 2: 루트 인터페이스 + 조립점

**Files:**
- Create: `SkillProject/Source/SkillProject/Character/CommonInterface.Character.h`
- Modify: `SkillProject/Source/SkillProject/Character/SpyCharacter.h`
- Modify: `SkillProject/Source/SkillProject/Character/SpyCharacter.cpp`
- Test: `SkillProject/Source/SkillProject/Character/Tests/SpyRootFacadeTests.cpp`

**Interfaces:**
- Consumes: Task 1 의 `ISpyParkourHost` · `ISpyTargetProvider` · `ISpyGrappleHost`
- Produces: `ISpyCharacterRoot` — `GetParkourHost()` / `GetTargetProvider()` / `GetGrappleHost()` / `PushCameraCollisionSuppress()` / `PopCameraCollisionSuppress()` / `AddMotionWarpTarget(FName, const FVector&, const FRotator&)`. Task 3~7 이 전부 이 인터페이스로 접근한다.

- [ ] **Step 1: `Character/CommonInterface.Character.h` 작성**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ManagerComponent/CommonInterface.Manager.h"

#include "CommonInterface.generated.h"

//# 캐릭터 도메인의 유일한 외부 진입점 (cpp-style §13).
//# 소비자는 하위 컴포넌트를 탐색하지 않고 루트에서 핸들을 받는다.
UINTERFACE(MinimalAPI)
class USpyCharacterRoot : public UInterface { GENERATED_BODY() };

class ISpyCharacterRoot
{
	GENERATED_BODY()

public:
	virtual TScriptInterface<ISpyParkourHost> GetParkourHost() const = 0;
	virtual TScriptInterface<ISpyTargetProvider> GetTargetProvider() const = 0;
	virtual TScriptInterface<ISpyGrappleHost> GetGrappleHost() const = 0;

	//# 벽 밀착 액션 동안 SpringArm 콜리전 테스트를 억제한다 (참조 카운트).
	virtual void PushCameraCollisionSuppress() = 0;
	virtual void PopCameraCollisionSuppress() = 0;

	//# 엔진 UMotionWarpingComponent 는 우리 인터페이스를 구현할 수 없다.
	//# 탐색을 없애기 위해 루트가 얇은 의도 API 하나만 노출한다.
	virtual void AddMotionWarpTarget(FName WarpName, const FVector& Loc, const FRotator& Rot) = 0;
};
```

- [ ] **Step 2: `ASpyCharacter` 가 implement — 헤더**

`SpyCharacter.h` 를 수정한다.

```cpp
//# include 추가
#include "Character/CommonInterface.Character.h"

//# 전방 선언 추가
class UMotionWarpingComponent;

//# 클래스 선언 변경
class ASpyCharacter : public AModularCharacter, public IAbilitySystemInterface, public ISpyCharacterRoot

//# public: 구역에 추가
public:
	//# ISpyCharacterRoot
	virtual TScriptInterface<ISpyParkourHost> GetParkourHost() const override { return CachedParkourHost; }
	virtual TScriptInterface<ISpyTargetProvider> GetTargetProvider() const override { return CachedTargetProvider; }
	virtual TScriptInterface<ISpyGrappleHost> GetGrappleHost() const override { return CachedGrappleHost; }
	virtual void AddMotionWarpTarget(FName WarpName, const FVector& Loc, const FRotator& Rot) override;
	//# ~ISpyCharacterRoot
```

기존 `PushCameraCollisionSuppress()` / `PopCameraCollisionSuppress()` 선언에 `virtual` + `override` 를 붙인다. `.cpp` 정의는 그대로.

`private:` 구역에 캐싱 핸들을 추가한다 (§13 체크리스트 — `private` + `Transient`).

```cpp
private:
	//# InitState DataInitialized 에서 1회 캐싱 (cpp-style §8·§13)
	UPROPERTY(Transient)
	TScriptInterface<ISpyParkourHost> CachedParkourHost;

	UPROPERTY(Transient)
	TScriptInterface<ISpyTargetProvider> CachedTargetProvider;

	UPROPERTY(Transient)
	TScriptInterface<ISpyGrappleHost> CachedGrappleHost;

	UPROPERTY(Transient)
	TObjectPtr<UMotionWarpingComponent> CachedMotionWarping;

	void AssembleComponents();
```

- [ ] **Step 3: 조립 구현 — `SpyCharacter.cpp`**

조립점은 **기존 `OnAbilitySystemInitialized()`** 다. 새 훅을 만들지 않는다 — 이 콜백은 생성자에서 `SpyPawnExtensionComponent->OnAbilitySystemInitialized_RegisterAndCall(...)`(SpyCharacter.cpp:69)로 이미 등록돼 있고, `USpyPawnExtensionComponent::InitializeAbilitySystem` 이 `InitState_DataInitialized` 단계에서 호출한다. 런타임 컴포넌트는 그보다 앞선 `Spawned → DataAvailable` 전이(`SpyPawnExtensionComponent.cpp:132-167`)에서 이미 등록·초기화돼 있으므로 이 시점에 전부 존재한다.

```cpp
//# include 추가
#include "MotionWarpingComponent.h"
#include "ManagerComponent/SpyParkourManagerComponent.h"
#include "ManagerComponent/SpyTargetingManagerComponent.h"
#include "ManagerComponent/SpyGrappleTargetingComponent.h"
#include "ManagerComponent/SpyGrappleUIComponent.h"
```

`OnAbilitySystemInitialized()` 의 기존 본문 **끝에** 한 줄을 추가한다 (기존 순서를 바꾸지 않는다).

```cpp
void ASpyCharacter::OnAbilitySystemInitialized()
{
	//# ... 기존 본문 그대로 ...

	//# 하위 컴포넌트 캐싱 + 형제 간 주입 — 루트가 조립점이다 (cpp-style §13)
	AssembleComponents();
}
```

```cpp
void ASpyCharacter::AssembleComponents()
{
	//# 컴포넌트가 없는 캐릭터도 있다 — 널 핸들은 정상 상태다.
	CachedParkourHost = TScriptInterface<ISpyParkourHost>(FindComponentByClass<USpyParkourManagerComponent>());
	CachedTargetProvider = TScriptInterface<ISpyTargetProvider>(FindComponentByClass<USpyTargetingManagerComponent>());
	CachedGrappleHost = TScriptInterface<ISpyGrappleHost>(FindComponentByClass<USpyGrappleTargetingComponent>());
	CachedMotionWarping = FindComponentByClass<UMotionWarpingComponent>();

	//# 형제끼리 서로 찾아가지 않게 루트가 주입한다.
	if (USpyCharacterMovementComponent* SpyMovement = GetSpyCharacterMovementComponent())
	{
		SpyMovement->InjectTargetProvider(CachedTargetProvider);
	}

	if (USpyGrappleUIComponent* GrappleUI = FindComponentByClass<USpyGrappleUIComponent>())
	{
		GrappleUI->InjectGrappleHost(CachedGrappleHost);
	}
}

void ASpyCharacter::AddMotionWarpTarget(FName WarpName, const FVector& Loc, const FRotator& Rot)
{
	if (IsValid(CachedMotionWarping) == false)
		return;

	CachedMotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(WarpName, Loc, Rot);
}
```

> `InjectTargetProvider` 는 Task 3, `InjectGrappleHost` 는 Task 4 에서 정의한다. 이 Task 에서는 두 호출을 주석 처리해 두고 각 Task 에서 활성화하거나, Task 3·4 를 먼저 진행한 뒤 이 Step 을 완성한다. **권장: 이 Step 에서 캐싱만 넣고 주입 2줄은 Task 3·4 에서 각각 추가한다.**

- [ ] **Step 4: 테스트 추가 — 조립 결과 검증**

`SpyRootFacadeTests.cpp` 에 아래 테스트를 추가한다. 실제 InitState 흐름은 월드가 필요하므로, 조립 함수의 계약만 검증한다 — 컴포넌트를 수동으로 붙인 캐릭터에서 `AssembleComponents` 가 핸들을 채우는지.

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyRootFacadeAssemblyFillsHandlesTest,
	"SkillProject.Character.RootFacade.AssemblyFillsHandles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyRootFacadeAssemblyFillsHandlesTest::RunTest(const FString& Parameters)
{
	ASpyCharacter* Character = NewObject<ASpyCharacter>();

	//# NewObject 로 만든 컴포넌트는 Outer 의 OwnedComponents 에 등록되므로
	//# FindComponentByClass 로 잡힌다 (SpyCharacterAIRotationTests.cpp:58 과 동일한 근거).
	USpyTargetingManagerComponent* Targeting = NewObject<USpyTargetingManagerComponent>(Character);
	USpyParkourManagerComponent* Parkour = NewObject<USpyParkourManagerComponent>(Character);

	TScriptInterface<ISpyCharacterRoot> Root(Character);
	TestNotNull(TEXT("ASpyCharacter 가 ISpyCharacterRoot 를 구현한다"), Root.GetInterface());

	//# 조립 전 — 핸들이 비어 있다
	TestNull(TEXT("조립 전 TargetProvider 핸들은 비어 있다"), Root->GetTargetProvider().GetInterface());

	Character->AssembleComponents();

	TestEqual(TEXT("조립 후 TargetProvider 핸들이 실제 컴포넌트를 가리킨다"),
		Root->GetTargetProvider().GetObject(), Cast<UObject>(Targeting));
	TestEqual(TEXT("조립 후 ParkourHost 핸들이 실제 컴포넌트를 가리킨다"),
		Root->GetParkourHost().GetObject(), Cast<UObject>(Parkour));

	return true;
}
```

> `AssembleComponents()` 는 `private` 이므로 테스트에서 부를 수 없다. **`protected` 로 두고 테스트 구조체를 `friend` 선언하지 않는다** — 대신 `AssembleComponents()` 를 `public` 이 아닌 `protected` 로 두고, 테스트에서는 `ASpyCharacter` 를 상속한 테스트 전용 서브클래스로 호출을 노출한다. 그게 번거로우면 `AssembleComponents()` 를 `public` 으로 두되 주석으로 "조립점 — 외부 호출 금지, 테스트 전용 노출" 을 명시한다. **권장: `public` + 주석.** §13 이 금지하는 것은 하위 컴포넌트 노출이지 조립 함수 노출이 아니다.

- [ ] **Step 5: 빌드 & 테스트 실행**

```
Automation RunTests SkillProject.Character.RootFacade
```
Expected: `ComponentsImplementInterfaces` PASS, `AssemblyFillsHandles` PASS.

- [ ] **Step 6: 스테이징 (커밋하지 않음)**

```bash
git add SkillProject/Source/SkillProject/Character/CommonInterface.Character.h \
        SkillProject/Source/SkillProject/Character/SpyCharacter.h \
        SkillProject/Source/SkillProject/Character/SpyCharacter.cpp \
        SkillProject/Source/SkillProject/Character/Tests/SpyRootFacadeTests.cpp
```
커밋 메시지(안): `[Refactor] SpyCharacter — ISpyCharacterRoot 구현 + 조립점 추가`

---

## Task 3: MovementComponent 주입 — 매 프레임 탐색 제거 (최우선 위험)

**Files:**
- Modify: `SkillProject/Source/SkillProject/Character/SpyCharacterMovementComponent.h`
- Modify: `SkillProject/Source/SkillProject/Character/SpyCharacterMovementComponent.cpp:80`
- Modify: `SkillProject/Source/SkillProject/Character/SpyCharacter.cpp` (Task 2 Step 3 의 주입 1줄 활성화)
- Test: `SkillProject/Source/SkillProject/Character/Tests/SpyCharacterAIRotationTests.cpp`

**Interfaces:**
- Consumes: Task 1 의 `ISpyTargetProvider`, Task 2 의 `AssembleComponents()`
- Produces: `USpyCharacterMovementComponent::InjectTargetProvider(TScriptInterface<ISpyTargetProvider>)` — Task 2 의 조립부가 호출한다.

**이 Task 가 이 플랜의 유일한 실질 회귀 위험 지점이다.** 아래 표를 그대로 구현한다.

| 상태 | 조건 | 행동 | 오늘의 대응 경로 |
|---|---|---|---|
| 주입 전 | `bTargetProviderResolved == false` | "타깃 없음" 경로 (`bOrientRotationToMovement = true` + `Super`) | 컴포넌트 존재 + 타깃 없음 |
| 주입됨·유효 | 핸들 유효 | 기존 타깃팅 분기 그대로 | 동일 |
| 주입됨·널 | `bTargetProviderResolved == true` + 핸들 널 | `break` | 컴포넌트 자체가 없는 캐릭터 |

**왜 "널 → break" 가 아닌가**: 오늘 `FindComponentByClass` 는 컴포넌트 등록 시점(`DataAvailable`)부터 항상 non-null 이라, `break` 는 타깃팅 컴포넌트가 아예 없는 캐릭터에서만 발화한다. 주입은 `DataInitialized` 에 성립하므로 그 사이 구간에서 컴포넌트를 가진 플레이어도 핸들이 널이 된다. 그때 `break` 하면 회전이 통째로 사라진다.

- [ ] **Step 1: 실패하는 테스트 작성**

`SpyCharacterAIRotationTests.cpp` 에 추가한다. **기존 테스트는 한 줄도 지우거나 약화하지 않는다.**

```cpp
//# ---------------------------------------------------------------------------
//# 주입 상태별 PhysicsRotation 동치 스위트 (§13 리팩터링 회귀 방어).
//# 주입 전 구간이 "컴포넌트는 있으나 타깃이 없음" 과 같은 경로를 타야 한다 —
//# 여기가 어긋나면 스폰 직후 몇 프레임 동안 플레이어 회전이 사라진다.
//# ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMovementRotationBeforeInjectionTest,
	"SkillProject.Character.Rotation.BeforeInjectionMatchesNoTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMovementRotationBeforeInjectionTest::RunTest(const FString& Parameters)
{
	ASpyCharacter* Character = NewObject<ASpyCharacter>();
	USpyCharacterMovementComponent* Movement =
		Cast<USpyCharacterMovementComponent>(Character->GetCharacterMovement());
	if (Movement == nullptr)
	{
		AddError(TEXT("USpyCharacterMovementComponent 를 얻지 못했다"));
		return false;
	}

	//# 플레이어 컨트롤러를 붙여 타깃팅 분기로 진입시킨다
	//# (SpyCharacterAIRotationTests 의 기존 픽스처와 동일한 이유로 Possess 대신 직접 대입)
	APlayerController* PC = NewObject<APlayerController>();
	Character->Controller = PC;

	//# 주입 전 상태 — bOrientRotationToMovement 가 true 로 켜져야 한다 ("타깃 없음" 경로)
	Movement->bOrientRotationToMovement = false;
	Movement->PhysicsRotation(0.016f);

	TestTrue(TEXT("주입 전에는 타깃 없음 경로를 타 bOrientRotationToMovement 가 true 가 된다"),
		Movement->bOrientRotationToMovement);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMovementRotationInjectedNullTest,
	"SkillProject.Character.Rotation.InjectedNullSkipsRotation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMovementRotationInjectedNullTest::RunTest(const FString& Parameters)
{
	ASpyCharacter* Character = NewObject<ASpyCharacter>();
	USpyCharacterMovementComponent* Movement =
		Cast<USpyCharacterMovementComponent>(Character->GetCharacterMovement());
	if (Movement == nullptr)
	{
		AddError(TEXT("USpyCharacterMovementComponent 를 얻지 못했다"));
		return false;
	}

	APlayerController* PC = NewObject<APlayerController>();
	Character->Controller = PC;

	//# 타깃팅 컴포넌트가 없는 캐릭터 — 조립이 널 핸들로 해결된 상태
	Movement->InjectTargetProvider(TScriptInterface<ISpyTargetProvider>(nullptr));

	Movement->bOrientRotationToMovement = false;
	Movement->PhysicsRotation(0.016f);

	TestFalse(TEXT("주입됐지만 핸들이 널이면 회전 분기를 건너뛴다 (기존 nullptr 경로와 동일)"),
		Movement->bOrientRotationToMovement);

	return true;
}
```

- [ ] **Step 2: 테스트가 실패하는지 확인**

Run: `Automation RunTests SkillProject.Character.Rotation`
Expected: 컴파일 실패 — `InjectTargetProvider` 미정의. 이것이 red 상태다.

- [ ] **Step 3: 헤더에 주입 API 추가**

`SpyCharacterMovementComponent.h`:

```cpp
//# include 추가
#include "ManagerComponent/CommonInterface.Manager.h"

//# public: 구역에 추가
public:
	//# 루트가 조립 시점에 주입한다 — 형제를 직접 탐색하지 않는다 (cpp-style §13)
	void InjectTargetProvider(TScriptInterface<ISpyTargetProvider> InProvider);

//# private: 구역에 추가
private:
	UPROPERTY(Transient)
	TScriptInterface<ISpyTargetProvider> TargetProvider;

	//# 주입이 실행됐는지. 널 핸들("컴포넌트 없음")과 주입 전 상태를 구분하기 위해 필요하다.
	bool bTargetProviderResolved = false;
```

- [ ] **Step 4: 주입 함수 구현 + `PhysicsRotation` 치환**

`SpyCharacterMovementComponent.cpp` 에 추가:

```cpp
void USpyCharacterMovementComponent::InjectTargetProvider(TScriptInterface<ISpyTargetProvider> InProvider)
{
	//# 핸들 유효성과 무관하게 해결됨으로 표시한다 — 널도 "컴포넌트 없음" 이라는 확정 정보다.
	TargetProvider = InProvider;
	bTargetProviderResolved = true;
}
```

`PhysicsRotation` 의 80-82줄을 아래로 교체한다. **그 위(60-78)와 아래(84-109)는 한 줄도 바꾸지 않는다.**

```cpp
			//# (기존) USpyTargetingManagerComponent* TargetingComp = OwnerCharacter->FindComponentByClass<...>();
			//# (기존) if (TargetingComp == nullptr) break;

			//# 주입 전이면 "타깃 없음" 경로로 흘린다 — 스폰 직후 프레임의 기존 동작과 동치.
			if (bTargetProviderResolved == false)
			{
				OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = true;
				Super::PhysicsRotation(DeltaTime);
				break;
			}

			//# 주입됐는데 널이면 타깃팅 컴포넌트가 없는 캐릭터다 — 기존 nullptr 경로와 동일하게 빠진다.
			ISpyTargetProvider* Provider = TargetProvider.GetInterface();
			if (Provider == nullptr)
				break;
```

이어지는 84-109 줄에서 `TargetingComp->` 를 `Provider->` 로 바꾼다 (`GetTarget()` 호출 3곳). 그 외 로직·주석은 그대로 둔다.

- [ ] **Step 5: 루트의 주입 호출 활성화**

`SpyCharacter.cpp` 의 `AssembleComponents()` 에서 Task 2 Step 3 에 명시한 주입 블록 중 Movement 쪽을 활성화한다.

```cpp
	if (USpyCharacterMovementComponent* SpyMovement = GetSpyCharacterMovementComponent())
	{
		SpyMovement->InjectTargetProvider(CachedTargetProvider);
	}
```

- [ ] **Step 6: 테스트 실행**

Run: `Automation RunTests SkillProject.Character.Rotation`
Expected: 기존 AI 스핀 테스트 전부 PASS + 신규 2개 PASS.

- [ ] **Step 7: 스테이징 (커밋하지 않음)**

```bash
git add SkillProject/Source/SkillProject/Character/SpyCharacterMovementComponent.h \
        SkillProject/Source/SkillProject/Character/SpyCharacterMovementComponent.cpp \
        SkillProject/Source/SkillProject/Character/SpyCharacter.cpp \
        SkillProject/Source/SkillProject/Character/Tests/SpyCharacterAIRotationTests.cpp
```
커밋 메시지(안): `[Refactor] SpyCharacterMovementComponent — 매 프레임 형제 탐색 제거`

---

## Task 4: GrappleUIComponent 주입

**Files:**
- Modify: `SkillProject/Source/SkillProject/ManagerComponent/SpyGrappleUIComponent.h`
- Modify: `SkillProject/Source/SkillProject/ManagerComponent/SpyGrappleUIComponent.cpp:19-30`
- Modify: `SkillProject/Source/SkillProject/Character/SpyCharacter.cpp` (주입 1줄 활성화)

**Interfaces:**
- Consumes: Task 1 의 `ISpyGrappleHost`, Task 2 의 `AssembleComponents()`
- Produces: `USpyGrappleUIComponent::InjectGrappleHost(TScriptInterface<ISpyGrappleHost>)`

**타이밍 변화 주의**: 오늘 델리게이트 구독은 `BeginPlay`(컴포넌트 등록 = `DataAvailable`) 에서 일어난다. 주입은 `DataInitialized` 라 **몇 프레임 늦어진다.** 그 사이 그래플 타깃이 바뀔 수 있는가? `USpyGrappleTargetingComponent::TickComponent` 는 `IsLocalController() == false` 면 즉시 반환하고, 로컬 컨트롤러 성립은 possess 이후다. 실질적으로 구독 누락은 발생하지 않지만, 안전을 위해 주입 시점에 **현재 타깃으로 1회 동기화**한다.

- [ ] **Step 1: 헤더에 주입 API 추가**

```cpp
//# include 추가
#include "ManagerComponent/CommonInterface.Manager.h"

//# public: 구역에 추가
public:
	//# 루트가 조립 시점에 주입한다 (cpp-style §13)
	void InjectGrappleHost(TScriptInterface<ISpyGrappleHost> InHost);

//# private: 구역에 추가
private:
	UPROPERTY(Transient)
	TScriptInterface<ISpyGrappleHost> GrappleHost;
```

- [ ] **Step 2: `BeginPlay` 에서 형제 탐색 제거**

`SpyGrappleUIComponent.cpp:23-30` 의 아래 블록을 **삭제**한다.

```cpp
	//# 삭제 대상
	AActor* Owner = GetOwner();
	if (Owner == nullptr) return;

	if (USpyGrappleTargetingComponent* TargetComp =
		Owner->FindComponentByClass<USpyGrappleTargetingComponent>())
	{
		TargetComp->OnGrappleTargetChanged.AddDynamic(this, &USpyGrappleUIComponent::OnTargetChanged);
	}
```

이어지는 `APawn* Pawn = Cast<APawn>(Owner);` 가 `Owner` 를 쓰므로, `Owner` 획득과 널 가드는 남긴다. 삭제하는 것은 `FindComponentByClass` 블록뿐이다.

- [ ] **Step 3: 주입 함수 구현**

```cpp
void USpyGrappleUIComponent::InjectGrappleHost(TScriptInterface<ISpyGrappleHost> InHost)
{
	GrappleHost = InHost;

	ISpyGrappleHost* Host = GrappleHost.GetInterface();
	if (Host == nullptr)
		return;

	//# 구독이 BeginPlay 보다 늦어지므로 현재 타깃으로 1회 동기화한다.
	Host->OnGrappleTargetChanged().AddUniqueDynamic(this, &USpyGrappleUIComponent::OnTargetChanged);
	OnTargetChanged(Host->GetLocalCachedTarget());
}
```

`EndPlay` 에 구독 해제가 이미 있는지 확인하고, 없으면 추가한다.

```cpp
	if (ISpyGrappleHost* Host = GrappleHost.GetInterface())
	{
		Host->OnGrappleTargetChanged().RemoveDynamic(this, &USpyGrappleUIComponent::OnTargetChanged);
	}
```

- [ ] **Step 4: 루트의 주입 호출 활성화**

`SpyCharacter.cpp` 의 `AssembleComponents()`:

```cpp
	if (USpyGrappleUIComponent* GrappleUI = FindComponentByClass<USpyGrappleUIComponent>())
	{
		GrappleUI->InjectGrappleHost(CachedGrappleHost);
	}
```

- [ ] **Step 5: 빌드 & 수동 확인**

에디터 PIE 로 그래플 타깃에 조준했을 때 프롬프트 위젯이 뜨고 커스텀 뎁스 하이라이트가 켜지는지 확인한다. 자동화 테스트는 위젯·뷰포트 의존이라 만들지 않는다.

- [ ] **Step 6: 스테이징 (커밋하지 않음)**

```bash
git add SkillProject/Source/SkillProject/ManagerComponent/SpyGrappleUIComponent.h \
        SkillProject/Source/SkillProject/ManagerComponent/SpyGrappleUIComponent.cpp \
        SkillProject/Source/SkillProject/Character/SpyCharacter.cpp
```
커밋 메시지(안): `[Refactor] SpyGrappleUIComponent — 형제 탐색 제거, 루트 주입 수신`

---

## Task 5: 파쿠르 GA 3종 치환 (10곳)

**Files:**
- Modify: `SkillProject/Source/SkillProject/AbilitySystem/Skill/Move/SpyGA_SkillMove_Vault.cpp:38,71,100,120`
- Modify: `SkillProject/Source/SkillProject/AbilitySystem/Skill/Move/SpyGA_SkillMove_HangUp.cpp:37,56,84,95`
- Modify: `SkillProject/Source/SkillProject/AbilitySystem/Movement/SpyGA_WallClimb.cpp:86,145`

**Interfaces:**
- Consumes: Task 2 의 `ISpyCharacterRoot` (`GetParkourHost()` / `AddMotionWarpTarget()` / `PushCameraCollisionSuppress()`)
- Produces: 없음 (말단 소비자)

**공통 변환 규칙** — 프로토콜 순서·`HasAuthority` 분기·`EndAbility` 조건을 바꾸지 않는다. 컴포넌트를 못 찾았을 때의 행동은 핸들이 널일 때 그대로 유지한다.

```cpp
//# (전) 구체 컴포넌트 탐색
if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
{
	if (USpyParkourManagerComponent* ParkourComponent = OwnerCharacter->FindComponentByClass<USpyParkourManagerComponent>())
	{
		//# ... ParkourComponent-> ...
	}
}

//# (후) 루트 인터페이스 경유
TScriptInterface<ISpyCharacterRoot> Root(GetAvatarActorFromActorInfo());
if (ISpyCharacterRoot* RootPtr = Root.GetInterface())
{
	if (ISpyParkourHost* Parkour = RootPtr->GetParkourHost().GetInterface())
	{
		//# ... Parkour-> ...
	}
}
```

델리게이트 접근은 멤버 직접 참조에서 접근자 호출로 바뀐다:
- `ParkourComponent->OnVaultMotionWarpingData.AddDynamic(...)` → `Parkour->OnVaultMotionWarping().AddDynamic(...)`
- `ParkourComponent->OnHangUpMotionWarpingData.AddDynamic(...)` → `Parkour->OnHangUpMotionWarping().AddDynamic(...)`
- `ParkourComponent->OnClimbData.AddUniqueDynamic(...)` → `Parkour->OnClimb().AddUniqueDynamic(...)`

- [ ] **Step 1: `SpyGA_SkillMove_Vault.cpp` 치환**

- 24-28줄: `Cast<ASpyCharacter>` + `PushCameraCollisionSuppress()` → `TScriptInterface<ISpyCharacterRoot>` 경유. `CameraSuppressedCharacter` 멤버 타입도 `TWeakObjectPtr<ASpyCharacter>` 에서 `TWeakInterfacePtr<ISpyCharacterRoot>` 로 바꾸거나, 액터 포인터를 유지하고 `PopCameraCollisionSuppress` 호출만 인터페이스로 한다. **권장: 액터 포인터 유지 + 호출만 인터페이스** — 수명 추적 로직을 건드리지 않는다.
- 36-50줄: 위 공통 규칙대로. `CanVaultAction()` / `OnVaultMotionWarping()` / `SetVaultMotionWarpingData()` / `EndAbility` 분기 유지.
- 69-82줄(`EndAbility`): `HasAuthority` 블록과 `bFreeMoveEngaged` 조건 그대로. `SetFreeMoveMode(false)` 만 인터페이스 경유.
- 98-124줄(`OnSyncMotionWarpingData`): 델리게이트 `RemoveDynamic`, `SetFreeMoveMode(true)`, 미션 진행 블록 순서 유지. **120-124줄의 `UMotionWarpingComponent` 탐색 2줄은 루트 API 로 대체**:

```cpp
	//# (전)
	if (UMotionWarpingComponent* MotionWarpingComponent = OwnerCharacter->FindComponentByClass<UMotionWarpingComponent>())
	{
		MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(MotionWarpingStartName, InVaultData.StartLoc, InVaultData.StartRot);
		MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(MotionWarpingEndName, InVaultData.EndLoc, InVaultData.EndRot);
	}

	//# (후)
	if (ISpyCharacterRoot* RootPtr = Root.GetInterface())
	{
		RootPtr->AddMotionWarpTarget(MotionWarpingStartName, InVaultData.StartLoc, InVaultData.StartRot);
		RootPtr->AddMotionWarpTarget(MotionWarpingEndName, InVaultData.EndLoc, InVaultData.EndRot);
	}
```

`SetMoveState(true); PlayMontage();` 는 위치·조건 그대로 둔다.

- [ ] **Step 2: `SpyGA_SkillMove_HangUp.cpp` 치환**

- 34-41줄: `OnHangUpMotionWarping().AddDynamic(...)` + `SetHangUpMotionWarpingData(LedgeLocation)`. `TriggerEventData` 널 체크와 `else { EndAbility(...) }` 분기 그대로.
- `EndAbility` 의 `HasAuthority` + `bFreeMoveEngaged` 블록: `SetFreeMoveMode(false)` 만 인터페이스 경유.
- `OnSyncMotionWarpingData`: `RemoveDynamic` → `SetFreeMoveMode(true)` → MotionWarping 2줄을 `AddMotionWarpTarget` 로. Vault 와 동일 형태.

- [ ] **Step 3: `SpyGA_WallClimb.cpp` 치환**

- 84-91줄(`TryToggleClimbAction`): `Parkour->OnClimb().AddUniqueDynamic(this, &USpyGA_WallClimb::StartWallClimb);` + `return Parkour->TryToggleClimbAction();`. 컴포넌트를 못 찾았을 때의 반환값(현재 함수 끝의 `return` 값)을 그대로 유지한다.
- 145-148줄: `Parkour->OnClimb().RemoveDynamic(this, &USpyGA_WallClimb::StartWallClimb);`

- [ ] **Step 4: 빌드 & 수동 확인**

에디터 PIE 로 3종 동작을 확인한다.
- Vault: 낮은 장애물 앞에서 넘기 입력 → 몽타주 + 워핑 정상, 끝나면 이동 모드 복구
- HangUp: 매달린 상태에서 올라서기 → 워핑 정상
- WallClimb: 벽 등반 진입/이탈, 이탈 후 이동 방향 회전 복구

자동화 테스트는 만들지 않는다 — 몽타주·워핑·레플리케이션 의존이라 월드 없이 재현할 수 없다. 이 Task 의 안전망은 "프로토콜을 바꾸지 않았다" 는 diff 검토다.

- [ ] **Step 5: 스테이징 (커밋하지 않음)**

```bash
git add SkillProject/Source/SkillProject/AbilitySystem/Skill/Move/SpyGA_SkillMove_Vault.cpp \
        SkillProject/Source/SkillProject/AbilitySystem/Skill/Move/SpyGA_SkillMove_HangUp.cpp \
        SkillProject/Source/SkillProject/AbilitySystem/Movement/SpyGA_WallClimb.cpp
```
커밋 메시지(안): `[Refactor] SpyGA_SkillMove — 파쿠르 컴포넌트 탐색을 루트 경유로`

---

## Task 6: 그래플 GA 치환 (3곳)

**Files:**
- Modify: `SkillProject/Source/SkillProject/AbilitySystem/Movement/SpyGA_GrappleHook.cpp:33,84,136`

**Interfaces:**
- Consumes: Task 2 의 `ISpyCharacterRoot::GetGrappleHost()`
- Produces: 없음

- [ ] **Step 1: 3곳 치환**

```cpp
//# (전) 33줄 — CanActivate 계열
USpyGrappleTargetingComponent* TargetComp = Pawn->FindComponentByClass<USpyGrappleTargetingComponent>();
return TargetComp && TargetComp->GetLocalCachedTarget() != nullptr;

//# (후)
TScriptInterface<ISpyCharacterRoot> Root(Pawn);
ISpyCharacterRoot* RootPtr = Root.GetInterface();
ISpyGrappleHost* Host = RootPtr ? RootPtr->GetGrappleHost().GetInterface() : nullptr;
return Host != nullptr && Host->GetLocalCachedTarget() != nullptr;
```

84줄(서버 분기)·136줄(클라 분기)도 같은 형태로 바꾼다. **`UE_LOG` 디버그 출력과 `EndAbility` 조건은 그대로 둔다** — 로그의 `TargetComp->GetClass()->GetName()` 은 인터페이스에서 얻을 수 없으므로 `Host` 의 `_getUObject()->GetClass()->GetName()` 으로 대체하거나, 로그 문구를 유지하되 널 여부만 출력하도록 최소 수정한다.

> `TargetComp == nullptr → EndAbility` 경로는 `Host == nullptr` 로 그대로 이어진다. 조건을 완화·강화하지 않는다.

- [ ] **Step 2: 빌드 & 수동 확인**

PIE 로 그래플 타깃 조준 → 발사 → 케이블 이동이 서버/클라 양쪽에서 정상인지 확인한다.

- [ ] **Step 3: 스테이징 (커밋하지 않음)**

```bash
git add SkillProject/Source/SkillProject/AbilitySystem/Movement/SpyGA_GrappleHook.cpp
```
커밋 메시지(안): `[Refactor] SpyGA_GrappleHook — 그래플 컴포넌트 탐색을 루트 경유로`

---

## Task 7: 타깃팅 소비자 4곳 치환

**Files:**
- Modify: `SkillProject/Source/SkillProject/AbilitySystem/Skill/SpyGA_Targeting.cpp:27`
- Modify: `SkillProject/Source/SkillProject/AbilitySystem/Skill/SpyGA_Death.cpp:20`
- Modify: `SkillProject/Source/SkillProject/System/SpyPlayerController.cpp:70,77`
- Modify: `SkillProject/Source/SkillProject/Character/AnimInstance/SpyCharacterAnimInstance.cpp:124`

**Interfaces:**
- Consumes: Task 2 의 `ISpyCharacterRoot::GetTargetProvider()`
- Produces: 없음

- [ ] **Step 1: GA 2곳 치환**

```cpp
//# SpyGA_Targeting.cpp:27 (전)
USpyTargetingManagerComponent* TargetingComp = OwnerCharacter->FindComponentByClass<USpyTargetingManagerComponent>();

//# (후)
TScriptInterface<ISpyCharacterRoot> Root(GetAvatarActorFromActorInfo());
ISpyCharacterRoot* RootPtr = Root.GetInterface();
ISpyTargetProvider* TargetingComp = RootPtr ? RootPtr->GetTargetProvider().GetInterface() : nullptr;
```

이후 가드 절과 호출부는 그대로 둔다 — `if (TargetingComp == nullptr) break;` · `FindTarget(500.f)` · `GetTarget()` 순서·조건 불변. 세 메서드 모두 Task 1 의 `ISpyTargetProvider` 에 이미 들어 있다.

`SpyGA_Death.cpp:20` 의 `TargetingComp->SetCurrentTarget(nullptr)` 도 같은 형태.

- [ ] **Step 2: `SpyPlayerController` 치환**

`TargetingComp` 멤버 타입을 `TObjectPtr<USpyTargetingManagerComponent>` 에서 `TScriptInterface<ISpyTargetProvider>` 로 바꾼다 (헤더 확인 후 수정). 70·77줄:

```cpp
//# (전)
TargetingComp = GetPawn()->FindComponentByClass<USpyTargetingManagerComponent>();

//# (후)
TScriptInterface<ISpyCharacterRoot> Root(GetPawn());
TargetingComp = Root.GetInterface() ? Root->GetTargetProvider() : TScriptInterface<ISpyTargetProvider>(nullptr);
```

나머지 사용처는 `SpyPlayerController.cpp:106` 한 곳이다. 널 판정을 인터페이스 기준으로 바꾼다:

```cpp
//# (전)
AActor* TargetActor = (ControlledPawn != nullptr && TargetingComp != nullptr) ? TargetingComp->GetTarget().Get() : nullptr;

//# (후)
AActor* TargetActor = (ControlledPawn != nullptr && TargetingComp.GetInterface() != nullptr) ? TargetingComp->GetTarget().Get() : nullptr;
```

헤더의 `TObjectPtr<USpyTargetingManagerComponent> TargetingComp;`(`SpyPlayerController.h:46`) 를 `TScriptInterface<ISpyTargetProvider> TargetingComp;` 로 바꾸고 `UPROPERTY(Transient)` 를 유지한다.

- [ ] **Step 3: `SpyCharacterAnimInstance` 치환**

```cpp
//# (전) 124줄
if (USpyTargetingManagerComponent* TargetingComp = Player->FindComponentByClass<USpyTargetingManagerComponent>())

//# (후)
TScriptInterface<ISpyCharacterRoot> Root(Player);
ISpyCharacterRoot* RootPtr = Root.GetInterface();
ISpyTargetProvider* TargetingComp = RootPtr ? RootPtr->GetTargetProvider().GetInterface() : nullptr;
if (TargetingComp != nullptr)
```

내부 `GetTarget()` / `IsTargeting = true` 로직은 그대로.

> **주의**: AnimInstance 는 `NativeUpdateAnimation` 에서 매 프레임 돈다. 위 코드는 탐색을 없애지만 매 프레임 `TScriptInterface` 생성은 남는다 — 이는 캐스팅 1회라 탐색과 비용이 다르다. 더 줄이려면 `NativeInitializeAnimation` 에서 루트 핸들을 1회 캐싱한다. **권장: 1회 캐싱** (§8).

- [ ] **Step 4: 최종 검증 — 탐색 잔여 0건 확인**

```bash
cd SkillProject/Source/SkillProject
grep -rn "FindComponentByClass<USpy" --include=*.cpp --include=*.h .
```

허용되는 잔여만 남아야 한다:
- `Character/SpyCharacter.cpp` — 루트가 자기 컴포넌트를 찾는 `AssembleComponents()` 와 `OnDeath`(사망 1회 경로)
- 정적 헬퍼 `FindHealthComponent` / `FindLevelComponent` / `FindMissionComponent` / `FindPawnExtensionComponent` / `FindInputComponent` (범위 밖)
- `Character/Tests/` 하위 테스트 픽스처

- [ ] **Step 5: 전체 테스트 실행**

Run: `Automation RunTests SkillProject`
Expected: 기존 테스트 전부 PASS + 신규 테스트 PASS.

- [ ] **Step 6: 스테이징 (커밋하지 않음)**

```bash
git add SkillProject/Source/SkillProject/AbilitySystem/Skill/SpyGA_Targeting.cpp \
        SkillProject/Source/SkillProject/AbilitySystem/Skill/SpyGA_Death.cpp \
        SkillProject/Source/SkillProject/System/SpyPlayerController.h \
        SkillProject/Source/SkillProject/System/SpyPlayerController.cpp \
        SkillProject/Source/SkillProject/Character/AnimInstance/SpyCharacterAnimInstance.h \
        SkillProject/Source/SkillProject/Character/AnimInstance/SpyCharacterAnimInstance.cpp
```
커밋 메시지(안): `[Refactor] SpyTargetingManagerComponent — 타깃팅 소비자를 루트 경유로`

---

## 완료 기준

- [ ] `FindComponentByClass<USpy*>` 잔여가 위 Step 4 의 허용 목록뿐이다
- [ ] `SkillProject.Character.Rotation` 스위트 전부 PASS (기존 + 신규 2)
- [ ] `SkillProject.Character.RootFacade` 스위트 전부 PASS
- [ ] PIE 수동 확인: Vault / HangUp / WallClimb / 그래플 / 타깃 락 동작 변화 없음
- [ ] cpp-style §13 체크리스트 ①③④ 통과 (②는 범위 밖, ⑤는 기존 통과)
- [ ] 커밋 0건 — 전부 stage 상태로 사용자에게 인계
