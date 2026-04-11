# UE 5.4 → 5.7 마이그레이션 구현 플랜

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** SpyProject를 UE 5.4에서 5.7로 마이그레이션하여 컴파일 성공 및 에디터 정상 실행 상태로 만든다.

**Architecture:** `.uproject` EngineAssociation 변경 → 빌드 캐시 클린 → 프로젝트 파일 재생성 → 컴파일 시도 → 에러 순차 수정.  
`ModularGameplayActors`는 프로젝트 내 소스를 보유하고 있으므로 5.7 API에 맞춰 직접 수정 가능하다.

**Tech Stack:** Unreal Engine 5.7 (`D:\UE_5.7`), Visual Studio 2022, C++20, GAS, ModularGameplayActors, Enhanced Input

---

## 수정 대상 파일 맵

| 파일 | 변경 유형 | 이유 |
|------|-----------|------|
| `SkillProject/SkillProject.uproject` | 수정 | EngineAssociation 변경 |
| `SkillProject/Intermediate/` | 삭제 | 5.4 빌드 캐시 제거 |
| `SkillProject/Binaries/` | 삭제 | 5.4 바이너리 제거 |
| `SkillProject/Plugins/ModularGameplayActors/Intermediate/` | 삭제 | 플러그인 빌드 캐시 제거 |
| `SkillProject/Plugins/ModularGameplayActors/Binaries/` | 삭제 | 플러그인 바이너리 제거 |
| `SkillProject/Source/SKGAS/Attribute/SKAttributeSet.h` | 수정 가능 | `FSKAttributeEvent` 델리게이트 시그니처 변경 여부 |
| `SkillProject/Source/SkillProject/Character/SpyHealthComponent.cpp` | 수정 가능 | `HandleHealthChanged` 시그니처 변경 여부 |
| `SkillProject/Source/SkillProject/Character/SpyPawnExtensionComponent.*` | 수정 가능 | `IGameFrameworkInitStateInterface` API 변경 여부 |
| `SkillProject/Source/SkillProject/Input/SpyInputComponent.*` | 수정 가능 | `IGameFrameworkInitStateInterface` API 변경 여부 |
| `SkillProject/Plugins/ModularGameplayActors/Source/**/*.cpp/.h` | 수정 가능 | 5.7 엔진 API 호환성 |

---

## Task 1: migration/5.7 브랜치 생성

**Files:**
- Git 브랜치 생성

- [ ] **Step 1: 브랜치 생성 및 체크아웃**

```bash
git checkout -b migration/5.7
```

Expected output: `Switched to a new branch 'migration/5.7'`

- [ ] **Step 2: 브랜치 확인**

```bash
git branch
```

Expected: `* migration/5.7` 가 활성 브랜치로 표시

---

## Task 2: EngineAssociation 변경

**Files:**
- Modify: `SkillProject/SkillProject.uproject`

- [ ] **Step 1: EngineAssociation 수정**

`SkillProject/SkillProject.uproject` 파일에서 2번째 줄 변경:

```json
"EngineAssociation": "5.4",
```
→
```json
"EngineAssociation": "5.7",
```

> **주의:** `"5.7"` 이 아닌 GUID 형태(`{XXXXXXXX-...}`)일 수도 있다.  
> 만약 에픽 런처에서 설치한 5.7이라면 `"5.7"`이 정상.  
> 직접 빌드 엔진(소스 빌드)이라면 등록된 GUID 사용 필요.  
> 확인 방법: `HKEY_CURRENT_USER\Software\Epic Games\Unreal Engine\Builds` 레지스트리 확인.

- [ ] **Step 2: 커밋**

```bash
git add SkillProject/SkillProject.uproject
git commit -m "[Migration] EngineAssociation 5.4 -> 5.7"
```

---

## Task 3: 빌드 캐시 클린

**Files:**
- 삭제: `SkillProject/Intermediate/`
- 삭제: `SkillProject/Binaries/`
- 삭제: `SkillProject/Plugins/ModularGameplayActors/Intermediate/`
- 삭제: `SkillProject/Plugins/ModularGameplayActors/Binaries/`

> **절대 삭제하지 말 것:** `SkillProject/Content/`, `SkillProject/Source/`, `SkillProject/Config/`, `.git/`

- [ ] **Step 1: 프로젝트 Intermediate 삭제**

Windows 탐색기 또는 터미널:
```bash
rm -rf SkillProject/Intermediate
rm -rf SkillProject/Binaries
```

- [ ] **Step 2: 플러그인 캐시 삭제**

```bash
rm -rf SkillProject/Plugins/ModularGameplayActors/Intermediate
rm -rf SkillProject/Plugins/ModularGameplayActors/Binaries
```

- [ ] **Step 3: .gitignore 확인 (Intermediate/Binaries가 추적되지 않는지 확인)**

```bash
git status
```

