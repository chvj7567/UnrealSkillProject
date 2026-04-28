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

## 기타

- `UFUNCTION(BlueprintCallable)` 없이 BP 노출 금지
- `Super::` 호출 누락 주의 (`BeginPlay`, `EndPlay`, `GetLifetimeReplicatedProps`)
- `IsValid()` 또는 null 체크 없이 UObject 포인터 역참조 금지
