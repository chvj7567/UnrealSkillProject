# C++ 코딩 스타일

## UE5 네이밍 접두사

| 접두사 | 대상 |
|--------|------|
| `U` | UObject 파생 클래스 |
| `A` | AActor 파생 클래스 |
| `F` | 일반 구조체 |
| `I` | 인터페이스 |
| `E` | enum class |
| `T` | 템플릿 클래스 |

## UPROPERTY 지정자

- 에디터에서만 설정: `EditDefaultsOnly`
- 런타임 읽기 전용 노출: `VisibleAnywhere`
- BP에서 읽기 허용: `BlueprintReadOnly`
- 레플리케이션: `Replicated` + `GetLifetimeReplicatedProps` 등록 필수
- UObject 포인터는 `TObjectPtr<>` 사용 (raw pointer 지양)

## 헤더 include 순서

```cpp
#include "MyClass.h"          // 자기 자신
#include "OtherUEHeader.h"    // UE 헤더
#include "ProjectHeader.h"    // 프로젝트 헤더
#include "MyClass.generated.h" // generated는 항상 마지막
```

**포매터는 이 순서를 강제하지 않는다.** 위 순서는 알파벳순이 아니라 의미 기반이라 clang-format 이 자동 판정할 수 없다 — UE 헤더(`GameFramework/Character.h`)와 프로젝트 헤더(`Character/SpyCharacter.h`)를 정규식으로 안정적으로 구분할 수 없기 때문이다. 그래서 `SkillProject/.clang-format` 에 `SortIncludes: Never` 를 지정해 포매터가 include 를 재정렬하지 못하게 막아 뒀다(그전에는 저장할 때마다 알파벳순으로 되돌려 이 룰을 깨뜨렸다). **순서 준수는 작성자와 code-reviewer 의 몫이다.**

## 주석 스타일

- 한 줄 주석은 항상 `//#`로 시작 — 일반 `//`나 `///`, `/* */` 사용 금지.
  - 헤더의 doxygen 스타일 주석도 `//#`로 통일.
  - **예외**: UE 자동 생성 저작권 헤더는 그대로 유지 (새 클래스 생성 시 자동 삽입되므로 매번 정리하지 않음).
    - `// Fill out your copyright notice in the Description page of Project Settings.`
    - `// Copyright Epic Games, Inc. All Rights Reserved.`

```cpp
//# 서버 권한에서 실행
if (HasAuthority(&ActivationInfo))
{
    //# 데미지 적용
    ApplyDamage(Target);
}
```

## 불리언/널 비교

- `!` 단항 부정 연산자 사용 금지 — 가독성과 의도 명확성을 위해 명시적 비교 사용.
  - 불리언: `!bFlag` → `bFlag == false`
  - 포인터: `!Ptr` → `Ptr == nullptr`
  - 함수 반환 불리언: `!IsValid(Obj)` → `IsValid(Obj) == false`
- 예외: STL/UE 표준 표현식(`!operator bool()` 의도가 명백한 경우)은 PR 검토 시 합의로 허용. 기본은 명시적 비교.

```cpp
//# OK
if (bFreeMoveMode == false) { ... }
if (TargetActor == nullptr) { ... }
if (IsValid(Comp) == false) { return; }

//# 금지
if (!bFreeMoveMode) { ... }
if (!TargetActor) { ... }
if (!IsValid(Comp)) { return; }
```

## 기타

- `UFUNCTION(BlueprintCallable)` 없이 BP 노출 금지
- `Super::` 호출 누락 주의 (`BeginPlay`, `EndPlay`, `GetLifetimeReplicatedProps`)
- `IsValid()` 또는 null 체크 없이 UObject 포인터 역참조 금지
