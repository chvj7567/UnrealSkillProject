# 오브젝트 상호작용 미션 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 레벨에 배치된 단일 액터를 F키로 상호작용해 완료되는 새 미션 타입(`ESpyMissionType::Interact`)을 추가한다. NPC 대화 없이, 상호작용 즉시 미션 진행도만 조용히 갱신되고 오브젝트는 1회성으로 소진된다.

**Architecture:** 신규 `Interactable/` 도메인(`ASpyInteractableObject` + `ISpyInteractableRoot`)을 NPC 파이프라인과 완전히 분리해 병렬로 추가한다. `USpyMissionComponent`의 자동 수락/보상 경고 조건을 `Interact` 타입까지 확장하고, `USpyInteractionComponent`에 NPC와 별개인 두 번째 추적 슬롯(`NearbyInteractable`)과 전용 RPC(`Server_RequestInteractObject`, void — Client RPC 없음)를 추가한다.

**Tech Stack:** Unreal Engine 5.7, C++, GAS(GameplayTag 매칭), Unreal Automation Test.

**Spec:** `docs/superpowers/specs/2026-08-03-interactable-object-mission-design.md` (사용자 승인 완료)

## Global Constraints

- 코딩 스타일은 `.claude/rules/cpp-style.md`를 그대로 따른다 — 특히: 한 줄 주석은 `//#`만 사용, `auto`/`!` 금지(`==false`/`==nullptr`로 명시), 가드 절은 중괄호 없이 개행, `TObjectPtr<>` 사용, include 순서(자기 헤더 → UE 헤더 → 프로젝트 헤더 → `.generated.h`), 헤더 include 정렬은 `SortIncludes: Never`라 clang-format이 건드리지 않으므로 그대로 유지.
- 커밋 메시지 포맷: `[Tag] ClassName — 요약` (`.claude/rules/git-conventions.md`). **`git commit`을 직접 실행하지 않는다** — 각 태스크 끝에서 관련 파일만 `git add`하고 커밋 메시지(안)를 제시한다. 최종 커밋은 사용자가 수행한다.
- 서버 권한 상태 변경은 `HasAuthority()` 체크 후 처리한다 (`.claude/rules/unreal-infra.md` §2).
- 새 게임플레이 태그는 `SpyGameplayTags.h`에 `UE_DECLARE_GAMEPLAY_TAG_EXTERN` + `.cpp`에 `UE_DEFINE_GAMEPLAY_TAG`로 등록한다. 문자열 리터럴로 태그를 직접 참조하지 않는다.
- **이 프로젝트의 Automation 테스트는 순수 함수(부수효과 없는 static/free 함수)만 대상으로 한다** — `HasAuthority()`가 필요한 액터/컴포넌트 메서드(`RequestInteract`, `AddProgress`, `ProcessProgress`, `GrantReward` 등)는 자동화 테스트로 검증할 수 없다(기존 `SpyMissionTests.cpp`/`SpyNPCDialogueEdgeCaseTests.cpp` 전례 확인됨 — 전부 `ResolveMissionProgress`/`ResolveNPCDialogueState` 같은 순수 함수만 테스트한다). 이번 계획에서 이 범주에 드는 항목은 **PIE 확인**으로 대체하며, 각 태스크에서 명시한다.
- 컴파일 검증은 Unreal Editor 또는 Visual Studio에서 사용자가 수행한다(전용 빌드 커맨드 없음, `.claude/project.md` "mcp" 항목). 각 태스크는 "빌드해서 컴파일 통과 확인" 스텝을 명시하되, 실제 빌드 실행은 사용자 몫이다.

---

### Task 1: `ESpyMissionType::Interact` 태그·enum 추가

**Files:**
- Modify: `SkillProject/Source/SkillProject/Util/SpyGameplayTags.h:132-136`
- Modify: `SkillProject/Source/SkillProject/Util/SpyGameplayTags.cpp:107-111`
- Modify: `SkillProject/Source/SkillProject/Util/DefineEnum.h:28-35`

**Interfaces:**
- Produces: `SpyGameplayTags::Event_Mission_Interact` (FGameplayTag, `"Event.Mission.Interact"`), `ESpyMissionType::Interact` (enum value) — Task 2·4가 소비한다.

- [ ] **Step 1: `SpyGameplayTags.h`에 태그 선언 추가**

`Util/SpyGameplayTags.h:136` (`Event_Mission_Report` 선언 바로 다음 줄)에 추가:

```cpp
	SKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Mission_Interact);
```

**Step 2: `SpyGameplayTags.cpp`에 태그 정의 추가**

`Util/SpyGameplayTags.cpp:111` (`Event_Mission_Report` 정의 바로 다음 줄)에 추가:

