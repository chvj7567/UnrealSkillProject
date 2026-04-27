# Grappling Hook Targeting UI & Highlight Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 기존 WorldStatic LineTrace 그래플링 훅을 GrappleAnchor Actor Tag 기반 사전 타겟팅 + UI 프롬프트 + Highlight 시스템으로 교체한다.

**Architecture:** `USpyGrappleTargetingComponent`가 로컬 클라이언트 Tick에서 거리 + 화면 중앙 AND 조건으로 GrappleAnchor 액터를 감지하고 `OnGrappleTargetChanged` Delegate를 발행한다. 서버는 `Server_SetGrappleTarget` RPC로 `CurrentGrappleTarget`(Replicated)을 유지하며 GA가 이를 읽어 `LaunchCharacter`를 호출한다.

**Tech Stack:** UE5.7, GAS (GameplayAbilities), Enhanced Input, UMG Widget, CustomDepth Post Process

---

## 파일 맵

| 파일 | 작업 |
|---|---|
| `SkillProject/Source/SkillProject/ManagerComponent/SpyGrappleTargetingComponent.h` | 신규 |
| `SkillProject/Source/SkillProject/ManagerComponent/SpyGrappleTargetingComponent.cpp` | 신규 |
| `SkillProject/Source/SkillProject/Data/SpyMovementConfig.h` | 필드 2개 추가 |
| `SkillProject/Source/SkillProject/AbilitySystem/Movement/SpyGA_GrappleHook.h` | TryLineTrace 제거, 컴포넌트 조회 추가 |
| `SkillProject/Source/SkillProject/AbilitySystem/Movement/SpyGA_GrappleHook.cpp` | TryLineTrace 구현 제거, ActivateAbility 수정 |

---

## Task 1: SpyMovementConfig 필드 추가

**Files:**
- Modify: `SkillProject/Source/SkillProject/Data/SpyMovementConfig.h`

- [ ] **Step 1: GrappleHook 카테고리 끝에 두 필드 추가**

`SkillProject/Source/SkillProject/Data/SpyMovementConfig.h`의 기존 `GrappleFlightTime` 필드 바로 아래에 추가:

```cpp
UPROPERTY(EditDefaultsOnly, Category = "GrappleHook")
float GrapplePromptRange = 1500.f;

UPROPERTY(EditDefaultsOnly, Category = "GrappleHook")
float GrappleTargetingScreenRadius = 150.f;
```

최종 GrappleHook 블록:

```cpp
// Grapple Hook
UPROPERTY(EditDefaultsOnly, Category = "GrappleHook")
float GrappleMaxRange = 3000.f;

UPROPERTY(EditDefaultsOnly, Category = "GrappleHook")
float GrappleArrivalThreshold = 150.f;

UPROPERTY(EditDefaultsOnly, Category = "GrappleHook")
float GrappleLaunchArcZScale = 0.4f;

UPROPERTY(EditDefaultsOnly, Category = "GrappleHook")
float GrappleFlightTime = 0.8f;

UPROPERTY(EditDefaultsOnly, Category = "GrappleHook")
float GrapplePromptRange = 1500.f;

UPROPERTY(EditDefaultsOnly, Category = "GrappleHook")
float GrappleTargetingScreenRadius = 150.f;
```

- [ ] **Step 2: 커밋**

```bash
git add SkillProject/Source/SkillProject/Data/SpyMovementConfig.h
git commit -m "[Data] SpyMovementConfig에 GrapplePromptRange / GrappleTargetingScreenRadius 필드 추가"
```

---

## Task 2: USpyGrappleTargetingComponent 구현

**Files:**
- Create: `SkillProject/Source/SkillProject/ManagerComponent/SpyGrappleTargetingComponent.h`
- Create: `SkillProject/Source/SkillProject/ManagerComponent/SpyGrappleTargetingComponent.cpp`

- [ ] **Step 1: SpyGrappleTargetingComponent.h 작성**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpyGrappleTargetingComponent.generated.h"

