# Claude 멀티에이전트 파이프라인 포팅 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Unity `Project_Lair`의 `.claude/` 멀티에이전트 개발 파이프라인을 SpyProject(UE5.7/C++)로 이식한다 — 에이전트 5종·인프라 룰·오케스트레이션 스킬·메타 파일을 Unreal 현실로 재구현.

**Architecture:** foundation-first. 먼저 에이전트가 참조할 기반(`project.md` 메타 + `unreal-infra.md` 룰 + concept 골격)을 세우고, gameplay-programmer를 수직 슬라이스로 완전 이식해 패턴을 검증한 뒤, 나머지 4 에이전트에 복제하고, 스킬 4종을 잇고, 통합 점검한다. 기존 `.claude` 자산(cpp-style·git-conventions·new-ability-checklist·settings.json 훅)은 미변경 보존.

**Tech Stack:** Markdown (에이전트/룰/스킬 정의), Claude Code `.claude/` 규약, ripgrep(`rg`) 검증, git.

**참조 문서:**
- Spec: `docs/superpowers/specs/2026-07-19-claude-pipeline-port-design.md` (특히 §6 에이전트별 결합 제거 체크리스트가 이식 변환의 SoT)
- 원본: `E:/UnityProject/Project_Lair/.claude/` (이식 소스)
- 대상 기존 규칙: `.claude/rules/cpp-style.md`, `git-conventions.md`, `new-ability-checklist.md`
- 도메인 근거: `CLAUDE.md` (Unreal 아키텍처·GAS 파이프라인·에셋 접근 규칙)

## Global Constraints

- **커밋 금지 — 스테이징만**: 각 Task는 `git commit`을 실행하지 않는다. `git add`(관련 파일만) + 커밋 메시지(안) 제시까지. (`.claude/rules/git-conventions.md`, CLAUDE.md 중요 규칙)
- **커밋 메시지 포맷**: `[Tag] ClassName — 요약` (Tag ∈ Feature/Fix/Refactor/Debug/Chore/Docs). 문서 작업은 `[Docs]`, `.claude` 인프라는 `[Chore]`.
- **기존 자산 미변경**: `settings.json`, `hooks/post-edit-clang-format.ps1`, `rules/cpp-style.md`, `rules/git-conventions.md`, `rules/new-ability-checklist.md`, `settings.local.json`은 이 계획에서 수정하지 않는다.
- **Unity 잔재 0**: 이식된 모든 파일은 `.claude/agents`·`.claude/skills`에서 Unity/C# 용어 잔재가 0이어야 한다 (검증 grep — Task 11 및 각 에이전트 Task).
- **참조는 이름으로**: 에이전트는 룰을 번호(`Rule NN`)가 아니라 파일명/주제(`cpp-style`, `unreal-infra`, `git-conventions`)로 참조한다.
- **concept 내용 금지**: `docs/design/spyproject_concept.md`는 **골격만** 생성. 게임 디자인 결정을 지어내지 않는다 (사용자 확정 대기).
- **주석 규약**: 예시 코드의 단일 라인 주석은 `//#` (cpp-style.md).

---

## File Structure

**신규 생성:**
- `.claude/project.md` — 에이전트 SoT (Unreal 키-값 메타 + 메인 오케스트레이터 행동 규칙)
- `.claude/rules/unreal-infra.md` — 인프라 룰 (AssetManager/GAS/DataAsset/모듈)
- `.claude/agents/gameplay-programmer.md` — C++ 구현 전담 (수직 슬라이스 — 패턴 기준)
- `.claude/agents/code-reviewer.md` — 코드 검토
- `.claude/agents/test-engineer.md` — Unreal Automation 테스트
- `.claude/agents/game-designer.md` — 기획
- `.claude/agents/design-reviewer.md` — 기획 검토
- `.claude/skills/start-develop/SKILL.md` — 승인 게이트 파이프라인
- `.claude/skills/start-develop-auto/SKILL.md` — 게이트 없는 파이프라인
- `.claude/skills/start-develop-simple/SKILL.md` — 프로토타입 파이프라인
- `.claude/skills/start-develop-quick/SKILL.md` — 사소 수정 파이프라인
- `.claude/.active-sessions.md` — 스킬 세션 레지스트리 (런타임 스크래치, 커밋 제외)
- `docs/design/spyproject_concept.md` — concept 골격

**미변경 보존:** `.claude/settings.json`, `.claude/hooks/`, `.claude/settings.local.json`, `.claude/rules/{cpp-style,git-conventions,new-ability-checklist}.md`

---

## Phase 0 — 기반 (Foundation)

### Task 1: project.md 메타 파일

**Files:**
- Create: `.claude/project.md`

**Interfaces:**
- Produces (이후 모든 에이전트/스킬이 이 키를 참조): `engine`, `language`, `code_root`, `test_paths.automation`, `test_framework`, `infrastructure.modules`, `infrastructure.asset_access`, `mcp`, `concept_doc`, `stage`, `docs.design`, `docs.specs`, `docs.plans`, `uses_superpowers`, 그리고 "메인 오케스트레이터 행동 규칙" + 후보 스킬 표.

- [ ] **Step 1: project.md 작성**

아래 전체 내용으로 생성한다. (원본 `E:/UnityProject/Project_Lair/.claude/project.md` 구조를 따르되 값은 Unreal. Unity 전용 키 `test_asmdef`·`edit_mode/play_mode`·`balance_config_asset`·`card_data_folder` 제거.)

