# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Unreal Engine **5.7** 기반 스파이 테마 3인칭 액션 게임. 데디케이티드 서버 멀티플레이어, Lyra 스타일 모듈형 아키텍처, 커스텀 GAS 프레임워크를 핵심으로 한다.

## 작업 시작 전 필독 — 룰 / 에이전트 / 스킬

이 저장소의 작업 규약은 `.claude/` 에 있다. **중복 기술하지 않는다 — 아래는 포인터이고, 각 파일이 SoT다.**

### 코딩 룰 (`.claude/rules/`)

코드를 한 줄이라도 만지기 전에 해당 룰을 읽는다.

| 룰 파일 | 적용 시점 |
|---|---|
| [cpp-style.md](.claude/rules/cpp-style.md) | 모든 `.h`/`.cpp` 작성·수정 시 (주석 `//#`, `!`·`auto` 금지, 가드 절, `TObjectPtr`, UPROPERTY 지정자, 종속성 최소화, 위젯 캡슐화, 공용 Enum/Interface 파일) |
| [git-conventions.md](.claude/rules/git-conventions.md) | 스테이징·커밋 메시지 작성 시 (`[Tag] ClassName — 요약`) |
| [unreal-infra.md](.claude/rules/unreal-infra.md) | 모듈 의존 방향·서버 권한·플러그인 소비 패턴 결정 시 |
| [plugin-skgas.md](.claude/rules/plugin-skgas.md) | GA/AttributeSet/Cue/태그 추가 시 (§6 새 어빌리티 체크리스트) |
| [plugin-skassetcore.md](.claude/rules/plugin-skassetcore.md) | 에셋 로드·DataAsset 추가 시 (하드코딩 경로 금지) |
| [plugin-skuicore.md](.claude/rules/plugin-skuicore.md) | UI 위젯·UI 매니저 작업 시 |
| [plugin-modulargameplayactors.md](.claude/rules/plugin-modulargameplayactors.md) | 액터 베이스 상속·InitState 초기화 흐름 작업 시 |
| [ui-workflow.md](.claude/rules/ui-workflow.md) | UI 편집 착수 **후** — unreal-mcp 편집 방법·함정 (착수 **전** 목업 승인 게이트는 `.claude/project.md`) |

### 에이전트 메타 (`.claude/project.md`)

`.claude/project.md` 는 서브에이전트가 작업 시작 시 가장 먼저 읽는 **구조화된 프로젝트 메타**다. 엔진·모듈·테스트 경로·문서 위치·협업 흐름·메인 오케스트레이터 행동 규칙이 여기 있다. **메인이 멈춰야 하는 게이트 3종**(스킬 후보 제시 · 에셋 한정 사이클 · UI 목업 승인)도 전부 이 파일에 모여 있다.  
**CLAUDE.md 와 `.claude/project.md` 에 같은 정보가 있으면 `.claude/project.md` 가 SoT다.**

### 서브에이전트 (`.claude/agents/`)

| 에이전트 | 담당 |
|---|---|
| `game-designer` | 기획서 작성 (`docs/design/`) — 코드 작성 금지 |
| `design-reviewer` | 기획서 1차 검토 (읽기 전용) |
| `gameplay-programmer` | C++ 구현 — `.h`/`.cpp` 변경은 전부 이 에이전트 |
| `code-reviewer` | 룰 준수 + 기획서 일치 검토 (읽기 전용) |
| `test-engineer` | Unreal Automation 테스트 스위트 |

### 개발 파이프라인 스킬 (`.claude/skills/`)

| 스킬 | 용도 |
|---|---|
| `/start-develop` | 본격 기능 — 기획서 후 **사용자 승인 게이트** 포함 (기본 권장) |
| `/start-develop-auto` | 본격 기능 — 승인 게이트 없이 끝까지 |
| `/start-develop-simple` | 프로토타입 — 리뷰 생략, 테스트 유지 |
| `/start-develop-quick` | 사소 수정·리네임·작은 버그 — 구현 + 코드리뷰만 |

