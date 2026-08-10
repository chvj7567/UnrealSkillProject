# 미션 타겟 네비 숨김 트리거 영역 — 설계

## 1. 배경 / 동기

`USpyNavigationComponent`(`docs/superpowers/specs/2026-08-04-mission-ground-navigation-design.md`)는 현재 NavMesh 경로 남은 길이를 `ArrivalHideDistanceCm`(300cm)/`ArrivalReshowDistanceCm`(400cm) 히스테리시스로 비교해 도착 시 네비 라인을 숨긴다. 이 값은 컴포넌트 하나에 고정된 원형 반경 개념이라, 미션 타겟마다 다른 모양(복도·방·비원형 공간)의 "숨김 구역"을 표현할 수 없다. 이번 작업은 레벨 디자이너가 타겟별로 박스 트리거를 직접 배치해 숨김 구역을 지정할 수 있게 확장한다.

## 2. 범위

**포함**:
- `ASpyMissionTargetPoint`(Gameplay형) / `ASpyNPCCharacter`(Dialogue형) / `ASpyInteractableObject`(Interact형) 세 미션 타겟 액터 모두에 선택적 `HideTriggerVolume`(`UBoxComponent`) 추가.
- 디자이너가 인스턴스별로 트리거 사용 여부(`bEnableHideTrigger`)와 박스 Extent/회전을 편집 가능.
- 트리거 활성 타겟은 오버랩 진입/이탈로 네비 숨김/표시를 즉시 제어(거리 히스테리시스 미사용).
- 트리거 비활성(기본값) 타겟은 기존 거리 히스테리시스 그대로 유지 — 하위 호환.
- `USpyMissionTargetRegistrySubsystem`에 타겟 액터 자체를 조회하는 API 추가.
- 신규 공용 인터페이스 `IMissionTargetHideVolume`.

**제외 (이번 스코프 아님)**:
- Box 외 형태(Sphere/Capsule) 지원 — Box 고정.
- 트리거 영역 내에서의 추가 연출(파티클·사운드 등) — 순수 표시/숨김 제어만.
- 서버 권한/레플리케이션 — `USpyNavigationComponent`와 동일하게 로컬 컨트롤 클라이언트 전용 연출이며, 트리거 콜리전도 각 클라이언트가 로컬로 판정한다(NPC/Interactable의 기존 `InteractionSphere`와 동일 패턴).
- 기존 `InteractionSphere`(상호작용 판정)와의 통합 — 완전히 별도의 컴포넌트로 분리하고 반경도 독립적으로 튜닝한다.
- `ArrivalHideDistanceCm`/`ArrivalReshowDistanceCm` 자체의 수치 변경 — 트리거 미배치 타겟의 폴백 경로는 그대로 둔다.

## 3. 레지스트리 확장 — `USpyMissionTargetRegistrySubsystem`

기존 `FindNPCLocation`/`FindMissionTargetLocation`은 `FVector`만 반환한다. 트리거 컴포넌트를 얻으려면 액터 자체가 필요하므로 아래를 추가한다(기존 API는 시그니처 변경 없이 그대로 유지 — 호출부·테스트 영향 없음):

```
AActor* FindNPCActor(int32 NPCId) const;
AActor* FindMissionTargetActor(FGameplayTag InTag) const;
```

- 내부적으로 이미 보관 중인 `TMap<int32, TWeakObjectPtr<AActor>> NPCLocations` / `TMap<FGameplayTag, TWeakObjectPtr<AActor>> MissionTargetLocations`에서 `TWeakObjectPtr::Get()`으로 조회. 키가 없거나 액터가 소멸했으면 `nullptr`.

## 4. 신규 인터페이스 — `IMissionTargetHideVolume`

`System/CommonInterface.System.h` 신설(`USpyMissionTargetRegistrySubsystem`이 세 타입을 이미 묶는 자리라 여기 둔다, cpp-style §12):

```cpp
UINTERFACE(MinimalAPI)
class USpyMissionTargetHideVolume : public UInterface { GENERATED_BODY() };

class IMissionTargetHideVolume
{
    GENERATED_BODY()
public:
    //# 트리거 비활성(bEnableHideTrigger == false) 인스턴스는 nullptr 반환 —
    //# 호출부(USpyNavigationComponent)는 nullptr 을 "폴백 신호"로 해석한다.
    virtual UPrimitiveComponent* GetHideTriggerComponent() const = 0;
};
```

`ASpyMissionTargetPoint` / `ASpyNPCCharacter` / `ASpyInteractableObject` 세 클래스가 이 인터페이스를 구현한다.

## 5. 타겟 액터 측 변경 (3개 클래스 공통)

각 액터에 추가:

```cpp
UPROPERTY(VisibleAnywhere, Category = "Navigation")
TObjectPtr<UBoxComponent> HideTriggerVolume;

//# 레벨 배치 시 인스턴스별로 켤지 결정 (cpp-style §15 — 기본값 false 로 기존 레벨 무변경)
UPROPERTY(EditAnywhere, Category = "Navigation")
bool bEnableHideTrigger = false;
```

