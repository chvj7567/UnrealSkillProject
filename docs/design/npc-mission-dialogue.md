# NPC 대화 기반 미션 수락+보고 시스템 — 도메인 기획 (컨셉·문구·배치·데이터값)

> **개정: 2026-08-01c 전면 재작성** — spec이 "보고"를 미션의 서브상태에서 **미션 배열 자체의 독립 항목**으로 바꾸고, 데이터를 `Mission`/`MissionReward`/`MissionCommunication`/`NPC`/`Dialogue` 5테이블로 정규화했다(`docs/superpowers/specs/2026-08-01-npc-mission-dialogue-design.md`, 개정 2026-08-01c). **이전 판(2026-08-01b — `ReadyToReport` 5상태, 대사 5줄/NPC, `OwnedMissionIndex` 단일 필드 모델)은 전부 폐기한다.** 이 판은 그 위에서 값만 다시 채운 결과다.
>
> **⚠ plan 미인용 안내**: `docs/superpowers/plans/2026-08-01-npc-mission-dialogue.md`는 아직 2026-08-01b(구 모델) 상태로 남아 있고 spec c판과 모순된다(`ReadyToReport`/`bObjectiveMet`/`ReportCurrentMission()`/5줄 로우 등). plan 재작성은 이 기획서의 범위 밖이므로, **이 문서는 plan을 인용하지 않고 spec § 번호만 인용한다.**
>
> **이 문서의 범위**: spec이 확정한 구조·클래스·판정 함수·서버 흐름 위에서, **비어 있는 도메인 결정**(NPC 컨셉·배치·대사 24줄·`DialogueId`/`MissionCommunication`/`MissionReward` 실제 값·카드 문구·프롬프트 문구)만 채운다. 구조·시그니처는 다시 제안하지 않는다.
> **선행 시스템**: [미션 시스템 기획서](mission-system.md) — 미션 6종의 `DisplayName`/`MatchTag`/`Mode`/`TargetCount`/`ExperienceReward`는 그 문서 §3-1에서 이미 확정됐고, 이 문서는 그 값을 **입력으로만** 쓴다(spec §4-9가 이미 정합성을 확인했다 — 이 문서에서 재검산하지 않는다).
>
> **(2026-08-03 후속 결정 — 1-based 전환)** 이 문서(2026-08-01c 작성)는 원래 `MissionId`/`NPCId`를 0-based(0~11 / 0~5)로 서술했다. 사용자가 이후 별도로 "ID는 1부터"를 지시했고 코드·라이브 데이터가 이미 1-based(`MissionId` 1~12, `NPCId` 1~6)로 전환돼, 아래 §1 핵심 메커니즘·§2 NPC 표를 포함한 문서 전체의 숫자를 1-based로 갱신했다(§3 헤더의 상세 노트도 참조). 같은 후속 결정에 `Mission` 데이터의 저장 방식 전환도 포함된다 — `Missions[]`(`UDataAsset`의 `TArray<FSpyMissionEntry>`)에서 `MissionTable`(`UDataTable*`, row struct `FSpyMissionRow`)로 바뀌었다(`docs/superpowers/specs/2026-08-03-mission-datatable-npcid-design.md` 참조) — §3-6·§6-3·§6-5의 표기도 그에 맞춰 갱신했다.

---

- **목표** — NPC 6명(미션 6쌍에 1:1 대응)의 컨셉·배치 원칙과, 5테이블(`Mission`/`MissionReward`/`MissionCommunication`/`NPC`/`Dialogue`)에 채울 실제 값(대사 24줄, `DialogueId` 채번, 미션 12행, 보상 6행, 미션-NPC 연결 12행) 그리고 대화·미션 카드(Offer 1종)·상호작용 프롬프트 텍스트를 확정한다.
- **검증 가설** — "NPC와 대화해야 미션이 시작되고, 목표를 달성한 뒤 그 NPC에게 다시 말을 걸면(추가 확인 없이) 그 자리에서 보상이 지급되며, 이어서 다음 NPC에게 이동해 새로 수락해야 다음 미션이 시작된다"는 흐름이 기존 순차 미션 체인의 완주 가능성(mission-system.md §4-2의 1인·2인 보장 기준)을 깨지 않으면서, 플레이어가 다음 NPC를 자연스럽게 찾아가는 동선이 성립하는가.
- **현재 단계 범위 적합성** — `project.md`의 `stage`/`stage_goal`은 (사용자 확정 대기)이고 컨셉서 §2·§4·§5도 미확정이다. mission-system.md와 동일한 원칙을 따라, **사용자 검토 대기 중인 spec(`2026-08-01-npc-mission-dialogue-design.md`, 개정 2026-08-01c)의 목표·비목표를 이 기획서의 범위 경계로 삼는다.** spec 비목표(대화 분기, NPC AI 행동, 대화 로그·보이스, 수락 이력 영속화, 거절 페널티, Threshold 모드 일반화, 대화 다회차 연출)는 이 문서에서도 다루지 않는다.
- **핵심 메커니즘** — `Mission` 배열 12행은 홀수 `MissionId`(1,3,5,7,9,11, `Gameplay` 타입 — 기존 6종)와 짝수 `MissionId`(2,4,6,8,10,12, `Dialogue` 타입 — "OO에게 보고하십시오")가 번갈아 온다. `Gameplay` 미션은 담당 NPC의 Offer 카드로 **수동 수락**해야 진행 이벤트가 카운트되고, 그 목표를 채우면 **보상 없이** 인덱스가 곧바로 다음(같은 NPC의 `Dialogue` 미션)으로 넘어가며 **자동 수락**된다. 플레이어가 **같은 NPC**에게 다시 말을 걸면(추가 카드 없이) 그 대화 안에서 서버가 진행 신호를 처리해 완료시키고, 그제서야 보상이 지급되며 인덱스가 다음 NPC의 `Gameplay` 미션(Offer)으로 넘어간다.

