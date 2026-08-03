# C++ 코딩 스타일

> §1~7 은 문법 스타일, §8~15 는 설계 원칙이다.
> 모듈 의존 방향·서버 권한은 [unreal-infra.md](unreal-infra.md), UI 매니저/위젯 베이스 규칙은 [plugin-skuicore.md](plugin-skuicore.md) 가 SoT다.
>
> **이 문서는 프로젝트 비의존이다 — 그대로 복사해 쓴다.** 예제의 플레이스홀더는 각 프로젝트 값으로 읽는다:
> `My*` = 프로젝트 클래스 접두사(`AMyCharacter` → `AFooCharacter`) · `<GameModule>` = 게임 모듈 경로 · `MYGAME_API` = 모듈 API 매크로 · `SK*` = 재사용 플러그인 베이스(그대로).

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

**포매터는 이 순서를 강제하지 않는다.** 위 순서는 알파벳순이 아니라 의미 기반이라 clang-format 이 자동 판정할 수 없다 — UE 헤더(`GameFramework/Character.h`)와 프로젝트 헤더(`Character/MyCharacter.h`)를 정규식으로 안정적으로 구분할 수 없기 때문이다. **프로젝트 루트 `.clang-format` 에 `SortIncludes: Never` 를 반드시 넣는다** — 없으면 저장할 때마다 알파벳순으로 되돌려 이 룰이 깨진다. **순서 준수는 작성자와 code-reviewer 의 몫이다.**

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
//# Pawn 확장 컴포넌트는 CharacterAssetData 레플리케이트 완료 + Controller 연동
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
FMyAbilitySet_GrantedHandles Handles = AbilityData->GiveToAbilitySystem(ASC);
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

> 컴포넌트를 데이터(Character 에셋 등) 목록 기반으로 런타임에 `NewObject & RegisterComponent` 주입하는 구성이라면([unreal-infra.md](unreal-infra.md) §2), 컴포넌트 참조의 정석은 **"에디터에서 미리 꽂는다"가 아니라 "InitState 초기화 직후 1회 캐싱한다"** 이다.

```cpp
//# (X) 구체 매니저 직접 참조
void AMyCharacter::OnHit()
{
    UMyUIManager::Get(this)->OpenUI(TEXT("HitFeedback"));
}

//# (O) 인터페이스/델리게이트로 역전
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCharacterHit, float);
FOnCharacterHit OnCharacterHit;   //# UI 계층이 구독한다
```

```cpp
//# (X) 매 프레임 컴포넌트 탐색 — 탐색 비용 + 결합
void AMyCharacter::Tick(float DeltaTime)
{
    FindComponentByClass<UMyTargetingComponent>()->UpdateTarget();
}

//# (O) 초기화 시점 1회 캐싱
UPROPERTY(Transient)
TObjectPtr<UMyTargetingComponent> CachedTargeting;

void AMyCharacter::OnDataInitialized()
{
    CachedTargeting = FindComponentByClass<UMyTargetingComponent>();
}

void AMyCharacter::Tick(float DeltaTime)
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

**`ModelViewViewModel` 플러그인을 켜지 않은 프로젝트에서는**, UI 생성/캐시/수명은 [plugin-skuicore.md](plugin-skuicore.md) 의 `USKUIManager` 가 이미 규정한다. 따라서 계층 분리는 아래 형태로만 적용한다.

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

//# (O) 위젯이 내부 위젯을 protected 소유 + 의도 API 만 노출
UCLASS()
class MYGAME_API UMyHPBar : public USKUserWidget
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
AMyCharacter* Owner = Cast<AMyCharacter>(GetOwner());
Owner->NotifyParkourFinished();

//# (O) 인터페이스로 참조
UINTERFACE(MinimalAPI, BlueprintType)
class UMyParkourHost : public UInterface { GENERATED_BODY() };

class IMyParkourHost
{
    GENERATED_BODY()
public:
    virtual void NotifyParkourFinished() = 0;
};

//# 소비 측 — 초기화 시점 1회 캐싱 (§8)
void UMyParkourManagerComponent::BeginPlay()
{
    Super::BeginPlay();
    CachedHost = TScriptInterface<IMyParkourHost>(GetOwner());
}
```

- ASC 접근은 이 규칙의 대표 사례다 — `Cast<AMyCharacter>` 대신 `IAbilitySystemInterface::GetAbilitySystemComponent()` 를 쓴다.
- 플러그인(SKGAS/SKUICore 등) 코드가 게임 타입을 알아야 하면 **반드시** 인터페이스로 뚫는다 (역방향 include 금지, [unreal-infra.md](unreal-infra.md) §1).

