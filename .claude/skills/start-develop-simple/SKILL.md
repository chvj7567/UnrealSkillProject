---
name: start-develop-simple
description: 사용자가 이 스킬을 이름으로 명시적으로 호출했을 때만 사용한다. 이 프로젝트의 기능 개발 파이프라인을 프로토타입용 간소 경로 — game-designer → gameplay-programmer → test-engineer, design-reviewer 와 code-reviewer 생략 — 로 실행한다. 리뷰 게이트를 속도와 맞바꾼다; 버릴 프로토타입 전용. 일반적인 기능 요청에서 자동 발동하지 않는다 — 명시적 호출 필수.
---

# start-develop-simple — 기획→구현→테스트 파이프라인 (프로토타입 간소 버전)

## 개요

사용자가 **명시적으로 호출했을 때만**, 이 프로젝트의 표준 협업 흐름(`.claude/project.md` "협업 흐름 (Workflow)" 표)에서 **리뷰어 2명(design-reviewer / code-reviewer) 을 모두 생략**하고 game-designer → gameplay-programmer → test-engineer 만으로 빠르게 돌린다. **프로토타입을 짧게 짜볼 때 시간 절약용**으로만 쓴다.

메인 오케스트레이터는 **직접 코드를 짜지 않는다** (`.claude/project.md` "메인 오케스트레이터 행동 규칙"). 각 단계를 해당 서브에이전트에 위임한다.

> 주의: 이 버전은 리뷰를 모두 건너뛴다. 본격 기능·머지 대상 코드는 `start-develop`(승인 게이트) 또는 `start-develop-auto`(전자동, 리뷰 포함)를 쓴다.

## 호출 시 입력

기능 설명을 인자로 받는다. 인자가 없으면 무엇을 만들지 사용자에게 먼저 묻는다 (이 한 번만 멈춘다).

## 사전 단계 — 슈퍼파워 분기 (프로토타입 정신 유지)

`.claude/project.md` 의 `uses_superpowers` 키를 확인해 시작 지점을 결정한다. **본 스킬은 "프로토타입을 가장 빠르게" 가 목적**이므로 슈퍼파워 단계를 돌리더라도 최소한으로.

- **uses_superpowers: true** — 아래 2단계를 **짧게** 수행 (긴 합의·세분화는 본 스킬과 모순):
  - **0. `superpowers:brainstorming`** → 1~2턴으로 의도·범위 합의만 끝내고 `docs/superpowers/specs/YYYY-MM-DD-[기능명]-design.md` 작성. 결정 락은 최소화 (프로토타입이라 바뀔 수 있음).
  - **1. `superpowers:writing-plans`** → 골격 plan 만 작성 (`docs/superpowers/plans/YYYY-MM-DD-[기능명].md`). TDD 5단계 강제 안 함, verification gate 도 가벼움.
  - **1 완료 후 — 실행 방식 선택 게이트**: writing-plans 산출물을 사용자에게 제시하고 다음 두 갈래를 선택하게 한다. 선택 전까지 진행하지 않는다.
    - **A. 슈퍼파워 실행** — `superpowers:subagent-driven-development` (태스크별 서브에이전트) 또는 `superpowers:executing-plans` (현 세션 일괄 실행) 으로 플랜을 직접 구현. game-designer 이후 파이프라인은 건너뜀.
    - **B. start-develop-simple 파이프라인 계속** — 아래 game-designer 단계부터 이어서 진행. spec + plan 경로를 game-designer에 함께 전달.
  - 긴 합의·정밀 plan 이 필요한 본격 기능이면 `start-develop` 또는 `start-develop-auto` 사용.
- **uses_superpowers: false** — 0·1 단계 생략. 메인이 사용자와 의도만 짧게 합의 후 아래 파이프라인 1번부터 시작.

## 파이프라인 (순서대로, 단계 간 멈춤 없이)

1. **game-designer** 위임 → `docs/design/[기능명].md` 기획서 작성.
   - 프로토타입 범위로 작성해도 됨을 위임 프롬프트에 명시한다 (수치는 임시값, 시너지 컬럼 생략 등 허용).