---

## §1 이 기획서가 입력으로 쓰는 확정값 (spec, 재확정 안 함)

| 항목 | 값 | 출처 |
|---|---|---|
| `Mission`/`MissionReward`/`MissionCommunication`/`NPC`/`Dialogue` 5테이블 스키마 전체 (필드명·타입·복합 키) | 구조 그대로 사용, 값만 이 문서가 채움 | spec §4 |
| `ESpyMissionType`(`Gameplay`/`Dialogue`) 기반 수락 방식 분기, `Mission.ExperienceReward` 삭제(→ `MissionReward`로 이동) | 재확정 안 함 | spec §4-2·§4-3 |
| `ESpyNPCDialogueState` **4상태**(`Default`/`Offer`/`InProgress`/`Report`) 판정 함수 `ResolveNPCDialogueState` | 재확정 안 함 | spec §6 |
| `USpyMissionComponent`의 `bAccepted` 단일 게이트 + `MissionType` 기반 자동 수락, `ProcessProgress`가 완료 즉시 보상+전진(2026-08-01 이전 동작으로 복귀) | 재확정 안 함 | spec §5 |
| 서버 흐름 — `RequestInteract` 한 호출 안에서 `Report` 상태 판정과 `AddProgress(Event_Mission_Report, 1)`을 함께 처리, 카드는 `Offer` 1종뿐(보고 확인 카드 없음) | 재확정 안 함 | spec §7 |
| 레벨(Threshold) 미션 수락 시점 스냅샷 재평가 — `AcceptCurrentMission()` 안에서 즉시 현재 레벨 재주입 | 재확정 안 함 | spec §5-5 |
| 미션 6쌍의 `DisplayName`/`MatchTag`/`Mode`/`TargetCount`/`ExperienceReward` 원본값(Gameplay 6종) | 값 그대로 입력으로만 사용 | mission-system.md §3-1 (확정) |

---

## §2 NPC 6종 컨셉

전원 같은 첩보 조직 소속으로, 신입 요원(플레이어)을 단계별로 검증·훈련시키는 **핸들러 체인**이라는 한 줄 설정으로 묶는다. `MissionCommunication` 관계 테이블에서 `Role = Offer` 행과 `Role = Report` 행이 **같은 `NPCId`**를 가리킨다는 사실(spec §4-8)이 곧 "수락도 보고도 같은 담당자에게 한다"는 뜻이다 — 별도의 "보고 전용 NPC"는 없다.

| `NPCId` | 담당 `MissionId`(Offer / Report) | 미션 `DisplayName` | NPC 역할·코드네임 | 담당 이유 |
|---|---|---|---|---|
| 1 | 1 / 2 | 적 1명 처치 | **정보원 "레이븐"** | 첫 실전 투입을 브리핑하는 창구. 조직의 첫 접점이라 가장 격식 있고 사무적인 톤 |
| 2 | 3 / 4 | 레벨 3 달성 | **교관 "팰컨"** | 전투 데이터를 평가해 숙련도를 판정하는 역할 — 레벨(숙련도) 미션과 결합 |
| 3 | 5 / 6 | 콤보 4회 연결 | **행동대장 "바이퍼"** | 근접 연계 전투를 직접 지도하는 현장 지휘관 — 콤보 훈련과 결합 |
| 4 | 7 / 8 | 장애물 넘기 5회 | **루트 플래너 "스패로우"** | 침투 동선을 설계하는 역할 — 장애물 통과(Vault)와 결합 |
| 5 | 9 / 10 | 벽 타기 3회 | **클라이밍 교관 "울프"** | 은밀 진입로(등반) 전문 — 벽타기(Climb)와 결합 |
| 6 | 11 / 12 | 그래플링 3회 | **기술관 "폭스"** | 장비(그래플 훅) 지급·운용을 담당하는 병참/기술 지원 — 그래플과 결합, 체인의 마지막을 "현장 투입 승인"으로 마무리 |

톤 일관성: 6명 모두 **감정을 드러내지 않는 프로 조직원** 톤(짧은 문장, 하게체·해라체 지시형). 서로 다른 것은 역할에서 오는 어휘뿐 — 레이븐(사무적)·팰컨(평가형)·바이퍼(거친 지시형)·스패로우(전술 브리핑형)·울프(엄격한 코치형)·폭스(차분한 기술자형).

---

## §3 데이터 값 — 5테이블 실제 값

**(2026-08-03 후속 결정 — 1-based 전환) 이 문서(2026-08-01c 작성)는 `MissionId`/`NPCId`를 0-based로 서술했으나, 사용자가 이후 별도로 "ID는 1부터"를 지시했고 코드(`SpyMissionComponent.h`/`SpyMissionConfig.cpp`)·라이브 데이터가 이미 1-based(`MissionId` 1~12, `NPCId` 1~6)로 전환됐다. 아래 §2~§6의 `MissionId`/`NPCId` 숫자는 전부 1-based로 갱신했다. `DialogueId`는 이 결정과 무관한 별도 ID 공간이라 값(0~53)은 그대로 두고, §3-1 채번 공식만 새 `NPCId`에 맞춰 재기술했다.**

