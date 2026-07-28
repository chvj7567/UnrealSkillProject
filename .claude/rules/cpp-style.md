# C++ 코딩 스타일

> §1~7 은 문법 스타일, §8~12 는 설계 원칙이다.
> 모듈 의존 방향·서버 권한은 [unreal-infra.md](unreal-infra.md), UI 매니저/위젯 베이스 규칙은 [plugin-skuicore.md](plugin-skuicore.md) 가 SoT다.

---

## 1. UE5 네이밍 접두사

| 접두사 | 대상 |
|--------|------|
| `U` | UObject 파생 클래스 |
| `A` | AActor 파생 클래스 |
| `F` | 일반 구조체 |
| `I` | 인터페이스 |
| `E` | enum class |
| `T` | 템플릿 클래스 |

---

## 2. UPROPERTY 지정자

- 에디터에서만 설정: `EditDefaultsOnly`
- 런타임 읽기 전용 노출: `VisibleAnywhere`
- BP에서 읽기 허용: `BlueprintReadOnly`
- 레플리케이션: `Replicated` + `GetLifetimeReplicatedProps` 등록 필수
- UObject 포인터는 `TObjectPtr<>` 사용 (raw pointer 지양)

---

## 3. 헤더 include 순서

```cpp
#include "MyClass.h"          // 자기 자신
#include "OtherUEHeader.h"    // UE 헤더
#include "ProjectHeader.h"    // 프로젝트 헤더
#include "MyClass.generated.h" // generated는 항상 마지막
```

**포매터는 이 순서를 강제하지 않는다.** 위 순서는 알파벳순이 아니라 의미 기반이라 clang-format 이 자동 판정할 수 없다 — UE 헤더(`GameFramework/Character.h`)와 프로젝트 헤더(`Character/SpyCharacter.h`)를 정규식으로 안정적으로 구분할 수 없기 때문이다. 그래서 `SkillProject/.clang-format` 에 `SortIncludes: Never` 를 지정해 포매터가 include 를 재정렬하지 못하게 막아 뒀다(그전에는 저장할 때마다 알파벳순으로 되돌려 이 룰을 깨뜨렸다). **순서 준수는 작성자와 code-reviewer 의 몫이다.**

---

## 4. 주석 스타일

- 한 줄 주석은 항상 `//#`로 시작 — 일반 `//`나 `///`, `/* */` 사용 금지.
  - 헤더의 doxygen 스타일 주석도 `//#`로 통일.
  - **예외**: UE 자동 생성 저작권 헤더는 그대로 유지 (새 클래스 생성 시 자동 삽입되므로 매번 정리하지 않음).
    - `// Fill out your copyright notice in the Description page of Project Settings.`
    - `// Copyright Epic Games, Inc. All Rights Reserved.`
  - `#if WITH_EDITOR` 등 전처리기 디렉티브는 주석이 아니므로 그대로.

```cpp
//# 서버 권한에서 실행
if (HasAuthority(&ActivationInfo))
{
    //# 데미지 적용
    ApplyDamage(Target);
}
```

**주석 분량 — 2줄 내외**: 한 블록 주석은 2줄을 넘기지 않는다. 긴 설계 사유·이력은 기획서(`docs/design/`)·plan·spec 으로 옮긴다. 코드 옆 주석은 "*무엇·왜* 한 문장"으로 압축.

```cpp
//# (X) 4줄짜리 사유 설명
//# SpyPawnExtensionComponent 는 CharacterAssetData 레플리케이트 완료 + Controller 연동
//# 이후에야 InitState_DataInitialized 로 넘어간다. 그 전에 GA 를 부여하면 ActorInfo 가
//# 아직 세팅되지 않아 ActivateAbility 시점에 ActorInfo->AbilitySystemComponent 가 널이고,
//# 그 결과 ...

//# (O) 2줄 — 핵심만, 디테일은 기획서 §X 로
//# ActorInfo 미설정 상태에서 부여하면 깨진다 → DataInitialized 이후에만 GA 부여.
//# 초기화 단계 표는 기획서 §2.6 참조.
```

---

## 5. 가드 절 — 중괄호 없이 개행

