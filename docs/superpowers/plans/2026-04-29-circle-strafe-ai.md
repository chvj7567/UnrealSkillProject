# Circle Strafe AI 구현 계획

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 공격 쿨다운 중 AI가 EQS로 플레이어 측면 위치를 찾아 공전하며, 항상 타겟을 응시하고 좌/우 스트레이프 애니메이션을 출력한다.

**Architecture:** `BTTask_CircleStrafe`가 EQS(`EQS_CircleStrafe`)를 비동기 실행 → Blackboard에 위치 저장 → `AIController->MoveTo` 호출. EQS Dot 테스트의 방향 벡터는 `UEnvQueryContext_StrafeDirection`이 BB의 `bStrafeLeft` 키를 읽어 제공. 공통 AI 헬퍼(`CanMove`, `CanTargetAttack`)는 `SpyAIUtils`로 분리해 양쪽 Task가 공유. `SpyAnimManagerComponent`에 정적 헬퍼 `CalcDirectionFromVelocity`를 추가하고 `SpyCharacterAnimInstance`에서 `DirectionAngle` 변수를 갱신.

**Tech Stack:** Unreal Engine 5.7, C++17, AIModule (EQS / BehaviorTree), GAS(GameplayAbilities)

---

## 파일 맵

| 구분 | 경로 | 역할 |
|------|------|------|
| 신규 | `Source/SkillProject/AI/SpyAIUtils.h` | CanMove / CanTargetAttack 공통 정적 헬퍼 |
| 신규 | `Source/SkillProject/AI/SpyAIUtils.cpp` | |
| 수정 | `Source/SkillProject/AI/BTTask_MoveToTarget.h/.cpp` | SpyAIUtils 사용으로 교체 |
| 신규 | `Source/SkillProject/AI/EnvQueryContext_StrafeDirection.h` | EQS 방향 Context |
| 신규 | `Source/SkillProject/AI/EnvQueryContext_StrafeDirection.cpp` | |
| 신규 | `Source/SkillProject/AI/BTTask_CircleStrafe.h` | 공전 BT Task |
| 신규 | `Source/SkillProject/AI/BTTask_CircleStrafe.cpp` | |
| 신규 | `Source/SkillProject/AI/Tests/SpyAICircleStrafeTests.cpp` | 자동화 테스트 |
| 수정 | `Source/SkillProject/ManagerComponent/SpyAnimManagerComponent.h` | CalcDirectionFromVelocity 정적 메서드 |
| 수정 | `Source/SkillProject/ManagerComponent/SpyAnimManagerComponent.cpp` | |
| 수정 | `Source/SkillProject/Character/AnimInstance/SpyCharacterAnimInstance.h` | DirectionAngle UPROPERTY |
| 수정 | `Source/SkillProject/Character/AnimInstance/SpyCharacterAnimInstance.cpp` | NativeThreadSafeUpdateAnimation에서 갱신 |

---

## Task 1: SpyAIUtils — 공통 헬퍼 분리

**Files:**
- Create: `Source/SkillProject/AI/SpyAIUtils.h`
- Create: `Source/SkillProject/AI/SpyAIUtils.cpp`
- Modify: `Source/SkillProject/AI/BTTask_MoveToTarget.h`
- Modify: `Source/SkillProject/AI/BTTask_MoveToTarget.cpp`

- [ ] **Step 1: SpyAIUtils.h 작성**

```cpp
// Source/SkillProject/AI/SpyAIUtils.h
#pragma once

#include "CoreMinimal.h"

class AAIController;
class ACharacter;
class UBlackboardComponent;

namespace SpyAIUtils
{
    // Death / Lock_Input_Move 태그 체크
    bool CanMove(AAIController* InAIController);

    // 타겟 생존 여부 체크, 사망이면 BB Key 클리어
    bool CanTargetAttack(ACharacter* InTarget, UBlackboardComponent* InBlackboard, FName TargetKeyName);
}
```

- [ ] **Step 2: SpyAIUtils.cpp 작성**

