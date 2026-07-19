# Claude 멀티에이전트 파이프라인 포팅 — Design Spec
Date: 2026-07-19

## 개요

Unity 프로젝트 `Project_Lair`의 `.claude/` 멀티에이전트 개발 파이프라인(에이전트·룰·스킬 시스템)을 SpyProject(UnrealSkillProject, UE5.7/C++)로 **이식(port)**한다. 목적은 그 아키텍처 — `project.md` 메타 파일 + 역할 분담 서브에이전트 파이프라인 + 오케스트레이션 스킬 — 을 Unreal/C++ 현실로 재구현하는 것이다.

**이것은 "파일 복사 후 C# 부분 삭제" 작업이 아니다.** 원본은 파일마다 Unity/C#이 여러 섹션에 촘촘히 박혀 있고, 에이전트·룰·스킬이 서로를 이름/번호로 참조하는 결합 시스템이다. 반쯤 이식된 에이전트(예: 여전히 "Object.Instantiate 금지 → CHMPool" 이라고 적힌 gameplay-programmer)는 Unreal 레포에서 능동적으로 해롭다. 따라서 **에이전트별 Unity 결합을 전부 열거해 제거**하고, **기반(project.md·rules·concept)을 먼저 세운 뒤** 에이전트를 이식한다.

원본 시스템은 잘 설계돼 있어 아키텍처 자체는 엔진 무관을 지향하지만, 실제 텍스트는 Unity/C#에 깊게 묶여 있다. 재사용 가치는 *파일*이 아니라 *아키텍처*이므로, 그 아키텍처를 SpyProject에 맞게 다시 구현한다.

---

## 1. 결정 사항 (확정)

| 항목 | 결정 | 비고 |
|------|------|------|
| 에이전트 범위 | **5종**: game-designer, design-reviewer, gameplay-programmer, code-reviewer, test-engineer | qa-simulator 제외 (로그라이크 밸런스 시뮬 — 스파이 액션 MP에 부적합) |
| 규칙 통합 | **기존 이름 규칙 유지 + 에이전트가 이름으로 참조** | 원본 번호 체계(Rule NN) 미채택. cpp-style/git-conventions/new-ability-checklist 보존 |
| 인프라 규칙 | **`unreal-infra.md` 신규** (원본 Rule 03/04 대체) | AssetManager·GAS·DataAsset·모듈 규약 |
| 설계 그라운딩 | **경량 concept 문서 신규** `docs/design/spyproject_concept.md` | 골격만 생성, **내용은 사용자 확정** |
| 메타 파일 | **`project.md` 신규** (에이전트 SoT, Unreal 키) | CLAUDE.md는 사람용 문서로 유지 |
| 커밋 규칙 | 기존 `git-conventions.md` (`[Tag] ClassName — 요약`) | 원본 Rule 01 커밋포맷·`.meta` 스테이징 **폐기** |
| 빌드/테스트 검증 | 에디터·VS에서 **사용자 수행** + unreal-mcp `execute_python` best-effort | 전용 recompile/test-run MCP 커맨드 없음 (§4 확인) |
| 보존(미변경) | `settings.json`(clang-format 훅), `hooks/`, `settings.local.json`, cpp-style, git-conventions, new-ability-checklist | 사용자 최초 우려(덮어쓰기) 방지 |

---

## 2. 산출물 구조

```
UnrealSkillProject/.claude/
├── project.md                    # 신규 — 에이전트 SoT (Unreal 키)
├── agents/                       # 신규 — 5종
│   ├── game-designer.md
│   ├── design-reviewer.md
│   ├── gameplay-programmer.md    # ← 수직 슬라이스로 먼저 완성, 나머지 복제
│   ├── code-reviewer.md
│   └── test-engineer.md
├── skills/                       # 신규 — start-develop 4종
│   ├── start-develop/SKILL.md
│   ├── start-develop-auto/SKILL.md
│   ├── start-develop-simple/SKILL.md
│   └── start-develop-quick/SKILL.md
├── rules/
│   ├── cpp-style.md              # 유지 (미변경)
│   ├── git-conventions.md        # 유지 (미변경)
│   ├── new-ability-checklist.md  # 유지 (미변경)
│   └── unreal-infra.md           # 신규
├── settings.json                 # 유지 (미변경 — clang-format 훅)
├── settings.local.json           # 유지/병합
├── hooks/post-edit-clang-format.ps1  # 유지 (미변경)
└── .active-sessions.md           # 신규 (스킬 세션 레지스트리 — 커밋 제외)

docs/design/spyproject_concept.md # 신규 — 골격만, 내용 사용자 확정
```