`nullptr` 체크·조건 체크로 즉시 리턴하는 가드 절은 중괄호 없이 개행+들여쓰기로 작성한다. 가드 절이 아닌 모든 분기는 한 줄이어도 중괄호 필수.

```cpp
//# (O) 가드 절
if (IsValid(Comp) == false)
    return;

if (TargetActor == nullptr)
    return;

//# (X) 가드 절에 중괄호
if (IsValid(Comp) == false) { return; }

//# (O) 일반 분기 — 한 줄이어도 중괄호
if (bIsAlive == false)
{
    HandleDeath();
}

//# (X) 일반 분기 중괄호 없음
if (bIsAlive == false)
    HandleDeath();
```

---

## 6. `auto` 금지 — 명시적 타입 표기

로컬 변수 선언 시 `auto` 대신 명시적 타입을 쓴다.

```cpp
//# (X)
auto* ASC = GetAbilitySystemComponent();
auto Handles = AbilityData->GiveToAbilitySystem(ASC);

//# (O)
USKAbilitySystemComponent* ASC = GetAbilitySystemComponent();
FSpyAbilitySet_GrantedHandles Handles = AbilityData->GiveToAbilitySystem(ASC);
```

**예외** — 타입을 적을 수 없거나 적으면 오히려 해로운 경우만:
- 람다·`TFunction` 캡처 결과, 이터레이터(`for (auto It = Map.CreateIterator(); ...)`)
- 범위 기반 for 의 `for (const TPair<FName, FSoftObjectPath>& Pair : Map)` 처럼 타입이 장황한 경우는 **명시 우선**, 템플릿 컨텍스트에서 타입이 결정 불가한 경우만 `auto&`.

---

## 7. 불리언/널 비교

- `!` 단항 부정 연산자 사용 금지 — 가독성과 의도 명확성을 위해 명시적 비교 사용. `!=` 는 허용.
  - 불리언: `!bFlag` → `bFlag == false`
  - 포인터: `!Ptr` → `Ptr == nullptr`
  - 함수 반환 불리언: `!IsValid(Obj)` → `IsValid(Obj) == false`
- 예외: STL/UE 표준 표현식(`!operator bool()` 의도가 명백한 경우)은 PR 검토 시 합의로 허용. 기본은 명시적 비교.

```cpp
//# OK
if (bFreeMoveMode == false)
    return;

if (TargetActor == nullptr)
    return;

if (IsValid(Comp) == false)
    return;

if (CurrentTag != PrevTag)   //# != 는 허용
{
    RefreshSkill();
}

//# 금지
if (!bFreeMoveMode) { ... }
if (!TargetActor) { ... }
if (!IsValid(Comp)) { return; }
```

---

## 8. 클래스 종속성 최소화

클래스는 다른 클래스와의 종속성을 최대한 배제하고 작성한다.

- 구체 클래스 직접 참조 대신 **인터페이스(`UINTERFACE` + `TScriptInterface<>`)/추상화**에 의존 (DIP)
- 매니저/서브시스템 정적 호출 최소화 — 의존성은 생성자·`Initialize`·메서드 인자로 주입하거나 델리게이트로 역전
- 의존이 늘어나면 책임 분리(SRP) 또는 이벤트 기반(델리게이트·GameplayEvent) 으로 분해
- 양방향 참조 금지 — 단방향 데이터/이벤트 흐름 유지 (모듈 간 역방향 include 금지는 [unreal-infra.md](unreal-infra.md) §1)
- **런타임 액터 탐색 금지**: `TActorIterator`, `UGameplayStatics::GetAllActorsOfClass` / `GetAllActorsWithTag` 로 매 프레임 월드를 훑지 않는다. 참조는 스폰/등록 시점에 넘겨받는다.
- **컴포넌트 탐색 지양**: `FindComponentByClass` / `GetComponentByClass` / `GetComponents` 를 `Tick`·이벤트 핸들러·매 프레임 반복 경로에서 직접 호출 금지. 의존은 `UPROPERTY(EditDefaultsOnly)` 데이터 참조로 미리 와이어링하거나, 불가피하면 `BeginPlay`/InitState 초기화 시점에 **1회만** 캐싱한다.