class USpyMovementConfig;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGrappleTargetChanged, AActor*, NewTarget);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SKILLPROJECT_API USpyGrappleTargetingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USpyGrappleTargetingComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Grapple")
    AActor* GetCurrentGrappleTarget() const { return CurrentGrappleTarget; }

    UPROPERTY(BlueprintAssignable, Category = "Grapple")
    FOnGrappleTargetChanged OnGrappleTargetChanged;

protected:
    UPROPERTY(EditDefaultsOnly, Category = "Config")
    TObjectPtr<USpyMovementConfig> MovementConfig;

private:
    UFUNCTION(Server, Reliable)
    void Server_SetGrappleTarget(AActor* NewTarget);

    AActor* FindBestTarget() const;

    UPROPERTY(Replicated)
    TObjectPtr<AActor> CurrentGrappleTarget;

    TWeakObjectPtr<AActor> LocalCachedTarget;
};
```

- [ ] **Step 2: SpyGrappleTargetingComponent.cpp 작성**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "SpyGrappleTargetingComponent.h"
#include "Data/SpyMovementConfig.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGrappleTargetingComponent)

USpyGrappleTargetingComponent::USpyGrappleTargetingComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
}

void USpyGrappleTargetingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(USpyGrappleTargetingComponent, CurrentGrappleTarget);
}

void USpyGrappleTargetingComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn) return;

    APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
    if (!PC || !PC->IsLocalController()) return;

    AActor* NewTarget = FindBestTarget();

    if (LocalCachedTarget != NewTarget)
    {
        LocalCachedTarget = NewTarget;
        OnGrappleTargetChanged.Broadcast(NewTarget);
        Server_SetGrappleTarget(NewTarget);
    }
}

AActor* USpyGrappleTargetingComponent::FindBestTarget() const
{
    if (!MovementConfig) return nullptr;

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    APlayerController* PC = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
    if (!PC) return nullptr;

    FVector2D ViewportSize;
    GEngine->GameViewport->GetViewportSize(ViewportSize);
    const FVector2D ViewportCenter = ViewportSize * 0.5f;

    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));

    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(GetOwner());

    TArray<AActor*> OutActors;
    UKismetSystemLibrary::SphereOverlapActors(
        GetWorld(),
        OwnerPawn->GetActorLocation(),
        MovementConfig->GrapplePromptRange,
        ObjectTypes,
        nullptr,
        ActorsToIgnore,
        OutActors
    );

    AActor* BestTarget = nullptr;
    float BestDistToCenter = MovementConfig->GrappleTargetingScreenRadius;

    for (AActor* Actor : OutActors)
    {
        if (!Actor || !Actor->Tags.Contains(FName("GrappleAnchor"))) continue;

        FVector2D ScreenPos;
        if (!PC->ProjectWorldLocationToScreen(Actor->GetActorLocation(), ScreenPos, true)) continue;

        const float DistToCenter = FVector2D::Distance(ScreenPos, ViewportCenter);
        if (DistToCenter < BestDistToCenter)
        {
            BestDistToCenter = DistToCenter;
            BestTarget = Actor;
        }
    }

    return BestTarget;
}

void USpyGrappleTargetingComponent::Server_SetGrappleTarget_Implementation(AActor* NewTarget)
{
    CurrentGrappleTarget = NewTarget;
}
```

- [ ] **Step 3: 커밋**

```bash
git add SkillProject/Source/SkillProject/ManagerComponent/SpyGrappleTargetingComponent.h SkillProject/Source/SkillProject/ManagerComponent/SpyGrappleTargetingComponent.cpp
git commit -m "[Feature] USpyGrappleTargetingComponent 구현 — GrappleAnchor 스캔 + Delegate + RPC"
```

---

## Task 3: SpyGA_GrappleHook 수정

**Files:**
- Modify: `SkillProject/Source/SkillProject/AbilitySystem/Movement/SpyGA_GrappleHook.h`
- Modify: `SkillProject/Source/SkillProject/AbilitySystem/Movement/SpyGA_GrappleHook.cpp`

