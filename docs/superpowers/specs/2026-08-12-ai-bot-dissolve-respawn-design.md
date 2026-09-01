# AI 봇 사망 디졸브 연출 + 재활용 리스폰 — 설계

## 1. 목표 / 범위

AI 봇(`SpySpawnBotManagerComponent` 가 관리하는 봇)이 사망하면:

1. 즉시 디졸브(투명화) 연출을 시작해 `DissolveDurationSeconds`(기본 2초) 동안 서서히 사라진다.
2. 연출이 끝나면 **자기 원래 스폰 지점**으로 재배치되어, **같은 Character/AIController 인스턴스를 재활용**해 다시 등장한다 (Destroy 후 재스폰 없음).

**대상 범위**: `SpySpawnBotManagerComponent` 가 스폰한 AI 봇만. 플레이어 캐릭터는 대상 아님.

**함께 고치는 선행 버그** (사망 처리 흐름에 직접 영향을 주므로 이번 작업 범위에 포함):

- `Plugins/SKGAS/Attribute/SKAttributeSet.cpp::PostGameplayEffectExecute` — Health 가 클램프 없이 음수로 내려가는 버그. `[0, MaxHealth]` 범위로 클램프한다.
- `Character/SpyHealthComponent.cpp` — `bDead` 래치가 없어 사망 후 추가 피해가 들어오면 `OnDeath` 델리게이트가 중복 발화될 수 있는 버그. 래치를 추가해 사망당 1회만 발화되게 하고, 리스폰 완료 시 래치를 해제한다.

## 2. 디졸브 연출 — GameplayCue 표준 경로

- `SpyGA_Death::ActivateAbility` 의 서버(Authority) 블록에서 `GameplayCue`(예: `Cue.Character.Death`)를 실행한다 — SKGAS 표준 Cue 경로(`plugin-skgas.md` §5)를 따르며, 서버→클라이언트 리플리케이션은 GAS 가 보장한다.
- 신규 `UGameplayCueNotify_Actor`(또는 `_Static`)를 작성해, 캐릭터 메시에 Dynamic Material Instance 를 생성하고 `Dissolve` 스칼라 파라미터를 `DissolveDurationSeconds` 동안 0→1 로 보간한다.
- 캐릭터 머티리얼에는 아직 `Dissolve` 파라미터가 없으므로, 구현 단계에서 unreal-mcp 로 기존 캐릭터 머티리얼(들)에 파라미터를 추가한다. (이 작업은 UMG 화면 배치가 아니므로 `.claude/project.md` 의 "UI 작업 — 목업 승인 게이트" 대상이 아니다.)
- Cue 는 순수 연출이며 게임플레이 상태(리스폰 타이밍)와는 독립적으로 동작한다 — 정밀 동기화가 필요한 로직이 아니다.

## 3. Config

- `SpyAIConfig` (Config `UDataAsset`) 에 `float DissolveDurationSeconds`(기본값 `2.0f`, `EditDefaultsOnly`) 를 추가한다 — 매직넘버 금지 규칙(`cpp-style.md` §15) 준수.

## 4. 재활용 리스폰 오케스트레이션

- `SpySpawnBotManagerComponent` 의 봇 트래킹 구조를 확장한다. 현재는 컨트롤러만 배열(`SpawnedBotList`)로 추적하는데, 여기에 **봇이 원래 배정받은 스폰 지점 트랜스폼**을 함께 기억하도록 (컨트롤러 ↔ 스폰 트랜스폼) 1:1 매핑을 추가한다.
- 봇 스폰 시점에 해당 봇의 `SpyHealthComponent::OnDeath` 델리게이트를 `SpySpawnBotManagerComponent` 가 구독한다. 사망 신호를 받으면 서버 전용 타이머를 `DissolveDurationSeconds` 후로 예약한다.
- 타이머 만료 시 `RespawnBot(BotInfo)` 를 호출해 다음을 수행한다:
  1. 원래 스폰 트랜스폼으로 텔레포트(`SetActorLocationAndRotation`).
  2. Health 어트리뷰트를 MaxHealth 로 복구.
  3. `Character.State.Death` 게임플레이 태그 제거(부여되어 있다면).
  4. 캡슐 콜리전을 사망 시 `ECR_Ignore` 로 바뀌었던 것을 원복, 무브먼트 재활성화.
  5. `AIController->GetBrainComponent()->RestartLogic()` 호출 + Perception/Blackboard 타겟 갱신.
  6. 메시 가시성 복구 + `Dissolve` 파라미터를 0 으로 리셋.
  7. `bDead` 래치 해제(§1 버그 픽스와 연동).
- **책임 분리**: 디졸브 트리거(`GA_Death` → Cue)와 리스폰 스케줄링(`SpySpawnBotManagerComponent`)은 같은 사망 신호(`OnDeath` 델리게이트)를 각자 독립적으로 구독한다. 두 시스템은 서로를 직접 참조하지 않는다 (`cpp-style.md` §8, 이벤트 기반 분리).

## 5. 에러 처리 / 엣지 케이스

- 리스폰 타이머 대기 중 봇 액터가 레벨에서 제거되면(`TWeakObjectPtr` 유효성 체크) 타이머 콜백은 안전하게 아무 것도 하지 않고 반환한다.
- `bDead` 래치로 디졸브 진행 중 추가 피해가 들어와도 `OnDeath` 재발화 및 중복 리스폰 타이머 예약을 막는다.
- 여러 봇이 동시에 사망해도 봇별로 독립된 타이머이므로 서로 간섭하지 않는다.
- Late-join 클라이언트가 디졸브 애니메이션 진행 중간에 합류하면 중간 상태를 정확히 재현하지 못할 수 있다 — 연출 오차로 허용한다(게임플레이 상태 자체는 서버 권한으로 항상 정확).

## 6. 테스트 관점 (test-engineer 단계에서 구체화)

- Health 가 0 미만으로 내려가지 않는지 (클램프 검증).
- 동일 봇에 사망 직후 추가 피해가 들어와도 `OnDeath` 가 1회만 발화되는지 (`bDead` 래치 검증).
- `DissolveDurationSeconds` 경과 후 정확히 `RespawnBot` 이 호출되는지 (타이머 검증).
- `RespawnBot` 이후 Health == MaxHealth, 콜리전 복구, AI 로직 재개(`RestartLogic` 호출) 여부.
- 봇이 항상 **자기 원래 스폰 지점**으로 복귀하는지 (스폰 지점 매핑 검증).