```cpp
// Source/SkillProject/AI/SpyAIUtils.cpp
#include "SpyAIUtils.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Util/SpyGameplayTags.h"

bool SpyAIUtils::CanMove(AAIController* InAIController)
{
    if (!InAIController) return false;

    APawn* Pawn = InAIController->GetPawn();
    if (!Pawn) return false;

    APlayerState* PS = Pawn->GetPlayerState();
    if (!PS) return false;

    UAbilitySystemComponent* ASC = PS->FindComponentByClass<UAbilitySystemComponent>();
    if (!ASC) return false;

    if (ASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Death)) return false;
    if (ASC->HasMatchingGameplayTag(SpyGameplayTags::Lock_Input_Move)) return false;

    return true;
}

bool SpyAIUtils::CanTargetAttack(ACharacter* InTarget, UBlackboardComponent* InBlackboard, FName TargetKeyName)
{
    if (!InTarget) return false;

    APlayerState* PS = InTarget->GetPlayerState();
    if (!PS) return false;

    UAbilitySystemComponent* ASC = PS->FindComponentByClass<UAbilitySystemComponent>();
    if (!ASC) return false;

    if (ASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Death))
    {
        if (InBlackboard)
            InBlackboard->ClearValue(TargetKeyName);
        return false;
    }

    return true;
}
```

- [ ] **Step 3: BTTask_MoveToTarget.h에서 CanMove / CanTargetAttack 선언 제거**

`BTTask_MoveToTarget.h`의 `protected:` 블록에서 아래 두 줄 삭제:
```cpp
bool CanMove(AAIController* InAIController);
bool CanTargetAttack(ACharacter* InTarget);
```

- [ ] **Step 4: BTTask_MoveToTarget.cpp에서 헬퍼 구현을 SpyAIUtils 호출로 교체**

파일 상단 include 추가:
```cpp
#include "SpyAIUtils.h"
```

`ExecuteTask` 내부의 두 if-블록을 교체:
```cpp
// 기존:
if (CanMove(AIController) == false) { ... }
if (CanTargetAttack(Target) == false) { ... }

// 변경:
if (!SpyAIUtils::CanMove(AIController))
{
    AIController->StopMovement();
    return EBTNodeResult::Failed;
}

if (!SpyAIUtils::CanTargetAttack(Target, BlackBoardComp, Key))
{
    AIController->StopMovement();
    return EBTNodeResult::Failed;
}
```

파일 하단의 `CanMove`, `CanTargetAttack` 구현 함수 전체 삭제.

- [ ] **Step 5: 빌드 확인**

Visual Studio에서 Development Editor / Win64 빌드 실행 후 오류 없음 확인.

- [ ] **Step 6: Commit**

```
Stage: BTTask_MoveToTarget.h/.cpp, SpyAIUtils.h/.cpp
Message: [Refactor] BTTask_MoveToTarget — CanMove/CanTargetAttack를 SpyAIUtils로 분리
```

---

## Task 2: UEnvQueryContext_StrafeDirection

**Files:**
- Create: `Source/SkillProject/AI/EnvQueryContext_StrafeDirection.h`
- Create: `Source/SkillProject/AI/EnvQueryContext_StrafeDirection.cpp`
- Create: `Source/SkillProject/AI/Tests/SpyAICircleStrafeTests.cpp` (방향 벡터 수학 테스트)

- [ ] **Step 1: 테스트 파일 작성 (실패 확인용)**

