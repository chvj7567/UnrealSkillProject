---
name: start-develop
description: Use ONLY when the user explicitly invokes this skill by name. Runs SpyProject's feature-development pipeline (game-designer through test-engineer) with a user-approval gate after the design doc. Do not auto-trigger from an ordinary feature request — explicit invocation required.
---

# start-develop — 기획→구현→테스트 파이프라인 (승인 게이트 버전)

## 개요

사용자가 **명시적으로 호출했을 때만**, SpyProject 의 표준 협업 흐름(`.claude/project.md` "협업 흐름 (Workflow)" 표)을 한 번에 오케스트레이션한다. 기획서 단계에서 **멈춰 사용자 승인을 받은 뒤** 구현으로 넘어간다.

메인 오케스트레이터는 **직접 코드를 짜지 않는다** (`.claude/project.md` "메인 오케스트레이터 행동 규칙"). 각 단계를 해당 서브에이전트에 위임한다.

## 호출 시 입력

기능 설명을 인자로 받는다. 인자가 없으면 무엇을 만들지 사용자에게 먼저 묻는다.

## 사전 단계 — 슈퍼파워 분기

`.claude/project.md` 의 `uses_superpowers` 키를 확인해 시작 지점을 결정한다.

- **uses_superpowers: true** — 아래 파이프라인 앞에 다음 2단계를 먼저 수행:
  - **0. `superpowers:brainstorming`** → `docs/superpowers/specs/YYYY-MM-DD-[기능명]-design.md` (의도·범위 합의)
  - **1. `superpowers:writing-plans`** → `docs/superpowers/plans/YYYY-MM-DD-[기능명].md` (Task 단계화)
  - **1 완료 후 — 실행 방식 선택 게이트**: writing-plans 산출물을 사용자에게 제시하고 다음 두 갈래를 선택하게 한다. 선택 전까지 진행하지 않는다.
    - **A. 슈퍼파워 실행** — `superpowers:subagent-driven-development` (태스크별 서브에이전트) 또는 `superpowers:executing-plans` (현 세션 일괄 실행) 으로 플랜을 직접 구현. game-designer 이후 파이프라인은 건너뜀.
    - **B. start-develop 파이프라인 계속** — 아래 game-designer 단계부터 이어서 진행. spec + plan 경로를 game-designer에 함께 전달.
- **uses_superpowers: false** — 0·1 단계 생략. 메인이 사용자와 의도·범위를 대화로만 합의한 뒤 아래 파이프라인 1번부터 시작.

## 파이프라인 (순서대로)

1. **game-designer** 위임 → `docs/design/[기능명].md` 기획서 작성.
2. **design-reviewer** 위임 → 기획서 검토.
   - BLOCKER 가 있으면 game-designer 에 수정 위임 후 design-reviewer 재검토. **최대 3회** 반복. 3회 후에도 남으면 사용자에게 보고하고 중단.
3. **⛔ 승인 게이트** — 기획서 요약 + design-reviewer 결과를 사용자에게 제시하고 **멈춘다.** 사용자가 승인하기 전까지 구현을 시작하지 않는다. 사용자가 수정을 요청하면 game-designer 에 반영 위임 후 다시 이 게이트로 돌아온다.
4. **gameplay-programmer** 위임 → 승인된 기획서대로 구현.
5. **code-reviewer** 위임 → 코딩 룰(`.claude/rules/` — cpp-style·unreal-infra·git-conventions) 준수 + 기획서 일치 검토.
   - BLOCKER 가 있으면 gameplay-programmer 에 수정 위임 후 code-reviewer 재검토. **최대 3회** 반복. 3회 후에도 남으면 사용자에게 보고하고 중단.
6. **test-engineer** 위임 → 본격 테스트 스위트(Unreal Automation) 작성.
7. **마무리** — 변경사항 요약 + 커밋 메시지(안) 제시. `git-conventions.md` 포맷(`[Tag] ClassName — 요약`) 준수 — `git commit` 직접 실행 금지, 관련 파일 `git add` 까지만.

## 규칙