```cpp
	UE_DEFINE_GAMEPLAY_TAG(Event_Mission_Interact, "Event.Mission.Interact");
```

- [ ] **Step 3: `DefineEnum.h`의 `ESpyMissionType`에 `Interact` 추가**

`Util/DefineEnum.h:28-35`의 기존 블록을 아래로 교체:

```cpp
//# 미션 1개의 수락 방식을 가른다. Gameplay는 NPC Offer 카드로 수동 수락,
//# Dialogue/Interact는 배열 진입과 동시에 자동 수락된다(카드를 보여줄 주체가 없다)
UENUM(BlueprintType)
enum class ESpyMissionType : uint8
{
	Gameplay,
	Dialogue,
	Interact,
};
```

- [ ] **Step 4: 컴파일 확인**

Unreal Editor 또는 Visual Studio에서 빌드해 컴파일 에러가 없는지 확인한다(신규 태그/enum 값 추가만이라 기존 `switch`문 등에서 경고가 나지 않는지도 함께 확인 — `ESpyMissionType`을 `switch`로 처리하는 코드가 있으면 `Interact` 케이스 누락 경고가 뜰 수 있다).

- [ ] **Step 5: git add + 커밋 메시지(안) 제시**

```bash
git add SkillProject/Source/SkillProject/Util/SpyGameplayTags.h SkillProject/Source/SkillProject/Util/SpyGameplayTags.cpp SkillProject/Source/SkillProject/Util/DefineEnum.h
```

커밋 메시지(안): `[Feature] ESpyMissionType — Interact 타입·Event_Mission_Interact 태그 추가`

---

### Task 2: `USpyMissionComponent` 자동 수락·보상 경고 조건 확장

**Files:**
- Modify: `SkillProject/Source/SkillProject/System/SpyMissionComponent.cpp:167-169` (`ProcessProgress`)
- Modify: `SkillProject/Source/SkillProject/System/SpyMissionComponent.cpp:236-246` (`GrantReward`)

**Interfaces:**
- Consumes: `ESpyMissionType::Interact` (Task 1).
- Produces: 없음(동작 변경만, 새 공개 API 없음).

- [ ] **Step 1: `ProcessProgress`의 자동 수락 조건 확장**

`System/SpyMissionComponent.cpp:167-169` 현재 코드:

```cpp
	//# 새로 진입한 미션이 Dialogue 타입이면 자동 수락 — NPC Offer 절차가 없다
	const FSpyMissionEntry* NewEntry = GetMissionEntry(MissionState.MissionIndex);
	MissionState.bAccepted = (NewEntry != nullptr && NewEntry->MissionType == ESpyMissionType::Dialogue);
```

아래로 교체:

```cpp
	//# 새로 진입한 미션이 Dialogue/Interact 타입이면 자동 수락 — 카드를 보여줄 NPC가 없다
	const FSpyMissionEntry* NewEntry = GetMissionEntry(MissionState.MissionIndex);
	MissionState.bAccepted = (NewEntry != nullptr &&
		(NewEntry->MissionType == ESpyMissionType::Dialogue || NewEntry->MissionType == ESpyMissionType::Interact));
```

- [ ] **Step 2: `GrantReward`의 보상 누락 경고 조건 확장**

`System/SpyMissionComponent.cpp:236-246` 현재 코드:

```cpp
	if (Reward <= 0.f)
	{
		//# Gameplay 타입은 보상이 없는 게 정상이다. Dialogue 타입인데 0이면
		//# MissionReward 행을 빠뜨린 에디터 데이터 실수일 수밖에 없다 — 경고로 구분한다
		if (Entry->MissionType == ESpyMissionType::Dialogue)
		{
			UE_LOG(LogTemp, Warning, TEXT("# [SpyMissionComponent] Dialogue 미션 %d 의 MissionReward 행이 없습니다 (데이터 누락 의심): %s"), InCompletedIndex, *GetNameSafe(GetOwner()));
		}

		return;
	}
```

아래로 교체:

```cpp
	if (Reward <= 0.f)
	{
		//# Gameplay 타입은 보상이 없는 게 정상이다. Dialogue/Interact 타입인데 0이면
		//# MissionReward 행을 빠뜨린 에디터 데이터 실수일 수밖에 없다 — 경고로 구분한다
		if (Entry->MissionType == ESpyMissionType::Dialogue || Entry->MissionType == ESpyMissionType::Interact)
		{
			UE_LOG(LogTemp, Warning, TEXT("# [SpyMissionComponent] Dialogue/Interact 미션 %d 의 MissionReward 행이 없습니다 (데이터 누락 의심): %s"), InCompletedIndex, *GetNameSafe(GetOwner()));
		}

		return;
	}
```