`Intermediate/`, `Binaries/` 폴더가 `git status`에 등장하지 않으면 정상 (이미 .gitignore에 포함).  
등장한다면 `.gitignore`에 아래 추가:
```
SkillProject/Intermediate/
SkillProject/Binaries/
```

---

## Task 4: 프로젝트 파일 재생성

**Files:**
- 재생성: `SkillProject/SkillProject.sln` 및 `.vcxproj` 파일들

- [ ] **Step 1: GenerateProjectFiles 실행**

명령 프롬프트(cmd)에서 실행:
```
"D:\UE_5.7\Engine\Build\BatchFiles\GenerateProjectFiles.bat" "C:\Users\Tae\Desktop\MyProject\SpyProject\UnrealSkillProject\SkillProject\SkillProject.uproject" -Game
```

Expected: `SUCCESS` 메시지 출력 및 `.sln` 파일 업데이트

- [ ] **Step 2: .sln 파일이 갱신됐는지 확인**

```bash
git status
```

`SkillProject.sln` 파일이 수정됨으로 표시되면 정상.

---

## Task 5: 첫 번째 컴파일 시도

**Files:**
- Visual Studio 2022에서 빌드

- [ ] **Step 1: Visual Studio 2022로 .sln 열기**

`SkillProject/SkillProject.sln` 파일을 Visual Studio 2022로 열기.

- [ ] **Step 2: 빌드 구성 설정**

상단 드롭다운에서:
- Solution Configuration: `Development Editor`
- Solution Platform: `Win64`

- [ ] **Step 3: SkillProject 빌드**

`Build` 메뉴 → `Build SkillProject` (또는 `Ctrl+Shift+B`)

- [ ] **Step 4: 에러 목록 수집**

`Output` 창 또는 `Error List` 창에서 컴파일 에러 전체 복사.  
에러가 발생하면 Task 6, 7, 8을 순서대로 적용.  
에러가 없으면 Task 9로 바로 이동.

---

## Task 6: [예상 에러] FSKAttributeEvent 델리게이트 시그니처 수정

> 5.5+ 에서 GAS Attribute 변경 델리게이트의 `FGameplayEffectSpec*` 파라미터가  
> `FGameplayEffectSpec` 래퍼 타입으로 변경됐을 경우 대응.  
> **에러 메시지 패턴:** `'FSKAttributeEvent': 'const FGameplayEffectSpec*' - no overloaded function`  

**Files:**
- Modify: `SkillProject/Source/SKGAS/Attribute/SKAttributeSet.h`
- Modify: `SkillProject/Source/SkillProject/Character/SpyHealthComponent.h`
- Modify: `SkillProject/Source/SkillProject/Character/SpyHealthComponent.cpp`

- [ ] **Step 1: 에러 확인**

에러 메시지에 `FGameplayEffectSpec` 시그니처 관련 내용이 있는지 확인.  
없으면 이 Task 스킵.

- [ ] **Step 2: `SKAttributeSet.h` 델리게이트 시그니처 확인**

`SkillProject/Source/SKGAS/Attribute/SKAttributeSet.h` 20번째 줄:

```cpp
// 현재 코드
DECLARE_MULTICAST_DELEGATE_SixParams(FSKAttributeEvent,
    AActor* /*EffectInstigator*/,
    AActor* /*EffectCauser*/,
    const FGameplayEffectSpec* /*EffectSpec*/,
    float /*EffectMagnitude*/,
    float /*OldValue*/,
    float /*NewValue*/);
```

5.7 에서 `FGameplayEffectSpec`이 deprecated 됐다는 에러가 있다면 컴파일러 에러 메시지의 대체 타입(예: `FGameplayEffectSpecHandle` 또는 래퍼 타입)으로 교체.  
에러가 없다면 이 Step 스킵.

- [ ] **Step 3: 커밋**

```bash
git add SkillProject/Source/SKGAS/Attribute/SKAttributeSet.h
git add SkillProject/Source/SkillProject/Character/SpyHealthComponent.h
git add SkillProject/Source/SkillProject/Character/SpyHealthComponent.cpp
git commit -m "[Migration] FSKAttributeEvent 시그니처 5.7 호환 수정"
```

---

## Task 7: [예상 에러] IGameFrameworkInitStateInterface 가상 함수 시그니처 수정

> 5.5~5.7 구간에서 `IGameFrameworkInitStateInterface`의 가상 함수 시그니처가 변경됐을 경우 대응.  
> **에러 메시지 패턴:** `cannot override ... different return type` 또는 `has no member named 'CanChangeInitState'`

**Files:**
- Modify: `SkillProject/Source/SkillProject/Character/SpyPawnExtensionComponent.h`
- Modify: `SkillProject/Source/SkillProject/Character/SpyPawnExtensionComponent.cpp`
- Modify: `SkillProject/Source/SkillProject/Input/SpyInputComponent.h`
- Modify: `SkillProject/Source/SkillProject/Input/SpyInputComponent.cpp`