- [ ] **Step 1: SpyGA_GrappleHook.h — TryLineTrace 제거, 컴포넌트 include 추가**

`SpyGA_GrappleHook.h`를 아래로 교체:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Ability/SKGameplayAbility.h"
#include "SpyGA_GrappleHook.generated.h"

class AGrappleCableActor;
class USpyMovementConfig;
class USpyGrappleTargetingComponent;

UCLASS()
class SKILLPROJECT_API USpyGA_GrappleHook : public USKGameplayAbility
{
    GENERATED_BODY()

public:
    USpyGA_GrappleHook();

protected:
    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    virtual void EndAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility,
        bool bWasCancelled) override;

private:
    UFUNCTION()
    void OnGrappleArrived();

    void LaunchToTarget(ACharacter* Character, const FVector& TargetLocation) const;

protected:
    UPROPERTY(EditDefaultsOnly, Category = "Config")
    TObjectPtr<USpyMovementConfig> MovementConfig;

    UPROPERTY(EditDefaultsOnly, Category = "Config")
    FName HandBoneName = FName("hand_r");

private:
    UPROPERTY()
    TObjectPtr<AGrappleCableActor> CableActor;
};
```

- [ ] **Step 2: SpyGA_GrappleHook.cpp — TryLineTrace 제거, ActivateAbility 수정**

`SpyGA_GrappleHook.cpp`를 아래로 교체:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "SpyGA_GrappleHook.h"
#include "GrappleCableActor.h"
#include "SpyAbilityTask_GrappleTick.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Data/SpyMovementConfig.h"
#include "Util/SpyGameplayTags.h"
#include "ManagerComponent/SpyGrappleTargetingComponent.h"
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGA_GrappleHook)

USpyGA_GrappleHook::USpyGA_GrappleHook()
{
    InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

    ActivationBlockedTags.AddTag(SpyGameplayTags::Character_State_Grapple);
}

void USpyGA_GrappleHook::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    CurrentSpecHandle     = Handle;
    CurrentActorInfo      = ActorInfo;
    CurrentActivationInfo = ActivationInfo;

    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        ASC->AddLooseGameplayTag(SpyGameplayTags::Lock_Input_Move);
        ASC->AddLooseGameplayTag(SpyGameplayTags::Character_State_Grapple);
    }

    if (!HasAuthority(&ActivationInfo))
        return;

    AActor* AvatarActor = GetAvatarActorFromActorInfo();
    USpyGrappleTargetingComponent* TargetComp =
        AvatarActor ? AvatarActor->FindComponentByClass<USpyGrappleTargetingComponent>() : nullptr;

    if (!TargetComp)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    AActor* Target = TargetComp->GetCurrentGrappleTarget();
    if (!Target)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    ACharacter* Char = Cast<ACharacter>(AvatarActor);
    if (!Char)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    const FVector ImpactPoint = Target->GetActorLocation();
    LaunchToTarget(Char, ImpactPoint);

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = Char;
    CableActor = GetWorld()->SpawnActor<AGrappleCableActor>(SpawnParams);
    if (!CableActor)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    CableActor->InitCable(Char, ImpactPoint, HandBoneName);

    const float Threshold = MovementConfig ? MovementConfig->GrappleArrivalThreshold : 150.f;
    USpyAbilityTask_GrappleTick* Task = USpyAbilityTask_GrappleTick::GrappleTick(this, CableActor, Threshold);
    Task->OnArrived.AddDynamic(this, &USpyGA_GrappleHook::OnGrappleArrived);
    Task->ReadyForActivation();

    const float Timeout = MovementConfig ? MovementConfig->GrappleFlightTime * 2.f : 2.f;
    UAbilityTask_WaitDelay* TimeoutTask = UAbilityTask_WaitDelay::WaitDelay(this, Timeout);
    TimeoutTask->OnFinish.AddDynamic(this, &USpyGA_GrappleHook::OnGrappleArrived);
    TimeoutTask->ReadyForActivation();
}

void USpyGA_GrappleHook::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        ASC->RemoveLooseGameplayTag(SpyGameplayTags::Lock_Input_Move);
        ASC->RemoveLooseGameplayTag(SpyGameplayTags::Character_State_Grapple);
    }

    if (HasAuthority(&ActivationInfo) && CableActor)
    {
        CableActor->Destroy();
        CableActor = nullptr;
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USpyGA_GrappleHook::OnGrappleArrived()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USpyGA_GrappleHook::LaunchToTarget(ACharacter* Character, const FVector& TargetLocation) const
{
    if (!MovementConfig) return;

    const FVector CharLoc  = Character->GetActorLocation();
    const FVector Direction = (TargetLocation - CharLoc).GetSafeNormal();
    const float   HorzDist  = FVector::Dist2D(CharLoc, TargetLocation);
    const float   Speed     = HorzDist / FMath::Max(MovementConfig->GrappleFlightTime, KINDA_SMALL_NUMBER);

    FVector LaunchVelocity  = Direction * Speed;
    LaunchVelocity.Z       += HorzDist * MovementConfig->GrappleLaunchArcZScale;

    Character->LaunchCharacter(LaunchVelocity, true, true);
}
```

