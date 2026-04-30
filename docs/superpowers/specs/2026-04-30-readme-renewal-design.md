# README 리뉴얼 — Design Spec

Date: 2026-04-30
Target file: `README.md` (전면 교체)
Author: chvj7567 + Claude

---

## 1. 배경 및 목적

### 1.1 현 상태
- 기존 README는 UE **5.4** 기준으로 작성되어 있고, 이후 6개월간 278 커밋이 누적되며 다수의 신규 시스템이 추가되었으나 README에는 반영되지 않음.
- 본문 8개 섹션(멀티플레이어 / 모듈화 / GAS / 데이터 / 에셋매니저 / 입력 / 파쿠르 / 콤보)은 모두 단단한 분량(1~2 문단 + 디테일 불릿)으로 작성되어 있으나, 신규 기능을 같은 깊이로 덧붙이면 1000줄에 육박해 가독성 저하 위험.

### 1.2 리뉴얼 목적
**포트폴리오용 쇼케이스.** 채용·협업 검토자가 30초 안에 임팩트를 받고, 관심 시스템만 펼쳐서 깊게 볼 수 있도록 한다. 빌드 가이드·기술 문서 성격은 부차적.

### 1.3 비목표
- 별도 영문판(README.en.md) 작성 — 기존 한글 본문 + 영문 괄호 병기 스타일을 유지한다.
- 데모 사이트, GitHub Pages 등 외부 호스팅 — README 단일 파일에 한정한다.

---

## 2. 신규 반영 대상 (현 README에 누락된 항목)

| 분류 | 신규 시스템 | 주요 클래스/모듈 |
|---|---|---|
| 엔진 | UE **5.7** 마이그레이션 | uproject `EngineAssociation` |
| 액션 | **그래플링 훅 시스템** | `SpyGA_GrappleHook`, `GrappleCableActor`, `SpyGrappleTargetingComponent`, `SpyGrappleUIComponent`, `SpyAbilityTask_GrappleTick` |
| 액션 | **홀드형 패링 시스템** | `SpyGameplayAbility_Parry`, `Skill_Parry_Hit`, `Character_State_Parry` |
| 전투 | **타겟팅 매니저** | `SpyTargetingManagerComponent` |
| 전투 | **무기 AnimTrail 이펙트** | `SpyWeapon` (GA_Skill 발동 시) |
| 전투 | **히트 카메라 셰이크** | `2026-04-24-hit-camera-shake-design.md` 산출물 |
| 전투 | **팀 시스템** | `FCharacterAssetEntry::TeamId`, NoTeam 기본값 |
| AI | **Behavior Tree Tasks** | `BTTask_ActivateAbility`, `BTTask_MoveToTarget`, `BTTask_CircleStrafe`, `BTTask_FindRandomPos`, `BTService_CheckCooldown` |
| AI | **EQS 시스템** | `EnvQueryContext_StrafeDirection` |
| AI | **SpawnBot 매니저** | `SpySpawnBotManagerComponent` |
| 툴 | **SpyDataEditorTool** | Assets / Ability / Config 3탭 일괄 편집기 (Editor 모듈) |
| 툴 | **SpyGACreatorTool** | Window 메뉴 → GA Blueprint 원클릭 생성 (Editor 모듈) |
| 툴 | **Unreal MCP 서버** | `tools/unreal-mcp/` Python MCP |
| 플러그인 | CableComponent, RemoteControl 추가 | uproject `Plugins` |

---

## 3. 결정사항 요약

| # | 결정 영역 | 선택 |
|---|---|---|
| Q1 | 1순위 목적 | **A. 포트폴리오용 쇼케이스** |
| Q2 | 신규 기능 통합 방식 | **D. 상단 하이라이트 + 6개 카테고리 + 부록** |
| Q3 | 시각 자료 정책 | **B. 일부만 보유 — 플레이스홀더 + 캡션 우선** |
| Q4 | 언어/톤 | **A. 한글 본문 + 핵심 용어 영문 괄호 병기** |
| § 2-1 | 한 줄 카피 | **D**. *UE 5.7 · Dedicated Server · 커스텀 GAS · 모듈형 아키텍처 — 확장 가능한 멀티플레이어 액션 프레임워크.* |
| § 2-2 | 배지 | UE 5.7 / C++17 / Dedicated Server / GAS / Modular (5개) |
| § 2-3 | 인트로 단락 | 합의안 그대로 (2 문단) |
| § 3 | 핵심 셀링포인트 | **1·3·4·7·8** (5개) |
| § 4-1 | 본문 톤 | **C. 압축 + `<details>` 토글** |
| § 4-2 | 포맷 요소 | **a · b · c · d 전부** (Mermaid · 코드 블록 · 폴더 트리 · GIF 자리) |
| § 5-1 | 빌드 가이드 | **A. 최소** — `Launch.bat` 대신 일반 generate 방식 |
| § 5-2 | 모듈 의존 그래프 | Mermaid 그대로 |
| § 5-3 | 폴더 트리 | 더 자세히 |
| § 5-4 | 기술 스택 부록 | **빼기** |
| § 5-5 | 추가 부록(라이선스/크레딧/참고/이력) | **모두 빼기** |