- [ ] **Step 1: 에러 확인**

`CanChangeInitState`, `HandleChangeInitState` 관련 에러가 있는지 확인.  
없으면 이 Task 스킵.

- [ ] **Step 2: 5.7 엔진 인터페이스 헤더 확인**

파일 확인:
```
D:\UE_5.7\Engine\Plugins\Runtime\ModularGameplay\Source\ModularGameplay\Public\GameFrameworkInitStateInterface.h
```

해당 헤더에서 `CanChangeInitState`, `HandleChangeInitState` 의 최신 시그니처 확인 후 프로젝트 코드에 동일하게 적용.

- [ ] **Step 3: 커밋**

```bash
git add SkillProject/Source/SkillProject/Character/SpyPawnExtensionComponent.h
git add SkillProject/Source/SkillProject/Character/SpyPawnExtensionComponent.cpp
git add SkillProject/Source/SkillProject/Input/SpyInputComponent.h
git add SkillProject/Source/SkillProject/Input/SpyInputComponent.cpp
git commit -m "[Migration] IGameFrameworkInitStateInterface 시그니처 5.7 호환 수정"
```

---

## Task 8: [예상 에러] ModularGameplayActors 플러그인 소스 수정

> 프로젝트 내 `ModularGameplayActors` 플러그인 소스가 5.7 엔진 API와 충돌할 경우 대응.  
> 이 플러그인은 `AModularCharacter`, `AModularAIController`, `AModularPlayerState` 등 base class를 제공한다.

**Files:**
- Modify: `SkillProject/Plugins/ModularGameplayActors/Source/ModularGameplayActors/Public/*.h`
- Modify: `SkillProject/Plugins/ModularGameplayActors/Source/ModularGameplayActors/Private/*.cpp`

- [ ] **Step 1: 플러그인 관련 에러 확인**

에러 파일 경로에 `Plugins/ModularGameplayActors` 가 포함된 에러가 있는지 확인.  
없으면 이 Task 스킵.

- [ ] **Step 2: 5.7 엔진 동일 플러그인 소스와 비교**

5.7 엔진에 `ModularGameplayActors`가 없으므로 에러 메시지를 기반으로 직접 수정.  
공통 수정 패턴:
- deprecated API → 에러 메시지의 대체 함수로 교체
- 헤더 include 경로 변경 → 에러 메시지의 `file not found` 경로 기반으로 수정

- [ ] **Step 3: 커밋**

```bash
git add SkillProject/Plugins/ModularGameplayActors/
git commit -m "[Migration] ModularGameplayActors 플러그인 5.7 호환 수정"
```

---

## Task 9: 컴파일 성공 확인 및 에디터 실행

**Files:**
- 없음 (실행 확인)

- [ ] **Step 1: Visual Studio에서 빌드 성공 확인**

```
Build succeeded.
========== Build: X succeeded, 0 failed ==========
```

- [ ] **Step 2: 에디터 실행**

`SkillProject.uproject` 파일을 더블클릭하거나 다음 커맨드로 실행:
```
"D:\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe" "C:\Users\Tae\Desktop\MyProject\SpyProject\UnrealSkillProject\SkillProject\SkillProject.uproject"
```

- [ ] **Step 3: Blueprint 재컴파일**

에디터 메뉴 → `Tools` → `Compile All Blueprints`  
Blueprint 컴파일 에러가 있으면 해당 Blueprint를 열어 수동 수정.

- [ ] **Step 4: 기본 동작 확인 체크리스트**

- [ ] PIE(Play In Editor) 실행 가능
- [ ] 캐릭터 이동 정상
- [ ] 기본 스킬 발동 (GAS 어빌리티 활성화)
- [ ] AI 스폰 및 순찰 동작
- [ ] 타겟팅 시스템 동작

- [ ] **Step 5: 최종 커밋**

```bash
git add -A
git commit -m "[Migration] UE 5.4 → 5.7 마이그레이션 완료"
```

---

## 컴파일 에러 대응 가이드

위 Task들로 해결되지 않는 에러가 발생할 경우 아래 패턴으로 대응:

| 에러 유형 | 대응 방법 |
|-----------|-----------|
| `'XXX' was not declared in this scope` | 헤더 include 누락 — 에러 발생 파일에 필요한 `#include` 추가 |
| `'XXX' is deprecated` | 에러 메시지의 대체 API로 교체 |
| `cannot open source file "XXX.h"` | 5.7에서 헤더 경로 변경 — 엔진 소스에서 새 경로 검색 후 수정 |
| `'XXX' has no member named 'YYY'` | API 삭제/이름 변경 — 5.7 헤더에서 대체 멤버 검색 |
| Linker error `LNK2019` | `.Build.cs`의 모듈 의존성 누락 — 에러 심볼 기반으로 모듈 추가 |