```markdown
# Project Meta

이 파일은 `.claude/agents/*.md` 의 모든 서브에이전트가 작업 시작 시 가장 먼저 읽는 **프로젝트 메타** 다. agent 정의는 도메인 정보를 직접 들고 있지 않고 이 파일을 통해서만 프로젝트를 인지한다.

`CLAUDE.md` 와 별개다 — `CLAUDE.md` 는 사람용 자유 양식 문서, 이 파일은 agent 가 읽는 구조화된 메타다. 중복 정보는 이 파일이 SoT.

---

## 프로젝트

- **name**: SpyProject (SkillProject)
- **one_liner**: 스파이 테마 3인칭 액션 — 데디케이티드 서버 멀티플레이어, Lyra 스타일 모듈형 아키텍처, 커스텀 GAS 프레임워크

## 컨셉 / 단계

- **concept_doc**: `docs/design/spyproject_concept.md`
- **stage**: (사용자 확정 대기)
- **stage_goal**: (사용자 확정 대기)

## 코드 / 인프라

- **engine**: Unreal Engine 5.7
- **language**: C++
- **architecture**: Lyra 스타일 모듈형 + GAS + InitState 초기화 흐름
- **code_root**: `SkillProject/Source/`
- **test_paths**
  - **automation**: `SkillProject/Source/SkillProject/**/Tests/`
- **test_framework**: Unreal Automation (SimpleAutomationTest / AutomationSpec)
- **test_method_naming**: english (기존 `SpyAICircleStrafeTests.cpp` 스타일 준수)
- **infrastructure**
  - **modules**: `SKGAS` (범용 GAS 래퍼), `SKAssetCore` (에셋 매니저 플러그인 — 분리 진행 중)
  - **asset_access**: `SpyAssetManager` — `LoadAssetSync` / `LoadAssetAsync`, `USKAssetData` 이름 룩업(`GetAssetByName`)
  - **module_dependency**: `SkillProject` → `SKGAS` → `GameplayAbilities` (역방향 참조 금지)
- **mcp**: `tools/unreal-mcp/` — 에디터 원격 제어 (`execute_python`, `get/set_asset_property`, `get_actors_in_level` 등). 전용 recompile/test-run 커맨드는 없음 — 빌드/테스트는 에디터·VS에서 사용자 수행.

## 문서 위치

- **docs**
  - **design**: `docs/design/` — game-designer 기획서
  - **specs**: `docs/superpowers/specs/` — superpowers:brainstorming 산출물
  - **plans**: `docs/superpowers/plans/` — superpowers:writing-plans 산출물

## 협업 흐름 (Workflow)

- **uses_superpowers**: true

### 표준 흐름 (uses_superpowers: true)

| 단계 | 주체 | 행위 | 산출물 |
|---|---|---|---|
| 0 | 메인 + `superpowers:brainstorming` | 의도·범위 합의, 결정 락 | `docs/superpowers/specs/YYYY-MM-DD-[기능명]-design.md` |
| 1 | 메인 + `superpowers:writing-plans` | spec 을 Task 단계로 분해 | `docs/superpowers/plans/YYYY-MM-DD-[기능명].md` |
| 2 | **game-designer** | 도메인 결정 채움 — 수치·UX·전투 감각·페이싱 | `docs/design/[기능명].md` |
| 3 | **design-reviewer** | 기획서 1차 검토 (사용자 리뷰 전) | 검토 보고 |
| 4 | **사용자** | 기획서 리뷰·승인 게이트 | 승인 |
| 5 | **gameplay-programmer** | spec + plan + 기획서 참조해 C++ 구현 | 코드 |
| 6 | **code-reviewer** | 룰 준수 + 기획서 일치 검토 | 검토 보고 |
| 7 | **test-engineer** | Unreal Automation 테스트 스위트 | 테스트 .cpp |
| 8 | 메인 | 변경 요약 + `git add` + 커밋 메시지(안) (git-conventions) | 스테이징 + 메시지(안) |

### 스킬 미지정 요청 — 후보 스킬 표

| 스킬 | 적합 작업 | 파이프라인 단계 |
|---|---|---|
| `/start-develop`       | 본격 기능 + 사람 검토 + 승인 게이트 | game-designer → design-reviewer → ⛔승인 → gameplay-programmer → code-reviewer → test-engineer |
| `/start-develop-auto`  | 본격 기능 + 자동 리뷰어, 게이트 없음 | 위와 동일하되 ⛔승인 생략 |
| `/start-develop-simple`| 프로토타입 — 리뷰 생략, 테스트 유지 | game-designer → gameplay-programmer → test-engineer |
| `/start-develop-quick` | 사소 수정·작은 버그·리네임 | gameplay-programmer → code-reviewer |

`uses_superpowers: true` 면 위 후보에 0·1단계(brainstorming·writing-plans) 가 앞단에 붙는다 — 단 `/start-develop-quick` 은 정의상 0·1 도 스킵.

## 메인 오케스트레이터 행동 규칙

메인(사용자와 직접 대화하는 최상위 Claude)이 사용자 메시지를 받았을 때의 정책. 도메인 비종속.

### 스킬 미지정 요청 — 후보 제시 게이트

사용자가 **코드/에셋 변경이 명확한 작업 요청**을 보냈는데 스킬 이름(`/<skill>`)을 명시하지 않으면, 메인은 즉시 시작하지 않고 위 후보 표를 제시하고 사용자가 선택할 때까지 멈춘다.

**제외 케이스** (후보 제시 없이 즉시 답변): 메타 질문, 단순 조회·탐색·파일 읽기, 조언·추천 요청, 일반 대화·모호한 발화.

**스킬 이름이 명시된 경우** — 후보 제시 생략, 즉시 진행.

**메인의 자체 분기 금지** — "이건 quick 으로 충분" 같은 임의 판단으로 직진하지 않는다. 사용자 선택이 단일 진실.

### 에셋 한정 사이클 — 리뷰 생략 선택지 게이트

구현 단계 변경이 **순수 에셋 등록/적용(코드 파일 변경 0건)** 이면 메인은 code-reviewer 를 곧장 spawn 하지 않고 "리뷰 진행 / 생략하고 커밋 메시지" 선택지를 제시하고 기다린다. 코드 변경이 1건이라도 있으면 이 게이트 없이 리뷰어로 직진.
```