### 3-1. `DialogueId` 채번 규칙 (이 문서가 확정)

`Dialogue` 테이블은 `DialogueId`+`DialogueIndex` 복합 키다. 이번 범위(spec §1 비목표)는 전부 **`DialogueIndex = 0` 한 줄**만 쓰므로, `DialogueId` 하나가 대사 한 줄과 1:1이다. 채번 규칙(1-based `NPCId` 기준으로 재기술 — 기존 `DialogueId` 값 자체는 변경 없음):

```
DialogueId = (NPCId - 1) * 10 + StateOffset
StateOffset: Default = 0, Offer = 1, InProgress = 2, Report = 3
```

예: 레이븐(`NPCId = 1`) → Default 0 / Offer 1 / InProgress 2 / Report 3. 폭스(`NPCId = 6`) → 50/51/52/53. 이 규칙 하나로 24개 `DialogueId`가 전부 결정되므로 §3-3 표에서 값을 다시 나열하되 규칙에서 벗어나는 예외는 없다.

### 3-2. `NPC` 테이블 (`DT_SpyNPC`, 6행)

| `NPCId` | `NPCDisplayName` | `DefaultDialogueId` |
|---|---|---|
| 1 | 정보원 "레이븐" | 0 |
| 2 | 교관 "팰컨" | 10 |
| 3 | 행동대장 "바이퍼" | 20 |
| 4 | 루트 플래너 "스패로우" | 30 |
| 5 | 클라이밍 교관 "울프" | 40 |
| 6 | 기술관 "폭스" | 50 |

**저작 경로**: `NPCDisplayName` 값에 큰따옴표가 포함돼(`"레이븐"` 등) CSV import 시 이스케이프 문제가 생길 수 있다 — 이 6개 로우는 엔진 기본 DataTable 에디터에서 직접 입력한다.

### 3-3. `Dialogue` 테이블 (`DT_SpyDialogue`, 24행)

전 행 `DialogueIndex = 0`. 4상태는 spec §6이 확정한 순서(`Default`/`Offer`/`InProgress`/`Report`) 그대로 나열한다.

**도달 가능성 확인**: 이전 판(5상태)과 달리 이번 4상태는 **6 NPC 전원에서 전부 도달 가능**한 살아있는 데이터다 — 죽은 로우가 없다. 다만 `Default`가 도달하는 시점의 의미가 NPC마다 다르다: 레이븐은 자신의 `Report`(MissionId 2)를 이미 끝낸 **뒤**(`CurrentMissionId ≥ 3`)에만 `Default`가 나오므로 순수 종료형 문구로 써도 안전하다. 팰컨~울프는 자기 차례가 **오기 전**(`CurrentMissionId <` 자신의 Offer)과 **끝난 뒤**(`CurrentMissionId >` 자신의 Report) 양쪽 모두에서 `Default`가 나오므로, 어느 쪽으로 읽어도 어색하지 않은 **시제 중립 문구**로 썼다(예: "지금은 ~ 없다"). 폭스는 자기 차례가 오기 전과 **전체 완료 시점**(`CurrentMissionId = 13`) 양쪽에서 나오므로 마찬가지로 중립 문구를 썼다. 체인의 진행 안내("다음은 OO")는 전부 `Report`(정확히 한 번만 발화하는 지점)로 몰아 뒀다 — §3-3-7 참고.

#### `NPCId = 1` — 정보원 "레이븐"

| State | `DialogueId` | Text |
|---|---|---|
| Default | 0 | 볼일은 끝났다. |
| Offer | 1 | 침투 구역에 감시 요원이 배치됐다. 하나만 처리하고 다시 찾아와. |
| InProgress | 2 | 아직인가. 조용히, 신속하게 끝내. |
| Report | 3 | 수고했다. 팰컨이 기다리고 있다. |

#### `NPCId = 2` — 교관 "팰컨"

| State | `DialogueId` | Text |
|---|---|---|
| Default | 10 | 지금은 볼일 없다. |
| Offer | 11 | 네 전투 데이터를 봤다. 숙련도를 3단계까지 끌어올려야 다음 단계로 넘어간다. |
| InProgress | 12 | 성장이 보인다. 계속 싸워서 실력을 증명해. |
| Report | 13 | 합격이다. 바이퍼에게 가서 연계 훈련을 받아라. |

#### `NPCId = 3` — 행동대장 "바이퍼"

| State | `DialogueId` | Text |
|---|---|---|
| Default | 20 | 지금은 볼일 없다. 비켜. |
| Offer | 21 | 한 방으론 안 죽는다. 연계 타격 4회, 끊기지 말고 이어붙여. |
| InProgress | 22 | 리듬이 끊기잖아. 다시. |
| Report | 23 | 제법이군. 스패로우가 침투 루트를 알려줄 거다. |

#### `NPCId = 4` — 루트 플래너 "스패로우"

| State | `DialogueId` | Text |
|---|---|---|
| Default | 30 | 지금은 안내할 경로가 없다. |
| Offer | 31 | 정문은 막혔다. 우회로를 확보하려면 장애물을 다섯 번은 넘어야 한다. |
| InProgress | 32 | 동작이 커. 낮고 빠르게 넘어. |
| Report | 33 | 루트 확보 완료. 울프한테 벽 타는 법을 배워둬. |