- [ ] **Step 3: 컴파일 확인**

두 조건문 모두 괄호 짝과 `||` 연산자가 올바른지, 가드 절이 아닌 일반 `if` 블록이라 중괄호가 그대로 있는지(§5 — 이 블록은 가드 절이 아니라 조건부 로그 출력이므로 중괄호 유지가 맞다) 확인 후 빌드.

- [ ] **Step 4: PIE 확인 항목 기록 (자동화 테스트 불가 — Global Constraints 참조)**

이 태스크의 실제 동작(자동 수락, 경고 로그)은 `HasAuthority()`가 필요한 컴포넌트 메서드라 Automation으로 검증 불가하다. Task 6 완료 후 진행할 인게임 확인 목록(Task 6 Step 6)에 아래 두 항목을 포함시킨다:
- `Interact` 타입 미션 인덱스 진입 시 카드 없이 즉시 `bAccepted == true`(HUD에 목표 문구가 뜨는지로 간접 확인)
- `MissionRewardTable`에 해당 `MissionId` 행을 빠뜨린 채 완료시켜 로그에 경고가 찍히는지(출력 로그 확인)

- [ ] **Step 5: git add + 커밋 메시지(안) 제시**

```bash
git add SkillProject/Source/SkillProject/System/SpyMissionComponent.cpp
```

커밋 메시지(안): `[Feature] USpyMissionComponent — Interact 타입 자동 수락·보상 경고 확장`

---

### Task 3: `ISpyInteractableRoot` 인터페이스 신설

**Files:**
- Create: `SkillProject/Source/SkillProject/Interactable/CommonInterface.Interactable.h`

**Interfaces:**
- Produces: `USpyInteractableRoot`(UInterface), `ISpyInteractableRoot`(네이티브 인터페이스) — `RequestInteract(APlayerController*)`, `IsPawnInRange(const AActor*) const`, `GetInteractVerb() const`. Task 4가 구현하고, Task 6이 `Cast<ISpyInteractableRoot>`로 소비한다.

- [ ] **Step 1: 새 도메인 폴더에 인터페이스 파일 작성**

`Interactable/CommonInterface.Interactable.h` (신규 파일):

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "CommonInterface.Interactable.generated.h"

class APlayerController;

//# 상호작용 오브젝트 도메인의 유일한 외부 진입점 (cpp-style §13, 하위 컴포넌트 1개라 §13
//# 예외 대상이나 소비자(USpyInteractionComponent)를 위해 인터페이스는 미리 뺀다).
UINTERFACE(MinimalAPI)
class USpyInteractableRoot : public UInterface
{
	GENERATED_BODY()
};

class ISpyInteractableRoot
{
	GENERATED_BODY()

public:
	//# 서버 권한에서만 유효. 상호작용 처리 + AddProgress + 소진 처리까지 이 안에서 끝낸다.
	virtual void RequestInteract(APlayerController* Requester) = 0;

	//# 서버 재검증 전용 — 트리거(오버랩)와 동일한 기하로 재확인한다
	//# (point-distance로 재구현 시 불일치 구간 발생, NPC 패턴과 동일 이유).
	virtual bool IsPawnInRange(const AActor* RequesterPawn) const = 0;