---

## 11. 공용 Enum — `Util/DefineEnum.h` 단일 파일

여러 시스템에서 참조되는 공용 `UENUM` 은 게임 모듈의 `<GameModule>/Util/DefineEnum.h` 한 파일에 모아 정의한다.

**공용 Enum 기준** (하나라도 해당하면 `DefineEnum.h`):
1. 에셋/UI 키 역할 — `EMyUIType` 등
2. 2개 이상 시스템/모듈에서 참조
3. 시스템 간 통신 계약 (레플리케이트되는 상태값 포함)

단일 클래스 내부에서만 쓰는 enum(구현 디테일)은 해당 헤더에 둔다.

```cpp
//# (X) 카테고리별 한 파일씩
//  Util/EMyUIType.h, Util/ECustomMovementMode.h ...

//# (O) 한 파일에 카테고리별 UENUM 정의 — Util/DefineEnum.h
UENUM(BlueprintType)
enum EMyUIType : uint8
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
- 게임플레이 태그는 enum 이 아니다 — 프로젝트 태그 헤더(`MyGameplayTags.h`) 규약을 따른다 ([plugin-skgas.md](plugin-skgas.md) §2).

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
//  Character/IMyMover.h, Character/IMyHealth.h ...

//# (O) 도메인 단일 파일 — Character/CommonInterface.h
UINTERFACE(MinimalAPI, BlueprintType)
class UMyMover : public UInterface { GENERATED_BODY() };
class IMyMover
{
    GENERATED_BODY()
public:
    virtual void MoveTo(const FVector& Target) = 0;
};

UINTERFACE(MinimalAPI, BlueprintType)
class UMyHealth : public UInterface { GENERATED_BODY() };
class IMyHealth
{
    GENERATED_BODY()
public:
    virtual bool IsAlive() const = 0;
};
```

- 파일이 200줄 초과 or 카테고리 6개 이상이면 `CommonInterface.Movement.h` 등 prefix 통일로 분할한다.
- **⚠ 한 모듈에 `CommonInterface.h` 를 두 개 둘 수 없다.** UHT 는 생성 헤더를 `Intermediate/Build/<Platform>/<Target>/Inc/<Module>/UHT/<파일명>.generated.h` 라는 **모듈당 플랫 경로**에 만든다 — 소스 폴더 구조를 미러링하지 않는다. 도메인 폴더가 달라도 basename 이 같으면 두 헤더가 같은 `CommonInterface.generated.h` 를 가리켜 빌드가 깨진다.
  - 따라서 한 모듈에서 도메인이 둘 이상이면 **처음부터 위 분할 규칙을 적용해** basename 을 유일하게 만든다: `Character/CommonInterface.Character.h`, `ManagerComponent/CommonInterface.Manager.h`.
  - 모듈이 다르면 basename 이 같아도 무방하다 (플러그인과 게임 모듈은 서로 다른 UHT 출력 폴더를 쓴다).
- **적용 범위 — 신규 게임 모듈 인터페이스만.** 이 규칙은 모듈 경계를 넘지 않는다: 도메인 폴더(`Character/`, `System/`, `AI/` …) 단위이며, 게임 모듈과 플러그인이 한 파일을 공유하지 않는다.
- **기존 1파일=1인터페이스는 grandfathered** — 이미 있는 단일 인터페이스 헤더(게임 모듈·플러그인 양쪽)는 그대로 둔다. 같은 도메인에 인터페이스를 **새로 추가**할 때 `CommonInterface.h` 로 모으기 시작한다.

---

## 13. 루트 파사드 — 도메인 하나에 진입 클래스 하나

여러 컴포넌트로 쪼개지는 도메인 액터(캐릭터·차량·적·건물 등)는 **루트 액터 하나가 그 도메인의 유일한 외부 진입점**이 된다. 상세 기능은 하위 컴포넌트로 나누되, 전부 루트가 `private`/`protected` `TObjectPtr` 로 소유하고 외부에 노출하지 않는다.

§9-2(위젯 캡슐화)의 게임플레이 판이다. UI 든 캐릭터든 원칙은 같다 — **내부 구성은 숨기고 의도 API 만 연다.**

### 루트의 책임 3가지