> 이 프로젝트의 컴포넌트는 `CharacterAssetData` 목록을 읽어 런타임에 `NewObject & RegisterComponent` 로 주입된다. 따라서 컴포넌트 참조의 정석은 **"에디터에서 미리 꽂는다"가 아니라 "InitState 초기화 직후 1회 캐싱한다"** 이다.

```cpp
//# (X) 구체 매니저 직접 참조
void ASpyCharacter::OnHit()
{
    USpyUIManager::Get(this)->OpenUI(TEXT("HitFeedback"));
}

//# (O) 인터페이스/델리게이트로 역전
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCharacterHit, float);
FOnCharacterHit OnCharacterHit;   //# UI 계층이 구독한다
```

```cpp
//# (X) 매 프레임 컴포넌트 탐색 — 탐색 비용 + 결합
void ASpyCharacter::Tick(float DeltaTime)
{
    FindComponentByClass<USpyTargetingComponent>()->UpdateTarget();
}

//# (O) 초기화 시점 1회 캐싱
UPROPERTY(Transient)
TObjectPtr<USpyTargetingComponent> CachedTargeting;

void ASpyCharacter::OnDataInitialized()
{
    CachedTargeting = FindComponentByClass<USpyTargetingComponent>();
}

void ASpyCharacter::Tick(float DeltaTime)
{
    if (IsValid(CachedTargeting) == false)
        return;

    CachedTargeting->UpdateTarget();
}
```

체크리스트:
- [ ] 알아야 하는 외부 타입 0~3개 이내인가?
- [ ] 매니저 직접 참조 대신 인터페이스/델리게이트로 분리 가능한가?
- [ ] Automation 테스트에서 모킹·단독 생성이 가능한 구조인가?
- [ ] `FindComponentByClass*` / `GetAllActorsOfClass` 가 `Tick`·이벤트 경로에 없는가? (초기화 1회 캐싱으로 대체)

---

## 9. UI 계층 분리 & 위젯 캡슐화

### 9-1. 계층 분리

**이 프로젝트에는 `ModelViewViewModel` 플러그인이 활성화돼 있지 않고**, UI 생성/캐시/수명은 [plugin-skuicore.md](plugin-skuicore.md) 의 `USKUIManager` 가 이미 규정한다. 따라서 계층 분리는 아래 형태로만 적용한다.

| 계층 | 역할 | UE 매핑 |
|---|---|---|
| **Model** | 순수 데이터/상태 + 규칙. UMG 의존 금지 | `UDataAsset`, `USKAttributeSet`, GameState/PlayerState |
| **중개** | 모델 변경을 위젯이 쓰기 좋은 값으로 통지 | ASC 델리게이트(`OnHealthChanged` 등), GameplayEvent, 컴포넌트 델리게이트 |
| **View** | 표시 + 입력 수신만. 게임플레이 로직 금지 | `USKUserWidget` 파생 위젯 |

- View → Model **폴링 금지**. 값 변경은 델리게이트 구독으로 받는다.
- View 는 게임플레이 상태를 직접 바꾸지 않는다 — 입력은 ASC(`AbilityLocalInputPressed`)·컨트롤러로 넘긴다.
- Model 쪽은 위젯 타입을 알지 않는다 (서버/데디 환경에서도 컴파일·동작해야 함).
- 위젯 열기/닫기는 `USKUIManager` API 로만 한다 (`CreateWidget` + `AddToViewport` 직접 호출 금지).

> ※ 향후 `ModelViewViewModel` 플러그인을 켤 경우에만 정식 ViewModel(`UMVVMViewModelBase` + `FieldNotify`) 계층을 도입한다. 그 전까지 "ViewModel" 이라는 이름의 클래스를 새로 만들지 않는다.

### 9-2. 위젯 캡슐화 — 내부 위젯 숨기고 의도 API 만 노출

위젯은 자신이 들고 있는 내부 UMG 위젯(`UImage` / `UTextBlock` / `UProgressBar` 등)을 **`protected`/`private` + `meta = (BindWidget)`** 로 소유하고, 외부에는 **"무엇을 보여줄지" 의도 단위 메서드 API 만** 노출한다.