- [ ] **Step 2: 필수 키 존재 검증**

Run: `rg -n "engine|language|code_root|test_paths|test_framework|infrastructure|mcp|concept_doc|uses_superpowers|docs" .claude/project.md`
Expected: 위 키가 모두 출력됨.

- [ ] **Step 3: Unity 전용 키 잔재 검증**

Run: `rg -n "asmdef|edit_mode|play_mode|balance_config_asset|card_data_folder|Unity|ChvjPackage|Addressable" .claude/project.md`
Expected: 매치 없음 (exit 1).

- [ ] **Step 4: 스테이징 + 커밋 메시지(안)**

Run: `git add .claude/project.md`
커밋 메시지(안):
```
[Chore] project.md — 에이전트 메타 파일 신규 (Unreal 키)
```

---

### Task 2: unreal-infra.md 인프라 룰

**Files:**
- Create: `.claude/rules/unreal-infra.md`

**Interfaces:**
- Produces (에이전트가 "unreal-infra 규칙"으로 참조): AssetManager 접근 규칙, GAS 데이터 파이프라인 규칙, DataAsset 계층 규칙, 모듈 의존 방향 규칙, 에셋/BP 규칙. 원본 Rule 03(ChvjPackage)/04(Unity 에셋) 대체.

- [ ] **Step 1: unreal-infra.md 작성**

아래 전체 내용으로 생성한다. (내용은 `CLAUDE.md`의 "Key Architectural Patterns"·"중요 규칙" 및 모듈 구조에서 도출.)

```markdown
# unreal-infra — SpyProject 인프라 규칙

> 원본 Unity 룰 03(ChvjPackage)·04(에셋) 대체. SpyProject 재사용 인프라(SKGAS·SKAssetCore·SpyAssetManager)와 GAS·DataAsset·모듈 규약을 정의한다.

---

## 1. 에셋 접근은 SpyAssetManager 경유

모든 에셋 로드는 `SpyAssetManager` 를 통한다. 하드코딩된 에셋 경로 직접 참조 금지.

- 동기: `USpyAssetManager::LoadAssetSync(Path)` / `GetAssetByName<T>(Name)` / `GetSubclassByName<T>(Name)`
- 비동기: `LoadAssetAsync(Path, Delegate)`
- 이름→경로 룩업은 `USKAssetData`(`GetAssetPathByName`) 를 통한다. 문자열 리터럴 경로 금지.

```cpp
//# (X) 하드코딩 경로
ConstructorHelpers::FObjectFinder<UTexture2D> Tex(TEXT("/Game/Spy/UI/Icon"));

//# (O) AssetManager 경유
const USKAssetData& Data = USpyAssetManager::Get().GetAssetData();
UTexture2D* Tex = USpyAssetManager::GetAssetByName<UTexture2D>(TEXT("Icon"));
```

체크리스트:
- [ ] 하드코딩된 `/Game/...` 경로 리터럴이 없는가?
- [ ] 에셋 접근이 SpyAssetManager API 를 통하는가?

---

## 2. GAS 데이터 파이프라인

- `USpyAbilityData`(DataAsset) → `GiveToAbilitySystem()` 로 AttributeSet 동적 생성·초기 GE 적용·GA 부여를 한 번에 수행.
- 모든 부여 핸들은 `FSpyAbilitySet_GrantedHandles` 로 트래킹 → 장착 해제·사망 시 반드시 `TakeFromAbilitySystem()` 으로 해제 (누수 금지).
- 입력은 `SpyEnhancedInputComponent` 에서 Gameplay Tag → ASC `AbilityLocalInputPressed/Released` 로 연결.
- GA 추가/ASC 조작은 `InitState_DataInitialized` 이후에만 (아래 §4).

체크리스트:
- [ ] 부여 핸들이 `FSpyAbilitySet_GrantedHandles` 에 저장되고 해제 경로가 있는가?
- [ ] 새 태그를 문자열이 아니라 `UE_DECLARE/DEFINE_GAMEPLAY_TAG` 로 등록했는가? (SpyGameplayTags.h/.cpp)

---

## 3. DataAsset 계층

```
USKAssetData (이름→경로 룩업 베이스)
└── USpyAssetData          # 전체 에셋 중앙 허브 (SpyAssetManager 가 시작 시 동기 로드)
USpyCharacterAssetData     # 캐릭터별 컴포넌트·어빌리티 세트·입력·콤보
USpyAbilityData            # GA/AttributeSet/GE 묶음
USpyComboAssetData / USpyAnimAssetData / Config DataAsset 들
```

- 하드코딩 수치·문자열은 Config DataAsset 으로 이전 (`docs/hardcoded-values.md` 참조).
- 새 에셋 타입 추가 시 `SpyDataEditorTool` 의 해당 탭 Slate 코드 + `SpyDataScanner` 동반 수정.

---

## 4. InitState 초기화 흐름

`SpyPawnExtensionComponent` 가 `IGameFrameworkInitStateInterface` 구현. `CharacterAssetData` 레플리케이트 완료 + Controller 연동 후 `InitState_DataInitialized` 단계에서 `InitAbilityActorInfo` 호출. **GA 추가·ASC 조작 코드는 반드시 이 흐름 이후에 실행.**

---

## 5. 모듈 의존 방향

```
SkillProject (게임)  →  SKGAS / SKAssetCore  →  UE 표준 모듈
```

- 역방향 참조 금지 (SKGAS/SKAssetCore 가 SkillProject 를 참조하지 않음).
- 공통 기능은 재사용 모듈(SKGAS/SKAssetCore)에 두고 게임 코드에서 중복 구현 금지.
- 새 의존성은 해당 모듈 `.Build.cs` 에 명시.

체크리스트:
- [ ] 재사용 모듈이 게임 모듈을 include 하지 않는가?
- [ ] `.Build.cs` 의존성이 올바른 방향인가?

---

## 6. 서버 권한 / 레플리케이션

- 게임플레이 상태 변경은 서버에서 실행하고 클라이언트에 레플리케이트.
- GA 내에서 `HasAuthority(&ActivationInfo)` 체크 후 서버 전용 로직. 클라 연출은 Authority 블록 밖.
- 레플리케이트 프로퍼티는 `Replicated` + `GetLifetimeReplicatedProps` 등록.
- 런타임 컴포넌트 추가는 `CharacterAssetData` 컴포넌트 목록을 읽어 `NewObject & RegisterComponent` — `BeginPlay` 하드코딩 금지.

---

## 7. 에셋/Blueprint

- 반복·재사용 구성은 Blueprint 또는 DataAsset 으로 (동일 구조 중복 금지).
- 런타임 참조는 하드 참조 대신 `TSoftObjectPtr`/`TSoftClassPtr` + AssetManager 로드.
- 패키지 빌드에서 BP 오브젝트(`BP_X.BP_X`)는 cook 시 stripped → generated class(`BP_X.BP_X_C`) 경로로 로드 (`GetSubclassByName` 이 `_C` 처리).
```