```cpp
// Source/SkillProject/AI/Tests/SpyAICircleStrafeTests.cpp
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Math/Vector.h"

// 스트레이프 방향 벡터 수학 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSpyStrafeDirVectorTest,
    "SkillProject.AI.CircleStrafe.StrafeDirVector",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpyStrafeDirVectorTest::RunTest(const FString& Parameters)
{
    // Querier=(0,0,0), Target=(100,0,0) → Forward=(1,0,0)
    // RightVec = Cross(Up, Forward) = Cross((0,0,1),(1,0,0)) = (0*0-1*0, 1*1-0*0, 0*0-0*1) = (0,1,0)
    FVector QuerierLoc(0.f, 0.f, 0.f);
    FVector TargetLoc(100.f, 0.f, 0.f);
    FVector Forward = (TargetLoc - QuerierLoc).GetSafeNormal2D();
    FVector RightVec = FVector::CrossProduct(FVector::UpVector, Forward);

    // 우측 공전: RightVec.Y == +1
    TestNearlyEqual(TEXT("Strafe right X"), RightVec.X, 0.f, 0.01f);
    TestNearlyEqual(TEXT("Strafe right Y"), RightVec.Y, 1.f, 0.01f);
    TestNearlyEqual(TEXT("Strafe right Z"), RightVec.Z, 0.f, 0.01f);

    // 좌측 공전: 부호 반전
    FVector LeftVec = -RightVec;
    TestNearlyEqual(TEXT("Strafe left Y"), LeftVec.Y, -1.f, 0.01f);

    // 45도 방향 타겟 (Forward = normalize(1,1,0))
    FVector Target45 = FVector(100.f, 100.f, 0.f);
    FVector Forward45 = (Target45 - QuerierLoc).GetSafeNormal2D();
    FVector Right45 = FVector::CrossProduct(FVector::UpVector, Forward45);
    TestNearlyEqual(TEXT("45deg strafe length"), Right45.Size(), 1.f, 0.01f);
    // Right45 perpendicular to Forward45: Dot == 0
    TestNearlyEqual(TEXT("45deg perp"), FVector::DotProduct(Forward45, Right45), 0.f, 0.01f);

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
```

- [ ] **Step 2: 에디터에서 테스트 실행 (실패 예상)**

에디터 메뉴: Window → Test Automation → 필터 `SkillProject.AI.CircleStrafe` → Run
(이 단계에서는 테스트 파일만 존재하므로 Passed여야 함 — 순수 수학 테스트)

- [ ] **Step 3: EnvQueryContext_StrafeDirection.h 작성**

```cpp
// Source/SkillProject/AI/EnvQueryContext_StrafeDirection.h
#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvQueryContext_StrafeDirection.generated.h"

// BB의 bStrafeLeft 키를 읽어 Querier→Target 방향의 수직(좌/우) 벡터 위치를 반환한다.
// EQS Dot 테스트의 Line A 컨텍스트로 사용한다.
UCLASS()
class SKILLPROJECT_API UEnvQueryContext_StrafeDirection : public UEnvQueryContext
{
    GENERATED_BODY()

public:
    virtual void ProvideContext(FEnvQueryInstance& QueryInstance,
                                FEnvQueryContextData& ContextData) const override;

    // BB에서 읽을 Target Actor 키 이름
    UPROPERTY(EditDefaultsOnly, Category = "EQS")
    FName TargetKeyName = TEXT("Target");

    // BB에서 읽을 Bool 키 이름 (true = 좌측, false = 우측)
    UPROPERTY(EditDefaultsOnly, Category = "EQS")
    FName StrafeLeftKeyName = TEXT("bStrafeLeft");

    // 방향 벡터를 나타낼 오프셋 거리 (EQS 스코어링에 영향 없음, 단위: cm)
    UPROPERTY(EditDefaultsOnly, Category = "EQS")
    float ContextOffset = 100.f;
};
```

- [ ] **Step 4: EnvQueryContext_StrafeDirection.cpp 작성**

```cpp
// Source/SkillProject/AI/EnvQueryContext_StrafeDirection.cpp
#include "EnvQueryContext_StrafeDirection.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EnvQueryContext_StrafeDirection)

void UEnvQueryContext_StrafeDirection::ProvideContext(
    FEnvQueryInstance& QueryInstance,
    FEnvQueryContextData& ContextData) const
{
    APawn* QuerierPawn = Cast<APawn>(QueryInstance.Owner.Get());
    if (!QuerierPawn) return;

    AAIController* AIController = Cast<AAIController>(QuerierPawn->GetController());
    if (!AIController) return;

    UBlackboardComponent* BB = AIController->GetBlackboardComponent();
    if (!BB) return;

    AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(TargetKeyName));
    if (!TargetActor) return;

    bool bStrafeLeft = BB->GetValueAsBool(StrafeLeftKeyName);

    FVector QuerierLoc = QuerierPawn->GetActorLocation();
    FVector TargetLoc  = TargetActor->GetActorLocation();

    // Forward: Querier → Target (Z 무시)
    FVector Forward = (TargetLoc - QuerierLoc).GetSafeNormal2D();
    if (Forward.IsNearlyZero()) return;

    // Cross(Up, Forward) = 오른쪽 수직 벡터 (UE 좌표계: X=Forward, Y=Right, Z=Up)
    FVector RightVec = FVector::CrossProduct(FVector::UpVector, Forward);
    FVector StrafeDir = bStrafeLeft ? -RightVec : RightVec;

    FVector ContextLocation = QuerierLoc + StrafeDir * ContextOffset;
    UEnvQueryItemType_Point::SetContextHelper(ContextData, ContextLocation);
}
```

