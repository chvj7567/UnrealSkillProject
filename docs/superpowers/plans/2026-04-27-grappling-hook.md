# Grappling Hook Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** GA 단일 클래스 + AbilityTask Tick 방식으로 풀 멀티플레이어 지원 그래플링 훅 시스템 구현

**Architecture:** `LocalPredicted` GA가 서버에서 LineTrace → LaunchCharacter → Replicated `AGrappleCableActor` 스폰. CableActor 자체 Tick이 케이블 위치를 매 프레임 갱신하고, `USpyAbilityTask_GrappleTick`이 서버 전용으로 도착 거리 체크 후 EndAbility 호출.

**Tech Stack:** UE5.7, GAS (GameplayAbilities), CableComponent Plugin, USKGameplayAbility 패턴

---

## 파일 맵

| 파일 | 작업 |
|---|---|
| `SkillProject/SkillProject.uproject` | CableComponent 플러그인 추가 |
| `SkillProject/Source/SkillProject/SkillProject.Build.cs` | CableComponent 모듈 추가 |
| `SkillProject/Source/SkillProject/Util/SpyGameplayTags.h` | 태그 2개 선언 |
| `SkillProject/Source/SkillProject/Util/SpyGameplayTags.cpp` | 태그 2개 정의 |
| `SkillProject/Source/SkillProject/Data/SpyMovementConfig.h` | 그래플 설정 필드 4개 추가 |
| `SkillProject/Source/SkillProject/AbilitySystem/Movement/GrappleCableActor.h` | 신규 |
| `SkillProject/Source/SkillProject/AbilitySystem/Movement/GrappleCableActor.cpp` | 신규 |
| `SkillProject/Source/SkillProject/AbilitySystem/Movement/SpyAbilityTask_GrappleTick.h` | 신규 |
| `SkillProject/Source/SkillProject/AbilitySystem/Movement/SpyAbilityTask_GrappleTick.cpp` | 신규 |
| `SkillProject/Source/SkillProject/AbilitySystem/Movement/SpyGA_GrappleHook.h` | 신규 |
| `SkillProject/Source/SkillProject/AbilitySystem/Movement/SpyGA_GrappleHook.cpp` | 신규 |

---

## Task 1: CableComponent 플러그인 + 모듈 설정

**Files:**
- Modify: `SkillProject/SkillProject.uproject`
- Modify: `SkillProject/Source/SkillProject/SkillProject.Build.cs`

- [ ] **Step 1: uproject에 CableComponent 플러그인 추가**

`SkillProject/SkillProject.uproject`의 `"Plugins"` 배열에 추가:

```json
{
    "Name": "CableComponent",
    "Enabled": true
}
```

최종 Plugins 배열:
```json
"Plugins": [
    { "Name": "ModelingToolsEditorMode", "Enabled": true, "TargetAllowList": ["Editor"] },
    { "Name": "GameplayAbilities", "Enabled": true },
    { "Name": "ModularGameplay", "Enabled": true },
    { "Name": "ModularGameplayActors", "Enabled": true },
    { "Name": "MotionWarping", "Enabled": true },
    { "Name": "RemoteControl", "Enabled": true },
    { "Name": "CableComponent", "Enabled": true }
]
```

- [ ] **Step 2: Build.cs에 CableComponent 모듈 추가**

`SkillProject/Source/SkillProject/SkillProject.Build.cs`의 `PublicDependencyModuleNames`에 `"CableComponent"` 추가:

```csharp
PublicDependencyModuleNames.AddRange(new string[]
{
    "Core",
    "CoreUObject",
    "Engine",
    "InputCore",
    "EnhancedInput",
    "ModularGameplay",
    "UMG",
    "Slate",
    "SlateCore",
    "Niagara",
    "GameplayAbilities",
    "GameplayTags",
    "GameplayTasks",
    "ModularGameplayActors",
    "MotionWarping",
    "SKGAS",
    "AIModule",
    "NavigationSystem",
    "CableComponent",
});
```