1. **조립점** — 하위 컴포넌트끼리 서로 주입하는 자리는 루트다. 컴포넌트를 데이터 목록 기반으로 런타임 주입하는 구성이라면, 조립은 InitState 를 관장하는 Pawn 확장 컴포넌트([plugin-modulargameplayactors.md](plugin-modulargameplayactors.md) §3)의 `DataInitialized` 콜백에서 **1회** 수행한다 (§8 캐싱과 같은 시점). 하위가 소유자·형제를 `FindComponentByClass` 로 거슬러 찾지 않는다 (§8).
   - 그 콜백은 조립 **시점**일 뿐 두 번째 진입점이 아니다 — 외부 API 는 여전히 루트에만 있다.
2. **프레임 순서 소유** — 하위 갱신 순서에 의존이 있으면 하위는 `PrimaryComponentTick.bCanEverTick = false` 로 두고 루트의 `Tick` 이 순서를 쥔다. TickGroup·컴포넌트 등록 순서에 의존하지 않는다. (`AddTickPrerequisiteComponent` 는 루트 소유 순서의 대체재가 아니다.)
3. **의도 API 노출** — 외부는 하위 컴포넌트가 아니라 루트에게 묻는다.

### 인터페이스는 미리 뺀다

루트 액터는 **대응 인터페이스를 함께 정의한다 — 외부 소비자가 아직 없어도 미리 뺀다.** 인터페이스는 도메인 `CommonInterface.h` (§12) 에 둔다.

- **이름은 액터와 겹치지 않게** 접미사를 붙인다(`AMyBot` + `IMyBotRoot`). UHT 는 접두사를 떼고 등록하므로 `AMyBot` 과 `UMyBot` 은 둘 다 `MyBot` 이 되어 중복 이름 에러가 난다.

소비자가 생긴 뒤에 승격하면 이미 구체 클래스로 물린 호출부를 전부 고쳐야 한다. 처음부터 인터페이스로 열어두면 그 비용이 0 이고, test-engineer 가 테스트 더블을 붙일 자리도 함께 생긴다.

§10 과 짝이다 — §10 은 **하위가 소유자를** 인터페이스로 참조하는 쪽, §13 은 **루트가 하위를** 인터페이스로 주입하는 쪽이다.

```cpp
//# (X) 하위 컴포넌트를 외부에 공개 — 결합도↑, 내부 구조 변경이 호출부로 샘
UCLASS()
class AMyBot : public AModularCharacter
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite)
    TObjectPtr<UMyBotMoverComponent> Mover;
};
//  호출부:
Bot->Mover->MoveTo(Target);

//# (O) 루트가 private 소유 + 인터페이스로 의도 API 만 노출
UINTERFACE(MinimalAPI, BlueprintType)
class UMyBotRoot : public UInterface { GENERATED_BODY() };
class IMyBotRoot
{
    GENERATED_BODY()
public:
    virtual FVector GetBotLocation() const = 0;
    virtual void SetControlEnabled(bool bEnabled) = 0;
};

UCLASS()
class AMyBot : public AModularCharacter, public IMyBotRoot
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;

    //# IMyBotRoot
    virtual FVector GetBotLocation() const override { return GetActorLocation(); }
    virtual void SetControlEnabled(bool bEnabled) override;

private:
    //# InitState DataInitialized 에서 1회 캐싱 (§8)
    UPROPERTY(Transient)
    TObjectPtr<UMyBotSensorComponent> CachedSensor;

    UPROPERTY(Transient)
    TObjectPtr<UMyBotMoverComponent> CachedMover;
};

void AMyBot::OnDataInitialized()
{
    CachedSensor = FindComponentByClass<UMyBotSensorComponent>();
    CachedMover = FindComponentByClass<UMyBotMoverComponent>();

    if (IsValid(CachedMover) == false)
        return;

    //# 조립점 — 하위끼리 직접 찾아가지 않게 루트가 인터페이스로 주입
    CachedMover->InjectSensor(TScriptInterface<IMySensor>(CachedSensor));
}

//# 프레임 순서 소유 — 감지 결과가 이동의 입력이라 순서 의존이 있다
void AMyBot::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (IsValid(CachedSensor) == false || IsValid(CachedMover) == false)
        return;

    CachedSensor->TickSensor(DeltaTime);
    CachedMover->TickMove(DeltaTime);
}
//  호출부 — 루트(또는 IMyBotRoot)만 참조:
Bot->SetControlEnabled(false);
```

### 금지