- 각 서브에이전트의 산출물·보고를 사용자에게 단계별로 간결히 전달한다.
- 룰·위임 기준은 `.claude/project.md` 의 "협업 흐름 (Workflow)" 표 및 "메인 오케스트레이터 행동 규칙" 섹션을 따른다.

## 에이전트 세션 관리

### 수정 루프 — 세션 유지 (`SendMessage`)
리뷰어가 BLOCKER 를 내서 같은 에이전트에게 수정을 다시 맡길 때 — design-reviewer → game-designer, code-reviewer → gameplay-programmer — 는 `Agent` 로 **새 호출(새 세션, cold start)하지 않고 `SendMessage` 로 기존 세션에 이어 보낸다.** 방금 자기가 작성한 기획서·코드·의도를 기억한 채로 고쳐야 정확하다.

- 단계 전진(game-designer → gameplay-programmer → code-reviewer → test-engineer)은 서로 다른 에이전트라 새 세션이 불가피하다 — 이때는 산출물(기획서/plan/코드 경로)을 위임 프롬프트에 명시해 컨텍스트를 잇는다.

### 완료 시 — 세션 종료 확인 게이트
마무리(7단계)에서 작업이 끝났다고 판단되면 **에이전트 세션을 바로 종료하지 않는다.** 사용자에게 **"에이전트 세션을 종료할지, 추가 수정을 위해 유지할지"** 를 묻고 멈춘다.

- 사용자가 **"완전히 해결됐다"** 고 확인하기 전까지 세션을 유지한다. 추가 수정 요청이 오면 해당 에이전트 세션에 `SendMessage` 로 이어간다 (새 `Agent` 호출 금지).
- 사용자가 해결을 확인하면 그때 세션을 종료한다.

### 컨텍스트 요약 대비 — 세션 레지스트리 (`.claude/.active-sessions.md`)
대화 컨텍스트가 요약되면 메인이 세션 ID 추적을 잃을 수 있다 (요약은 대화창만 압축하고 파일은 건드리지 않는다). 이를 대비해 **best-effort 복구**용 레지스트리를 둔다.

- 에이전트를 `Agent` 로 spawn 하거나 `SendMessage` 로 이어갈 때마다 `.claude/.active-sessions.md` 에 `에이전트 종류 | 세션 ID(또는 이름) | 상태(대기/수정중/완료)` 를 한 줄로 갱신한다.
- 컨텍스트 요약 직후 첫 행동에서 이 파일을 **먼저 읽어** 세션 맵을 복구한 뒤 `SendMessage` 대상을 정한다.
- 사용자가 "완전히 해결됐다" 고 확인하면 세션을 종료하고 레지스트리를 비운다.
- **한계(best-effort)**: 하네스가 세션을 이미 폐기했으면 ID 가 있어도 `SendMessage` 가 실패한다 — 이때는 산출물(기획서/plan/코드 경로)로 컨텍스트를 재구성해 cold start 한다.
- 이 파일은 런타임 스크래치다 — 커밋하지 않는다 (`git add` 범위에서 제외).

## 흔한 실수

- 승인 게이트(3단계)를 건너뛰고 구현으로 직진 — 금지. 이 버전의 존재 이유가 게이트다. 게이트 없는 흐름은 `/start-develop-auto`.
- 메인이 직접 `.h`/`.cpp` 를 수정 — 금지. gameplay-programmer 에 위임.
- BLOCKER 무한 루프 — 최대 3회 후 사용자에게 에스컬레이션.
- 수정 루프에서 같은 에이전트를 `Agent` 로 새로 호출(cold start) — 금지. `SendMessage` 로 기존 세션 유지.
- 완료라고 판단하고 세션 종료·마무리 직행 — 금지. 사용자에게 종료 여부를 먼저 묻고, 해결 확인 후에만 종료.
- 컨텍스트 요약 후 `.claude/.active-sessions.md` 를 안 읽고 곧장 새 `Agent` 호출 — 금지. 먼저 레지스트리로 세션 맵 복구.
- 끝나고 자동 커밋 — 금지. 커밋 메시지(안)만 제시하고 `git commit` 은 사용자 몫.