- [ ] **Step 2: Unity 용어 잔재 검증**

Run: `rg -ni "chvjpackage|CHMResource|CHMUI|CHMPool|CHText|addressable|scriptableobject|monobehaviour|prefab|asmdef" .claude/rules/unreal-infra.md`
Expected: 매치 없음 (exit 1).

- [ ] **Step 3: 스테이징 + 커밋 메시지(안)**

Run: `git add .claude/rules/unreal-infra.md`
커밋 메시지(안):
```
[Chore] unreal-infra — AssetManager/GAS/모듈 인프라 룰 신규 (Unity 03/04 대체)
```

---

### Task 3: spyproject_concept.md 골격

**Files:**
- Create: `docs/design/spyproject_concept.md`

**Interfaces:**
- Consumes: `project.md` 의 `concept_doc` 키가 이 파일을 가리킴 (Task 1).
- Produces: game-designer/design-reviewer 가 근거로 읽는 §1~§5 구조. **내용은 골격 + 사용자 확정 마커.**

- [ ] **Step 1: concept 골격 작성 (내용 지어내지 말 것)**

```markdown
# SpyProject Concept

> game-designer / design-reviewer 가 기획·검토의 근거로 읽는 프로젝트 컨셉서.
> ⚠️ 아래 "(사용자 확정 대기)" 항목은 실제 게임 디자인 결정이 필요하다 — 임의로 채우지 말 것.

## §1 장르 / 톤
- 스파이 테마 3인칭 액션. 데디케이티드 서버 멀티플레이어.
- (세부 톤·레퍼런스: 사용자 확정 대기)

## §2 코어 루프
- (사용자 확정 대기)

## §3 기능 특징 / 톤
- 커스텀 GAS 프레임워크 기반 어빌리티/콤보
- 파쿠르 시스템 (Vault/WallClimb/HangUp)
- 타게팅·스폰봇·애님 레이어 컴포넌트
- (우선순위·재미 축: 사용자 확정 대기)

## §4 현 단계 범위 (stage scope)
- (사용자 확정 대기 — 이번 단계에서 만드는 것 / 제외하는 것)

## §5 밸런싱 / 페이싱 기준
- (사용자 확정 대기 — 없으면 "플레이테스트 기반 결정")
```

- [ ] **Step 2: 골격 확인**

Run: `rg -n "사용자 확정 대기" docs/design/spyproject_concept.md`
Expected: §2·§4·§5 등에 마커 존재 (내용을 지어내지 않았음을 확인).

- [ ] **Step 3: 스테이징 + 커밋 메시지(안) + 사용자 알림**

Run: `git add docs/design/spyproject_concept.md`
커밋 메시지(안):
```
[Docs] spyproject_concept — 컨셉서 골격 신규 (내용 사용자 확정 대기)
```
보고에 "concept §2/§4/§5 내용은 사용자 확정 필요" 를 명시한다.

---

## Phase 1 — 수직 슬라이스 (패턴 검증)

### Task 4: gameplay-programmer.md (완전 이식 — 패턴 기준)

**Files:**
- Create: `.claude/agents/gameplay-programmer.md`
- 소스: `E:/UnityProject/Project_Lair/.claude/agents/gameplay-programmer.md`
- 변환 SoT: Spec §6.1

**Interfaces:**
- Consumes: `project.md` 키(`engine`·`code_root`·`test_paths`·`infrastructure`), 룰 `cpp-style`·`unreal-infra`·`git-conventions`, `CLAUDE.md`(GAS·InitState·에셋 접근), `unreal-mcp` 커맨드.
- Produces: code-reviewer/test-engineer/skills 가 이름 `gameplay-programmer` 로 참조하는 구현 에이전트. 보고 형식.

- [ ] **Step 1: 소스 읽고 Spec §6.1 변환 적용해 작성**

원본 gameplay-programmer.md 를 기반으로 아래를 **전부** 변환/교체한다 (Spec §6.1):
1. frontmatter `description`: "C# 코드(.cs)" → "C++ 코드(.h/.cpp)", "ChvjPackage 연동·MVVM·ScriptableObject" → "SKGAS/SKAssetCore/SpyAssetManager 연동·GAS·DataAsset"
2. 프로젝트 컨텍스트: 읽는 키를 `engine·code_root·test_paths·infrastructure` 로 (Unreal)
3. 작업 시작 전 절차: "ChvjPackage 우선 확인" → "SKGAS/SKAssetCore/SpyAssetManager 우선 확인", "asmdef 구성" → "모듈(.Build.cs) 구성"
4. 룰 매핑표: `02(C# 스타일)`→`cpp-style`, `03(ChvjPackage)`→`unreal-infra`, `04(Unity 에셋)`→`unreal-infra`
5. **"ChvjPackage 핵심 API" 블록 전체 삭제** → "SpyAssetManager / GAS 핵심 API" 블록 신규 작성:
   - `USpyAssetManager::LoadAssetSync/LoadAssetAsync`, `GetAssetByName<T>`, `GetSubclassByName<T>` (`_C` 처리)
   - GAS: `USpyAbilityData::GiveToAbilitySystem()` / `FSpyAbilitySet_GrantedHandles` / `TakeFromAbilitySystem()`
   - 입력: `SpyEnhancedInputComponent` Tag 바인딩
   - (출처: CLAUDE.md GAS 파이프라인 + unreal-infra.md)