- 하위 컴포넌트를 `public` 필드 / `BlueprintReadWrite` 로 노출
- 외부가 하위 컴포넌트를 직접 참조·조작 (`FindComponentByClass` 로 꺼내 만지는 것 포함, §8)
- 하위끼리 서로를 구체 클래스로 직접 찾아가기 — 루트가 인터페이스로 주입한다

### 체크리스트

- [ ] 이 도메인의 외부 진입 클래스가 **하나**인가?
- [ ] 캐싱된 하위 컴포넌트 핸들이 모두 `private`/`protected` + `UPROPERTY(Transient)` 인가?
- [ ] 루트에 대응 인터페이스가 정의돼 있는가? (소비자 유무와 무관)
- [ ] 하위가 소유자/형제를 `FindComponentByClass` 로 찾지 않고 루트에게 주입받는가?
- [ ] (순서 의존이 있다면) 하위 Tick 이 꺼져 있고 루트가 갱신 순서를 쥐는가?

---

## 14. 게임 데이터 저작 — DataTable 우선, DataAsset 은 정적 설정/에셋 참조 전용

**콘텐츠·밸런스 데이터**(무기·장비·적 스탯 등 반복 로우가 있고 플레이 밸런싱으로 자주 바뀌는 값)는 **`UDataTable`**(CSV/JSON import + `FTableRowBase` 서브클래스 row struct)로 저작한다. **`UDataAsset` 은 오브젝트 참조가 필요하거나 거의 바뀌지 않는 정적 배선**(이름→경로 룩업, Config 수치, 컴포넌트/어빌리티 세트 조립 등)에 한정한다.

이 구분은 로우 개수가 아니라 **자주 바뀌는가 + 오브젝트 참조가 필요한가** 로 가른다.

**판단 기준**:
- **`UDataTable` 대상**: 반복 로우가 있는 밸런스 데이터(무기 여러 종·적 스탯 테이블), 자주 튜닝되는 수치. 기획자가 CSV/JSON 로 직접 편집하는 형태가 적합한 데이터.
- **`UDataAsset` 대상**: `TSoftObjectPtr`/`TSoftClassPtr` 등 **에셋 참조를 필드로 가져야 하는 데이터**(`UDataTable` 의 row struct 는 `USTRUCT` 라 소프트 참조는 담을 수 있지만 다른 `UDataAsset`·컴포넌트 목록 조립처럼 구조가 복잡한 배선은 `UDataAsset` 이 자연스럽다), 이름→경로 룩업(`USKAssetData`), Config 수치(`SpyMovementConfig` 등 거의 안 바뀌는 인프라성 값).

```cpp
//# (X) 반복되는 밸런스 로우를 DataAsset 배열 필드로
UCLASS()
class UMyWeaponData : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly)
    TArray<FMyWeaponRow> Weapons;   //# 무기 종류가 늘어날 때마다 이 배열을 직접 편집
};

//# (O) 밸런스 로우는 DataTable row struct 로만
USTRUCT(BlueprintType)
struct FMyWeaponRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    int32 Damage = 10;

    UPROPERTY(EditAnywhere)
    float RoundsPerMinute = 600.f;
};
//  로드: AssetManager 경유 이름 룩업으로 UDataTable 을 얻고 FindRow<FMyWeaponRow>(RowName) 로 조회 (plugin-skassetcore.md §2)
```

**예외** — `UDataAsset` 로 남기는 경우:
- 다른 `UDataAsset`·컴포넌트 클래스·어빌리티 세트처럼 **오브젝트 참조 조립**이 핵심인 데이터 (`USpyCharacterAssetData`, `USpyAbilityData` 등 기존 파이프라인)
- 이름→경로 룩업, 빌드/인프라성 Config 값

체크리스트:
- [ ] 이 데이터가 "반복 로우가 있는 밸런스 콘텐츠"인가, "오브젝트 참조 조립/정적 설정"인가?
- [ ] 반복 로우형이면 `UDataAsset` 배열 필드 대신 `UDataTable` + `FTableRowBase` row struct 로 저작했는가?
- [ ] row struct 에셋 로드가 `USKAssetManager` 경유인가 (하드코딩 경로 금지, plugin-skassetcore.md §2)?

### 14-1. `DataTable` row struct 설계 절차 — 정규화

**새 `DataTable`의 row struct 를 설계할 때는, 지금 당장 다른 테이블과 연관이 없어 보여도 처음부터 아래 절차를 따른다.** 나중에 관계가 생겨서 리팩터링하는 게 아니라, 설계 시점에 미리 분해해 둔다.