- [ ] **Step 3: 커밋**

```bash
git add SkillProject/SkillProject.uproject SkillProject/Source/SkillProject/SkillProject.Build.cs
git commit -m "[Build] CableComponent 플러그인 및 모듈 추가"
```

---

## Task 2: Gameplay Tag 등록

**Files:**
- Modify: `SkillProject/Source/SkillProject/Util/SpyGameplayTags.h`
- Modify: `SkillProject/Source/SkillProject/Util/SpyGameplayTags.cpp`

- [ ] **Step 1: SpyGameplayTags.h에 태그 선언 추가**

`//# 캐릭터 상태` 블록 끝에 추가:
```cpp
SKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Grapple);
```

`//# 이동 스킬` 블록 끝에 추가:
```cpp
SKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Move_GrappleHook);
```

- [ ] **Step 2: SpyGameplayTags.cpp에 태그 정의 추가**

`UE_DEFINE_GAMEPLAY_TAG(Character_State_Movement_Climb, ...)` 바로 아래에 추가:
```cpp
UE_DEFINE_GAMEPLAY_TAG(Character_State_Grapple, "Character.State.Grapple");
```

`UE_DEFINE_GAMEPLAY_TAG(Skill_Move_Jump, ...)` 바로 아래에 추가:
```cpp
UE_DEFINE_GAMEPLAY_TAG(Skill_Move_GrappleHook, "Skill.Move.GrappleHook");
```

- [ ] **Step 3: 커밋**

```bash
git add SkillProject/Source/SkillProject/Util/SpyGameplayTags.h SkillProject/Source/SkillProject/Util/SpyGameplayTags.cpp
git commit -m "[Tag] Character_State_Grapple / Skill_Move_GrappleHook 태그 등록"
```

---

## Task 3: SpyMovementConfig에 그래플 설정 필드 추가

**Files:**
- Modify: `SkillProject/Source/SkillProject/Data/SpyMovementConfig.h`

- [ ] **Step 1: SpyMovementConfig.h에 필드 추가**

클래스 본문 끝(`};` 바로 위)에 추가:

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
```

- [ ] **Step 2: 커밋**

```bash
git add SkillProject/Source/SkillProject/Data/SpyMovementConfig.h
git commit -m "[Data] SpyMovementConfig에 그래플링 훅 설정 필드 추가"
```

---

## Task 4: AGrappleCableActor 구현

**Files:**
- Create: `SkillProject/Source/SkillProject/AbilitySystem/Movement/GrappleCableActor.h`
- Create: `SkillProject/Source/SkillProject/AbilitySystem/Movement/GrappleCableActor.cpp`

- [ ] **Step 1: GrappleCableActor.h 작성**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrappleCableActor.generated.h"

class UCableComponent;

UCLASS()
class SKILLPROJECT_API AGrappleCableActor : public AActor
{
    GENERATED_BODY()

public:
    AGrappleCableActor();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void InitCable(ACharacter* InOwnerCharacter, const FVector& InTargetLocation, const FName& InHandBoneName);

    FORCEINLINE FVector GetTargetLocation() const { return TargetLocation; }

private:
    void UpdateCableTransform();

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UCableComponent> CableComponent;

    UPROPERTY(Replicated)
    FVector TargetLocation = FVector::ZeroVector;

    UPROPERTY(Replicated)
    FName HandBoneName;

    UPROPERTY(Replicated)
    TObjectPtr<ACharacter> OwnerCharacter;
};
```

- [ ] **Step 2: GrappleCableActor.cpp 작성**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "GrappleCableActor.h"
#include "CableComponent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GrappleCableActor)

AGrappleCableActor::AGrappleCableActor()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;

    CableComponent = CreateDefaultSubobject<UCableComponent>(TEXT("CableComponent"));
    SetRootComponent(CableComponent);

    CableComponent->bAttachStart = true;
    CableComponent->bAttachEnd   = false;
    CableComponent->NumSegments  = 8;
    CableComponent->CableLength  = 0.f;
    CableComponent->CableWidth   = 2.f;
}

void AGrappleCableActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AGrappleCableActor, TargetLocation);
    DOREPLIFETIME(AGrappleCableActor, HandBoneName);
    DOREPLIFETIME(AGrappleCableActor, OwnerCharacter);
}

void AGrappleCableActor::BeginPlay()
{
    Super::BeginPlay();
    UpdateCableTransform();
}

void AGrappleCableActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateCableTransform();
}

void AGrappleCableActor::InitCable(ACharacter* InOwnerCharacter, const FVector& InTargetLocation, const FName& InHandBoneName)
{
    OwnerCharacter = InOwnerCharacter;
    TargetLocation = InTargetLocation;
    HandBoneName   = InHandBoneName;
    UpdateCableTransform();
}

void AGrappleCableActor::UpdateCableTransform()
{
    if (!OwnerCharacter || !CableComponent) return;

    USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
    if (!Mesh) return;

    FVector HandBoneWorld        = Mesh->GetBoneLocation(HandBoneName);
    SetActorLocation(HandBoneWorld);
    CableComponent->EndLocation  = TargetLocation - HandBoneWorld;
}
```

- [ ] **Step 3: 커밋**

```bash
git add SkillProject/Source/SkillProject/AbilitySystem/Movement/GrappleCableActor.h SkillProject/Source/SkillProject/AbilitySystem/Movement/GrappleCableActor.cpp
git commit -m "[Feature] AGrappleCableActor 구현 — 케이블 시각화 Replicated Actor"
```

---

## Task 5: USpyAbilityTask_GrappleTick 구현

**Files:**
- Create: `SkillProject/Source/SkillProject/AbilitySystem/Movement/SpyAbilityTask_GrappleTick.h`
- Create: `SkillProject/Source/SkillProject/AbilitySystem/Movement/SpyAbilityTask_GrappleTick.cpp`

- [ ] **Step 1: SpyAbilityTask_GrappleTick.h 작성**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "SpyAbilityTask_GrappleTick.generated.h"

class AGrappleCableActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGrappleArrivedDelegate);

UCLASS()
class SKILLPROJECT_API USpyAbilityTask_GrappleTick : public UAbilityTask
{
    GENERATED_BODY()

public:
    USpyAbilityTask_GrappleTick();

    UFUNCTION(BlueprintCallable, Category = "Ability|Tasks",
              meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = true))
    static USpyAbilityTask_GrappleTick* GrappleTick(
        UGameplayAbility* OwningAbility,
        AGrappleCableActor* InCableActor,
        float InArrivalThreshold);

    virtual void Activate() override;
    virtual void TickTask(float DeltaTime) override;

    UPROPERTY(BlueprintAssignable)
    FGrappleArrivedDelegate OnArrived;

private:
    TWeakObjectPtr<AGrappleCableActor> CableActor;
    float ArrivalThreshold = 150.f;
};
```

- [ ] **Step 2: SpyAbilityTask_GrappleTick.cpp 작성**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "SpyAbilityTask_GrappleTick.h"
#include "GrappleCableActor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyAbilityTask_GrappleTick)

USpyAbilityTask_GrappleTick::USpyAbilityTask_GrappleTick()
{
    bTickingTask = true;
}

USpyAbilityTask_GrappleTick* USpyAbilityTask_GrappleTick::GrappleTick(
    UGameplayAbility* OwningAbility,
    AGrappleCableActor* InCableActor,
    float InArrivalThreshold)
{
    USpyAbilityTask_GrappleTick* Task = NewAbilityTask<USpyAbilityTask_GrappleTick>(OwningAbility);
    Task->CableActor       = InCableActor;
    Task->ArrivalThreshold = InArrivalThreshold;
    return Task;
}

void USpyAbilityTask_GrappleTick::Activate()
{
    // Tick handles everything
}