6. 사고/작업 원칙: MVVM → GAS·컴포넌트·InitState 흐름. "종속성 최소화·모킹 가능 구조" 유지
7. TDD 흐름: NUnit → Unreal Automation (`IMPLEMENT_SIMPLE_AUTOMATION_TEST`/`DEFINE_SPEC`), `SpyAICircleStrafeTests.cpp` 참조. "실패 먼저→최소 구현→통과" 순서 유지
8. Bug Fix(systematic-debugging): "Unity 라이프사이클(Awake/OnEnable/Start)" → "Unreal 라이프사이클(BeginPlay/InitState/레플리케이션 시점)". 나머지 유지
9. receiving-code-review: 유지 (엔진 무관)
10. **"완료 선언 전 검증" evidence 테이블** → Spec §4 규약: 빌드는 에디터/VS 사용자 수행(자체 단정 금지), 에셋/런타임 상태는 `execute_python`/`get_*` 확인, 불가 시 "검증 보류 — 사용자 빌드 필요"
11. Self-Review 룰 위반 스캔: `//`→`//#`, `Object.Instantiate`/Legacy Text/`Resources/` → "하드코딩 에셋 경로·AssetManager 우회" (unreal-infra §1)
12. **"절대 하지 말 것" 전체 재작성**: CHMPool/CHText/Instantiate/Resources/`//` → 하드코딩 경로 금지·AssetManager 경유·`//#`·서버 권한(HasAuthority)·부여 핸들 해제·재사용 모듈 역참조 금지
13. 보고 형식 "File Structure": "asmdef 영향" → "모듈(.Build.cs) 영향"
14. **커밋 메시지 블록** `# [feat] - ...` → `[Tag] ClassName — 요약` (git-conventions). Unity `.meta` 스테이징 문구 삭제

- [ ] **Step 2: Unity 잔재 grep 게이트**

Run: `rg -ni "\.cs\b|c#|monobehaviour|scriptableobject|asmdef|nunit|editmode|playmode|chvjpackage|chmresource|chmui|chmpool|chtext|addressable|instantiate|createprimitive|findobjectoftype|\.meta\b|# \[feat\]" .claude/agents/gameplay-programmer.md`
Expected: 매치 없음 (exit 1). 매치가 있으면 해당 섹션을 마저 이식하고 재실행.

- [ ] **Step 3: 참조 정합 확인**

Run: `rg -n "cpp-style|unreal-infra|git-conventions|project.md|SpyAssetManager|GiveToAbilitySystem|Automation" .claude/agents/gameplay-programmer.md`
Expected: 룰 파일명·인프라 API·Automation 참조가 존재 (번호 `Rule NN` 참조는 없어야 함).

Run: `rg -n "Rule 0[0-9]" .claude/agents/gameplay-programmer.md`
Expected: 매치 없음 (exit 1).

- [ ] **Step 4: 스테이징 + 커밋 메시지(안)**

Run: `git add .claude/agents/gameplay-programmer.md`
커밋 메시지(안):
```
[Chore] gameplay-programmer — C++/GAS 구현 에이전트 이식 (수직 슬라이스)
```

---

## Phase 2 — 나머지 4 에이전트 복제

> 각 Task는 Task 4에서 검증된 패턴(소스 읽기 → Spec §6.x 변환 → grep 게이트 → 참조 정합 → 스테이징)을 그대로 따른다.

### Task 5: code-reviewer.md

**Files:**
- Create: `.claude/agents/code-reviewer.md`
- 소스: `E:/UnityProject/Project_Lair/.claude/agents/code-reviewer.md`
- 변환 SoT: Spec §6.3

**Interfaces:**
- Consumes: `project.md` 키(`infrastructure`·`code_root`), 룰 `cpp-style`·`unreal-infra`·`git-conventions`, `docs.design` 기획서.
- Produces: skills 가 `code-reviewer` 로 참조. BLOCKER/권장수정/의견 등급 보고.

- [ ] **Step 1: 소스 읽고 Spec §6.3 변환 적용**
  1. description: "C# 코드(.cs)" → "C++ 코드(.h/.cpp)"
  2. 필수 절차: `infrastructure·namespace·architecture·code_root` Unreal 키. "ChvjPackage 실제 API 확인" → "SKGAS/SKAssetCore/SpyAssetManager 실제 시그니처 확인"
  3. 룰 매핑표: 02→cpp-style, 03/04→unreal-infra
  4. 검토 체크리스트: "종속성 — FindObjectOfType/GameObject.Find" → Unreal(`TActorIterator` 남용·하드 참조·재사용 모듈 역참조), "풀 안전 §4" → unreal-infra 해당, "async void 예외" → 레이턴트/델리게이트 누수, MVVM → GAS·InitState 흐름 준수
  5. 등급/보고 형식: 유지, "C# 파일" → "C++ 파일"
  6. 커밋 블록 → `[Tag]` 포맷