---

## 4. 최종 README 구조 (목차)

```
# UnrealSkillProject : Spy Project
└─ 한 줄 카피 (Q § 2-1)
└─ 배지 5개 (UE 5.7 · C++17 · Dedicated Server · GAS · Modular)
└─ 인트로 2 문단

🎬 Showcase
└─ GIF/영상 자리 5칸 (플레이스홀더 + 캡션)
   파쿠르 · 콤보 · 그래플링 · 패링 · AI 전투

✨ 핵심 셀링포인트
└─ 5개 bullet (1·3·4·7·8)

────────────────────────────────────────────
🛠️ 시스템 아키텍처  (모든 섹션: 1문단 요약 + <details> 토글)
────────────────────────────────────────────

1. 🌐 코어 프레임워크
   1-1. 데디케이티드 서버 멀티플레이어
   1-2. 모듈형 아키텍처 + InitState 동기화
   1-3. Enhanced Input × Gameplay Tag

2. ⚡ GAS & 데이터 파이프라인
   2-1. SKGAS 모듈 (커스텀 GAS 래퍼)
   2-2. Data-Driven GiveAbility (USpyAbilityData)
   2-3. DataAsset 계층 + SpyAssetManager
   2-4. SKCueManager 비동기 프리로딩

3. 🏃 캐릭터 액션
   3-1. 파쿠르 (Vault / WallClimb / HangUp)
   3-2. 데이터 지향 콤보 시스템
   3-3. 🆕 그래플링 훅 (타겟팅 + 케이블 + UI 프롬프트)
   3-4. 🆕 홀드형 패링 시스템

4. ⚔️ 전투 / 인터랙션  🆕
   4-1. 타겟팅 매니저
   4-2. 무기 AnimTrail 이펙트
   4-3. 히트 카메라 셰이크
   4-4. 팀 시스템 (TeamId)

5. 🤖 AI 시스템  🆕
   5-1. Behavior Tree Tasks
   5-2. EQS + StrafeDirection Context
   5-3. SpawnBot 매니저

6. 🧰 에디터 툴체인 & 워크플로우  🆕
   6-1. SpyDataEditorTool — 3탭 데이터 일괄 편집기
   6-2. SpyGACreatorTool  — GA Blueprint 원클릭 생성
   6-3. Unreal MCP 서버    — Python 원격 제어

────────────────────────────────────────────
📎 부록
────────────────────────────────────────────
A. 빌드 방법 (최소 — generate VS project files → .sln 빌드)
B. 모듈 의존 그래프 (Mermaid)
C. 폴더 구조 (자세히)
```

🆕 = 기존 README에 없던 신규 섹션/항목.

---

## 5. 본문 섹션 작성 규칙 (모든 섹션 공통)

C 옵션(압축 + `<details>` 토글)에 따라 모든 본문 섹션은 동일한 패턴으로 작성한다.

### 5.1 패턴 템플릿

```markdown
### N. 🔧 섹션 제목

> 1~2 문장 핵심 요약 (왜 만들었는지 + 어떻게 만들었는지).

<details>
<summary>자세히 보기</summary>

#### N-1. 하위 항목 제목
2~4 문단의 상세 설명. 핵심 클래스명·핵심 흐름.

```cpp
// 4~8줄짜리 핵심 스니펫 (필요 시)
```

```mermaid
%% 시퀀스/플로우/그래프 (필요 시)
```

<!-- TODO: GIF/스크린샷 — 캡션: ... -->

#### N-2. 다음 하위 항목
...

</details>
```

### 5.2 포맷 요소 가이드