- 내부 위젯을 `public` 필드/`BlueprintReadWrite` 로 열지 않는다
- 외부(상위 위젯·구독부)는 그 위젯의 **API 만 호출** — 자식 위젯을 직접 참조하거나 `GetWidgetFromName` 으로 꺼내 만지지 않는다 (§8 탐색 지양과 연결)
- 위젯 내부 구조(어떤 UMG 위젯으로 그리는지)가 바뀌어도 호출부는 영향받지 않는다

```cpp
//# (X) 외부가 내부 위젯을 직접 참조/조작 — 결합도↑, 구조 변경이 호출부로 샘
UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
TObjectPtr<UProgressBar> PB_HPBar;
//  호출부:
HPBar->PB_HPBar->SetPercent(Cur / Max);

//# (O) 위젯이 내부 위젯을 protected 소유 + 의도 API 만 노출 (정석: USpyHPBar)
UCLASS()
class SKILLPROJECT_API USpyHPBar : public USKUserWidget
{
    GENERATED_BODY()

public:
    void UpdateHP(float InTargetHP, float InMaxHP);

protected:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UProgressBar> PB_HPBar;
};
//  호출부 — 위젯만 참조:
HPBar->UpdateHP(Cur, Max);
```

체크리스트:
- [ ] 자식 위젯이 모두 `protected`/`private` + `BindWidget` 인가? (`public`·`BlueprintReadWrite` 노출 없음)
- [ ] 외부가 자식 위젯을 직접 만지지 않고 의도 API 로만 호출하는가?
- [ ] 위젯이 값을 폴링하지 않고 델리게이트로 구독하는가?

---

## 10. 소유/상위 액터는 인터페이스로 참조

컴포넌트·자식 위젯·자식 액터가 소유자(Owner)나 상위 액터의 기능을 써야 할 경우, 구체 클래스 캐스팅 대신 인터페이스를 통해 접근한다.

```cpp
//# (X) 소유 구체 클래스 직접 캐스팅
ASpyCharacter* Owner = Cast<ASpyCharacter>(GetOwner());
Owner->NotifyParkourFinished();

//# (O) 인터페이스로 참조
UINTERFACE(MinimalAPI, BlueprintType)
class USpyParkourHost : public UInterface { GENERATED_BODY() };

class ISpyParkourHost
{
    GENERATED_BODY()
public:
    virtual void NotifyParkourFinished() = 0;
};

//# 소비 측 — 초기화 시점 1회 캐싱 (§8)
void USpyParkourManagerComponent::BeginPlay()
{
    Super::BeginPlay();
    CachedHost = TScriptInterface<ISpyParkourHost>(GetOwner());
}
```

- ASC 접근은 이 규칙의 대표 사례다 — `Cast<ASpyCharacter>` 대신 `IAbilitySystemInterface::GetAbilitySystemComponent()` 를 쓴다.
- 플러그인(SKGAS/SKUICore 등) 코드가 게임 타입을 알아야 하면 **반드시** 인터페이스로 뚫는다 (역방향 include 금지, [unreal-infra.md](unreal-infra.md) §1).

---

## 11. 공용 Enum — `Util/DefineEnum.h` 단일 파일

여러 시스템에서 참조되는 공용 `UENUM` 은 `SkillProject/Source/SkillProject/Util/DefineEnum.h` 에 모아 정의한다.

**공용 Enum 기준** (하나라도 해당하면 `DefineEnum.h`):
1. 에셋/UI 키 역할 — `ESpyUIType` 등
2. 2개 이상 시스템/모듈에서 참조
3. 시스템 간 통신 계약 (레플리케이트되는 상태값 포함)

단일 클래스 내부에서만 쓰는 enum(구현 디테일)은 해당 헤더에 둔다.

```cpp
//# (X) 카테고리별 한 파일씩
//  Util/ESpyUIType.h, Util/ECustomMovementMode.h ...

//# (O) 한 파일에 카테고리별 UENUM 정의 — Util/DefineEnum.h
UENUM(BlueprintType)
enum ESpyUIType : uint8
{
    None    UMETA(DisplayName = "None"),
    MainHUD UMETA(DisplayName = "MainHUD"),
};

UENUM(BlueprintType)
enum ECustomMovementMode : uint8
{
    MOVE_Default   UMETA(DisplayName = "Default"),
    MOVE_WallClimb UMETA(DisplayName = "Wall Climb"),
};
```