void USpyAbilityTask_GrappleTick::TickTask(float DeltaTime)
{
    Super::TickTask(DeltaTime);

    AActor* Avatar = GetAvatarActor();
    if (!Avatar || !Avatar->HasAuthority()) return;

    if (!CableActor.IsValid())
    {
        EndTask();
        return;
    }

    const float Distance = FVector::Dist(Avatar->GetActorLocation(), CableActor->GetTargetLocation());
    if (Distance <= ArrivalThreshold)
    {
        OnArrived.Broadcast();
        EndTask();
    }
}
```

- [ ] **Step 3: 커밋**

```bash
git add SkillProject/Source/SkillProject/AbilitySystem/Movement/SpyAbilityTask_GrappleTick.h SkillProject/Source/SkillProject/AbilitySystem/Movement/SpyAbilityTask_GrappleTick.cpp
git commit -m "[Feature] USpyAbilityTask_GrappleTick 구현 — 서버 도착 거리 체크 AbilityTask"
```

---

## Task 6: USpyGA_GrappleHook 구현

**Files:**
- Create: `SkillProject/Source/SkillProject/AbilitySystem/Movement/SpyGA_GrappleHook.h`
- Create: `SkillProject/Source/SkillProject/AbilitySystem/Movement/SpyGA_GrappleHook.cpp`

- [ ] **Step 1: SpyGA_GrappleHook.h 작성**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Ability/SKGameplayAbility.h"
#include "SpyGA_GrappleHook.generated.h"

class AGrappleCableActor;
class USpyMovementConfig;

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

    bool TryLineTrace(FVector& OutImpactPoint) const;
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

- [ ] **Step 2: SpyGA_GrappleHook.cpp 작성**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "SpyGA_GrappleHook.h"
#include "GrappleCableActor.h"
#include "SpyAbilityTask_GrappleTick.h"
#include "Data/SpyMovementConfig.h"
#include "Util/SpyGameplayTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
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

    CurrentSpecHandle      = Handle;
    CurrentActorInfo       = ActorInfo;
    CurrentActivationInfo  = ActivationInfo;

    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        ASC->AddLooseGameplayTag(SpyGameplayTags::Lock_Input_Move);
        ASC->AddLooseGameplayTag(SpyGameplayTags::Character_State_Grapple);
    }

    if (!HasAuthority(&ActivationInfo))
        return;

    FVector ImpactPoint;
    if (!TryLineTrace(ImpactPoint))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    ACharacter* Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (!Char)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    LaunchToTarget(Char, ImpactPoint);

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = Char;
    CableActor = GetWorld()->SpawnActor<AGrappleCableActor>(SpawnParams);
    if (CableActor)
    {
        CableActor->InitCable(Char, ImpactPoint, HandBoneName);
    }

    const float Threshold = MovementConfig ? MovementConfig->GrappleArrivalThreshold : 150.f;
    USpyAbilityTask_GrappleTick* Task = USpyAbilityTask_GrappleTick::GrappleTick(this, CableActor, Threshold);
    Task->OnArrived.AddDynamic(this, &USpyGA_GrappleHook::OnGrappleArrived);
    Task->ReadyForActivation();
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

bool USpyGA_GrappleHook::TryLineTrace(FVector& OutImpactPoint) const
{
    APawn* OwnerPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
    APlayerController* PC = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
    if (!PC || !MovementConfig) return false;

    FVector  CamLoc;
    FRotator CamRot;
    PC->GetPlayerViewPoint(CamLoc, CamRot);

    const FVector TraceEnd = CamLoc + CamRot.Vector() * MovementConfig->GrappleMaxRange;

    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(GetAvatarActorFromActorInfo());

    if (!GetWorld()->LineTraceSingleByChannel(HitResult, CamLoc, TraceEnd, ECC_WorldStatic, Params))
        return false;

    OutImpactPoint = HitResult.ImpactPoint;
    return true;
}

void USpyGA_GrappleHook::LaunchToTarget(ACharacter* Character, const FVector& TargetLocation) const
{
    if (!MovementConfig) return;

    const FVector CharLoc   = Character->GetActorLocation();
    const FVector Direction  = (TargetLocation - CharLoc).GetSafeNormal();
    const float   HorzDist   = FVector::Dist2D(CharLoc, TargetLocation);
    const float   Speed      = HorzDist / FMath::Max(MovementConfig->GrappleFlightTime, KINDA_SMALL_NUMBER);

    FVector LaunchVelocity   = Direction * Speed;
    LaunchVelocity.Z        += HorzDist * MovementConfig->GrappleLaunchArcZScale;

    Character->LaunchCharacter(LaunchVelocity, true, true);
}
```