#### `NPCId = 5` — 클라이밍 교관 "울프"

| State | `DialogueId` | Text |
|---|---|---|
| Default | 40 | 지금은 가르칠 게 없다. |
| Offer | 41 | 정면 돌파는 못 해. 벽을 세 번은 타야 감이 잡힌다. |
| InProgress | 42 | 손이 흔들리는데. 힘 빼고 다시. |
| Report | 43 | 됐다. 마지막은 폭스다, 장비 챙기러 가. |

#### `NPCId = 6` — 기술관 "폭스"

| State | `DialogueId` | Text |
|---|---|---|
| Default | 50 | 지금은 내줄 장비가 없다. |
| Offer | 51 | 이 훅건으로 고지를 잡아라. 앵커 세 개는 걸어봐야 손에 익는다. |
| InProgress | 52 | 조준을 화면 중앙에 맞춰. 서두르지 말고. |
| Report | 53 | 훈련 끝. 넌 이제 현장에 나갈 준비가 됐다. |

**`Report` 문구의 역할(§3-3-7)**: `Report` 상태는 서버가 `RequestInteract` 한 호출 안에서 판정과 완료 처리를 동시에 끝내는 유일한 지점이라(spec §7), "다음은 어디로 가야 하는가"를 정확히 한 번만 말해도 되는 유일한 슬롯이다. 레이븐→팰컨→바이퍼→스패로우→울프→폭스 순으로 다음 담당자를 명시했고, **폭스의 `Report`만 체인 종결 문구**(현장 투입 승인)로 다음 NPC를 언급하지 않는다 — `MissionId 12` 완료가 곧 전체 완료(`MissionIndex`가 마지막 미션(12)을 넘어 13이 됨, spec §9)이기 때문이다.

### 3-4. `MissionCommunication` 테이블 (`DT_SpyMissionCommunication`, 12행)

`Role`이 태그드 유니온 판별자이므로(spec §4-6), 미사용 필드는 구조체 기본값(`0`)으로 둔다 — 아래 표의 "—"는 "이 값은 코드가 절대 읽지 않는다"는 표시이지 실제로 셀을 비운다는 뜻이 아니다(No-Placeholder 예외, spec §4-6이 이미 이 설계를 확정).

| `MissionId` | `NPCId` | `Role` | `OfferDialogueId` | `InProgressDialogueId` | `ReportDialogueId` |
|---|---|---|---|---|---|
| 1 | 1 | Offer | 1 | 2 | — (미사용) |
| 2 | 1 | Report | — (미사용) | — (미사용) | 3 |
| 3 | 2 | Offer | 11 | 12 | — (미사용) |
| 4 | 2 | Report | — (미사용) | — (미사용) | 13 |
| 5 | 3 | Offer | 21 | 22 | — (미사용) |
| 6 | 3 | Report | — (미사용) | — (미사용) | 23 |
| 7 | 4 | Offer | 31 | 32 | — (미사용) |
| 8 | 4 | Report | — (미사용) | — (미사용) | 33 |
| 9 | 5 | Offer | 41 | 42 | — (미사용) |
| 10 | 5 | Report | — (미사용) | — (미사용) | 43 |
| 11 | 6 | Offer | 51 | 52 | — (미사용) |
| 12 | 6 | Report | — (미사용) | — (미사용) | 53 |

### 3-5. `MissionReward` 테이블 (`DT_SpyMissionReward`, 6행 — `Dialogue` 타입에만 존재)

| `MissionId` | `ExperienceReward` |
|---|---|
| 2 | 20 |
| 4 | 10 |
| 6 | 10 |
| 8 | 10 |
| 10 | 15 |
| 12 | 15 |

**검산**: 20+10+10+10+15+15 = **80** — mission-system.md §3-1 보상 총합과 동일. `Gameplay` 타입 행(1/3/5/7/9/11)은 이 테이블에 행 자체가 없다 — 0으로 채우는 게 아니라 "관계 없음"이다(spec §4-3).

### 3-6. `Mission` 데이터 (`MissionTable`, 12행)

| `MissionId` | `MissionType` | `DisplayName` | `MatchTag` | `Mode` | `TargetCount` | `Description` |
|---|---|---|---|---|---|---|
| 1 | Gameplay | 적 1명 처치 | `Event.Mission.Kill` | Accumulate | 1 | 목표 : 감시 중인 적 요원 1명을 제거하라 |
| 2 | Dialogue | 레이븐에게 보고하십시오 | `Event.Mission.Report` | Accumulate | 1 | *(빈 문자열 — 카드 자체가 없다, spec §4-2)* |
| 3 | Gameplay | 레벨 3 달성 | `Event.Mission.Level` | Threshold | 3 | 목표 : 전투 숙련도를 3단계까지 끌어올려라 |
| 4 | Dialogue | 팰컨에게 보고하십시오 | `Event.Mission.Report` | Accumulate | 1 | *(빈 문자열)* |
| 5 | Gameplay | 콤보 4회 연결 | `Event.Mission.Combo` | Accumulate | 4 | 목표 : 근접 연계 공격을 끊지 않고 4회 이어가라 |
| 6 | Dialogue | 바이퍼에게 보고하십시오 | `Event.Mission.Report` | Accumulate | 1 | *(빈 문자열)* |
| 7 | Gameplay | 장애물 넘기 5회 | `Skill.Move.Vault` | Accumulate | 5 | 목표 : 장애물을 5회 뛰어넘어 침투 루트를 확보하라 |
| 8 | Dialogue | 스패로우에게 보고하십시오 | `Event.Mission.Report` | Accumulate | 1 | *(빈 문자열)* |
| 9 | Gameplay | 벽 타기 3회 | `Skill.Move.Climb` | Accumulate | 3 | 목표 : 벽면을 3회 등반해 은밀 진입로를 익혀라 |
| 10 | Dialogue | 울프에게 보고하십시오 | `Event.Mission.Report` | Accumulate | 1 | *(빈 문자열)* |
| 11 | Gameplay | 그래플링 3회 | `Skill.Move.GrappleHook` | Accumulate | 3 | 목표 : 그래플 훅으로 3회 이동해 고지 접근 루트를 확보하라 |
| 12 | Dialogue | 폭스에게 보고하십시오 | `Event.Mission.Report` | Accumulate | 1 | *(빈 문자열)* |