- [ ] **Step 3: 커밋**

```bash
git add SkillProject/Source/SkillProject/AbilitySystem/Movement/SpyGA_GrappleHook.h SkillProject/Source/SkillProject/AbilitySystem/Movement/SpyGA_GrappleHook.cpp
git commit -m "[Fix] SpyGA_GrappleHook — TryLineTrace 제거, SpyGrappleTargetingComponent 기반 타겟 조회로 교체"
```

---

## Task 4: 빌드 및 에디터 작업

이 Task는 Unreal Editor에서 수행한다 (C++ 변경 없음).

- [ ] **Step 1: 솔루션 재생성 및 빌드**

`SkillProject/Launch.bat` 실행 후 Visual Studio에서 빌드.
컴파일 에러 없이 완료 확인.

- [ ] **Step 2: SpyMovementConfig DataAsset 값 입력**

`Content/Spy/Data/Config/` 경로의 `SpyMovementConfig` DataAsset 열기.
GrappleHook 카테고리:

| 필드 | 값 |
|---|---|
| `GrapplePromptRange` | `1500` |
| `GrappleTargetingScreenRadius` | `150` |

Save.

- [ ] **Step 3: BP_SpyGrappleTargetingComponent 블루프린트 생성**

`Content/Spy/Components/` (또는 `Content/Spy/Blueprints/Components/`)에서
`SpyGrappleTargetingComponent`를 부모 클래스로 Blueprint 생성 → 이름: `BP_SpyGrappleTargetingComponent`.

Config 카테고리 `MovementConfig` 프로퍼티에 SpyMovementConfig DataAsset 할당.
Save & Compile.

- [ ] **Step 4: SpyCharacterAssetData에 컴포넌트 등록**

`Content/Spy/Data/` 경로의 `SpyCharacterAssetData` DataAsset 열기.
`CommonComponentClasses` 배열에 `BP_SpyGrappleTargetingComponent` 추가.
Save.

- [ ] **Step 5: WBP_GrapplePrompt 위젯 블루프린트 생성**

`Content/Spy/UI/` 경로에 Widget Blueprint 생성 → 이름: `WBP_GrapplePrompt`.

Canvas Panel 추가 후 중앙에 Text Block:
- Text: `E 키를 눌러 그래플링`
- 폰트 크기 / 색상은 프로젝트 스타일에 맞게 설정.

Save & Compile.

- [ ] **Step 6: Post Process Volume — CustomDepth 활성화**

레벨의 Post Process Volume 선택.
`Rendering Features → Custom Depth-Stencil Pass` → `Enabled` 설정.
(이미 활성화된 경우 생략)