**메인 오케스트레이터 규칙** (`.claude/project.md` "메인 오케스트레이터 행동 규칙"):
- 코드/에셋 변경이 명확한 요청인데 스킬 이름이 없으면 → 위 후보 표를 제시하고 **사용자 선택까지 멈춘다.** 메인이 임의로 "이건 quick 이면 충분" 판단해 직진 금지.
- 메타 질문·단순 조회·조언 요청은 게이트 없이 즉시 답변.
- 메인은 직접 `.h`/`.cpp` 를 수정하지 않는다 — `gameplay-programmer` 에 위임.
- **`git commit` 자동 실행 금지** — 관련 파일 `git add` 까지만 하고 커밋 메시지(안)를 제시한다.

## Build & Development

**프로젝트 파일 생성 (solution 재생성 필요 시):**
```
SkillProject/Launch.bat
```
이 배치 파일이 `Build.bat -projectfiles`를 호출한다. 실제 빌드는 Unreal Editor 또는 Visual Studio에서 수행한다.

**주요 파일:**
- `SkillProject/SkillProject.uproject` — 엔진 버전 및 모듈/플러그인 등록
- `SkillProject/SkillProject.sln` — Visual Studio 솔루션

**새 C++ 클래스 추가 후:** Unreal Editor의 Tools > Refresh Visual Studio Project 또는 uproject 우클릭 > Generate Visual Studio project files 실행 필요.

## Source Module Architecture

재사용 레이어는 `SkillProject/Plugins/` 로 분리되어 있고, 게임/에디터 모듈만 `Source/` 에 남는다:

```
SkillProject/Plugins/
├── SKGAS/                  # 범용 GAS 코어 (Runtime) — 프로젝트 비의존
│   └── Source/SKGAS/
│       ├── Ability/        # SKGameplayAbility 베이스 클래스 계층
│       ├── Attribute/      # SKAttributeSet 베이스
│       ├── Calculation/    # SKBaseMagnitudeCalculation / SKExecCalculation
│       └── Cue/            # SKCueManager (비동기 프리로딩), SKCueActorPool
├── SKAssetCore/            # 범용 에셋 매니저 + 이름→경로 룩업 DataAsset
├── SKUICore/               # 범용 UI 매니저(open/cache/reuse) + 위젯 베이스
└── ModularGameplayActors/  # 에픽 ModularGameplay 통합 플러그인

SkillProject/Source/
├── SkillProject/           # 게임 로직 메인 모듈 (Runtime)
│   ├── AbilitySystem/      # GA 구현체 (Skill, Movement, Calculation)
│   ├── Character/          # SpyCharacter + 컴포넌트들
│   ├── Data/               # 모든 DataAsset 클래스 정의
│   ├── Input/              # Enhanced Input + 태그 바인딩
│   ├── Manager/            # SpyAssetManager, SpyUIManager
│   ├── ManagerComponent/   # Parkour, Anim, Targeting, SpawnBot 컴포넌트
│   ├── System/             # GameMode, GameState, PlayerController, PlayerState
│   ├── AI/                 # SpyAIController, BT Task들
│   ├── Item/               # SpyWeapon
│   ├── UI/                 # HUD, Widget 클래스
│   └── Util/               # DefineEnum.h, SpyGameplayTags
├── SpyDataEditorTool/      # 에디터 전용 데이터 세팅 툴 (Editor)
│   ├── Tabs/               # SSpyAssetsTab, SSpyAbilityTab, SSpyConfigTab
│   ├── Customizations/     # IDetailCustomization, IPropertyTypeCustomization
│   └── Utils/              # SpyDataScanner, SpyEditorUtils
├── SpyGACreatorTool/       # GA 클래스 생성 툴 (Editor)
└── SpyTagManagerTool/      # 게임플레이 태그 관리 툴 (Editor)
```

**모듈 의존 방향:** `SkillProject` → `SKGAS` → `GameplayAbilities`. `SpyDataEditorTool`은 `SkillProject` + 에디터 전용 모듈에만 의존.

## Key Architectural Patterns

### InitState 기반 초기화 흐름
`SpyPawnExtensionComponent`가 `IGameFrameworkInitStateInterface`를 구현한다. `CharacterAssetData`가 서버에서 클라이언트로 레플리케이트 완료 + Controller 연동 후 `InitState_DataInitialized` 단계에서 `InitAbilityActorInfo`를 호출한다. GA를 추가하거나 ASC를 건드리는 코드는 반드시 이 흐름 이후에 실행되어야 한다.