- [ ] **Step 2: grep 게이트** — Run: `rg -ni "\.cs\b|c#|monobehaviour|chvjpackage|chm[a-z]+|addressable|findobjectoftype|asmdef|nunit|Rule 0[0-9]|# \[feat\]" .claude/agents/code-reviewer.md` → 매치 없음 (exit 1)
- [ ] **Step 3: 참조 정합** — Run: `rg -n "cpp-style|unreal-infra|gameplay-programmer|test-engineer" .claude/agents/code-reviewer.md` → 존재 확인
- [ ] **Step 4: 스테이징** — `git add .claude/agents/code-reviewer.md` / 메시지(안): `[Chore] code-reviewer — C++ 코드 검토 에이전트 이식`

---

### Task 6: test-engineer.md

**Files:**
- Create: `.claude/agents/test-engineer.md`
- 소스: `E:/UnityProject/Project_Lair/.claude/agents/test-engineer.md`
- 변환 SoT: Spec §6.2
- 참조 패턴: `SkillProject/Source/SkillProject/AI/Tests/SpyAICircleStrafeTests.cpp`

**Interfaces:**
- Consumes: `project.md` 키(`test_paths.automation`·`test_framework`·`code_root`·`infrastructure`), 룰 `cpp-style`·`unreal-infra`.
- Produces: skills 가 `test-engineer` 로 참조. Automation 테스트 스위트 보고.

- [ ] **Step 1: 소스 읽고 Spec §6.2 변환 적용**
  1. description·필수 절차: `test_asmdef`·`edit_mode/play_mode`·`NUnit` → `test_paths.automation`·Unreal Automation. `SpyAICircleStrafeTests.cpp` 스타일 확인
  2. 룰 매핑: 02→cpp-style, 03 §4→unreal-infra 해당 절
  3. 사고 원칙: `IHealth↔FakeHealth` 인터페이스-더블 패턴 유지(C++ 추상 인터페이스/모킹). ViewModel POCO → 순수 로직 클래스
  4. EditMode/PlayMode → `EAutomationTestFlags`(EditorContext/ClientContext/ProductFilter 등)
  5. Test Failure systematic-debugging: "Unity 라이프사이클·Time.timeScale·CharacterRegistry 정적 상태" → Unreal 정적 상태(GEngine/월드 싱글턴·서브시스템), "간헐 재현 = setup 격리 누수" 유지
  6. evidence 테이블 → Spec §4 (Automation 실행은 Session Frontend/`-ExecCmds`/사용자, 기본 "검증 보류")
  7. Self-Review Setup/Teardown 격리: Unity 예시 → Unreal 예시
  8. 커밋 블록 → `[Tag]` 포맷
- [ ] **Step 2: grep 게이트** — Run: `rg -ni "\.cs\b|c#|nunit|editmode|playmode|asmdef|monobehaviour|time\.timescale|chm[a-z]+|Rule 0[0-9]|# \[test\]" .claude/agents/test-engineer.md` → 매치 없음 (exit 1)
- [ ] **Step 3: 참조 정합** — Run: `rg -n "Automation|IMPLEMENT_|DEFINE_SPEC|cpp-style|gameplay-programmer|SpyAICircleStrafeTests" .claude/agents/test-engineer.md` → 존재 확인
- [ ] **Step 4: 스테이징** — `git add .claude/agents/test-engineer.md` / 메시지(안): `[Chore] test-engineer — Unreal Automation 테스트 에이전트 이식`

---

### Task 7: game-designer.md

**Files:**
- Create: `.claude/agents/game-designer.md`
- 소스: `E:/UnityProject/Project_Lair/.claude/agents/game-designer.md`
- 변환 SoT: Spec §6.4

**Interfaces:**
- Consumes: `project.md`(`concept_doc`·`docs.design`·`stage`), `docs/design/spyproject_concept.md` (Task 3).
- Produces: skills·design-reviewer·gameplay-programmer 가 `game-designer` 로 참조. `docs/design/[기능명].md` 기획서 + "구현 요청사항" 섹션.