1. **이 로우의 핵심 엔티티가 무엇인지 먼저 정한다** — "이 테이블은 무엇 하나를 표현하는가"를 한 문장으로 답할 수 있어야 한다.
2. **그 엔티티 고유의 데이터만 로우에 남긴다** — 다른 개념(다른 엔티티, 다른 로우, 외부 콘텐츠)을 가리키는 필드는 전부 후보에서 뺀다.
3. **관계로 보이는 필드는 "선택적 관계"인지 "필수 1:1 관계"인지로 가른다.**
   - **선택적 관계**(일부 로우에만 존재) → 별도 "관계 테이블"로 분리하고 상위 엔티티의 ID로 매칭한다. 관계가 있는 로우만 존재하게 해서 `-1`/`0`/`None` 같은 sentinel 값을 쓰지 않는다 — sentinel 값은 "관계가 없다"를 "관계가 있는데 값이 -1이다"로 오독하게 만든다. 지금은 상대편 테이블이 없어도, 그 관계 자체를 처음부터 별도 테이블로 모델링한다.
   - **필수 1:1 관계**(모든 로우에 항상 정확히 하나씩 존재) → sentinel 문제가 애초에 없으므로, 엔티티가 그 참조를 관계 테이블 없이 직접 필드로 들어도 된다.
4. **복합 키는 "하나의 로우가 그룹 내 여러 항목 중 하나"일 때 쓴다** — 단일 ID 하나로는 "이 로우들이 한 그룹에 속하고, 그 그룹 안에서 순서·구분이 있다"는 사실을 표현할 수 없다. 이럴 때 `GroupId`(그룹 식별) + `Index`(그룹 내 순서/구분) 조합을 키로 쓴다. 그룹 안에 원소가 정확히 하나뿐인 관계라면 쓰지 않는다 — 단일 ID로 충분하다.
5. **관계 테이블 이름은 부모 엔티티 이름을 접두사로 언더스코어(`_`)로 잇는다** — `<부모>_<관계명>` 형태로 짓는다(예: `Quest` 엔티티의 보상 관계 테이블 → `Quest_Reward`). 그 관계 테이블 아래에 또 하위 관계가 있으면 한 단계씩 계속 이어 붙인다 — `<부모>_<관계명>_<하위관계명>`. 이름만 보고 어느 엔티티에서 몇 단계 파생됐는지 알 수 있어야 한다.

```cpp
//# (X) 관계·선택적 데이터를 엔티티 로우에 직접 얹고, 없는 경우 sentinel 로 채움
USTRUCT(BlueprintType)
struct FMyQuestRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere) FText Title;
    UPROPERTY(EditAnywhere) int32 GiverNPCId = -1;   //# NPC 관련 아니면 -1 (sentinel)
    UPROPERTY(EditAnywhere) int32 RewardGold = 0;    //# 보상 없으면 0 (sentinel)
};

//# (O) 엔티티는 고유 데이터만, 선택적 관계는 이름 규칙(§14-1-5)을 따르는 별도 테이블로
USTRUCT(BlueprintType)
struct FMyQuestRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere) FText Title;
};

USTRUCT(BlueprintType)
struct FMyQuest_GiverRow : public FTableRowBase   //# Quest 의 선택적 관계 — NPC가 있는 퀘스트만 로우 존재
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere) int32 QuestId = 0;
    UPROPERTY(EditAnywhere) int32 NPCId = 0;
};

USTRUCT(BlueprintType)
struct FMyQuest_RewardRow : public FTableRowBase  //# Quest 의 선택적 관계 — 보상이 있는 퀘스트만 로우 존재
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere) int32 QuestId = 0;
    UPROPERTY(EditAnywhere) int32 RewardItemId = 0;   //# 필수 1:1이 아니라 "어떤 아이템인지"를 가리키는 선택적 관계라 별도 테이블에 둔다
};

USTRUCT(BlueprintType)
struct FMyItemRow : public FTableRowBase   //# Item 은 Quest 와 무관한 별개의 최상위 엔티티
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere) FText ItemName;
    UPROPERTY(EditAnywhere) int32 ItemEffectId = 0;  //# 모든 아이템이 항상 하나씩 갖는 필수 1:1 관계라
                                                      //# 관계 테이블 없이 직접 참조해도 된다 (§14-1-3)
};

USTRUCT(BlueprintType)
struct FMyQuest_StepRow : public FTableRowBase   //# Quest 의 그룹 내 여러 로우 — 복합 키(§14-1-4)
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere) int32 QuestId = 0;    //# 그룹 식별
    UPROPERTY(EditAnywhere) int32 StepIndex = 0;  //# 그룹 내 순서
    UPROPERTY(EditAnywhere) FText StepText;
};
```