- `Gameplay` 행 6종의 `DisplayName`/`MatchTag`/`Mode`/`TargetCount`는 mission-system.md §3-1 값 그대로(변경 없음). `Description`도 같은 문서 §5(이전 판) 값을 그대로 재사용한다.
- `Dialogue` 행 6종의 `DisplayName`(spec §4-8 예시를 이 문서가 최종 확정값으로 채택)은 그 자체가 HUD "시스템 메시지"다. `Description`은 **의도적으로 빈 문자열**이다 — `Dialogue` 타입은 수락 카드가 없으므로 채울 자리가 없다(spec §4-2). 이는 누락이 아니라 확정된 설계다.
- **검산**: `Description`에 등장하는 숫자(1/3/4/5/3/3)가 `DisplayName`의 목표 수(1/3/4/5/3/3)와 전부 일치 — mission-system.md §3-1과 모순 없음.

---

## §4 NPC 배치 가이드라인

### 4-1. 정확성 제약 — 레이븐(`NPCId 1`)만

`bAccepted == false`인 동안의 진행 이벤트는 큐에도 안 쌓이고 버려진다(spec §5-1). 봇은 6기, 리스폰이 없다(mission-system.md §1-1). 플레이어가 레이븐과 아예 대화하기 전에 봇을 죽이면 그 킬은 완전히 낭비되고, 낭비가 반복되면 처치 미션(목표 1킬)을 채울 자원 자체가 고갈될 수 있다.

**대응(확정, 이전 판과 동일 원칙)**: **정보원 "레이븐"(`NPCId 1`)을 전투 구역 진입로에, 봇 스폰 지점보다 먼저 지나야 하는 위치에 배치한다.** 미수락 상태로 봇에 접근할 수 없게 만들어 낭비 경로 자체를 차단한다. 이것이 이 문서에 남은 **유일한 정확성 제약**이다.

`Role = Report` 관계는 항상 `Role = Offer`와 **같은 `NPCId`**를 가리키므로(§3-4), "보고"는 별도 위치로의 이동을 요구하지 않는다 — 같은 레이븐에게 다시 말을 거는 것으로 끝난다. 즉 이 배치 결정 하나가 수락 게이트와 보고 게이트 모두를 커버한다.

### 4-2. 노출 구간 — 두 게이트로 늘어남 (레벨 재평가로 정확성은 무관)

레벨(`Threshold`) 미션(`MissionId 3`, 팰컨)의 진행 신호(승급 이벤트)는 그 미션이 현재 미션이면서 **수락된 상태**일 때만 유효하다. 처치 완료부터 팰컨 수락까지가 "신호가 소실될 수 있는 노출 구간"이고, 이번 판에서 그 구간에 게이트가 둘 낀다:

1. **레이븐에게 재접근해 `Report` 상태를 트리거** — 이동은 거의 없다(같은 NPC, §4-1 배치 근거). 이 상호작용 안에서 서버가 `AddProgress(Event_Mission_Report, 1)`을 함께 처리해 `MissionId 2`를 완료시키고, `MissionReward`의 보상(+20)을 지급하며 인덱스를 `MissionId 3`(팰컨 Offer)로 전진시킨다.
2. **팰컨에게 이동해 Offer 카드에서 [수락]** — 이 순간 `AcceptCurrentMission()`이 호출되고, spec §5-5의 레벨 재평가(현재 레벨을 그 자리에서 직접 재주입)가 발동한다.

**재평가는 스냅샷 기반이라 이 구간의 길이와 무관하게 항상 정확하다**(spec §2-6) — 게이트가 1개든 2개든, `AcceptCurrentMission()` 호출 시점의 실제 레벨을 그대로 읽으므로 결과는 동일하다. 게이트가 늘어난 것은 노출 시간(플레이어가 그 사이에 레벨업할 기회)만 늘릴 뿐, 정확성에는 영향을 주지 않는다.

### 4-3. 검산 — Lv3 타이밍 + 퇴화 케이스(이중 상호작용)

**검산 1 — 보상 지급 시점이 바뀌어도 총량은 안 바뀐다**: Lv3에 필요한 누적 XP는 여전히 60 = 킬1(+20) + 레이븐 보고 시 지급되는 `MissionId 2` 보상(+20) + 킬2(+20)이다. 최소 필요 킬 수는 여전히 2, 2인 세션 합계 4/봇 6기로 mission-system.md §4-2의 완주 보장 기준이 그대로 유지된다. 달라진 것은 그 +20이 **자동으로 나가지 않고 "레이븐에게 재접근"이라는 명시적 행동을 거쳐야 나간다**는 것뿐이다.