- 생성자에서 `HideTriggerVolume`을 항상 생성(기존 루트/InteractionSphere 하위에 부착)하되 `SetCollisionEnabled(ECollisionEnabled::NoCollision)`로 시작.
- `BeginPlay`에서 `bEnableHideTrigger == true`일 때만 `SetCollisionEnabled(ECollisionEnabled::QueryOnly)`로 전환. 오버랩 델리게이트는 `USpyNavigationComponent`가 직접 구독하므로 이 액터 쪽에서 별도 핸들러를 만들지 않는다(§6).
- `GetHideTriggerComponent()` 구현: `bEnableHideTrigger ? HideTriggerVolume : nullptr`.
- `ASpyMissionTargetPoint`는 `RootScene` 하위, `ASpyNPCCharacter`/`ASpyInteractableObject`는 각각 기존 루트(캡슐/`InteractionSphere`) 하위에 부착.

## 6. `USpyNavigationComponent` 측 변경

- `TryResolveTarget()`이 좌표를 찾을 때(`FindNPCLocation`/`FindMissionTargetLocation` 성공 시) 대응하는 `FindNPCActor`/`FindMissionTargetActor`로 액터도 함께 얻는다.
- `StartPathTo()` 진입 시 액터를 `Cast<IMissionTargetHideVolume>`으로 캐스팅해 `GetHideTriggerComponent()` 호출:
  - `nullptr`이면 기존 거리 히스테리시스 경로 그대로(§2 제외 항목대로 무변경).
  - 유효한 컴포넌트면 `TWeakObjectPtr<UPrimitiveComponent> BoundHideTrigger`에 저장하고 `OnComponentBeginOverlap`/`OnComponentEndOverlap`을 구독, 구독 직후 `BoundHideTrigger->IsOverlappingActor(GetOwner())`로 `bInsideHideTrigger` 초기값을 동기화(이미 박스 안에서 미션이 시작되는 경우 대비).
- 신규 핸들러(둘 다 `OtherActor == GetOwner()` 가드 후 처리, 아니면 즉시 return — 다른 로컬 폰이 같은 트리거에 겹치는 케이스 방지):
  - `HandleHideTriggerBeginOverlap`: `bInsideHideTrigger = true` → `HideVisual()` 즉시 호출.
  - `HandleHideTriggerEndOverlap`: `bInsideHideTrigger = false` → `bPathVisible = true`로 시드 → `RecomputePath()` 즉시 호출(0.75초 타이머를 기다리지 않음).
- **트리거 바인딩된 타겟은 히스테리시스를 전혀 쓰지 않는다(개정 — code-reviewer 발견, 최초 §6은 "안"만 우회하고 "밖"은 여전히 히스테리시스를 태워 half-extent가 `ArrivalHideDistanceCm`(300cm) 미만인 타겟(§3-4가 승인한 75cm/300cm 축소 케이스 포함)에서 나가자마자 같은 프레임에 다시 숨는 재발 결함이 있었다)**:
  - `RecomputePath()` 최상단 가드 — `bInsideHideTrigger == true`면 NavMesh 질의 자체를 생략하고 `HideVisual()` 후 return("안"을 무조건 처리).
  - `ApplyPathPoints()` 진입 시 `BoundHideTrigger.IsValid()`면 `EvaluateHysteresisVisibility` 호출을 완전히 건너뛰고 `bPathVisible = true`로 무조건 확정("밖"을 무조건 처리 — 이 함수에 도달했다는 것 자체가 `RecomputePath` 가드를 통과했다는 뜻이라 이미 "밖"이다). 트리거 미바인딩 타겟만 기존 `EvaluateHysteresisVisibility` 경로를 그대로 탄다.
  - `NotifyHideTriggerExited`의 `bPathVisible = true` 시드는 이 바이패스와 별개로 유지한다 — `RecomputePath()`가 World/NavSystem 없음 등으로 `ApplyPathPoints()`에 못 미쳐 조기 반환하는 프레임에도 "밖" 상태를 동기적으로 반영하기 위함(테스트 환경 포함).
- `StopPath()`/타겟 전환(`HandleMissionProgressChanged`가 새 타겟으로 재조회) 시 이전 `BoundHideTrigger`가 유효하면 델리게이트 구독 해제 후 `BoundHideTrigger = nullptr`, `bInsideHideTrigger = false`로 리셋.

## 7. 테스트 가능 범위

- `USpyMissionTargetRegistrySubsystem`: `FindNPCActor`/`FindMissionTargetActor` 신규 유닛 테스트(등록/해제/미존재 케이스) — 기존 `SpyMissionTargetRegistrySubsystemTests.cpp` 패턴 그대로 확장.
- `USpyNavigationComponent`: 실제 물리 오버랩 없이도 `HandleHideTriggerBeginOverlap`/`HandleHideTriggerEndOverlap`을 테스트에서 직접 호출 가능하도록 `protected`로 유지(기존 `SetMissionTargetRegistry` 테스트 주입 패턴과 동일 사상, §5-7). `OtherActor` 가드가 올바르게 다른 액터를 무시하는지도 케이스에 포함.
- 회귀: `bEnableHideTrigger == false`인 기존 타겟들이 여전히 거리 히스테리시스로만 동작하는지(기존 `SpyNavigationComponentTests.cpp`가 이미 이 경로를 커버 — 신규 코드가 그 결과를 바꾸지 않는지 재확인).

## 8. 열린 질문 (구현 단계에서 확정)

- `UBoxComponent`의 콜리전 프로파일(Preset)을 `InteractionSphere`와 동일한 것을 재사용할지, 트리거 전용 프로파일을 새로 둘지는 gameplay-programmer 단계에서 프로젝트 콜리전 채널 설정을 보고 결정한다.