- [ ] **Step 5: 빌드 확인**

Visual Studio에서 빌드 후 오류 없음 확인.

- [ ] **Step 6: Commit**

```
Stage: EnvQueryContext_StrafeDirection.h/.cpp, Tests/SpyAICircleStrafeTests.cpp
Message: [Feature] EnvQueryContext_StrafeDirection — EQS 스트레이프 방향 컨텍스트 추가
```

---

## Task 3: UBTTask_CircleStrafe

**Files:**
- Create: `Source/SkillProject/AI/BTTask_CircleStrafe.h`
- Create: `Source/SkillProject/AI/BTTask_CircleStrafe.cpp`
- Modify: `Source/SkillProject/AI/Tests/SpyAICircleStrafeTests.cpp` (CalcDirectionAngle 테스트 추가 — Task 4 이후 실행)

- [ ] **Step 1: BTTask_CircleStrafe.h 작성**

```cpp
// Source/SkillProject/AI/BTTask_CircleStrafe.h
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "BTTask_CircleStrafe.generated.h"

class UEnvQuery;
class UBehaviorTreeComponent;

UCLASS()
class SKILLPROJECT_API UBTTask_CircleStrafe : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_CircleStrafe();

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp,
                                            uint8* NodeMemory) override;
    virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp,
                                          uint8* NodeMemory) override;

private:
    void OnQueryFinished(TSharedPtr<FEnvQueryResult> Result,
                         TWeakObjectPtr<UBehaviorTreeComponent> WeakOwner);

    void DrawDebugEQSResults(UWorld* World,
                             const TSharedPtr<FEnvQueryResult>& Result) const;

protected:
    // 에디터에서 EQS_CircleStrafe 에셋을 연결
    UPROPERTY(EditAnywhere, Category = "EQS")
    TObjectPtr<UEnvQuery> EQSQuery;

    UPROPERTY(EditAnywhere, Category = "Config")
    FBlackboardKeySelector TargetKey;

    UPROPERTY(EditAnywhere, Category = "Config")
    FBlackboardKeySelector StrafeLocationKey;

    UPROPERTY(EditAnywhere, Category = "Config")
    FBlackboardKeySelector StrafeLeftKey;

    UPROPERTY(EditAnywhere, Category = "Config")
    float StrafeAcceptanceRadius = 50.f;

    UPROPERTY(EditAnywhere, Category = "Debug")
    float DebugDrawDuration = 2.f;

private:
    int32 ActiveQueryId = INDEX_NONE;
};
```

- [ ] **Step 2: BTTask_CircleStrafe.cpp 작성**

