---
name: start-develop-quick
description: 사용자가 이 스킬을 이름으로 명시적으로 호출했거나, 메인 오케스트레이터의 파이프라인 선택 프롬프트에서 사용자가 선택한 경우에만 사용한다. 이 프로젝트의 가장 가벼운 경로 — gameplay-programmer → code-reviewer 만 — 로 작은 버그 수정 / 리네임 / 사소한 편집을 기획·테스트 작업 없이 처리한다. game-designer, design-reviewer, 사용자 승인 게이트, test-engineer, superpowers 0·1 단계를 건너뛴다. 일반적인 기능 요청에서 자동 발동하지 않는다.
---

# start-develop-quick — 간단 수정 전용 파이프라인 (최경량 버전)

## 개요

사용자가 **명시적으로 호출했거나, 메인 오케스트레이터가 제시한 후보 중 사용자가 선택한 경우에만** 발동한다. 이 프로젝트의 협업 흐름 4종 중 가장 가볍다 — `gameplay-programmer` 와 `code-reviewer` 만 돌린다.

오타 · 리네임 · 문구·색상값 변경 · 단일 함수 안의 소규모 버그 수정 같은 **사소한 수정** 전용. 본격 기능 · 머지 대상 · 회귀 위험 있는 변경은 `start-develop` 또는 `start-develop-auto` / `start-develop-simple` 을 쓴다.

메인 오케스트레이터는 **직접 코드를 짜지 않는다** (`.claude/project.md` "메인 오케스트레이터 행동 규칙"). 각 단계를 해당 서브에이전트에 위임한다.

## 호출 시 입력

수정 내용을 인자로 받는다. 인자가 없으면 무엇을 고칠지 사용자에게 1회만 묻는다.

## 사전 단계 — 슈퍼파워 분기

`.claude/project.md` 의 `uses_superpowers` 키 값과 무관하게 **0단계(`superpowers:brainstorming`) · 1단계(`superpowers:writing-plans`) 를 완전 스킵** 한다. `start-develop-simple` 의 "짧게 수행" 과 달리 본 스킬은 아예 호출하지 않는다 — 사소한 수정에 spec/plan 문서를 작성하는 비용이 의도와 모순되기 때문.

긴 합의·정밀 plan 이 필요한 작업이면 본 스킬 대신 `start-develop` 또는 `start-develop-auto` 사용.

## 파이프라인 (순서대로, 단계 간 멈춤 없이)

1. **gameplay-programmer** 위임 → 수정 구현.
   - "정상 케이스 + 엣지 케이스 1개" 수준의 스모크 확인을 gameplay-programmer 가 자체 수행한다 (`.claude/agents/gameplay-programmer.md` 의 self-review 정의).
2. **code-reviewer** 위임 → 코딩 룰(`.claude/rules/` — cpp-style·unreal-infra·git-conventions) 준수 + 사용자 의도와의 부합 검토.
   - BLOCKER 가 있으면 gameplay-programmer 에 수정 위임 후 code-reviewer 재검토. **최대 3회** 반복. 3회 후에도 남으면 사용자에게 보고하고 중단.
   - **에셋 한정 게이트** — 1단계 변경이 순수 에셋 등록/적용(코드 변경 0건)이면 code-reviewer spawn 전에 사용자에게 "리뷰 진행 / 생략하고 커밋 메시지" 선택지를 먼저 제시한다 (`.claude/project.md` "에셋 한정 사이클 — 리뷰 생략 선택지 게이트" 참조). 코드 변경이 1건이라도 있으면 게이트 없이 직진.
3. **마무리** — 변경사항 요약 + 한글 커밋 메시지(안) 제시. `git-conventions.md` 포맷(`[Tag] ClassName — 요약`) 준수 — `git commit` 직접 실행 금지, 관련 변경 파일 `git add` 까지만.

## 사후 안전망 — 에스컬레이션 출구

gameplay-programmer 가 작업에 들어간 뒤 **"이 수정은 quick 수준이 아니다 — game-designer / 기획서가 필요하다"** 라고 판단하면 즉시 사용자에게 보고하고 멈춘다. 사용자는 다음 중 선택:

- **(a)** `start-develop` (또는 `-auto` / `-simple`) 으로 흐름 재시작
- **(b)** 위험 인지하에 본 흐름 그대로 계속 진행

이 출구가 "메인이 임의 판단으로 큰 작업을 quick 으로 직진" 을 막는 마지막 안전망. 메인 자체 분기를 도입하지 않은 이유이기도 하다.

## 규칙