---

## 3. 핵심 매핑 (Unity → Unreal)

| Unity 원본 | Unreal 이식 |
|---|---|
| C# `.cs` / MVVM / ScriptableObject | C++ `.h/.cpp` / GAS·ModularGameplay / `UDataAsset` |
| ChvjPackage (CHMResource/CHMUI/CHMPool/CHText) | **SKGAS · SKAssetCore · SpyAssetManager** |
| Addressables / `Resources/` / Enum 키 로드 | AssetManager `LoadAssetSync`/`LoadAssetAsync`, `USKAssetData` 이름 룩업 (`GetAssetByName`) |
| Prefab / `.meta` / Prefab Variant | Blueprint / DataAsset / BP 상속 |
| NUnit EditMode/PlayMode, asmdef, `Fake*` 더블 | Unreal Automation (`IMPLEMENT_SIMPLE_AUTOMATION_TEST` / `DEFINE_SPEC`), `SpyAICircleStrafeTests.cpp` 패턴 |
| UnityMCP `editor_recompile`/`editor_read_log`/`editor_execute_menu` | unreal-mcp `execute_python` + 사용자 에디터/VS 빌드 (§4) |
| 커밋 `# [주제] - 요약` + `.meta` 스테이징 | `[Tag] ClassName — 요약` (git-conventions.md), `.meta` 개념 없음 |
| 로그라이크 카드 밸런스 도메인 | 스파이 3인칭 액션 데디 서버 MP 도메인 |
| `.mockups` (Unity 톤 #262626/Jua, node 8777) | 재그라운딩 또는 제거 (§6 game-designer) |

---

## 4. unreal-mcp 커맨드 surface (확인 결과 — evidence 테이블 근거)

`tools/unreal-mcp/server.py` 확인. 제공 커맨드는 **authoring + 인스펙션**:

- 에셋: `get/set_asset_property`, `save_asset`, `list_assets`, `find_assets_by_class`
- 액터: `get_actors_in_level`, `get/set_actor_property`, `spawn_actor`, `delete_actor`
- **`execute_python(script)`** — 에디터 내 임의 파이썬 실행 (탈출구)
- Spy 데이터 authoring: `get/save_spy_asset_data`, `add/set/remove_asset_entry`, `add_asset_group`
- BP CDO: `get/set_blueprint_cdo_property`
- 데이터 조회: `get_ability_data`, `get/set_anim_layer`, `get_character_asset_data`, `get/add/remove_combo_set`

**전용 `recompile` / `read_log` / `run_automation_tests` 커맨드는 없음.** 따라서 evidence 규약:

| 주장 | Unreal evidence |
|---|---|
| "컴파일 0 에러" | 사용자가 에디터/VS에서 빌드 (CLAUDE.md 명시). 에이전트는 정적 확인 + (선택) `execute_python`으로 Live Coding 트리거 best-effort. 자체 빌드 단정 금지 → "검증 보류 — 사용자 빌드 필요" 허용 |
| "에셋/런타임 상태 확인" | unreal-mcp `execute_python` / `get_*` 로 실제 에셋·CDO·액터 속성 확인 후 인용 |
| "Automation 테스트 통과" | 사용자가 Session Frontend 또는 `-ExecCmds="Automation RunTests ..."` 실행. `execute_python`으로 콘솔 커맨드 best-effort 가능하나 결과 캡처 불안정 → 기본은 "검증 보류 — 사용자/에디터 실행 필요" |

evidence 없는 주장은 보고에 적지 않고 "검증 보류 — 사유"로 처리한다 (원본 Evidence Before Assertions 원칙 유지, 커맨드만 Unreal 현실로 교체).

---

## 5. 실행 순서 (foundation-first + 수직 슬라이스)

에이전트는 모두 `project.md` 키(engine·code_root·test_paths·infrastructure·concept_doc)와 룰을 참조하므로, 기반을 먼저 세우지 않으면 존재하지 않는 키/룰을 가리킨다.

1. **기반 확정** — `project.md` 키 스키마 + `unreal-infra.md` + `spyproject_concept.md` 골격
2. **gameplay-programmer 1종 완전 이식** (수직 슬라이스) — Unity 잔재 0 확인. 이식 함정을 한 번만 발견하고 패턴을 검증
3. **나머지 4 에이전트 복제** — 2에서 검증된 패턴으로 game-designer / design-reviewer / code-reviewer / test-engineer
4. **skills 4종** — 5-agent 파이프라인으로 (qa-simulator 참조 제거)
5. **통합 점검** — 참조 정합(에이전트↔룰↔스킬), dangling 0, Unity 용어 잔재 grep 0

---

## 6. 에이전트별 Unity 결합 제거 체크리스트 (half-port 방지)

각 에이전트는 여러 섹션에 Unity/C#이 박혀 있다. 이식 시 아래를 **전부** 교체/제거한다. (룰 03/04를 지우면 참조가 깨지던 것과 같은 dangling 문제가 에이전트 *내부*에 있음.)

### 6.1 gameplay-programmer (수직 슬라이스 — 먼저)
- 프로젝트 컨텍스트: `engine·namespace·architecture·code_root·test_paths·infrastructure` 읽기 → Unreal 키로
- 작업 시작 전 절차: "ChvjPackage 우선 확인" → **SKGAS/SKAssetCore/SpyAssetManager 우선 확인**. "asmdef 구성" → **모듈(.Build.cs) 구성**
- 룰 매핑표: `02(C# 스타일)` → `cpp-style`, `03(ChvjPackage)` → `unreal-infra`, `04(Unity 에셋)` → `unreal-infra`(에셋/BP 절)
- **"ChvjPackage 핵심 API" 블록 전체 삭제** → SpyAssetManager/GAS 핵심 API로 재작성 (`LoadAssetSync`/`LoadAssetAsync`, `GetAssetByName`, GAS `GiveToAbilitySystem` 등 CLAUDE.md 참조)
- 사고/작업 원칙: MVVM → Unreal 아키텍처(GAS·컴포넌트·InitState), "종속성 최소화 — 모킹" 유지(엔진 무관)
- TDD 흐름: NUnit → Unreal Automation. "실패 테스트 먼저"는 유지, 러너만 교체
- Bug Fix (systematic-debugging): 엔진 무관 — 유지. "Unity 라이프사이클 오해" → "Unreal 라이프사이클(BeginPlay/InitState/레플리케이션 시점) 오해"
- receiving-code-review: 엔진 무관 — 유지
- **"완료 선언 전 검증" evidence 테이블 → §4 Unreal 규약으로 교체**
- Self-Review "룰 위반 스캔": `//` → `//#`(cpp-style), `Object.Instantiate`/Legacy Text/Resources → **하드코딩 에셋 경로·AssetManager 우회**(CLAUDE.md 규칙)
- **"절대 하지 말 것" 전체 재작성**: `Object.Instantiate`·`CHMPool`·`CHText`·`Resources/`·`//` → Unreal 대응(하드코딩 경로 금지·AssetManager 경유·`//#`·서버 권한/HasAuthority 등)
- 보고 형식 "File Structure": asmdef 영향 → 모듈(.Build.cs) 영향
- **커밋 메시지 블록 `# [feat] - ...` → `[Tag] ClassName — 요약`** (git-conventions.md)

### 6.2 test-engineer
- 필수 절차: `test_paths.edit_mode/play_mode`·`test_asmdef`·`test_framework(NUnit)`·`test_method_naming` → Unreal Automation 키. `SpyAICircleStrafeTests.cpp` 스타일 확인
- 룰 매핑: 02 → cpp-style, 03 §4(CHMPool 풀링) → unreal-infra 해당 절
- 사고 원칙: `IHealth↔FakeHealth` 인터페이스-더블 패턴은 **유지**(엔진 무관, C++ 인터페이스/모킹). ViewModel POCO → 순수 로직 클래스
- EditMode/PlayMode → Automation의 `EAutomationTestFlags`(EditorContext/ClientContext 등) 구분
- Test Failure systematic-debugging: "Unity 라이프사이클" → Unreal, "Time.timeScale·풀 잔존·CharacterRegistry 정적 상태" → Unreal 정적 상태(월드/GEngine 싱글턴 등)
- evidence 테이블 → §4 규약 (Automation 실행은 사용자/Session Frontend)
- Self-Review Setup/Teardown 격리: Unity 정적 상태 예시 → Unreal 예시
- 커밋 블록 → `[Tag]` 포맷

### 6.3 code-reviewer
- 필수 절차: `infrastructure·namespace·architecture·code_root` → Unreal 키. "ChvjPackage 실제 API 확인" → SKGAS/SKAssetCore/SpyAssetManager 실제 시그니처
- 룰 매핑표: 02(C#) → cpp-style, 03/04 → unreal-infra
- 검토 체크리스트: "종속성 Rule 02 §5 — FindObjectOfType/GameObject.Find" → Unreal(`GetWorld()->GetFirstPlayerController()` 남용·`TActorIterator`·하드 참조 등), "풀 안전 §4" → unreal-infra 해당, "async void 예외" → Unreal 비동기(레이턴트/델리게이트) 누수
- 커밋 블록 → `[Tag]` 포맷

### 6.4 game-designer
- **로그라이크 카드 프레이밍 장르 재작성**: "카드·유닛·시스템", "빌드/플레이 방향", "시너지 가시성", "픽률/승률/빌드 다양성/핵심 시각" → 스파이 액션 MP 도메인(어빌리티·무브먼트·전투 감각·레벨·네트워크 페이싱 등). concept 문서의 코어 루프·기능 톤 기준
- concept_doc 의존: `docs/design/spyproject_concept.md` 참조 (§7)
- "구현 요청사항": Enum/Interface/에셋 키/SO 스키마 → Gameplay Tag/인터페이스/DataAsset 스키마/GA·GE 명세 (CLAUDE.md GAS 파이프라인 참조)
- No Placeholders 원칙: **유지**(엔진 무관). 데이터 없음 처리 문구에서 qa-simulator 참조 제거(밸런스 시뮬 에이전트 없음 → "사용자/플레이테스트 확인 후 결정")
- **`.mockups` HTML 목업 섹션 재그라운딩 또는 제거**: Unity 비주얼 톤(#262626/Dark UI/Jua/CHText, node 8777)은 SpyProject와 무관. UI 기획이 드물면 **제거**, 유지 시 SpyProject UI 톤으로 재작성 + 서버는 프로젝트 실정에 맞게
- 커밋 블록 → `[Tag]` 포맷

### 6.5 design-reviewer
- concept_doc·`docs/design` 의존 → SpyProject concept 문서 (§7)
- 검토 체크리스트: "단계 범위(메타/서버/사운드/아트)", "밸런스·페이싱(핵심 시각)", "시너지 가시성" → 스파이 액션 도메인 기준으로 재작성
- "코드 현실 정합"(기획서 주장 vs 코드) — **유지**(엔진 무관, 오히려 C++/GAS에서 더 중요)
- 나머지(내부 일관성·YAGNI·명확성·스코프) 엔진 무관 — 유지

### 6.6 공통 (5종 전부)
- 커밋 메시지 블록 `# [주제] - 요약` → `[Tag] ClassName — 요약`
- 원본 Rule 01의 **Unity `.meta` 스테이징 규칙 삭제** (Unreal에 `.meta` 없음)
- "Rule NN" 번호 참조 → 규칙 파일명 참조(`cpp-style`, `unreal-infra`, `git-conventions`)
- `//` → `//#` 주석 규약 언급은 cpp-style로 연결
- Unity 용어(asmdef·prefab·ScriptableObject·MonoBehaviour·CHM*) grep 잔재 0

---

## 7. concept 문서 (골격만 — 내용 사용자 확정)

`docs/design/spyproject_concept.md` 를 **구조만** 생성한다. 게임 디자인 결정은 지어내지 않고 사용자가 채운다.

골격 섹션:
- **§1 장르/톤** — 스파이 테마 3인칭 액션, 데디케이티드 서버 MP (CLAUDE.md 기반, 세부는 사용자)
- **§2 코어 루프** — (사용자 확정)
- **§3 기능 특징/톤** — 파쿠르·콤보·GAS·타게팅 등 (CLAUDE.md에서 초안, 사용자 검토)
- **§4 현 단계 범위(stage scope)** — (사용자 확정)
- **§5 밸런싱/페이싱 기준** — (사용자 확정, 없으면 "미정 — 플레이테스트 기반")

`project.md`의 `concept_doc` 키가 이 파일을 가리킨다. game-designer/design-reviewer는 이 문서를 근거로 동작한다.

---

## 8. project.md 키 스키마 (Unreal)

원본 Rule 00 키-값 규약을 따르되 값을 Unreal로. 필수 키 예시:

```markdown
## 코드 / 인프라
- **engine**: Unreal Engine 5.7
- **language**: C++
- **code_root**: `SkillProject/Source/`
- **test_paths**
  - **automation**: `SkillProject/Source/SkillProject/**/Tests/`
- **test_framework**: Unreal Automation (AutomationSpec / SimpleAutomationTest)
- **infrastructure**   # ChvjPackage 대체 — 프로젝트 재사용 레이어
  - **modules**: SKGAS, SKAssetCore
  - **asset_access**: SpyAssetManager (LoadAssetSync/Async, USKAssetData 이름 룩업)
- **mcp**: `tools/unreal-mcp/` (execute_python 등)

## 컨셉 / 단계
- **concept_doc**: `docs/design/spyproject_concept.md`
- **stage**: (사용자 확정)

## 문서 위치
- **docs**
  - **design**: `docs/design/`
  - **specs**: `docs/superpowers/specs/`
  - **plans**: `docs/superpowers/plans/`
```

- Unity 전용 키(`test_asmdef`·`edit_mode`/`play_mode`·`balance_config_asset`·`card_data_folder`) 제거
- `uses_superpowers: true` (SpyProject는 이미 superpowers 사용 — specs/plans 존재)
- "메인 오케스트레이터 행동 규칙"(스킬 미지정 후보 게이트·에셋 한정 사이클) 섹션은 엔진 무관 — 이식하되 후보 스킬 표를 SpyProject 스킬(start-develop 4종)로

---

## 9. skills (start-develop 4종)

오케스트레이션 로직(위임·게이트·세션관리)은 엔진 무관 — 대부분 재사용. 이식 시 조정:
- 파이프라인에서 **qa-simulator 참조 제거** (이미 원본 start-develop도 qa 미포함이나, 규칙 문구·후보표 정리)
- 에이전트 5종 이름 참조 정합
- `superpowers:brainstorming`/`writing-plans` 분기 유지 (uses_superpowers: true)
- 커밋 마무리 문구 → git-conventions.md 포맷
- `.active-sessions.md` 세션 레지스트리 유지 (커밋 제외 명시)
- CLAUDE.md §참조를 project.md/실제 문서로 교정 (원본은 "CLAUDE.md §5/§6/§7" 참조 — SpyProject엔 해당 섹션 없음 → project.md 워크플로 섹션으로)

4종 역할(원본 유지):
| 스킬 | 파이프라인 |
|---|---|
| `/start-develop` | game-designer → design-reviewer → ⛔승인 → gameplay-programmer → code-reviewer → test-engineer |
| `/start-develop-auto` | 위와 동일, ⛔승인 생략 |
| `/start-develop-simple` | game-designer → gameplay-programmer → test-engineer (리뷰 생략) |
| `/start-develop-quick` | gameplay-programmer → code-reviewer (0·1단계도 스킵) |

---

## 10. 검증 (Verification)

1. **참조 정합**: 에이전트가 참조하는 룰 파일명(`cpp-style`·`unreal-infra`·`git-conventions`)·`project.md` 키·다른 에이전트 이름이 모두 실존. dangling 0
2. **Unity 잔재 grep 0**: 이식 완료 후 `.claude/agents`·`skills`에서 `asmdef|prefab|ScriptableObject|MonoBehaviour|CHM|Addressable|NUnit|EditMode|PlayMode|Instantiate|\.cs\b|C#` 검색 잔재 0 (문맥상 정당한 언급 제외)
3. **보존 확인**: `settings.json`(clang-format 훅)·`hooks/`·기존 rules 3종 미변경 (git diff로 확인)
4. **project.md 완전성**: 5 에이전트가 읽는 모든 키 존재, Unity 전용 키 제거
5. **드라이런**: `/start-develop-quick`으로 사소한 변경 1건 흐름을 밟아 gameplay-programmer→code-reviewer 위임·룰 참조·커밋 포맷이 정상 동작하는지 확인
6. **concept 골격**: `docs/design/spyproject_concept.md` 구조 생성, 내용 사용자 확정 대기 표시

---

## 11. 리스크 및 대응

| 리스크 | 대응 |
|--------|------|
| 반쯤 이식된 에이전트에 Unity 잔재 | §6 에이전트별 체크리스트 + §10.2 grep 게이트 |
| 기존 `.claude` 자산 덮어씀 | 신규 파일만 추가, 기존 3 rules·settings·hooks 미변경 (§10.3) |
| 에이전트가 없는 키/룰 참조 | foundation-first 순서(§5) — 기반 먼저 |
| concept 내용을 지어냄 | 골격만 생성, 내용 사용자 확정 (§7) — 하드 경계 |
| evidence 테이블이 실재하지 않는 MCP 커맨드 가정 | §4에서 확인된 unreal-mcp surface 기준 |
| 커밋 포맷 이중화 | 원본 Rule 01 폐기, git-conventions.md 단일 (§1) |

---

## 12. 범위 외 (Non-Goals)

- qa-simulator 이식 (제외 확정)
- concept 문서 **내용** 작성 (골격만 — 사용자 확정)
- 기존 cpp-style/git-conventions/new-ability-checklist/settings.json 변경
- CLAUDE.md 재작성 (사람용 문서 — 그대로. 필요 시 project.md와 상호 참조만)
- 새 MCP 커맨드(recompile/test-run) 추가 — 별도 작업
- SpyProject 실제 게임 기능 구현 (파이프라인 인프라 한정)