```cpp
// Source/SkillProject/AI/BTTask_CircleStrafe.cpp
#include "BTTask_CircleStrafe.h"
#include "SpyAIUtils.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BrainComponent.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "GameFramework/Character.h"
#include "Navigation/PathFollowingComponent.h"
#include "DrawDebugHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BTTask_CircleStrafe)

UBTTask_CircleStrafe::UBTTask_CircleStrafe()
{
    NodeName = TEXT("Circle Strafe");

    TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_CircleStrafe, TargetKey), ACharacter::StaticClass());
    StrafeLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_CircleStrafe, StrafeLocationKey));
    StrafeLeftKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_CircleStrafe, StrafeLeftKey));
}

EBTNodeResult::Type UBTTask_CircleStrafe::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!AIController || !BB) return EBTNodeResult::Failed;

    ACharacter* Target = Cast<ACharacter>(BB->GetValueAsObject(TargetKey.SelectedKeyName));

    if (!SpyAIUtils::CanMove(AIController))
    {
        AIController->StopMovement();
        return EBTNodeResult::Failed;
    }

    if (!SpyAIUtils::CanTargetAttack(Target, BB, TargetKey.SelectedKeyName))
    {
        AIController->StopMovement();
        return EBTNodeResult::Failed;
    }

    if (!EQSQuery) return EBTNodeResult::Failed;

    // 매 실행마다 랜덤으로 좌/우 결정
    BB->SetValueAsBool(StrafeLeftKey.SelectedKeyName, FMath::RandBool());

    // EQS 비동기 실행
    UEnvQueryManager* EQSManager = UEnvQueryManager::GetCurrent(GetWorld());
    if (!EQSManager) return EBTNodeResult::Failed;

    TWeakObjectPtr<UBehaviorTreeComponent> WeakOwner = &OwnerComp;

    FEnvQueryRequest QueryRequest(EQSQuery, AIController->GetPawn());
    FQueryFinishedSignature FinishedDelegate = FQueryFinishedSignature::CreateWeakLambda(
        this,
        [this, WeakOwner](TSharedPtr<FEnvQueryResult> Result)
        {
            OnQueryFinished(Result, WeakOwner);
        });
    ActiveQueryId = QueryRequest.Execute(EEnvQueryRunMode::SingleResult, FinishedDelegate);

    return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UBTTask_CircleStrafe::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    if (ActiveQueryId != INDEX_NONE)
    {
        UEnvQueryManager* EQSManager = UEnvQueryManager::GetCurrent(GetWorld());
        if (EQSManager)
        {
            // UE5 버전에 따라 AbortQuery(int32) 또는 RemoveAllQueriesByOwner(UObject*) 사용
            EQSManager->AbortQuery(ActiveQueryId);
        }
        ActiveQueryId = INDEX_NONE;
    }

    AAIController* AIController = OwnerComp.GetAIOwner();
    if (AIController)
        AIController->StopMovement();

    return Super::AbortTask(OwnerComp, NodeMemory);
}

void UBTTask_CircleStrafe::OnQueryFinished(
    TSharedPtr<FEnvQueryResult> Result,
    TWeakObjectPtr<UBehaviorTreeComponent> WeakOwner)
{
    ActiveQueryId = INDEX_NONE;

    if (!WeakOwner.IsValid()) return;
    UBehaviorTreeComponent* OwnerComp = WeakOwner.Get();

    // 디버그 시각화
    DrawDebugEQSResults(GetWorld(), Result);

    if (!Result.IsValid() || Result->IsSuccessful() == false || Result->Items.IsEmpty())
    {
        FinishLatentTask(*OwnerComp, EBTNodeResult::Failed);
        return;
    }

    FVector StrafeLocation = Result->GetItemAsLocation(0);

    UBlackboardComponent* BB = OwnerComp->GetBlackboardComponent();
    if (BB)
        BB->SetValueAsVector(StrafeLocationKey.SelectedKeyName, StrafeLocation);

    AAIController* AIController = OwnerComp->GetAIOwner();
    if (!AIController)
    {
        FinishLatentTask(*OwnerComp, EBTNodeResult::Failed);
        return;
    }

    // 이동 중 항상 타겟 응시
    UBlackboardComponent* BBComp = OwnerComp->GetBlackboardComponent();
    if (AActor* Target = BBComp ? Cast<AActor>(BBComp->GetValueAsObject(TargetKey.SelectedKeyName)) : nullptr)
        AIController->SetFocus(Target);

    FAIMoveRequest MoveReq(StrafeLocation);
    MoveReq.SetAcceptanceRadius(StrafeAcceptanceRadius);
    MoveReq.SetUsePathfinding(true);

    FPathFollowingRequestResult MoveResult = AIController->MoveTo(MoveReq);

    if (MoveResult.Code == EPathFollowingRequestResult::RequestSuccessful)
    {
        WaitForMessage(*OwnerComp, UBrainComponent::AIMessage_MoveFinished);
    }
    else if (MoveResult.Code == EPathFollowingRequestResult::AlreadyAtGoal)
    {
        FinishLatentTask(*OwnerComp, EBTNodeResult::Succeeded);
    }
    else
    {
        FinishLatentTask(*OwnerComp, EBTNodeResult::Failed);
    }
}

void UBTTask_CircleStrafe::DrawDebugEQSResults(
    UWorld* World,
    const TSharedPtr<FEnvQueryResult>& Result) const
{
#if ENABLE_DRAW_DEBUG
    if (!World || !Result.IsValid()) return;

    for (int32 i = 0; i < Result->Items.Num(); ++i)
    {
        const FEnvQueryItem& Item = Result->Items[i];
        FVector Loc = Result->GetItemAsLocation(i);

        float Score = Item.Score;
        bool bValid = !Item.IsDiscarded();

        FColor Color = bValid ? FColor::Green : FColor::Red;
        DrawDebugSphere(World, Loc, 30.f, 8, Color, false, DebugDrawDuration);
        DrawDebugString(World, Loc + FVector(0.f, 0.f, 50.f),
                        FString::Printf(TEXT("%.2f"), Score),
                        nullptr, FColor::White, DebugDrawDuration);
    }

    // 최적 위치 강조
    if (Result->IsSuccessful() && !Result->Items.IsEmpty())
    {
        FVector BestLoc = Result->GetItemAsLocation(0);
        DrawDebugSphere(World, BestLoc, 50.f, 12, FColor::Yellow, false, DebugDrawDuration);
    }
#endif
}
```