**검산 2 — 퇴화 케이스: 수락과 보고가 같은 자리에서 F 두 번이 되는 경우**: 플레이어가 처치 미션 목표를 채운 뒤 레이븐에게 보고하기 전에 봇을 한 마리 더 잡으면(그 킬 자체는 미션에 카운트되지 않지만 XP는 정상 적용된다), 누적 XP가 40이 된다. 이후 레이븐에게 보고하면 `MissionId 2` 보상(+20)이 더해져 누적 60 = **Lv3 즉시 도달**. 플레이어가 팰컨에게 이동해 Offer를 **수락하는 순간** §5-5 재평가가 바로 발동해, 어떤 전투 행동도 없이 즉시 `MissionId 3`가 완료되고 인덱스가 `MissionId 4`(팰컨 `Report`)으로 넘어간다 — 하지만 이 완료는 **카드나 대화 갱신 없이 서버 안에서만 일어난다**(§7 흐름상 `AcceptCurrentMission()`은 새 대화 결과를 클라이언트에 push하지 않는다). 플레이어는 팰컨의 `InProgress` 문구를 한 번도 못 보고, **수락 직후 같은 자리에서 F를 다시 눌러야** `Report` 문구와 완료를 확인하게 된다 — "수락"과 "보고"가 같은 위치에서의 **이중 상호작용**이 된다.

이는 mission-system.md §2-3이 기각했던 "즉시완료로 학습 가치가 0" 패턴과 형태가 같지만, 여기서는 완주를 막지 않고 `MissionId 3`(팰컨) 하나의 체감 진행감만 줄어드는 부작용이므로 **이 문서는 수용한다**. 다만 새로 생기는 이중 상호작용 UX가 위화감을 주는지는 §4-4 D3 메트릭으로 관측한다.

### 4-4. 결정 메트릭

| ID | 메트릭 | 목표 범위 | 어긋나면 |
|---|---|---|---|
| D1 | `MissionId 1`(처치) Offer를 수락하기 전에 발생하는 킬 비율 | 0% | 1회라도 관측되면 레이븐 배치(진입로 차단)를 재점검 |
| D2 | 팰컨 Offer 수락 시점에 이미 Lv3 이상이면, 수락과 같은 서버 호출 안에서 즉시 `Report` 상태로 전환되는가(§5-5 재평가) | 항상 즉시 전환 | 재현 안 되면 재평가 전제(ASC 세팅 순서)를 재확인 — 팰컨을 레이븐과 인접 배치하는 것을 정확성 제약으로 재승격 |
| D3 | §4-3 검산 2의 퇴화 케이스(수락 즉시 완료)에서, "같은 자리에서 F 재입력"이라는 이중 상호작용을 플레이어가 위화감 없이 수행하는가(구두 확인) | 위화감 보고 없음 | 보고되면 카드/대화 UX 개선이 필요 — 코드 변경이 필요한 별도 안건(범위 밖, 수치로 해결 불가) |

### 4-5. `NPCId` 2~6 — 학습 곡선·동선 (정확성 제약 없음)

팰컨(2)은 §4-2 재평가가 정상 동작하는 한 정확성 제약에서 벗어난다. 바이퍼(3)~폭스(6)가 담당하는 미션(콤보/넘기/벽타기/그래플)은 전부 **반복 가능한 행동**이라(mission-system.md §1-2 결론) 수락 전에 미리 수행해도 소모되는 자원이 없다. 따라서 이 다섯은 **순수 동선 문제**로 배치한다.

mission-system.md §2-4가 정리한 구역(전투 구역 / 파쿠르 레인 / 그래플 타워)에 맞춰:

| NPC | 구역 | 배치 방향 |
|---|---|---|
| 팰컨(2) | 전투 구역 | 레이븐과 같은 시야, 도보 몇 걸음 이내 — 레이븐에게 보고하고 나면 바로 다음 담당자가 눈에 들어오는 것이 자연스러운 동선(§4-2 "전제"가 깨질 경우 이 인접 배치가 정확성 제약으로 재승격된다는 점도 함께 고려해 가깝게 유지) |
| 바이퍼(3) | 전투 구역 | 레이븐·팰컨 근처 — 콤보는 전투 구역 안에서 이동 없이 이어지는 것이 자연스럽다(mission-system.md §2-4 근거와 동일) |
| 스패로우(4) | 파쿠르 레인 초입 | 전투 구역에서 파쿠르 레인으로 넘어가는 길목 — 바이퍼와 대화를 마치고 이동하면 시야에 들어오는 위치 |
| 울프(5) | 파쿠르 레인 안쪽 | 스패로우보다 Vault 오브젝트들을 지난 안쪽, Climb 오브젝트 근처 — 넘기 레인을 통과하며 자연히 마주치는 동선 |
| 폭스(6) | 그래플 타워 초입 | 파쿠르 레인에서 타워로 이어지는 진입부 |

### 4-6. 확인 필요 — 정확한 좌표

위 배치는 **구역 단위 방향성**이며, mission-system.md §2-4의 좌표 대역은 그 문서 스스로 "낡았을 수 있는 참고값"으로 명시한 값이다. **NPC 6종의 정확한 트랜스폼 좌표는 맵 MCP 재조회로 확인 필요** — 이 기획서는 구역·상대 위치·시야 확보라는 방향만 확정하고, 실제 좌표 배치는 에셋 작업 단계(사용자/MCP)에서 위 방향을 기준으로 결정한다.