- [ ] **Step 3: 커밋**

```bash
git add SkillProject/Source/SkillProject/AbilitySystem/Movement/SpyGA_GrappleHook.h SkillProject/Source/SkillProject/AbilitySystem/Movement/SpyGA_GrappleHook.cpp
git commit -m "[Feature] USpyGA_GrappleHook 구현 — LocalPredicted 그래플링 훅 GA"
```

---

## Task 7: 에디터에서 DataAsset 설정 및 GA 등록

이 Task는 Unreal Editor에서 수행합니다 (코드 변경 없음).

- [ ] **Step 1: 프로젝트 솔루션 재생성 및 빌드**

`SkillProject/Launch.bat` 실행 후 Visual Studio에서 빌드.  
또는 Unreal Editor > Tools > Refresh Visual Studio Project.

- [ ] **Step 2: SpyMovementConfig DataAsset에 그래플 값 입력**

에디터에서 `Content/Spy/Data/` 경로의 `SpyMovementConfig` DataAsset 열기.  
GrappleHook 카테고리에서:
- `GrappleMaxRange`: 3000
- `GrappleArrivalThreshold`: 150
- `GrappleLaunchArcZScale`: 0.4
- `GrappleFlightTime`: 0.8

- [ ] **Step 3: BP_SpyGA_GrappleHook 블루프린트 생성**

`Content/Spy/Abilities/Movement/` 에서 `SpyGA_GrappleHook`을 부모로 블루프린트 생성.  
`MovementConfig` 프로퍼티에 SpyMovementConfig DataAsset 할당.  
`HandBoneName`은 기본값 `hand_r` 확인.

- [ ] **Step 4: SpyAbilityData에 GA 등록**

캐릭터의 `SpyAbilityData` DataAsset에서 Abilities 배열에 `BP_SpyGA_GrappleHook` 추가.  
`Skill_Move_GrappleHook` 태그를 Ability Tag로 설정.

- [ ] **Step 5: 입력 바인딩**

`SpyInputConfig` DataAsset에서 빈 슬롯을 찾아 Input Tag `Input.Ability.Skill.X`와 `BP_SpyGA_GrappleHook`을 연결.

---

## 멀티플레이어 동작 요약 (참고)

```
[클라이언트] 입력 → GA Activate
  ├─ 즉시: Lock_Input_Move 태그 추가 (로컬)
  ├─ 즉시: Character_State_Grapple 태그 추가 (로컬)
  └─ (서버 확인 대기)

[서버] GA Activate
  ├─ 태그 추가 (서버)
  ├─ LineTrace → 미스 시: EndAbility(true,true) → 클라이언트 태그 제거
  ├─ 히트 시: LaunchCharacter → 캐릭터 날아감 (CharacterMovement가 클라이언트에 레플리케이트)
  ├─ AGrappleCableActor 스폰 (bReplicates=true → 클라이언트에 자동 복제)
  │    └─ 클라이언트: BeginPlay + Tick이 케이블 시각 갱신
  └─ GrappleTick Task 시작 (서버 전용)
       └─ 거리 ≤ 150 → OnArrived → EndAbility(true,false)
            └─ 클라이언트: EndAbility 복제 → 태그 제거
```