- [ ] **Step 7: BP_SpyCharacter — Delegate 구독 + Highlight + UI 구현**

`Content/Spy/Blueprints/` 경로의 `BP_SpyCharacter` 열기.

**BeginPlay에 추가:**
```
GetComponentByClass(SpyGrappleTargetingComponent) → GrappleComp 변수 저장
GrappleComp → OnGrappleTargetChanged → Bind Event → HandleGrappleTargetChanged
WBP_GrapplePrompt → Create Widget → Ref 변수 저장 → Add to Viewport
WBP_GrapplePrompt → SetVisibility(Hidden)
```

**HandleGrappleTargetChanged(NewTarget) 커스텀 이벤트:**
```
// OldTarget 캐시 변수(Actor 타입) 활용
[NewTarget != null 분기]
  OldTargetCache 유효 시:
    OldTargetCache → GetComponentByClass(StaticMeshComponent or SkeletalMeshComponent)
    → SetRenderCustomDepth(false)
  NewTarget:
    → GetComponentByClass(StaticMeshComponent or SkeletalMeshComponent)
    → SetRenderCustomDepth(true)
    → SetCustomDepthStencilValue(1)  // Post Process 아웃라인 구분용
  WBP_GrapplePrompt → SetVisibility(Visible)
  OldTargetCache = NewTarget

[NewTarget == null 분기]
  OldTargetCache 유효 시:
    → GetComponentByClass(StaticMeshComponent or SkeletalMeshComponent)
    → SetRenderCustomDepth(false)
  WBP_GrapplePrompt → SetVisibility(Hidden)
  OldTargetCache = null
```

Save & Compile.

- [ ] **Step 8: IMC_Default 입력 바인딩 확인**

`Content/Spy/Input/Mapping/IMC_Default` 열기.
`IA_Grapple` Action이 `E 키`에 바인딩되어 있는지 확인.
없으면: `Input/Actions/` 에 `IA_Grapple` (Digital) 생성 후 IMC에 E 키 바인딩 추가.
`SpyInputConfig` DataAsset에서 `Input.Ability.Skill.11` 태그 슬롯에 `IA_Grapple` 할당 확인.

Save.

---

## Task 5: 인게임 검증

이 Task는 PIE(Play In Editor) 또는 Standalone으로 검증한다.

- [ ] **Step 1: 씬에 GrappleAnchor 액터 배치**

레벨에 `StaticMeshActor` (큐브 등) 배치.
Details 패널 → `Actor` 카테고리 → `Tags` 배열에 `GrappleAnchor` 추가.
캐릭터에서 1500 유닛 이내 위치에 배치.

- [ ] **Step 2: 타겟팅 UI 확인**

PIE 실행.
액터 방향으로 카메라를 돌려 화면 중앙 150px 반경 내에 위치시킨다.
→ `WBP_GrapplePrompt`("E 키를 눌러 그래플링") 표시 확인.
→ 액터에 Highlight(아웃라인) 적용 확인.
카메라를 다른 방향으로 돌리면 UI 사라짐 확인.

- [ ] **Step 3: 1500 유닛 거리 밖 UI 미표시 확인**

캐릭터를 액터에서 1500 유닛 이상 떨어진 위치로 이동.
화면 중앙에 액터가 있어도 UI가 표시되지 않음 확인.

- [ ] **Step 4: E 키 그래플링 발동 확인**

타겟이 잡힌 상태(UI 표시 중)에서 E 키 입력.
→ 캐릭터가 포물선을 그리며 앵커 방향으로 날아감 확인.
→ 케이블 비주얼 표시 확인.
→ 앵커 근처(150 유닛) 도달 시 자동 종료 확인.

- [ ] **Step 5: 타겟 없는 상태에서 E 키 무반응 확인**

타겟이 없는 방향으로 카메라를 돌린 뒤 E 키 입력.
→ GA가 조용히 취소(cancelled) 처리되고 캐릭터가 날아가지 않음 확인.