2. **gameplay-programmer** 위임 → 기획서대로 구현. (design-reviewer 생략)
3. **test-engineer** 위임 → 본격 테스트 스위트(Unreal Automation) 작성. (code-reviewer 생략)
4. **마무리** — 변경사항 요약 + 커밋 메시지(안) 제시. `git-conventions.md` 포맷(`[Tag] ClassName — 요약`) 준수 — `git commit` 직접 실행 금지, 관련 파일 `git add` 까지만.

## 규칙

- 리뷰어 생략은 **이 스킬 한정**이다. 다른 스킬/흐름의 단계를 임의로 빼지 않는다.
- 코딩 룰(`.claude/rules/` — cpp-style·unreal-infra·git-conventions)은 그대로 적용된다. 단계가 빠질 뿐 룰이 사라진 게 아니다.
- gameplay-programmer 가 자체적으로 "정상 케이스 + 엣지 케이스 1개" 수준의 스모크 확인을 수행해야 한다 (`.claude/agents/gameplay-programmer.md` 의 self-review 정의).
- 다음 경우엔 멈춘다:
  - 호출 시 기능 설명이 없을 때 — 무엇을 만들지 묻는다.
  - gameplay-programmer / test-engineer 가 자체 보고한 **컴파일 실패·테스트 실패**가 풀리지 않을 때 — 사용자에게 에스컬레이션.
  - 최종 커밋 — **절대 자동 커밋하지 않는다.** `git add` + 커밋 메시지(안) 까지만.

## 사용 시점 가이드

| 상황 | 사용 스킬 |
|---|---|
| 본격 기능, 사람 검토 필요 | `start-develop` |
| 본격 기능, 검토는 자동 리뷰어로 대체 | `start-develop-auto` |
| **버릴 수도 있는 프로토타입, 가장 빠르게** | **`start-develop-simple` (이 스킬)** |

## 에이전트 세션 관리

### 수정 루프 — 세션 유지 (`SendMessage`)
이 스킬은 리뷰어가 없지만, gameplay-programmer / test-engineer 가 자체 보고한 컴파일·테스트 실패를 같은 에이전트에게 다시 고치게 할 때는 `Agent` 로 **새 호출(새 세션, cold start)하지 않고 `SendMessage` 로 기존 세션에 이어 보낸다.** 방금 자기가 작성한 코드·의도를 기억한 채로 고쳐야 정확하다.

- 단계 전진(game-designer → gameplay-programmer → test-engineer)은 서로 다른 에이전트라 새 세션이 불가피하다 — 이때는 산출물(기획서/plan/코드 경로)을 위임 프롬프트에 명시해 컨텍스트를 잇는다.

### 완료 시 — 세션 종료 확인 게이트
프로토타입이라도 마무리(4단계)에서 작업이 끝났다고 판단되면 **에이전트 세션을 바로 종료하지 않는다.** 사용자에게 **"에이전트 세션을 종료할지, 추가 수정을 위해 유지할지"** 를 묻고 멈춘다.

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

- 프로토타입이라며 코딩 룰(`.claude/rules/`)을 무시 — 금지. 룰은 그대로다.
- 메인이 직접 `.h`/`.cpp` 를 수정 — 금지. gameplay-programmer 에 위임.
- 프로토타입 코드를 그대로 머지 — 권장하지 않음. 머지 전엔 `start-develop` 또는 `start-develop-auto` 로 리뷰를 한 번 받는다.
- 수정 루프에서 같은 에이전트를 `Agent` 로 새로 호출(cold start) — 금지. `SendMessage` 로 기존 세션 유지.
- 완료라고 판단하고 세션 종료·마무리 직행 — 금지. 사용자에게 종료 여부를 먼저 묻고, 해결 확인 후에만 종료.
- 컨텍스트 요약 후 `.claude/.active-sessions.md` 를 안 읽고 곧장 새 `Agent` 호출 — 금지. 먼저 레지스트리로 세션 맵 복구.
- 끝나고 자동 커밋 — 금지. 커밋 메시지(안)만 제시하고 `git commit` 은 사용자 몫.