---

## §5 인터랙션·대화·미션 카드 UI 텍스트

### 5-1. 상호작용 프롬프트 문구 + 판정 반경

| 위치 | 문구 | 비고 |
|---|---|---|
| 근접 상호작용 프롬프트 | **`F  대화하기`** | 상호작용 입력 액션(`IA_Interact`)이 실제로 `F` 키에 매핑돼야 문구와 실제 조작이 일치한다 |

**상호작용 판정 반경 — `300cm`(이 문서가 확정)**: NPC의 상호작용 `SphereComponent` 트리거 반경과 서버의 거리 재검증(spec §3-1, §9 "보고 위조 방지") 임계값을 **같은 값 하나**로 통일한다. 두 값을 따로 두면(예: 트리거 300cm·재검증 400cm) 클라이언트에서 프롬프트가 사라진 뒤에도 서버가 요청을 승인하는 불일치 구간이 생긴다 — 재검증 반경을 트리거 반경과 동일하게 두면 이 불일치가 구조적으로 없어진다. 300cm는 이 프로젝트의 기존 근접 판정 스케일(예: 파쿠르·그래플 판정 반경이 수백 cm 단위)과 같은 자릿수로, 캐릭터가 대화 상대의 얼굴이 보이는 거리에서 자연스럽게 트리거된다.

### 5-2. 대화창 4종 — "계속"/"닫기" 분기

| 대화 상태 | 버튼 | 다음 동작 |
|---|---|---|
| `Offer` | 계속 | 대화창이 닫히고 **미션 수락 카드**로 전환 |
| `InProgress` | 닫기 | 카드 없이 대화창만 닫힘 |
| `Report` | 닫기 | 카드 없이 대화창만 닫힘 — **서버는 이 대화가 열리기 전에 이미 완료 처리를 끝냈다**(spec §7 3단계, `RequestInteract` 안에서 판정과 `AddProgress`가 동시에 일어난다) |
| `Default` | 닫기 | 카드 없이 대화창만 닫힘 |

버튼 라벨은 "계속"/"닫기" 두 가지로 고정한다. `[Space ▶ ...]` 같은 키 힌트 표기는 쓰지 않는다 — "계속"/"닫기" 전용 키 입력 액션은 이번 범위에 없으므로 마우스 클릭 버튼 + 텍스트만 사용한다.

### 5-3. 미션 카드 — `Offer` 1종뿐

`USpyMissionOfferWidget::ShowMission(Title, Description)`은 **Offer 전용**이다(spec §8-1) — `Report`는 카드가 없으므로 겸용 대상 자체가 없다. **`RewardText` 인자는 없다** — `Gameplay` 타입 미션은 `MissionReward` 행이 없으므로(§3-5) 보상이 애초에 존재하지 않고, 카드에도 보상 텍스트가 표시되지 않는다(spec §6·§8-1).

| 항목 | 값 |
|---|---|
| 카드 헤더 | **`새 임무`** |
| 확인 버튼 | **`수락`** |
| 취소 버튼 | **`거절`** |
| `Title` | `Entry->DisplayName` 그대로 전달 |
| `Description` | `Entry->Description` 그대로 전달 (§3-6 표 값) |

---

## §6 구현 요청사항 (gameplay-programmer 용)

이 문서는 spec이 확정한 구조·클래스·함수 시그니처를 다시 정의하지 않는다. 아래는 **이 문서가 채운 값**과 그 값이 성립하기 위한 조건만 명시한다.

### 6-1. Gameplay Tag

**신규 태그 없음.** spec §10 "변경 파일 목록"이 `Util/SpyGameplayTags.h|.cpp`에 `Event_Mission_Report` 신규 등록을 이미 명시했다. 이 문서는 값을 추가하지 않는다.

### 6-2. C++ 인터페이스

**신규 없음.** `ISpyNPCRoot`/`ISpyInteractionHost`/`ISpyCharacterRoot`의 상호작용 컴포넌트 접근자는 spec §3이 이미 확정했다.

### 6-3. DataAsset·DataTable 스키마 — 값 입력

| 대상 | 값 | 근거 |
|---|---|---|
| `DT_SpyNPC`(6행) | §3-2 표 그대로 | spec §4-4 |
| `DT_SpyDialogue`(24행) | §3-3 6개 하위 표 그대로 (`DialogueIndex` 전부 0) | spec §4-5 |
| `DT_SpyMissionCommunication`(12행) | §3-4 표 그대로 | spec §4-6 |
| `DT_SpyMissionReward`(6행) | §3-5 표 그대로 | spec §4-3 |
| `DA_SpyMissionConfig`의 `MissionTable`(DataTable, 12행) | §3-6 표 그대로 (`MissionId` = 1~12, 1-based — 배열 위치와 무관한 명시적 필드) | spec §4-2 |
| `DA_SpyNPCConfig`(NPC 블루프린트 6종이 참조하는 허브) | `NPCTable`/`DialogueTable`/`MissionCommunicationTable` 3개 지정 | spec §4-7 |
| NPC 블루프린트 6종의 `NPCId` | §2 표의 `NPCId` 열(1~6) — `MissionCommunication.NPCId`와 정확히 일치해야 한다 | spec §3-1 |
| `IA_Interact` InputAction → IMC 키 매핑 | **`F`** | §5-1 |
| NPC 상호작용 `SphereComponent` 반경 + 서버 재검증 임계값 | **300cm** (동일 값) | §5-1 |
| `WBP_MissionOffer` 카드 헤더·버튼 라벨 | `새 임무`/`수락`/`거절` | §5-3 |

