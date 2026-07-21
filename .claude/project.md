# Project Meta

이 파일은 `.claude/agents/*.md` 의 모든 서브에이전트가 작업 시작 시 가장 먼저 읽는 **프로젝트 메타** 다. agent 정의는 도메인 정보를 직접 들고 있지 않고 이 파일을 통해서만 프로젝트를 인지한다.

`CLAUDE.md` 와 별개다 — `CLAUDE.md` 는 사람용 자유 양식 문서, 이 파일은 agent 가 읽는 구조화된 메타다. 중복 정보는 이 파일이 SoT.

---

## 프로젝트

- **name**: SpyProject (게임 모듈명 `SkillProject`)
- **one_liner**: 스파이 테마 3인칭 액션 — 데디케이티드 서버 멀티플레이어, 모듈형 액터 + 커스텀 GAS. 재사용 플러그인(SKGAS·SKAssetCore·SKUICore·ModularGameplayActors) 기반.

## 컨셉 / 단계

- **concept_doc**: `docs/design/spyproject_concept.md` (§4 현 단계 범위 포함 — 다수 항목이 "사용자 확정 대기")
- **stage**: (사용자 확정 대기)
- **stage_goal**: (사용자 확정 대기 — 컨셉서 §2 코어 루프·§4 단계 범위가 미확정. 기획 작업 전에 사용자에게 먼저 확인할 것)

## 코드 / 인프라

- **engine**: Unreal Engine 5.7
- **language**: C++
- **architecture**: 모듈형 액터(ModularGameplayActors) + GAS(SKGAS) + InitState 초기화 흐름
- **code_root**: `SkillProject/Source/`
- **test_paths**
  - **automation**: `SkillProject/Source/SkillProject/**/Tests/`
- **test_framework**: Unreal Automation (SimpleAutomationTest / AutomationSpec)
- **test_method_naming**: english — 기존 `SkillProject/Source/SkillProject/AI/Tests/SpyAICircleStrafeTests.cpp` 스타일 준수. 구조체 `F<Domain><Case>Test`, 등록 문자열 `"SkillProject.도메인.기능.케이스"`, 파일 전체를 `#if WITH_DEV_AUTOMATION_TESTS` 로 감싼다.
- **infrastructure**
  - **modules** (모두 `SkillProject/Plugins/` 플러그인, `.uplugin` `EnabledByDefault: true` 로 자동 활성): `SKGAS` (GAS 코어 — ASC·AttributeSet·Ability·Cue·Calculation·Tag), `SKAssetCore` (에셋 매니저 + 이름→경로 룩업), `SKUICore` (UI 매니저 + 위젯 베이스), `ModularGameplayActors` (모듈형 액터 베이스)
  - **game_modules** (`SkillProject/Source/`): `SkillProject` (게임 로직, Runtime), `SpyDataEditorTool` / `SpyGACreatorTool` / `SpyTagManagerTool` (에디터 전용 툴)
  - **asset_access**: `USpyAssetManager`(`Manager/SpyAssetManager.h`, `USKAssetManager` 서브클래스 — `DefaultEngine.ini` 의 `AssetManagerClassName` 로 등록됨) — `LoadAssetSync` / `LoadAssetAsync`, `USKAssetData` 이름 룩업(`GetAssetByName` / `GetSubclassByName`). UI 는 `USpyUIManager`(`USKUIManager` 서브클래스) 경유.
  - **module_dependency**: 게임 모듈 `SkillProject` → `SKGAS` / `SKUICore` / `SKAssetCore` / `ModularGameplayActors` → UE 표준 모듈. `SKUICore → SKAssetCore` (역방향 참조 금지, unreal-infra §1). 에디터 툴 모듈은 `SkillProject` + 에디터 전용 모듈에만 의존.
- **mcp**: `tools/unreal-mcp/` — 에디터 원격 제어 (`execute_python`, `get_asset_properties`/`set_asset_property`, `get_actors_in_level` 등). 전용 recompile/test-run 커맨드는 없음 — 빌드/테스트는 에디터·VS에서 사용자 수행.

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