체크리스트 (새 row struct 를 설계할 때마다 순서대로 확인):
- [ ] 이 테이블의 핵심 엔티티를 한 문장으로 정의했는가?
- [ ] 로우에 다른 엔티티의 ID를 직접 들고 있는 필드가 있는가? 있다면 모든 로우에 항상 존재하는 필수 1:1 관계인가, 일부 로우에만 있는 선택적 관계인가?
- [ ] 선택적 관계인데 sentinel 값(`-1`/`0`/`None`)으로 채우고 있지 않은가? (관계 테이블로 분리했는가)
- [ ] 한 로우가 "그룹 내 여러 항목 중 하나"를 표현하는가? 그렇다면 단일 ID 대신 `GroupId`+`Index` 복합 키를 썼는가?
- [ ] 관계 테이블 이름이 `<부모>_<관계명>`(중첩이면 `<부모>_<관계명>_<하위관계명>`) 규칙을 따르는가?

---

## 15. 매직 넘버 금지 — 이름 있는 상수/Config 데이터로 추출

로직 안에 의미를 알 수 없는 숫자 리터럴(매직 넘버)을 직접 쓰지 않는다.

- **반복 사용되거나 게임플레이 튜닝 대상인 수치**는 하드코딩 금지 — §14 기준에 따라 Config `UDataAsset` 또는 `UDataTable` 로 이전한다.
- **코드 전용 상수**(버퍼 크기, 타임아웃, 반경 등)는 이름 있는 상수(`static constexpr`)로 선언하거나 `UPROPERTY(EditDefaultsOnly)` 로 노출한다.
- `0`/`1`/`-1`처럼 배열 인덱스·불리언 대용·루프 경계로 의미가 자명한 값은 매직 넘버로 보지 않는다.

```cpp
//# (X) 의미를 알 수 없는 리터럴
if (Distance < 300.f)
{
    TryInteract();
}

//# (O) 이름 있는 상수 또는 Config 로 노출
static constexpr float InteractRangeCm = 300.f;

if (Distance < InteractRangeCm)
{
    TryInteract();
}
```

체크리스트:
- [ ] 코드에 의미를 알 수 없는 숫자 리터럴이 없는가?
- [ ] 튜닝 대상 수치는 Config `DataAsset`/`DataTable` 로 노출됐는가? (§14)
- [ ] 코드 전용 상수는 이름 있는 상수로 선언됐는가?

---

## 16. 기타

- `UFUNCTION(BlueprintCallable)` 없이 BP 노출 금지
- `Super::` 호출 누락 주의 (`BeginPlay`, `EndPlay`, `GetLifetimeReplicatedProps`, `ActivateAbility`)
- `IsValid()` 또는 null 체크 없이 UObject 포인터 역참조 금지

---

## 적용 범위

- 신규 작성 코드 전체
- 기존 코드는 **해당 줄을 수정할 때 함께** 변환한다. 도입 이전 코드에 §6(`auto`)·§8(컴포넌트 탐색) 위반이 남아 있어도, 일괄 개조를 목적으로 별도 커밋을 만들지 않는다.
- code-reviewer 는 변경된 hunk 기준으로만 지적한다 — 손대지 않은 주변 코드의 위반은 리뷰 대상이 아니다.

## 예외

- **§1~7 (문법 스타일): 예외 없음.** 단 §6 은 위에 명시한 "타입을 적을 수 없는 경우"만 허용.
- **§8~15 (설계 원칙)**: 아래는 허용된 이탈이다.
  - §9 — 정식 ViewModel 계층은 `ModelViewViewModel` 플러그인을 켜기 전까지 도입하지 않는다.
  - §12 — 기존 1파일=1인터페이스 유지, 단일 구현체 internal 추상화 분리 유지.
  - §13 — 하위 컴포넌트가 1개뿐이면 루트 파사드 생략 가능.
  - §14 — 기존 `USpyAbilityData`/`USpyCharacterAssetData`/`USpyComboAssetData`/`USpyAnimAssetData` 등 이미 구축된 `UDataAsset` 파이프라인은 grandfathered. 이 룰은 **신규 반복 로우형 밸런스 데이터**부터 적용한다.
  - 엔진/플러그인 오버라이드 시그니처가 강제하는 형태(예: 엔진이 요구하는 `virtual` 시그니처)는 스타일보다 우선한다.