- [ ] **Step 3: 빌드 확인**

빌드 후 오류 없음 확인. 특히 `UEnvQueryManager::AbortQuery` 시그니처 불일치 시:
```cpp
// AbortQuery 대신 아래로 대체:
EQSManager->RemoveAllQueriesByOwner(this);
```

- [ ] **Step 4: Commit**

```
Stage: BTTask_CircleStrafe.h/.cpp
Message: [Feature] BTTask_CircleStrafe — EQS 기반 공전 BT Task 추가
```

---

## Task 4: SpyAnimManagerComponent — Direction 계산 추가

**Files:**
- Modify: `Source/SkillProject/ManagerComponent/SpyAnimManagerComponent.h`
- Modify: `Source/SkillProject/ManagerComponent/SpyAnimManagerComponent.cpp`
- Modify: `Source/SkillProject/Character/AnimInstance/SpyCharacterAnimInstance.h`
- Modify: `Source/SkillProject/Character/AnimInstance/SpyCharacterAnimInstance.cpp`
- Modify: `Source/SkillProject/AI/Tests/SpyAICircleStrafeTests.cpp`

- [ ] **Step 1: 테스트 코드 추가 (CalcDirectionFromVelocity)**

`SpyAICircleStrafeTests.cpp` 하단(`#endif` 앞)에 추가:

```cpp
#include "ManagerComponent/SpyAnimManagerComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSpyCalcDirectionTest,
    "SkillProject.AI.CircleStrafe.CalcDirection",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpyCalcDirectionTest::RunTest(const FString& Parameters)
{
    // 전방 이동 (Velocity=+X, Rotation=0) → 0도
    float Dir = USpyAnimManagerComponent::CalcDirectionFromVelocity(
        FVector(100.f, 0.f, 0.f), FRotator::ZeroRotator);
    TestNearlyEqual(TEXT("Forward = 0 deg"), Dir, 0.f, 0.5f);

    // 우측 이동 (Velocity=+Y) → +90도
    Dir = USpyAnimManagerComponent::CalcDirectionFromVelocity(
        FVector(0.f, 100.f, 0.f), FRotator::ZeroRotator);
    TestNearlyEqual(TEXT("Right = +90 deg"), Dir, 90.f, 0.5f);

    // 좌측 이동 (Velocity=-Y) → -90도
    Dir = USpyAnimManagerComponent::CalcDirectionFromVelocity(
        FVector(0.f, -100.f, 0.f), FRotator::ZeroRotator);
    TestNearlyEqual(TEXT("Left = -90 deg"), Dir, -90.f, 0.5f);

    // 후방 이동 (Velocity=-X) → ±180도 근사
    Dir = USpyAnimManagerComponent::CalcDirectionFromVelocity(
        FVector(-100.f, 0.f, 0.f), FRotator::ZeroRotator);
    TestTrue(TEXT("Backward near ±180"), FMath::Abs(Dir) > 170.f);

    // 액터가 90도 회전된 상태에서 World +X 이동 → 로컬로는 -Y → -90도
    Dir = USpyAnimManagerComponent::CalcDirectionFromVelocity(
        FVector(100.f, 0.f, 0.f), FRotator(0.f, 90.f, 0.f));
    TestNearlyEqual(TEXT("Rotated actor right = -90 deg"), Dir, -90.f, 0.5f);

    return true;
}
```