- [ ] **Step 1: 소스 읽고 Spec §6.4 변환 적용**
  1. **로그라이크 카드 프레이밍 장르 재작성**: "카드·유닛·시스템", "빌드/플레이 방향", "시너지 가시성", "픽률/승률/빌드 다양성/핵심 시각" → 스파이 액션 MP 도메인(어빌리티·무브먼트·전투 감각·레벨·네트워크 페이싱). concept 문서 §2 코어 루프·§3 기능 톤 기준
  2. concept_doc 의존: `docs/design/spyproject_concept.md` 참조
  3. "구현 요청사항": Enum/Interface/에셋 키/SO 스키마 → **Gameplay Tag / C++ 인터페이스 / DataAsset 스키마 / GA·GE 명세** (CLAUDE.md GAS 파이프라인)
  4. No Placeholders 원칙 유지. 단 "데이터 없음" 처리의 qa-simulator 참조 제거 → "사용자/플레이테스트 확인 후 결정 + 결정 메트릭"
  5. **`.mockups` HTML 목업 섹션 처리**: Unity 톤(#262626/Dark UI/Jua/CHText, node 8777)은 무관 → **섹션 제거** (SpyProject UI 기획이 드묾). 유지가 필요하면 별도 요청으로 재작성
  6. 커밋 블록 → `[Tag]` 포맷
- [ ] **Step 2: grep 게이트** — Run: `rg -ni "\.cs\b|c#|scriptableobject|카드|로그라이크|픽률|승률|빌드 다양성|chvjpackage|chm[a-z]+|#262626|jua|8777|node|Rule 0[0-9]|# \[feat\]" .claude/agents/game-designer.md` → 매치 없음 (exit 1)
- [ ] **Step 3: 참조 정합** — Run: `rg -n "spyproject_concept|Gameplay Tag|DataAsset|design-reviewer|gameplay-programmer" .claude/agents/game-designer.md` → 존재 확인
- [ ] **Step 4: 스테이징** — `git add .claude/agents/game-designer.md` / 메시지(안): `[Chore] game-designer — 스파이 액션 도메인 기획 에이전트 이식`

---

### Task 8: design-reviewer.md

**Files:**
- Create: `.claude/agents/design-reviewer.md`
- 소스: `E:/UnityProject/Project_Lair/.claude/agents/design-reviewer.md`
- 변환 SoT: Spec §6.5

**Interfaces:**
- Consumes: `project.md`(`concept_doc`·`docs.design`), `docs/design/spyproject_concept.md`.
- Produces: skills 가 `design-reviewer` 로 참조. BLOCKER/권장수정/의견 검토 보고.

- [ ] **Step 1: 소스 읽고 Spec §6.5 변환 적용**
  1. concept_doc·`docs/design` 의존 → SpyProject concept 문서
  2. 검토 체크리스트: "단계 범위(메타/서버/사운드/아트)"·"밸런스·페이싱(핵심 시각)"·"시너지 가시성" → 스파이 액션 도메인 기준으로 재작성 (concept §4/§5 근거)
  3. "코드 현실 정합"(기획 주장 vs 코드) 유지 — C++/GAS 파일 경로·클래스명 확인으로
  4. 내부 일관성·YAGNI·명확성·스코프 유지 (엔진 무관)
  5. 보고 형식 유지
- [ ] **Step 2: grep 게이트** — Run: `rg -ni "\.cs\b|c#|카드|로그라이크|픽률|승률|scriptableobject|chm[a-z]+|Rule 0[0-9]" .claude/agents/design-reviewer.md` → 매치 없음 (exit 1)
- [ ] **Step 3: 참조 정합** — Run: `rg -n "spyproject_concept|game-designer" .claude/agents/design-reviewer.md` → 존재 확인
- [ ] **Step 4: 스테이징** — `git add .claude/agents/design-reviewer.md` / 메시지(안): `[Chore] design-reviewer — 기획 검토 에이전트 이식`

---

## Phase 3 — 스킬

### Task 9: start-develop.md + .active-sessions.md

**Files:**
- Create: `.claude/skills/start-develop/SKILL.md`
- Create: `.claude/.active-sessions.md`
- 소스: `E:/UnityProject/Project_Lair/.claude/skills/start-develop/SKILL.md`

**Interfaces:**
- Consumes: `project.md`(`uses_superpowers`·후보 스킬 표·워크플로), 에이전트 5종 이름, `superpowers:brainstorming`/`writing-plans`/`subagent-driven-development`/`executing-plans`.
- Produces: `/start-develop` 슬래시 커맨드. `.active-sessions.md` 레지스트리 파일.

- [ ] **Step 1: .active-sessions.md 빈 레지스트리 생성**

```markdown
# Active Agent Sessions

> 스킬 오케스트레이션용 런타임 스크래치. 컨텍스트 요약 후 세션 맵 복구용.
> 커밋하지 않는다 (git add 범위 제외).

| 에이전트 | 세션 ID/이름 | 상태 |
|---|---|---|
```

- [ ] **Step 2: start-develop/SKILL.md 이식**
  1. 개요/프로젝트명: "Project Lair" → "SpyProject", "CLAUDE.md §7" 등 원본 참조 → `project.md` 워크플로 섹션 참조 (SpyProject CLAUDE.md엔 §5/6/7 없음)
  2. 파이프라인: game-designer → design-reviewer → ⛔승인 → gameplay-programmer → code-reviewer → test-engineer (5종, qa-simulator 없음 — 원본도 미포함이나 문구 정리)
  3. superpowers 분기(brainstorming/writing-plans/subagent-driven/executing-plans) 유지
  4. 세션 관리(`SendMessage` 수정 루프·완료 게이트·`.active-sessions.md` 레지스트리) 유지
  5. 마무리 커밋 문구 → git-conventions.md 포맷, `.meta` 문구 제거
- [ ] **Step 3: grep 게이트** — Run: `rg -ni "Project Lair|CLAUDE.md §|qa-simulator|\.cs\b|\.meta\b|# \[feat\]|Unity" .claude/skills/start-develop/SKILL.md` → 매치 없음 (exit 1)
- [ ] **Step 4: 참조 정합** — Run: `rg -n "game-designer|design-reviewer|gameplay-programmer|code-reviewer|test-engineer|project.md|active-sessions" .claude/skills/start-develop/SKILL.md` → 5 에이전트 모두 존재
- [ ] **Step 5: 스테이징** — `git add .claude/skills/start-develop/SKILL.md` (`.active-sessions.md` 는 제외 — 런타임 스크래치) / 메시지(안): `[Chore] start-develop — 승인 게이트 파이프라인 스킬 이식`

---

### Task 10: start-develop-auto / -simple / -quick

**Files:**
- Create: `.claude/skills/start-develop-auto/SKILL.md`
- Create: `.claude/skills/start-develop-simple/SKILL.md`
- Create: `.claude/skills/start-develop-quick/SKILL.md`
- 소스: 원본 동명 3개

**Interfaces:**
- Consumes: Task 9와 동일 (에이전트 5종, project.md, superpowers).
- Produces: `/start-develop-auto`·`/start-develop-simple`·`/start-develop-quick` 슬래시 커맨드.

- [ ] **Step 1: 3개 이식 (Task 9와 동일 변환 규칙 적용)**
  - `-auto`: start-develop과 동일하되 ⛔승인 게이트 생략
  - `-simple`: game-designer → gameplay-programmer → test-engineer (리뷰·시뮬 생략)
  - `-quick`: gameplay-programmer → code-reviewer (0·1단계도 스킵)
  - 3개 모두: "Project Lair"→"SpyProject", CLAUDE.md §참조→project.md, qa-simulator 참조 제거, 커밋 포맷 교정
- [ ] **Step 2: grep 게이트 (3파일)** — Run: `rg -ni "Project Lair|CLAUDE.md §|qa-simulator|\.cs\b|\.meta\b|Unity" .claude/skills/start-develop-auto/SKILL.md .claude/skills/start-develop-simple/SKILL.md .claude/skills/start-develop-quick/SKILL.md` → 매치 없음 (exit 1)
- [ ] **Step 3: 참조 정합** — Run: `rg -n "gameplay-programmer|code-reviewer" .claude/skills/start-develop-quick/SKILL.md` → 존재 확인
- [ ] **Step 4: 스테이징** — `git add .claude/skills/start-develop-auto .claude/skills/start-develop-simple .claude/skills/start-develop-quick` / 메시지(안): `[Chore] start-develop-auto/simple/quick — 파이프라인 변형 스킬 3종 이식`

---

## Phase 4 — 통합 점검

### Task 11: 통합 검증

**Files:** (검증만 — 생성 없음. 문제 발견 시 해당 파일 수정 후 재검증)

- [ ] **Step 1: 전역 Unity 잔재 grep (agents + skills)**

Run: `rg -ni "monobehaviour|scriptableobject|asmdef|nunit|editmode|playmode|chvjpackage|chm[a-z]+|addressable|\.instantiate|createprimitive|findobjectoftype|\bprefab\b|\.meta\b|Project Lair|# \[feat\]" .claude/agents .claude/skills`
Expected: 매치 없음 (exit 1). 문맥상 정당한 언급(예: "Unreal Automation") 외 잔재 0.

- [ ] **Step 2: 번호 규칙 참조 잔재**

Run: `rg -n "Rule 0[0-9]|Rule NN|03\(ChvjPackage\)|04\(Unity" .claude/agents .claude/skills`
Expected: 매치 없음 (exit 1).

- [ ] **Step 3: 참조 정합 — 룰 파일 실존**

Run: `rg -oN "cpp-style|unreal-infra|git-conventions|new-ability-checklist" .claude/agents .claude/skills | sort -u`
그리고 `ls .claude/rules/` 와 대조 — 참조된 룰 파일이 모두 실존하는지 확인.
Expected: 참조된 모든 룰 파일명이 `.claude/rules/` 에 존재.

- [ ] **Step 4: 참조 정합 — 에이전트 이름 실존**

Run: `rg -oN "game-designer|design-reviewer|gameplay-programmer|code-reviewer|test-engineer|qa-simulator" .claude/skills | sort -u`
Expected: `qa-simulator` 매치 없음(제외됨). 나머지 5종만, 모두 `.claude/agents/` 에 파일 존재.

- [ ] **Step 5: 기존 자산 미변경 확인**

Run: `git status --short .claude/rules/cpp-style.md .claude/rules/git-conventions.md .claude/rules/new-ability-checklist.md .claude/settings.json .claude/hooks/`
Expected: 출력 없음 (변경/스테이징 안 됨 — 보존).

- [ ] **Step 6: project.md 키 완전성 — 에이전트가 읽는 키가 모두 존재**

Run: `rg -oN "project.md 의 \`?[a-z_.]+\`?|`[a-z_]+`" .claude/agents | rg -o "engine|code_root|test_paths|infrastructure|concept_doc|docs\.|mcp" | sort -u`
그리고 `.claude/project.md` 에 해당 키가 모두 있는지 육안 대조.
Expected: 에이전트가 참조하는 모든 키가 project.md 에 존재.

- [ ] **Step 7: 드라이런 (선택 — 스모크)**

`/start-develop-quick` 를 사소한 변경 1건(예: 주석 오타 수정)으로 밟아본다: gameplay-programmer 위임 → 룰(cpp-style/unreal-infra) 참조 → code-reviewer 위임 → 커밋 메시지(안) `[Tag]` 포맷 확인. 정상 흐름이면 통과.
(환경상 드라이런 불가하면 "스모크 보류 — 사용자 확인 필요" 로 기록.)

- [ ] **Step 8: 최종 스테이징 요약 + 커밋 메시지(안)**

전체 신규 파일이 스테이징됐는지 확인 (`.active-sessions.md` 제외):
Run: `git status --short .claude docs/design`
통합 커밋 메시지(안) (또는 Task별 개별 커밋):
```
[Chore] .claude — Unity 멀티에이전트 파이프라인 Unreal 포팅 (에이전트 5종·인프라 룰·스킬 4종·메타)
```
사용자에게 concept 문서 내용 확정 필요를 재알림.

---

## Self-Review (작성자 점검 결과)

**1. Spec 커버리지:**
- §1 결정사항 → Task 1~10 전반 반영 ✅
- §2 산출물 구조 → File Structure + Task 1~10 ✅
- §3 매핑 → Task 4~10 변환 단계 ✅
- §4 unreal-mcp evidence → Task 4/6 evidence 교체 ✅
- §5 실행 순서 → Phase 0→1→2→3→4 ✅
- §6 에이전트별 체크리스트 → Task 4~8 각 Step 1이 §6.x를 직접 참조 ✅
- §7 concept 골격 → Task 3 ✅
- §8 project.md 스키마 → Task 1 ✅
- §9 skills → Task 9~10 ✅
- §10 검증 → Task 11 ✅
- §11 리스크 → grep 게이트·보존 확인·foundation-first로 대응 ✅
- §12 범위 외(qa-simulator·concept 내용) → 계획에서 제외/골격만 ✅

**2. Placeholder 스캔:** concept 골격의 "(사용자 확정 대기)" 는 의도된 하드 경계(Global Constraint) — 플랜 결함 아님. 그 외 TBD/TODO 없음.

**3. 타입/이름 일관성:** 에이전트 이름 5종(game-designer·design-reviewer·gameplay-programmer·code-reviewer·test-engineer), 룰 파일명(cpp-style·unreal-infra·git-conventions·new-ability-checklist), 커밋 포맷(`[Tag] ClassName — 요약`) 이 전 Task에서 동일하게 사용됨. qa-simulator는 어디에도 산출물로 없음(제외 일관).