- 코딩 룰(`.claude/rules/` — cpp-style·unreal-infra·git-conventions)은 그대로 적용된다. 단계가 빠질 뿐 룰이 사라진 게 아니다.
- `test-engineer` 를 스킵한다. 회귀 위험은 gameplay-programmer 의 자체 스모크 확인에 의존하며, 본격 회귀 테스트가 필요한 작업이면 본 스킬 대신 `start-develop-simple` 이상을 사용한다.
- 최종 커밋 — **절대 자동 커밋하지 않는다.** `git add` + 커밋 메시지(안) 까지만.

## 사용 시점 가이드

| 상황 | 사용 스킬 |
|---|---|
| 본격 기능, 사람 검토 게이트 필요 | `start-develop` |
| 본격 기능, 검토는 자동 리뷰어로 대체 | `start-develop-auto` |
| 버릴 수도 있는 프로토타입, 가장 빠르게 | `start-develop-simple` |
| **사소한 수정 · 작은 버그 수정 · 리네임 · 문구 변경** | **`start-develop-quick` (이 스킬)** |

## 에이전트 세션 관리

### 수정 루프 — 세션 유지 (`SendMessage`)
code-reviewer 가 BLOCKER 를 내서 gameplay-programmer 에게 수정을 다시 맡길 때는 `Agent` 로 **새 호출(새 세션, cold start)하지 않고 `SendMessage` 로 기존 gameplay-programmer 세션에 이어 보낸다.** 방금 자기가 작성한 코드·의도를 기억한 채로 고쳐야 정확하다.

### 완료 시 — 세션 종료 확인 게이트
마무리(3단계)에서 작업이 끝났다고 판단되면 **에이전트 세션을 바로 종료하지 않는다.** 사용자에게 **"에이전트 세션을 종료할지, 추가 수정을 위해 유지할지"** 를 묻고 멈춘다.

- 사용자가 **"완전히 해결됐다"** 고 확인하기 전까지 세션을 유지한다. 추가 수정 요청이 오면 gameplay-programmer 세션에 `SendMessage` 로 이어간다 (새 `Agent` 호출 금지).
- 사용자가 해결을 확인하면 그때 세션을 종료한다.

### 컨텍스트 요약 대비 — 세션 레지스트리 (`.claude/.active-sessions.md`)
대화 컨텍스트가 요약되면 메인이 세션 ID 추적을 잃을 수 있다 (요약은 대화창만 압축하고 파일은 건드리지 않는다). 이를 대비해 **best-effort 복구**용 레지스트리를 둔다.

- 에이전트를 `Agent` 로 spawn 하거나 `SendMessage` 로 이어갈 때마다 `.claude/.active-sessions.md` 에 `에이전트 종류 | 세션 ID(또는 이름) | 상태(대기/수정중/완료)` 를 한 줄로 갱신한다.
- 컨텍스트 요약 직후 첫 행동에서 이 파일을 **먼저 읽어** 세션 맵을 복구한 뒤 `SendMessage` 대상을 정한다.
- 사용자가 "완전히 해결됐다" 고 확인하면 세션을 종료하고 레지스트리를 비운다.
- **한계(best-effort)**: 하네스가 세션을 이미 폐기했으면 ID 가 있어도 `SendMessage` 가 실패한다 — 이때는 산출물(코드 경로·사용자 의도)로 컨텍스트를 재구성해 cold start 한다.
- 이 파일은 런타임 스크래치다 — 커밋하지 않는다 (`git add` 범위에서 제외).

## 흔한 실수

- 큰 작업을 quick 으로 진행 — gameplay-programmer 가 발각하면 에스컬레이션으로 빠져나옴. 사용자 측에서도 quick 후보 선택 전 의심되면 다른 스킬로.
- 메인이 직접 `.h`/`.cpp` 수정 — 금지. gameplay-programmer 에 위임.
- "사소하니까" 라며 코딩 룰(`.claude/rules/`)을 무시 — 금지. code-reviewer 가 BLOCKER 로 차단.
- test-engineer 도 스킵이라며 정상 케이스 확인까지 빼먹기 — gameplay-programmer 자체 스모크는 반드시 수행.
- 수정 루프에서 gameplay-programmer 를 `Agent` 로 새로 호출(cold start) — 금지. `SendMessage` 로 기존 세션 유지.
- 완료라고 판단하고 세션 종료·마무리 직행 — 금지. 사용자에게 종료 여부를 먼저 묻고, 해결 확인 후에만 종료.
- 컨텍스트 요약 후 `.claude/.active-sessions.md` 를 안 읽고 곧장 새 `Agent` 호출 — 금지. 먼저 레지스트리로 세션 맵 복구.
- 끝나고 자동 커밋 — 금지. 커밋 메시지(안)만 제시하고 `git commit` 은 사용자 몫.