- [ ] **Step 2: 에디터에서 테스트 실행 (실패 예상 — CalcDirectionFromVelocity 미구현)**

Window → Test Automation → `SkillProject.AI.CircleStrafe.CalcDirection` → Run → FAIL 확인.

- [ ] **Step 3: SpyAnimManagerComponent.h에 정적 메서드 선언 추가**

기존 `public:` 섹션에 추가:
```cpp
public:
    // Velocity를 ActorRotation 기준 로컬 공간으로 변환해 방향각(-180~180도)을 반환한다.
    // 전방=0, 우=+90, 좌=-90, 후=±180
    static float CalcDirectionFromVelocity(const FVector& WorldVelocity, const FRotator& ActorRotation);
```

- [ ] **Step 4: SpyAnimManagerComponent.cpp에 구현 추가**

파일 하단에 추가:
```cpp
float USpyAnimManagerComponent::CalcDirectionFromVelocity(
    const FVector& WorldVelocity, const FRotator& ActorRotation)
{
    if (WorldVelocity.IsNearlyZero()) return 0.f;

    FVector LocalVel = ActorRotation.UnrotateVector(WorldVelocity);
    return FMath::RadiansToDegrees(FMath::Atan2(LocalVel.Y, LocalVel.X));
}
```

- [ ] **Step 5: 테스트 재실행 (Pass 예상)**

Window → Test Automation → `SkillProject.AI.CircleStrafe.CalcDirection` → Run → PASS 확인.

- [ ] **Step 6: SpyCharacterAnimInstance.h에 DirectionAngle 프로퍼티 추가**

기존 `UPROPERTY(BlueprintReadOnly)` 블록에 추가:
```cpp
UPROPERTY(BlueprintReadOnly)
float DirectionAngle;
```

- [ ] **Step 7: SpyCharacterAnimInstance.cpp에서 DirectionAngle 갱신**

`NativeThreadSafeUpdateAnimation` 내부, `Velocity` 계산 블록 바로 뒤에 추가:
```cpp
//# Set DirectionAngle (스트레이프 애니메이션용)
{
    DirectionAngle = USpyAnimManagerComponent::CalcDirectionFromVelocity(
        Velocity, Player->GetActorRotation());
}
```

상단 include에 추가 (아직 없으면):
```cpp
#include "ManagerComponent/SpyAnimManagerComponent.h"
```

- [ ] **Step 8: 빌드 확인**

빌드 후 오류 없음 확인.

- [ ] **Step 9: Commit**

```
Stage: SpyAnimManagerComponent.h/.cpp, SpyCharacterAnimInstance.h/.cpp, Tests/SpyAICircleStrafeTests.cpp
Message: [Feature] SpyAnimManagerComponent — CalcDirectionFromVelocity 추가, AnimInstance DirectionAngle 연동
```

---

## Task 5: 에디터 설정 (BB Keys / EQS Asset / BT / ABP)

이 Task는 Unreal Editor에서 수동으로 수행한다.

### 5-1. BB_SpyAI 키 추가

- [ ] `Content/Spy/AI/BB_SpyAI` 열기
- [ ] Key 추가: Name=`StrafeLocation`, Type=`Vector`
- [ ] Key 추가: Name=`bStrafeLeft`, Type=`Bool`
- [ ] 저장

### 5-2. EQS_CircleStrafe 에셋 생성

- [ ] Content Browser → `Content/Spy/AI/` 우클릭 → Artificial Intelligence → Environment Query → 이름 `EQS_CircleStrafe`
- [ ] 열기 후 다음 설정:

**Generator:**
- 우클릭 → Add Generator → `Donut`
- Center: `Querier` (기본값)
- Inner Radius: `500`
- Outer Radius: `800`
- Number of Rings: `2`
- Points Per Ring: `16`

**Test 1 — Distance:**
- Generator 노드 우클릭 → Add Test → `Distance`
- Distance To: `Target`
- Test Purpose: `Filter and Score`
- Filter Type: `Range`, Min=`500`, Max=`800`
- Scoring Factor: `1.0`, Scoring Equation: `Linear`

