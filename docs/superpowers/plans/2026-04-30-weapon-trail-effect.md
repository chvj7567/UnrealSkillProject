# Weapon Trail Effect Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** `GA_Skill*` GA가 활성화되는 동안 `ASpyWeapon`에 `P_Kallari_Primary_Trail` 파티클 트레일 이펙트를 표시한다.

**Architecture:** `ASpyWeapon`에 `UParticleSystemComponent`를 추가하고 `ActivateTrail`/`DeactivateTrail` 인터페이스를 제공한다. `USpyGameplayAbility_SkillAction` 베이스 클래스의 `ActivateAbility`/`EndAbility`에서 이 함수를 호출하며, 모든 `GA_Skill*` Blueprint GA에 자동 적용된다. 이펙트는 비주얼 전용이므로 `HasAuthority()` 블록 밖에서 실행한다.

**Tech Stack:** Unreal Engine 5.7, C++, Cascade Particle System (`UParticleSystemComponent`)

---

## File Map

| 파일 | 역할 |
|------|------|
| `SkillProject/Source/SkillProject/Item/SpyWeapon.h` | `TrailEffect`, `WeaponTrailComponent` 프로퍼티 및 `ActivateTrail`/`DeactivateTrail` 선언 추가 |
| `SkillProject/Source/SkillProject/Item/SpyWeapon.cpp` | 생성자에서 `WeaponTrailComponent` 초기화, `ActivateTrail`/`DeactivateTrail` 구현 |
| `SkillProject/Source/SkillProject/AbilitySystem/Skill/SpyGameplayAbility_SkillAction.h` | `EndAbility` 오버라이드 선언 추가 |
| `SkillProject/Source/SkillProject/AbilitySystem/Skill/SpyGameplayAbility_SkillAction.cpp` | `ActivateAbility`에 트레일 활성화 코드 추가, `EndAbility` 구현 |

---

### Task 1: ASpyWeapon 헤더 수정

**Files:**
- Modify: `SkillProject/Source/SkillProject/Item/SpyWeapon.h`

- [ ] **Step 1: SpyWeapon.h에 include 및 멤버 추가**

`SpyWeapon.h`의 전체 내용을 아래로 교체한다:

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NativeGameplayTags.h"
#include "Particles/ParticleSystemComponent.h"

#include "SpyWeapon.generated.h"

class UBoxComponent;
class USkeletalMeshComponent;
class UParticleSystem;

UCLASS()
class SKILLPROJECT_API ASpyWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	ASpyWeapon();

protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION()
	void EquipWeapon();

	UFUNCTION()
	void UnEquipWeapon();

	UFUNCTION(BlueprintCallable)
	void ActivateTrail();

	UFUNCTION(BlueprintCallable)
	void DeactivateTrail();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USkeletalMeshComponent> WeaponSkeletalMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FX")
	TObjectPtr<UParticleSystemComponent> WeaponTrailComponent;

	UPROPERTY(EditDefaultsOnly, Category = "FX")
	TObjectPtr<UParticleSystem> TrailEffect;
};
```

---

### Task 2: ASpyWeapon 구현 수정

**Files:**
- Modify: `SkillProject/Source/SkillProject/Item/SpyWeapon.cpp`

- [ ] **Step 1: SpyWeapon.cpp 전체 내용 교체**

```cpp
#include "SpyWeapon.h"
#include "Character/SpyCharacter.h"
#include "Particles/ParticleSystemComponent.h"

ASpyWeapon::ASpyWeapon()
{
	bReplicates = true;

	WeaponSkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponSkeletalMeshComponent"));
	RootComponent = WeaponSkeletalMeshComponent;

	WeaponTrailComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("WeaponTrailComponent"));
	WeaponTrailComponent->SetupAttachment(WeaponSkeletalMeshComponent);
	WeaponTrailComponent->bAutoActivate = false;
}

void ASpyWeapon::BeginPlay()
{
	Super::BeginPlay();
}

void ASpyWeapon::EquipWeapon()
{
}

void ASpyWeapon::UnEquipWeapon()
{
}

void ASpyWeapon::ActivateTrail()
{
	if (!IsValid(WeaponTrailComponent) || !IsValid(TrailEffect))
		return;

	WeaponTrailComponent->SetTemplate(TrailEffect);
	WeaponTrailComponent->Activate(true);
}

void ASpyWeapon::DeactivateTrail()
{
	if (!IsValid(WeaponTrailComponent))
		return;

	WeaponTrailComponent->Deactivate();
}
```

- [ ] **Step 2: 빌드 확인**

Visual Studio에서 `Ctrl+Shift+B` 또는 Unreal Editor에서 `Tools > Compile`을 실행한다.  
예상 결과: 컴파일 에러 없음. `ASpyWeapon` 클래스에 `WeaponTrailComponent`가 추가된 상태.

- [ ] **Step 3: 스테이지 등록 — 커밋 메시지 준비**

```
[Feature] SpyWeapon — 무기 트레일 이펙트 컴포넌트 및 ActivateTrail/DeactivateTrail 추가
```

---

### Task 3: USpyGameplayAbility_SkillAction 헤더 수정

**Files:**
- Modify: `SkillProject/Source/SkillProject/AbilitySystem/Skill/SpyGameplayAbility_SkillAction.h`

- [ ] **Step 1: EndAbility 오버라이드 선언 추가**

`SpyGameplayAbility_SkillAction.h` 전체 내용을 아래로 교체한다:

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Ability/SKGameplayAbility_SkillAction.h"

#include "SpyGameplayAbility_SkillAction.generated.h"

UCLASS()
class SKILLPROJECT_API USpyGameplayAbility_SkillAction : public USKGameplayAbility_SkillAction
{
	GENERATED_BODY()
	
public:
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

	virtual void InputPressed(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;
};
```