### 6-4. GA·GE 명세

**신규 GA·GE 없음.** 보상 GE는 기존 `USpyGE_ExperienceGain` 재사용(mission-system.md §6-4). 매그니튜드는 §3-5 `MissionReward` 값(20/10/10/10/15/15) 그대로이며, 지급 호출 지점은 `USpyMissionComponent::ProcessProgress` 안의 `GrantReward()`(spec §5-2) — 완료된 `Dialogue` 미션의 `MissionId`로 `GetMissionReward()`를 조회해 적용한다.

### 6-5. ⚠ 에디터 데이터 조건

| # | 조건 | 어기면 |
|---|---|---|
| 1 | `MissionTable` 12행이 §3-6 표 순서(홀수=Gameplay, 짝수=Dialogue) 그대로 | 순서가 바뀌면 §4-1의 정확성 제약(레이븐이 봇 스폰보다 먼저)이 엉뚱한 미션에 걸리게 되어 배치 근거가 무너진다 |
| 2 | 레이븐(`NPCId 1`)이 전투 구역 봇 스폰 지점보다 먼저 지나는 진입로에 배치 | §4-4 D1 메트릭(0% 목표) 초과 가능성 — `MissionId 1`의 봇 자원이 수락 전에 낭비될 수 있다 |
| 3 | NPC 블루프린트 6종의 `NPCConfig`에 `DA_SpyNPCConfig`(3개 테이블 묶음)를 반드시 지정 | `BeginPlay` 캐싱이 실패해 `NPCName`/대사가 전부 빈 텍스트로 표시되는 **무증상 실패**다 |
| 4 | `DT_SpyMissionCommunication`에서 `NPCId`별로 **정확히 2행**(`Role = Offer` 1개 + `Role = Report` 1개)이 존재 | 한 NPC에 Offer/Report 행이 없거나 2개 이상이면 `CachedOfferMissionId`/`CachedReportMissionId`가 정의되지 않는다(spec §9 엣지케이스) |
| 5 | `DT_SpyDialogue` 24행 모두 `Text` 필드를 채운다 | 하나라도 비면 해당 상태에서만 재현되는 **부분적 무증상 실패**가 나온다 — 예: `NPCId 4`의 `Report`(33)만 비면 스패로우에게 보고할 때만 재현돼 QA에서 늦게 발견된다 |
| 6 | `IA_Interact`를 `F` 키로 매핑하기 전, 기존 `IMC`에 `F` 키가 다른 액션에 이미 쓰이고 있지 않은지 확인 | 충돌 시 프롬프트 문구("F 대화하기")와 실제 조작이 어긋나거나 기존 기능이 깨진다. 이 프로젝트의 스킬 입력은 `1~6` 숫자 키를 쓰므로 `F`는 비어 있을 가능성이 높지만, 실제 IMC 애셋은 구현 시점에 재확인한다 |
| 7 | `WBP_MissionOffer`에 보상 텍스트 UI 요소(텍스트 블록 등)를 두지 않는다 | `Gameplay` 미션은 보상이 없다(§3-5) — 보상 UI 요소가 있으면 항상 빈 값이 노출되거나, 개발자가 임의로 값을 채워 넣는 실수를 유발한다 |

---

## §7 이번 범위에서 명시 제외

spec §1의 비목표를 그대로 따른다 — 대화 분기·선택지, NPC AI 행동(순찰·전투), 대화 로그·히스토리·보이스, 미션 수락 이력의 세이브·로드 영속화, 거절에 대한 페널티, Threshold 모드 미션 일반화(레벨 미션 하나에 국한된 특화 처리 — YAGNI), 대화 다회차(멀티라인) 연출. 추가로 이 문서가 다루지 않는 것:

| 항목 | 사유 |
|---|---|
| 보상 텍스트의 UI 노출 | spec §6·§8-1에 `MissionRewardText` 필드/`RewardText` 인자가 없다 — 카드에 보상이 보이지 않는 것이 **의도된 동작**이다(§3-5, `Gameplay` 미션은 보상 자체가 없다) |
| 위젯 인스턴스 접근 경로(방금 연 위젯에 `ShowLine`/`ShowMission`을 호출하는 배선 방식) | 코드 구조 결정 — gameplay-programmer가 구현 단계에서 확정 |
| NPC 정확한 트랜스폼 좌표 | §4-6 — 맵 MCP 재조회로 확인 필요, 구역·상대 배치 방향만 이 문서가 확정 |
| HUD 상 "다음 NPC 위치" 마커·미니맵 표시 | spec 비목표 범위 밖, §3-3-7의 `Report` 문구 핸드오프로 대체 |
| `Mission`/`MissionReward`/`MissionCommunication`/`NPC`/`Dialogue` 필드 추가·변경 | spec §4 확정 스키마 — 이 문서는 값만 채운다 |
| `docs/superpowers/plans/2026-08-01-npc-mission-dialogue.md`의 재작성 | 이 plan은 아직 2026-08-01b(구 모델) 상태다. plan 재작성은 이 기획서의 범위 밖이며, 이 문서는 그 plan을 참조하지 않는다 |