**Test 2 — Trace:**
- Add Test → `Trace`
- Trace Channel: `Visibility`
- Context: `Target` (BB에서 읽어오는 액터)
- Test Purpose: `Filter Only`
- Bool Match: `True` (시야 확보된 포인트만 통과)

**Test 3 — Dot:**
- Add Test → `Dot`
- Line A: `Two Points`, From=`Querier`, To=`EnvQueryContext_StrafeDirection`
- Line B: `Two Points`, From=`Querier`, To=`Item`
- Test Purpose: `Score Only`
- Scoring Factor: `2.0` (Distance, Trace보다 가중치 높게)

- [ ] 저장

### 5-3. BT_SpyAI 수정

- [ ] `Content/Spy/AI/BT_SpyAI` 열기
- [ ] 기존 이동 노드 위에 Selector 추가
- [ ] Selector의 첫 번째 자식: `Sequence`
  - Decorator 추가 → `Blackboard` (또는 커스텀 쿨다운 Decorator)
    - Blackboard Key: 쿨다운 Bool 키 (기존 쿨다운 구현에 맞게 설정)
  - Task 추가 → `Circle Strafe`
    - EQS Query: `EQS_CircleStrafe`
    - Target Key: `Target`
    - Strafe Location Key: `StrafeLocation`
    - Strafe Left Key: `bStrafeLeft`
    - Strafe Acceptance Radius: `50`
  - Task 추가 → `Wait` (Duration: 0.1)
- [ ] Selector의 두 번째 자식: 기존 `Move To Target` 노드
- [ ] 저장

### 5-4. ABP_SpyCharacter_Base에 DirectionAngle 연결

- [ ] `Content/Spy/Animation/ABP_SpyCharacter_Base` 열기
- [ ] Event Graph → `NativeUpdateAnimation` 또는 AnimBP가 사용하는 업데이트 노드에서:
  - `Get Owning Pawn` → `Get Component By Class (SpyAnimManagerComponent)` → `CalcDirectionFromVelocity`
  - **또는** BlueprintReadOnly인 `DirectionAngle`을 `SpyCharacterAnimInstance`에서 직접 읽기 (프로퍼티 접근)
- [ ] BS_Movement_Targeting 노드의 `Direction` 핀에 `DirectionAngle` 변수 연결
- [ ] 저장 및 컴파일

### 5-5. AI 캐릭터 CharacterMovementComponent 설정

- [ ] AI 봇 캐릭터 BP(`BP_SpyCharacter_A` 또는 AI 전용 BP) 열기
- [ ] CharacterMovement 컴포넌트 선택 → Details:
  - `Orient Rotation to Movement`: **False**
  - `Use Controller Desired Rotation`: **False**
- [ ] 저장

---

## Task 6: PIE 통합 테스트

- [ ] **Step 1: PIE 실행**

에디터에서 Play → AI 봇 스폰 확인

- [ ] **Step 2: EQS 디버그 확인**

콘솔 열기(`~`) → `ShowDebug AI` 입력  
AI 봇 선택 → EQS 탭에서 도넛 형태의 포인트 분포 확인

- [ ] **Step 3: DrawDebug 구 확인**

AI가 공전 상태 진입 시 (공격 쿨다운 Decorator 조건 충족):
- 초록 구체: 유효 EQS 포인트 + 점수 텍스트
- 빨간 구체: Trace 탈락 포인트
- 노랑 구체: 선택된 목적지

- [ ] **Step 4: 공전 동작 확인**

- AI가 플레이어 측면(500~800 거리)으로 이동하는지 확인
- 이동 중 AI가 항상 플레이어를 바라보는지 확인 (`SetFocus` 동작)
- 매 공전마다 좌/우 방향이 랜덤으로 바뀌는지 확인

- [ ] **Step 5: 애니메이션 확인**

- AI가 좌측 공전 시 `DirectionAngle` < 0 → BS_Movement_Targeting 좌측 애니메이션 출력
- AI가 우측 공전 시 `DirectionAngle` > 0 → 우측 애니메이션 출력
- Persona 또는 AnimDebug(`ShowDebug Animation`)로 DirectionAngle 값 확인

- [ ] **Step 6: Commit**

```
Stage: (에디터 에셋은 자동 감지)
Message: [Feature] CircleStrafe — EQS 공전 AI 완성: EQS, BT, ABP 에디터 설정 적용
```