	virtual FText GetInteractVerb() const = 0;
};
```

- [ ] **Step 2: 컴파일 확인**

새 UHT 타입(`USpyInteractableRoot`)이 다른 모듈 타입과 이름 충돌 없는지 확인 후 빌드(신규 파일만 추가한 상태라 이 파일 자체는 아무 데서도 include되지 않아 빌드에 영향 없음 — Task 4에서 include됨).

- [ ] **Step 3: git add + 커밋 메시지(안) 제시**

```bash
git add SkillProject/Source/SkillProject/Interactable/CommonInterface.Interactable.h
```

커밋 메시지(안): `[Feature] ISpyInteractableRoot — 상호작용 오브젝트 도메인 인터페이스 신설`

---

### Task 4: `ASpyInteractableObject` 액터 구현

**Files:**
- Create: `SkillProject/Source/SkillProject/Interactable/SpyInteractableObject.h`
- Create: `SkillProject/Source/SkillProject/Interactable/SpyInteractableObject.cpp`

**Interfaces:**
- Consumes: `ISpyInteractableRoot`(Task 3), `SpyGameplayTags::Event_Mission_Interact`(Task 1, 기본값 참고용 — 실제 값은 인스턴스별 `EditAnywhere`), `USpyMissionComponent::FindMissionComponent(const AActor*)`(기존), `USpyMissionComponent::AddProgress(FGameplayTag, int32)`(기존), `ISpyCharacterRoot::GetInteractionHost()`(기존, `Character/CommonInterface.Character.h`), `ISpyInteractionHost::NotifyInteractableRangeChanged(AActor*, bool)`(Task 5에서 인터페이스에 추가 — 이 태스크에서는 헤더만 include하고 실제 함수는 Task 5 완료 후에야 링크된다. **Task 5를 먼저 완료하거나 Task 4/5를 같은 빌드 사이클에 함께 반영한다.**)
- Produces: `ASpyInteractableObject` 클래스, `MissionEventTag`/`InteractVerb`(`EditAnywhere`), `bConsumed`(Replicated).

> ⚠ **순서 주의**: 이 태스크는 `ISpyInteractionHost::NotifyInteractableRangeChanged`를 호출한다(Task 5 산출물). Task 5가 이 태스크보다 먼저 끝나 있어야 컴파일이 통과한다. 순서대로 실행한다면 Task 5를 이 태스크 앞으로 옮기거나, subagent-driven-development 진행자가 Task 4/5를 같은 리뷰 사이클로 묶는다.

- [ ] **Step 1: 헤더 작성**

`Interactable/SpyInteractableObject.h` (신규 파일):

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable/CommonInterface.Interactable.h"

#include "SpyInteractableObject.generated.h"

class USphereComponent;

//# 레벨 배치형 상호작용 오브젝트. F키로 1회 상호작용하면 지정된 미션 태그로
//# 진행도를 올리고 스스로 소진된다.
UCLASS()
class SKILLPROJECT_API ASpyInteractableObject : public AActor, public ISpyInteractableRoot
{
	GENERATED_BODY()

public:
	ASpyInteractableObject();

	//# ISpyInteractableRoot
	virtual void RequestInteract(APlayerController* Requester) override;
	virtual bool IsPawnInRange(const AActor* RequesterPawn) const override;
	virtual FText GetInteractVerb() const override { return InteractVerb; }

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void OnRep_Consumed();

	//# 소진 직후 이 오브젝트와 여전히 오버랩 중인 로컬 폰들에게 범위 이탈을 알린다 —
	//# 콜리전을 끄는 것만으로는 EndOverlap 발화가 보장되지 않아 명시적으로 호출한다
	void NotifyLocalOverlapEnd();

	//# 연출 훅 — VFX/사운드는 이번 범위 밖
	UFUNCTION(BlueprintImplementableEvent, Category = "Interactable")
	void OnConsumed();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Interactable")
	TObjectPtr<USphereComponent> InteractionSphere;

	//# 이 오브젝트가 상호작용 시 발신할 미션 진행 태그 (Event.Mission.Interact 계열)
	UPROPERTY(EditAnywhere, Category = "Interactable")
	FGameplayTag MissionEventTag;

	//# 근접 프롬프트에 표시할 동사
	UPROPERTY(EditAnywhere, Category = "Interactable")
	FText InteractVerb;

	UPROPERTY(ReplicatedUsing = OnRep_Consumed)
	bool bConsumed = false;
};
```

- [ ] **Step 2: cpp 작성**

`Interactable/SpyInteractableObject.cpp` (신규 파일):

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "Interactable/SpyInteractableObject.h"
#include "Components/SphereComponent.h"
#include "Character/CommonInterface.Character.h"
#include "ManagerComponent/CommonInterface.Manager.h"
#include "System/SpyMissionComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

ASpyInteractableObject::ASpyInteractableObject()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	SetRootComponent(InteractionSphere);
	InteractionSphere->SetSphereRadius(300.f);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	InteractVerb = NSLOCTEXT("SpyInteractable", "DefaultInteractVerb", "조사하기");
}

void ASpyInteractableObject::BeginPlay()
{
	Super::BeginPlay();

	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ASpyInteractableObject::OnInteractionSphereBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &ASpyInteractableObject::OnInteractionSphereEndOverlap);
}

void ASpyInteractableObject::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASpyInteractableObject, bConsumed);
}

void ASpyInteractableObject::OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const APawn* OtherPawn = Cast<APawn>(OtherActor);
	if (OtherPawn == nullptr || OtherPawn->IsLocallyControlled() == false)
		return;

	//# TScriptInterface(RawPtr) 생성자는 인터페이스 미구현이어도 ObjectPointer 를 그대로 저장한다 —
	//# GetObject() 널체크로는 구현 여부를 걸러낼 수 없다. Cast<Interface> 로 먼저 판정한다.
	ISpyCharacterRoot* CharRoot = Cast<ISpyCharacterRoot>(OtherActor);
	if (CharRoot == nullptr)
		return;

	TScriptInterface<ISpyInteractionHost> Host = CharRoot->GetInteractionHost();
	if (Host.GetObject() == nullptr)
		return;

	Host->NotifyInteractableRangeChanged(this, true);
}