- 파일이 200줄 초과 or enum 카테고리 6개 이상이면 `DefineEnum.Asset.h`, `DefineEnum.Battle.h` 등 prefix 통일로 분할한다.
- **플러그인(SKGAS/SKUICore/SKAssetCore)의 enum 은 여기에 넣지 않는다** — 게임 모듈 헤더이므로 역방향 의존이 된다. 각 플러그인 자체 헤더에 둔다.
- 게임플레이 태그는 enum 이 아니다 — `SpyGameplayTags.h` 규약을 따른다 ([plugin-skgas.md](plugin-skgas.md) §2).

---

## 12. 공용 Interface — 도메인별 `CommonInterface.h`

여러 시스템에서 참조되는 공용 인터페이스는 도메인 폴더별 `CommonInterface.h` 한 파일에 모아 정의한다.

**공용 Interface 기준**:
1. 동일 도메인의 여러 구현체가 implement
2. 2개 이상 시스템에서 참조
3. `TScriptInterface<>` 로 넘겨 받는 계약

단일 구현체만 있는 내부 추상화는 해당 클래스 헤더 옆에 둔다.

```cpp
//# (X) 인터페이스마다 한 파일씩
//  Character/ISpyMover.h, Character/ISpyHealth.h ...

//# (O) 도메인 단일 파일 — Character/CommonInterface.h
UINTERFACE(MinimalAPI, BlueprintType)
class USpyMover : public UInterface { GENERATED_BODY() };
class ISpyMover
{
    GENERATED_BODY()
public:
    virtual void MoveTo(const FVector& Target) = 0;
};

UINTERFACE(MinimalAPI, BlueprintType)
class USpyHealth : public UInterface { GENERATED_BODY() };
class ISpyHealth
{
    GENERATED_BODY()
public:
    virtual bool IsAlive() const = 0;
};
```

- 파일이 200줄 초과 or 카테고리 6개 이상이면 `CommonInterface.Movement.h` 등 prefix 통일로 분할한다.
- **적용 범위 — 신규 게임 모듈 인터페이스만.** 이 규칙은 모듈 경계를 넘지 않는다: 도메인 폴더(`Character/`, `System/`, `AI/` …) 단위이며, 게임 모듈과 플러그인이 한 파일을 공유하지 않는다.
- **기존 1파일=1인터페이스는 grandfathered** — `System/SpyTeamAgentInterface.h`, 플러그인의 `SKAbilitySourceInterface.h` 등은 그대로 둔다. 같은 도메인에 인터페이스를 **새로 추가**할 때 `CommonInterface.h` 로 모으기 시작한다.

---

## 13. 기타

- `UFUNCTION(BlueprintCallable)` 없이 BP 노출 금지
- `Super::` 호출 누락 주의 (`BeginPlay`, `EndPlay`, `GetLifetimeReplicatedProps`, `ActivateAbility`)
- `IsValid()` 또는 null 체크 없이 UObject 포인터 역참조 금지

---

## 적용 범위

- 신규 작성 코드 전체
- 기존 코드는 **해당 줄을 수정할 때 함께** 변환한다. §6(`auto`)·§8(컴포넌트 탐색)에 걸리는 기존 코드가 다수 남아 있으나, 일괄 개조를 목적으로 별도 커밋을 만들지 않는다.
- code-reviewer 는 변경된 hunk 기준으로만 지적한다 — 손대지 않은 주변 코드의 위반은 리뷰 대상이 아니다.

## 예외

- **§1~7 (문법 스타일): 예외 없음.** 단 §6 은 위에 명시한 "타입을 적을 수 없는 경우"만 허용.
- **§8~12 (설계 원칙)**: 아래는 허용된 이탈이다.
  - §9 — 정식 ViewModel 계층은 `ModelViewViewModel` 플러그인을 켜기 전까지 도입하지 않는다.
  - §12 — 기존 1파일=1인터페이스 유지, 단일 구현체 internal 추상화 분리 유지.
  - 엔진/플러그인 오버라이드 시그니처가 강제하는 형태(예: 엔진이 요구하는 `virtual` 시그니처)는 스타일보다 우선한다.