- **Mermaid 다이어그램 적용 우선순위**:
  1. § 1-2 InitState 동기화 시퀀스
  2. § 2-2 Data-Driven GiveAbility 파이프라인
  3. § 3-1 파쿠르 LineTrace 흐름 (Forward → Top-Down → Depth)
  4. § 5-1 Behavior Tree 노드 흐름
  5. § 부록 B 모듈 의존 그래프
- **C++ 코드 블록**: § 2-1, § 2-2, § 3-3, § 3-4 등 핵심 호출 패턴 시그니처 위주. 절대 50줄 넘기지 않음.
- **폴더 트리**: 부록 C에서만 사용. 본문에서는 인라인 코드(``)로만 경로 언급.
- **GIF 자리**: 시그니처 5개 시스템 + 「🎬 Showcase」 상단 영역. `<!-- TODO: GIF -->` HTML 주석 + 한 줄 캡션 필수.

### 5.3 톤 가이드

- 기존 README의 어조(존댓말, 진지한 기술 어조) 유지.
- 핵심 용어는 영문 괄호 병기: *데디케이티드 서버(Dedicated Server)*, *모션 워핑(Motion Warping)*.
- 한글 본문 안에서 클래스명·태그명·파일명은 ``\` 백틱으로 감쌈.
- 자랑 톤(superlative)은 절제. *"완벽한"*, *"극대화"* 같은 표현은 1~2회만 허용.

---

## 6. 작성 단위 (implementation plan에서 분할 기준)

writing-plans 단계에서 다음 단위로 작업을 쪼갠다.

| Step | 내용 |
|---|---|
| 1 | 헤더 (제목 + 카피 + 배지 + 인트로) |
| 2 | 🎬 Showcase + ✨ 핵심 셀링포인트 |
| 3 | § 1 코어 프레임워크 (3 하위) |
| 4 | § 2 GAS & 데이터 (4 하위) |
| 5 | § 3 캐릭터 액션 (4 하위 — 신규 2개 포함) |
| 6 | § 4 전투/인터랙션 (4 하위 — 전체 신규) |
| 7 | § 5 AI 시스템 (3 하위 — 전체 신규) |
| 8 | § 6 에디터 툴체인 (3 하위 — 전체 신규) |
| 9 | 부록 A (빌드) + B (의존 그래프) + C (폴더 트리) |
| 10 | 최종 검토 — 톤 일관성 / Mermaid 렌더 검증 / 링크 깨짐 검증 |

---

## 7. 성공 기준 (검수 체크리스트)

- [ ] § 2 표의 모든 신규 시스템(엔진/액션/전투/AI/툴/플러그인)이 README 어딘가에 명시적으로 등장.
- [ ] § 1 핵심 셀링포인트 5개(1·3·4·7·8)가 상단 30줄 안에 위치.
- [ ] 각 본문 섹션이 1~2 문장 요약 + `<details>` 토글 패턴을 일관되게 따름.
- [ ] Mermaid 다이어그램 ≥ 3개 (5.2의 우선순위 1·2·5는 필수).
- [ ] GIF 플레이스홀더 ≥ 5개, 각 캡션 명시.
- [ ] 빌드 부록에 `Launch.bat` 노출 없음 (커스텀 파일이므로 일반 generate 방식만 안내).
- [ ] 라이선스 / 크레딧 / 참고자료 / 변경이력 부록 모두 부재 확인.
- [ ] UE 버전 표기가 5.7로 일관됨 (5.4 잔재 0).

---

## 8. 리스크 및 완화책

| 리스크 | 완화책 |
|---|---|
| `<details>` 토글이 일부 마크다운 뷰어(특히 한국 IDE 플러그인)에서 펼침 상태로만 보일 수 있음 | GitHub 렌더링이 1순위 환경. 기타 뷰어는 비목표. |
| Mermaid 다이어그램이 GitHub Web에서만 렌더되고 일부 모바일/뷰어에서 코드 블록으로 보일 수 있음 | 다이어그램 위/아래에 텍스트 설명을 함께 두어 의미 손실 최소화. |
| 신규 시스템 6개를 동시에 추가하다 보니 클래스 명세에 오타·잘못된 경로가 들어갈 위험 | 작성 후 모든 클래스명·경로를 `Grep`으로 실재 확인. |
| 분량이 의도보다 길어질 위험 | 모든 본문 1문단 요약은 4줄 이하로 제한. `<details>` 안 깊이는 자유. |