void ASpyInteractableObject::OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	const APawn* OtherPawn = Cast<APawn>(OtherActor);
	if (OtherPawn == nullptr || OtherPawn->IsLocallyControlled() == false)
		return;

	ISpyCharacterRoot* CharRoot = Cast<ISpyCharacterRoot>(OtherActor);
	if (CharRoot == nullptr)
		return;

	TScriptInterface<ISpyInteractionHost> Host = CharRoot->GetInteractionHost();
	if (Host.GetObject() == nullptr)
		return;

	Host->NotifyInteractableRangeChanged(this, false);
}

bool ASpyInteractableObject::IsPawnInRange(const AActor* RequesterPawn) const
{
	if (RequesterPawn == nullptr || InteractionSphere == nullptr)
		return false;

	return InteractionSphere->IsOverlappingActor(RequesterPawn);
}

void ASpyInteractableObject::RequestInteract(APlayerController* Requester)
{
	//# 게임플레이 상태(미션 진행)를 바꾸는 함수라 호출부 권한 체크에만 기대지 않는다 — 자체 방어
	if (HasAuthority() == false)
		return;

	if (bConsumed)
		return;

	if (Requester == nullptr)
		return;

	APawn* RequesterPawn = Requester->GetPawn();
	if (RequesterPawn == nullptr)
		return;

	APlayerState* RequesterPS = RequesterPawn->GetPlayerState();
	if (RequesterPS == nullptr)
		return;

	USpyMissionComponent* MissionComp = USpyMissionComponent::FindMissionComponent(RequesterPS);
	if (MissionComp == nullptr)
		return;

	MissionComp->AddProgress(MissionEventTag, 1);

	bConsumed = true;

	//# 서버는 자신의 OnRep 콜백이 발화하지 않는다 — 직접 호출해 소진 처리를 공유한다
	OnRep_Consumed();
}

void ASpyInteractableObject::OnRep_Consumed()
{
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NotifyLocalOverlapEnd();
	OnConsumed();
}

void ASpyInteractableObject::NotifyLocalOverlapEnd()
{
	TArray<AActor*> OverlappingPawns;
	InteractionSphere->GetOverlappingActors(OverlappingPawns, APawn::StaticClass());

	for (AActor* OverlappingActor : OverlappingPawns)
	{
		const APawn* OverlappingPawn = Cast<APawn>(OverlappingActor);
		if (OverlappingPawn == nullptr || OverlappingPawn->IsLocallyControlled() == false)
			continue;

		ISpyCharacterRoot* CharRoot = Cast<ISpyCharacterRoot>(OverlappingActor);
		if (CharRoot == nullptr)
			continue;

		TScriptInterface<ISpyInteractionHost> Host = CharRoot->GetInteractionHost();
		if (Host.GetObject() == nullptr)
			continue;

		Host->NotifyInteractableRangeChanged(this, false);
	}
}
```

- [ ] **Step 3: 컴파일 확인**

Task 5(`ISpyInteractionHost::NotifyInteractableRangeChanged`)가 먼저 반영돼 있어야 `Host->NotifyInteractableRangeChanged(...)` 호출부가 링크된다. 빌드해 컴파일 에러 없는지 확인.

- [ ] **Step 4: PIE 확인 항목 기록 (자동화 테스트 불가)**

`RequestInteract`/`IsPawnInRange`/오버랩 콜백은 `HasAuthority()`와 실제 월드 물리 컨텍스트가 필요해 Automation 커버 불가(Global Constraints 참조). Task 6 Step 6의 인게임 확인 목록에 아래를 포함:
- 최초 1회 F 상호작용 시 진행도 반영 확인
- 재상호작용 시 `bConsumed` 가드로 무시되는지(F 연타해도 진행도가 2번 오르지 않는지)
- 소진 후 프롬프트가 실제로 사라지는지(`NotifyLocalOverlapEnd` 동작 확인)

- [ ] **Step 5: git add + 커밋 메시지(안) 제시**

```bash
git add SkillProject/Source/SkillProject/Interactable/SpyInteractableObject.h SkillProject/Source/SkillProject/Interactable/SpyInteractableObject.cpp
```

커밋 메시지(안): `[Feature] ASpyInteractableObject — 1회성 오브젝트 상호작용 미션 트리거 액터`

---

### Task 5: `ISpyInteractionHost`에 `NotifyInteractableRangeChanged` 추가

**Files:**
- Modify: `SkillProject/Source/SkillProject/ManagerComponent/CommonInterface.Manager.h:175-183`

**Interfaces:**
- Produces: `ISpyInteractionHost::NotifyInteractableRangeChanged(AActor* InteractableActor, bool bInRange)` — Task 4가 호출부(소비), Task 6이 구현부(정의)를 제공한다.

> ⚠ **먼저 실행**: Task 4가 이 메서드를 호출하므로, subagent-driven-development로 태스크별 서브에이전트를 붙일 경우 이 태스크를 Task 4보다 먼저 완료해야 중간 빌드가 깨지지 않는다.

- [ ] **Step 1: 인터페이스에 메서드 추가**

`ManagerComponent/CommonInterface.Manager.h:175-183` 현재 코드:

```cpp
class ISpyInteractionHost
{
	GENERATED_BODY()

public:
	//# NPCActor 는 상호작용 대상 액터(내부적으로 ISpyNPCRoot 캐스팅은 구현부가 담당).
	//# bInRange가 false면 이 NPCActor를 현재 대상에서 해제한다.
	virtual void NotifyNPCRangeChanged(AActor* NPCActor, bool bInRange) = 0;