### GAS 데이터 파이프라인
- `USpyAbilityData` (DataAsset) → `GiveToAbilitySystem()` 호출 시 AttributeSet 동적 생성, 초기 GE 적용, GA 부여를 한 번에 수행
- 모든 부여 핸들은 `FSpyAbilitySet_GrantedHandles`로 트래킹 → 장착 해제/사망 시 반드시 `TakeFromAbilitySystem()`으로 해제
- 입력은 `SpyEnhancedInputComponent`에서 Gameplay Tag → ASC `AbilityLocalInputPressed/Released`로 연결

### DataAsset 계층
```
USKAssetData (이름→경로 룩업 베이스)
└── USpyAssetData          # 전체 에셋 중앙 허브 (SpyAssetManager가 게임 시작 시 동기 로드)
USpyCharacterAssetData     # 캐릭터별 컴포넌트 목록 + 어빌리티 세트 + 입력 설정 + 콤보 데이터
USpyAbilityData            # GAS 어빌리티/AttributeSet/GameplayEffect 묶음
USpyComboAssetData         # StartSkillTag → ComboTag 딕셔너리
USpyAnimAssetData          # AnimLayer 맵 (FName → TSoftClassPtr)
```
Config DataAsset: `SpyAIConfig`, `SpyCharacterConfig`, `SpyInputConfig`, `SpyMovementConfig` — 하드코딩된 수치들을 이쪽으로 이전하는 작업이 진행 중 (docs/hardcoded-values.md 참고).

### 파쿠르 시스템
`SpyParkourManagerComponent`가 서버에서 다중 LineTrace로 장애물 분석 → `FMotionWarpingData` 계산 → 클라이언트에 `OnRep_*MotionWarpingData`로 레플리케이트. 파쿠르 액션은 `GA_Vault`, `GA_WallClimb`, `GA_HangUp` GA로 캡슐화.

### 콤보 시스템
`SpyAnimNotify_State_Combo`가 콤보 윈도우 동안 ASC에 `Character_State_Combo` Loose Tag를 부여/해제. 입력 시 현재 스킬 태그를 `SpyComboAssetData`에서 색인해 다음 GA를 즉시 Activate.

### SpyDataEditorTool
에디터 탭 3개(Assets / Ability / Config)로 `Content/Spy/Data/` DataAsset을 일괄 편집. Scan → 검토/편집 → Apply 흐름. 새 에셋 타입을 추가하면 해당 탭 Slate 코드(`Tabs/` 폴더)와 `SpyDataScanner`를 함께 수정해야 한다.

## Gameplay Tags

`SkillProject/Source/SkillProject/Util/SpyGameplayTags.h` — 프로젝트 전역 태그 선언.  
`SkillProject/Plugins/SKGAS/Source/SKGAS/SKGameplayTags.h` — SKGAS 레이어 공용 태그.  
새 태그는 반드시 이 파일들에 `UE_DECLARE_GAMEPLAY_TAG_EXTERN` + .cpp에 `UE_DEFINE_GAMEPLAY_TAG`로 등록할 것. 문자열 리터럴로 태그를 직접 참조하지 말 것.

## 중요 규칙

- **서버 권한**: 게임플레이 상태 변경은 서버에서 실행하고 클라이언트에 레플리케이트. GA 내에서 `HasAuthority()` 체크 후 처리.
- **에셋 접근**: `SpyAssetManager`의 `LoadAssetSync` / `LoadAssetAsync`를 통해 접근. 하드코딩된 에셋 경로 직접 참조 금지.
- **컴포넌트 주입**: 런타임 컴포넌트 추가는 `CharacterAssetData`의 컴포넌트 목록을 읽어 `NewObject & RegisterComponent`로 수행 — `BeginPlay`에 하드코딩하지 말 것.
- **하드코딩된 수치**: `docs/hardcoded-values.md`에 정리된 매직 넘버/문자열은 점진적으로 Config DataAsset으로 이전 중.

## External Tools

- `tools/unreal-mcp/` — Unreal Editor 원격 제어용 Python MCP 서버 (`server.py` + `unreal_client.py`).