---

### Task 4: USpyGameplayAbility_SkillAction 구현 수정

**Files:**
- Modify: `SkillProject/Source/SkillProject/AbilitySystem/Skill/SpyGameplayAbility_SkillAction.cpp`

- [ ] **Step 1: include 추가 및 ActivateAbility/EndAbility 수정**

`SpyGameplayAbility_SkillAction.cpp` 전체 내용을 아래로 교체한다:

```cpp
#include "SpyGameplayAbility_SkillAction.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "Util/SpyGameplayTags.h"
#include "System/SpyPlayerState.h"
#include "GameFramework/Character.h"
#include "Data/SpyCharacterAssetData.h"
#include "ManagerComponent/SpyTargetingManagerComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Character/SpyCharacter.h"
#include "Item/SpyWeapon.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGameplayAbility_SkillAction)

void USpyGameplayAbility_SkillAction::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    do
    {
        /*ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
        if (OwnerCharacter == nullptr)
            break;

        USpyTargetingManagerComponent* TargetingComp = OwnerCharacter->FindComponentByClass<USpyTargetingManagerComponent>();
        if (TargetingComp == nullptr)
            break;

        if (TargetingComp->GetTarget().IsValid())
        {
            FVector LookDir = TargetingComp->GetTarget()->GetActorLocation() - OwnerCharacter->GetActorLocation();
            LookDir.Z = 0.f;
            FRotator TargetRot = LookDir.Rotation();

            OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
            OwnerCharacter->SetActorRotation(TargetRot);
            OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = true;
        }*/

    } while (false);

    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    ASpyCharacter* SpyChar = Cast<ASpyCharacter>(GetAvatarActorFromActorInfo());
    if (IsValid(SpyChar) && IsValid(SpyChar->GetSpyWeapon()))
    {
        SpyChar->GetSpyWeapon()->ActivateTrail();
    }
}

void USpyGameplayAbility_SkillAction::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    ASpyCharacter* SpyChar = Cast<ASpyCharacter>(GetAvatarActorFromActorInfo());
    if (IsValid(SpyChar) && IsValid(SpyChar->GetSpyWeapon()))
    {
        SpyChar->GetSpyWeapon()->DeactivateTrail();
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USpyGameplayAbility_SkillAction::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (OwnerCharacter == nullptr)
        return;

    ASpyPlayerState* OwnerPS = OwnerCharacter->GetPlayerState<ASpyPlayerState>();
    USpyAbilitySystemComponent* OwnerASC = Cast<USpyAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get());
    if (OwnerPS == nullptr || OwnerASC == nullptr)
        return;

    if (OwnerASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Combo))
    {
        FGameplayTag MyTag = AbilityTags.GetByIndex(0);
        if (MyTag.IsValid() == false)
            return;
        
        if (USpyCharacterAssetData* CharacterAssetData = OwnerPS->GetCharacterAssetData())
        {
            FGameplayTag Tag = CharacterAssetData->GetComboTag(SpyGameplayTags::Character_Class_Normal, MyTag);
            if (Tag.IsValid())
            {
                FGameplayEventData Payload;
                Payload.EventTag = Tag;

                UE_LOG(LogTemp, Warning, TEXT("# Combo %s"), *Tag.ToString());
                OwnerASC->HandleGameplayEvent(Payload.EventTag, &Payload);
                return;
            }
        }
    }
}
```

- [ ] **Step 2: 빌드 확인**

Visual Studio에서 `Ctrl+Shift+B` 또는 Unreal Editor에서 `Tools > Compile`을 실행한다.  
예상 결과: 컴파일 에러 없음.

- [ ] **Step 3: 스테이지 등록 — 커밋 메시지 준비**

```
[Feature] SpyGameplayAbility_SkillAction — GA_Skill 활성화 구간 무기 트레일 이펙트 연동
```

---

### Task 5: 에디터에서 TrailEffect 에셋 지정

**Files:**
- 에디터 작업 (BP 에셋 수정)

- [ ] **Step 1: 무기 Blueprint 열기**

Unreal Editor Content Browser에서 무기 Blueprint(예: `BP_SpyWeapon` 또는 `SpyWeapon` 기반 BP)를 더블클릭해서 연다.

- [ ] **Step 2: TrailEffect 지정**

Details 패널 > **FX** 카테고리 > **Trail Effect** 슬롯에  
`Content/Spy/FX/Particles/Kallari/Abilities/Primary/FX/P_Kallari_Primary_Trail`을 할당한다.

- [ ] **Step 3: 저장**

`Ctrl+S`로 Blueprint 저장.

---

### Task 6: 인게임 검증

- [ ] **Step 1: PIE 실행**

Unreal Editor에서 Play In Editor(PIE)를 시작한다.

- [ ] **Step 2: GA_Skill 발동**

캐릭터로 `GA_SkillA`~`GA_SkillF` 중 하나를 입력해 발동한다.

- [ ] **Step 3: 트레일 확인**

어빌리티 활성화 중 무기에 트레일 파티클이 보이는지 확인한다.  
어빌리티 종료(몽타쥬 완료 또는 취소) 후 트레일이 사라지는지 확인한다.

- [ ] **Step 4: 비정상 케이스 확인**

어빌리티 도중 취소(`CancelAbility`)가 발생해도 트레일이 정상 종료되는지 확인한다.  
(`bWasCancelled = true`로 `EndAbility`가 호출되므로 별도 처리 불필요)