	//# 입력 바인딩(Interact 액션)이 호출한다. 로컬에서 근접 NPC가 없으면 아무 일도 하지 않는다.
	virtual void TryInteract() = 0;
```

아래로 교체(새 메서드 1줄만 추가):

```cpp
class ISpyInteractionHost
{
	GENERATED_BODY()

public:
	//# NPCActor 는 상호작용 대상 액터(내부적으로 ISpyNPCRoot 캐스팅은 구현부가 담당).
	//# bInRange가 false면 이 NPCActor를 현재 대상에서 해제한다.
	virtual void NotifyNPCRangeChanged(AActor* NPCActor, bool bInRange) = 0;

	//# InteractableActor 는 오브젝트 상호작용 대상 액터(내부적으로 ISpyInteractableRoot 캐스팅은
	//# 구현부가 담당). NPC 경로와 별개 슬롯으로 추적된다.
	virtual void NotifyInteractableRangeChanged(AActor* InteractableActor, bool bInRange) = 0;

	//# 입력 바인딩(Interact 액션)이 호출한다. 로컬에서 근접 NPC가 없으면 아무 일도 하지 않는다.
	virtual void TryInteract() = 0;
```

- [ ] **Step 2: 컴파일 확인**

`ISpyInteractionHost`를 구현하는 클래스는 현재 `USpyInteractionComponent` 하나뿐이다(Task 6에서 구현 추가 전까지는 순수 가상 함수 미구현으로 컴파일 에러가 난다) — **이 태스크만 단독으로 빌드하면 에러가 나는 게 정상이다.** Task 6과 함께 빌드해야 통과한다.

- [ ] **Step 3: git add + 커밋 메시지(안) 제시**

```bash
git add SkillProject/Source/SkillProject/ManagerComponent/CommonInterface.Manager.h
```

커밋 메시지(안): `[Feature] ISpyInteractionHost — NotifyInteractableRangeChanged 추가`

---

### Task 6: `USpyInteractionComponent` 확장 — 오브젝트 상호작용 경로 연결

**Files:**
- Modify: `SkillProject/Source/SkillProject/ManagerComponent/SpyInteractionComponent.h`
- Modify: `SkillProject/Source/SkillProject/ManagerComponent/SpyInteractionComponent.cpp`

**Interfaces:**
- Consumes: `ISpyInteractionHost::NotifyInteractableRangeChanged`(Task 5, 이 태스크가 구현), `ISpyInteractableRoot`(Task 3, `Cast<>` 대상).
- Produces: `NearbyInteractable` 필드, `Server_RequestInteractObject(AActor*)` RPC — 이번 계획의 최종 연결 지점.

- [ ] **Step 1: 헤더에 필드·메서드 선언 추가**

`ManagerComponent/SpyInteractionComponent.h` 상단 include 블록(`#include "NPC/CommonInterface.NPC.h"` 다음 줄)에 추가:

```cpp
#include "Interactable/CommonInterface.Interactable.h"
```

`ISpyInteractionHost` 오버라이드 목록(`virtual void NotifyNPCRangeChanged(...)` 다음 줄, 원본 `.h:24` 직후)에 추가:

```cpp
	virtual void NotifyInteractableRangeChanged(AActor* InteractableActor, bool bInRange) override;
```

`protected:` 블록의 RPC 선언들(`Server_RequestInteract` 다음, 원본 `.h:56` 직후)에 추가:

```cpp
	UFUNCTION(Server, Reliable)
	void Server_RequestInteractObject(AActor* TargetObject);
```

`NearbyNPC` 필드 선언(원본 `.h:68` 직후)에 추가:

```cpp
	UPROPERTY(Transient)
	TObjectPtr<AActor> NearbyInteractable;
```

- [ ] **Step 2: `HandleInteractPromptCloseFailsafe`가 두 슬롯을 모두 확인하도록 수정**

`ManagerComponent/SpyInteractionComponent.cpp:67-77` 현재 코드:

```cpp
void USpyInteractionComponent::HandleInteractPromptCloseFailsafe()
{
	//# 재검증 시점에 다시 NPC 범위 안이면(재진입) 닫지 않는다
	if (NearbyNPC != nullptr)
		return;

	if (USpyUIManager* UIManager = Cast<USpyUIManager>(USKUIManager::Get(this)))
	{
		UIManager->CloseSpyUI(ESpyUIType::InteractPrompt);
	}
}
```

아래로 교체:

```cpp
void USpyInteractionComponent::HandleInteractPromptCloseFailsafe()
{
	//# 재검증 시점에 다시 NPC/오브젝트 범위 안이면(재진입) 닫지 않는다
	if (NearbyNPC != nullptr || NearbyInteractable != nullptr)
		return;

	if (USpyUIManager* UIManager = Cast<USpyUIManager>(USKUIManager::Get(this)))
	{
		UIManager->CloseSpyUI(ESpyUIType::InteractPrompt);
	}
}
```

- [ ] **Step 3: `NotifyInteractableRangeChanged` 구현 추가**

`ManagerComponent/SpyInteractionComponent.cpp`의 `NotifyNPCRangeChanged` 함수(원본 `.cpp:21-65`) 바로 다음에 추가:

```cpp
void USpyInteractionComponent::NotifyInteractableRangeChanged(AActor* InteractableActor, bool bInRange)
{
	if (bInRange)
	{
		NearbyInteractable = InteractableActor;

		//# NPC 경로는 동사를 "대화하기"로 고정했지만, 오브젝트는 각자의 동사를 그대로 쓴다
		if (const ISpyInteractableRoot* InteractableRoot = Cast<ISpyInteractableRoot>(InteractableActor))
			CachedInteractVerb = InteractableRoot->GetInteractVerb();

		if (USpyUIManager* UIManager = Cast<USpyUIManager>(USKUIManager::Get(this)))
		{
			UIManager->OpenSpyUI(ESpyUIType::InteractPrompt);
		}
		return;
	}

	if (NearbyInteractable != InteractableActor)
		return;

	NearbyInteractable = nullptr;

	if (USpyUIManager* UIManager = Cast<USpyUIManager>(USKUIManager::Get(this)))
	{
		UIManager->CloseSpyUI(ESpyUIType::InteractPrompt);
	}

	//# OpenUI 로드가 이 시점에 아직 안 끝났으면 위 CloseSpyUI 는 no-op 이다 —
	//# 지연 뒤 재검증해 한 번 더 닫아 프롬프트가 남는 걸 방지한다 (NPC 경로와 동일 이유)
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			InteractPromptCloseFailsafeTimerHandle,
			this,
			&USpyInteractionComponent::HandleInteractPromptCloseFailsafe,
			UICloseFailsafeDelaySec,
			false);
	}
}
```

`Cast<ISpyInteractableRoot>`를 값(`const ISpyInteractableRoot*`)으로 받는 이 형태는 §8 "GetObject() 널체크는 구현 여부를 걸러내지 못한다"는 함정과 무관하다 — 여기서는 `TScriptInterface`가 아니라 `Cast<Interface>` 자체를 조건문에 바로 쓰므로 안전하다(널이면 분기 진입 안 함).

- [ ] **Step 4: `TryInteract()`에 오브젝트 분기 추가**

`ManagerComponent/SpyInteractionComponent.cpp:79-91` 현재 코드:

```cpp
void USpyInteractionComponent::TryInteract()
{
	if (bDialogueOpen)
	{
		AdvanceOrCloseDialogue();
		return;
	}

	if (NearbyNPC == nullptr)
		return;

	Server_RequestInteract(NearbyNPC);
}
```

아래로 교체:

```cpp
void USpyInteractionComponent::TryInteract()
{
	if (bDialogueOpen)
	{
		AdvanceOrCloseDialogue();
		return;
	}

	//# NPC 가 오브젝트보다 우선한다 — 이번 범위에서 실제로 겹칠 일은 없지만 기본 순서를 명시한다
	if (NearbyNPC != nullptr)
	{
		Server_RequestInteract(NearbyNPC);
		return;
	}

	if (NearbyInteractable != nullptr)
		Server_RequestInteractObject(NearbyInteractable);
}
```

- [ ] **Step 5: `Server_RequestInteractObject` 구현 추가**

`ManagerComponent/SpyInteractionComponent.cpp`의 `Server_RequestInteract_Implementation` 함수(원본 `.cpp:143-171`) 바로 다음에 추가:

```cpp
void USpyInteractionComponent::Server_RequestInteractObject_Implementation(AActor* TargetObject)
{
	if (TargetObject == nullptr)
		return;

	AActor* Owner = GetOwner();
	if (Owner == nullptr || Owner->HasAuthority() == false)
		return;

	//# GetObject() 널체크는 인터페이스 미구현을 걸러내지 못한다(클라 RPC 파라미터라 임의 액터 가능) —
	//# Cast<Interface> 로 먼저 판정해 서버 크래시를 막는다.
	ISpyInteractableRoot* InteractableRoot = Cast<ISpyInteractableRoot>(TargetObject);
	if (InteractableRoot == nullptr)
		return;

	//# point-distance 대신 오브젝트의 상호작용 SphereComponent 오버랩을 그대로 재확인한다
	const bool bInRange = InteractableRoot->IsPawnInRange(Owner);
	if (bInRange == false)
		return;

	APlayerController* PC = Cast<APlayerController>(Owner->GetInstigatorController());
	if (PC == nullptr)
		return;

	//# NPC 경로와 달리 결과를 Client RPC 로 돌려보내지 않는다 — 대화 UI 가 없고,
	//# 미션 HUD 는 USpyMissionComponent::OnMissionProgressChanged(레플리케이션 기반)가 자동 갱신한다
	InteractableRoot->RequestInteract(PC);
}
```

이 함수는 `Cast<ISpyInteractableRoot>`를 쓰므로 `.cpp` 상단에 `#include "Interactable/CommonInterface.Interactable.h"`가 필요하다 — Step 1에서 `.h`에 이미 추가했으므로 `.cpp`는 `.h`를 통해 전이 include된다(별도 추가 불필요).

- [ ] **Step 6: 컴파일 확인 + 인게임(PIE) 확인**

전체 6개 태스크가 반영된 상태로 빌드해 컴파일 통과를 확인한다. 이후 아래 PIE 체크리스트를 1인 Standalone에서 수행한다(Task 2/4에서 유보한 항목 포함):

1. `DA_SpyMissionConfig`에 `MissionType = Interact`, `MatchTag = Event.Mission.Interact`, `TargetCount = 1`인 임시 미션 엔트리 1개 추가(사용자가 에디터에서 수행 — 이번 계획 범위 밖).
2. `ASpyInteractableObject`(또는 그 블루프린트 서브클래스)를 DevMap에 배치, `MissionEventTag = Event.Mission.Interact` 지정(사용자가 MCP/에디터에서 수행 — 이번 계획 범위 밖).
3. 근접 시 프롬프트에 `"조사하기"`(또는 지정한 `InteractVerb`)가 표시되는지 — NPC의 `"대화하기"`와 다른 문구인지 확인.
4. F 상호작용 → HUD 진행도가 즉시 갱신되는지(대화창 없이 조용히).
5. 곧바로 자동 수락됐는지 — 별도 [수락] 카드 없이 목표 문구가 뜨는지.
6. 재상호작용 시도(F 연타) → 진행도가 2번 오르지 않는지, 프롬프트가 사라지는지(콜리전 비활성화 + `NotifyLocalOverlapEnd` 동작 확인).
7. `MissionRewardTable`에 해당 `MissionId` 행을 일부러 빼고 완료시켜 출력 로그에 경고가 찍히는지.
8. (선택) NPC와 오브젝트가 동시에 범위 안에 있는 상황을 만들어 F키가 NPC를 우선하는지 확인.

- [ ] **Step 7: git add + 커밋 메시지(안) 제시**

```bash
git add SkillProject/Source/SkillProject/ManagerComponent/SpyInteractionComponent.h SkillProject/Source/SkillProject/ManagerComponent/SpyInteractionComponent.cpp
```

커밋 메시지(안): `[Feature] USpyInteractionComponent — 오브젝트 상호작용 경로 추가(F키 일반화)`

---

## 완료 후 남는 것 (이 계획 범위 밖, spec §7 "에셋" 항목)

- `BP_InteractableObject`(또는 `ASpyInteractableObject` 직접 배치) — DevMap 실제 배치, `MissionEventTag`/`InteractVerb` 지정: **사용자**.
- `DA_SpyMissionConfig`에 실제 콘텐츠용 `Interact` 타입 미션 엔트리 추가: **사용자/game-designer**(밸런스 결정 필요 시).
- code-reviewer 검토(cpp-style/git-conventions 준수, 기획 spec 일치) — 이 plan을 `/start-develop` 파이프라인으로 실행한다면 Task 6 이후 자동으로 이어진다.
- test-engineer의 정식 Automation 테스트 스위트 작성 — 이 기능은 대부분 PIE로만 검증 가능하다는 제약을 test-engineer에게도 전달해야 한다(Global Constraints 참조).
