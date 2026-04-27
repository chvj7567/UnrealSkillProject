# 패링 시스템 구현 계획

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 플레이어가 버튼을 홀드하는 동안 정면 공격을 막고 방어자가 뒤로 LaunchCharacter되는 패링 시스템 구현

**Architecture:** AnimNotifyState로 클라이언트 태그 동기화, GA_Parry가 서버 측 `Character_State_Parry` 태그를 직접 관리. 공격자 GA(`SKGameplayAbility_SkillAction`)가 타겟에 데미지 이벤트 전송 직전에 패링 여부를 체크하여 성공 시 `Skill_Parry_Hit` 이벤트를 방어자에게 전송하고 데미지를 스킵. 방어자의 GA_Parry가 이벤트를 수신하여 LaunchCharacter 적용.

**Tech Stack:** Unreal Engine 5.7, GAS (GameplayAbilitySystem), AnimNotifyState, C++

---

## 파일 맵

| 파일 | 역할 | 신규/수정 |
|------|------|-----------|
| `Source/SKGAS/SKGameplayTags.h` | `Character_State_Parry`, `Skill_Parry_Hit` extern 선언 | 수정 |
| `Source/SKGAS/SKGameplayTags.cpp` | 위 태그 define | 수정 |
| `Source/SkillProject/Util/SpyGameplayTags.h` | `Input_Ability_Parry` extern 선언 | 수정 |
| `Source/SkillProject/Util/SpyGameplayTags.cpp` | `Input_Ability_Parry` define | 수정 |
| `Source/SkillProject/Character/AnimInstance/SpyAnimNotify_State_Parry.h` | 클라이언트 패링 윈도우 태그 동기화 AnimNotifyState | 신규 |
| `Source/SkillProject/Character/AnimInstance/SpyAnimNotify_State_Parry.cpp` | 위 구현 | 신규 |
| `Source/SkillProject/AbilitySystem/Parry/SpyGameplayAbility_Parry.h` | 홀드형 패링 GA 선언 | 신규 |
| `Source/SkillProject/AbilitySystem/Parry/SpyGameplayAbility_Parry.cpp` | GA 구현 (태그 관리 + 넉백) | 신규 |
| `Source/SKGAS/Ability/SKGameplayAbility_SkillAction.cpp` | `SendTagToTargetByWeapon/Sphere`에 패링 체크 삽입 | 수정 |

---

## Task 1: Gameplay Tags 추가

**Files:**
- Modify: `SkillProject/Source/SKGAS/SKGameplayTags.h`
- Modify: `SkillProject/Source/SKGAS/SKGameplayTags.cpp`
- Modify: `SkillProject/Source/SkillProject/Util/SpyGameplayTags.h`
- Modify: `SkillProject/Source/SkillProject/Util/SpyGameplayTags.cpp`

- [ ] **Step 1: SKGameplayTags.h에 태그 extern 추가**

`SKGameplayTags.h`의 `Character_State_SuperArmor` 선언 바로 아래에 추가:

```cpp
SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_SuperArmor);
SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Parry);   // 추가
SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Combo);

// ... (기존 태그들)

//# 패링
SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Parry_Hit);          // 추가 (파일 하단 적절한 위치)
```

실제 삽입 위치 — `Character_State_SuperArmor` 라인 바로 다음:
```cpp
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_SuperArmor);
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Parry);
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Combo);
```

그리고 `Skill_Hit_Back` 선언 아래 새 섹션:
```cpp
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Hit_Back);

	//# 패링
	SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Parry_Hit);

	SKGAS_API FGameplayTag GetSkillActionTag(const UAbilitySystemComponent* ASC);
```

- [ ] **Step 2: SKGameplayTags.cpp에 태그 define 추가**

`SKGameplayTags.cpp`의 `Character_State_SuperArmor` define 바로 다음:
```cpp
UE_DEFINE_GAMEPLAY_TAG(Character_State_SuperArmor, "Character.State.SuperArmor");
UE_DEFINE_GAMEPLAY_TAG(Character_State_Parry, "Character.State.Parry");
UE_DEFINE_GAMEPLAY_TAG(Character_State_Combo, "Character.State.Combo");
```

그리고 `Skill_Hit_Back` define 아래:
```cpp
UE_DEFINE_GAMEPLAY_TAG(Skill_Hit_Back, "Skill.Hit.Back");

UE_DEFINE_GAMEPLAY_TAG(Skill_Parry_Hit, "Skill.Parry.Hit");
```

- [ ] **Step 3: SpyGameplayTags.h에 Input_Ability_Parry extern 추가**

`SpyGameplayTags.h`의 `Input_Ability_Skill_10` 선언 아래:
```cpp
	SKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_Skill_10);
	SKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_Parry);
```

- [ ] **Step 4: SpyGameplayTags.cpp에 Input_Ability_Parry define 추가**

`SpyGameplayTags.cpp`의 `Input_Ability_Skill_10` define 아래:
```cpp
UE_DEFINE_GAMEPLAY_TAG(Input_Ability_Skill_10, "Input.Ability.Skill.10");
UE_DEFINE_GAMEPLAY_TAG(Input_Ability_Parry, "Input.Ability.Parry");
```

- [ ] **Step 5: 빌드하여 컴파일 오류 없는지 확인**

Unreal Editor에서 **Tools > Compile** 또는 Visual Studio에서 빌드.  
예상 결과: 컴파일 성공, 새 태그 심볼 오류 없음.

- [ ] **Step 6: 커밋**

```bash
git add SkillProject/Source/SKGAS/SKGameplayTags.h
git add SkillProject/Source/SKGAS/SKGameplayTags.cpp
git add SkillProject/Source/SkillProject/Util/SpyGameplayTags.h
git add SkillProject/Source/SkillProject/Util/SpyGameplayTags.cpp
git commit -m "[Feature] 패링 시스템 Gameplay Tags 추가 (Character_State_Parry, Skill_Parry_Hit, Input_Ability_Parry)"
```

---

## Task 2: SpyAnimNotify_State_Parry 구현

**Files:**
- Create: `SkillProject/Source/SkillProject/Character/AnimInstance/SpyAnimNotify_State_Parry.h`
- Create: `SkillProject/Source/SkillProject/Character/AnimInstance/SpyAnimNotify_State_Parry.cpp`

참고: `SpyAnimNotify_State_Combo`와 동일한 패턴. 클라이언트에서 `Character_State_Parry` 태그를 Add/Remove하는 역할.

- [ ] **Step 1: 헤더 파일 생성**

`SkillProject/Source/SkillProject/Character/AnimInstance/SpyAnimNotify_State_Parry.h`:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"

#include "SpyAnimNotify_State_Parry.generated.h"

UCLASS()
class SKILLPROJECT_API USpyAnimNotify_State_Parry : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
```

- [ ] **Step 2: 구현 파일 생성**

`SkillProject/Source/SkillProject/Character/AnimInstance/SpyAnimNotify_State_Parry.cpp`:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "SpyAnimNotify_State_Parry.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "SKGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyAnimNotify_State_Parry)

void USpyAnimNotify_State_Parry::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp->GetOwner()))
		{
			ASC->AddLooseGameplayTag(SKGameplayTags::Character_State_Parry);
		}
	}
}

void USpyAnimNotify_State_Parry::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp->GetOwner()))
		{
			ASC->RemoveLooseGameplayTag(SKGameplayTags::Character_State_Parry);
		}
	}
}
```

- [ ] **Step 3: 빌드하여 컴파일 오류 없는지 확인**

Unreal Editor에서 **Tools > Compile**.  
예상 결과: 컴파일 성공.

- [ ] **Step 4: 커밋**

```bash
git add "SkillProject/Source/SkillProject/Character/AnimInstance/SpyAnimNotify_State_Parry.h"
git add "SkillProject/Source/SkillProject/Character/AnimInstance/SpyAnimNotify_State_Parry.cpp"
git commit -m "[Feature] SpyAnimNotify_State_Parry 추가 - 클라이언트 패링 윈도우 태그 동기화"
```

---

## Task 3: SpyGameplayAbility_Parry 구현

**Files:**
- Create: `SkillProject/Source/SkillProject/AbilitySystem/Parry/SpyGameplayAbility_Parry.h`
- Create: `SkillProject/Source/SkillProject/AbilitySystem/Parry/SpyGameplayAbility_Parry.cpp`

핵심 동작:
- 활성화 시 서버에서 `Character_State_Parry` 태그 추가 + `Skill_Parry_Hit` 이벤트 대기
- `Skill_Parry_Hit` 이벤트 수신 시 방어자 뒤로 `LaunchCharacter`
- 입력 해제 시 태그 제거 + 어빌리티 종료
- `EndAbility`에서 태그 잔류 방지 안전 처리

- [ ] **Step 1: 헤더 파일 생성**

`SkillProject/Source/SkillProject/AbilitySystem/Parry/SpyGameplayAbility_Parry.h`:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Ability/SKGameplayAbility.h"

#include "SpyGameplayAbility_Parry.generated.h"

UCLASS()
class SKILLPROJECT_API USpyGameplayAbility_Parry : public USKGameplayAbility
{
	GENERATED_BODY()

public:
	USpyGameplayAbility_Parry();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void InputReleased(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	virtual void OnWaitGameplayEvent(FGameplayEventData Payload) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parry")
	TObjectPtr<UAnimMontage> ParryMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parry")
	float KnockbackForce = 500.0f;
};
```

- [ ] **Step 2: 구현 파일 생성**

`SkillProject/Source/SkillProject/AbilitySystem/Parry/SpyGameplayAbility_Parry.cpp`:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "SpyGameplayAbility_Parry.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameFramework/Character.h"
#include "SKGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGameplayAbility_Parry)

USpyGameplayAbility_Parry::USpyGameplayAbility_Parry()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void USpyGameplayAbility_Parry::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (CommitAbility(Handle, ActorInfo, ActivationInfo) == false)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (HasAuthority(&ActivationInfo))
	{
		ActorInfo->AbilitySystemComponent->AddLooseGameplayTag(SKGameplayTags::Character_State_Parry);

		if (UAbilityTask_WaitGameplayEvent* WaitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, SKGameplayTags::Skill_Parry_Hit, nullptr, false, false))
		{
			WaitTask->EventReceived.AddDynamic(this, &USpyGameplayAbility_Parry::OnWaitGameplayEvent);
			WaitTask->ReadyForActivation();
		}
	}

	if (ParryMontage)
	{
		if (UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, ParryMontage))
		{
			MontageTask->OnInterrupted.AddDynamic(this, &USpyGameplayAbility_Parry::OnMontageInterrupted);
			MontageTask->OnCancelled.AddDynamic(this, &USpyGameplayAbility_Parry::OnMontageCancelled);
			MontageTask->ReadyForActivation();
		}
	}
}

void USpyGameplayAbility_Parry::InputReleased(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	CancelAbility(Handle, ActorInfo, ActivationInfo, true);
}

void USpyGameplayAbility_Parry::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (HasAuthority(&ActivationInfo))
	{
		UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
		if (ASC && ASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Parry))
		{
			ASC->RemoveLooseGameplayTag(SKGameplayTags::Character_State_Parry);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USpyGameplayAbility_Parry::OnWaitGameplayEvent(FGameplayEventData Payload)
{
	// Super::OnWaitGameplayEvent 호출 금지 — 기본 구현은 EndAbility를 호출함
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (OwnerCharacter == nullptr)
		return;

	if (HasAuthority(&CurrentActivationInfo) == false)
		return;

	FVector KnockbackDir = -OwnerCharacter->GetActorForwardVector();
	OwnerCharacter->LaunchCharacter(KnockbackDir * KnockbackForce, true, false);
}
```

- [ ] **Step 3: 빌드하여 컴파일 오류 없는지 확인**

Unreal Editor에서 **Tools > Compile**.  
예상 결과: 컴파일 성공. `USpyGameplayAbility_Parry` 클래스가 BP 생성 목록에 나타남.

- [ ] **Step 4: 커밋**

```bash
git add "SkillProject/Source/SkillProject/AbilitySystem/Parry/SpyGameplayAbility_Parry.h"
git add "SkillProject/Source/SkillProject/AbilitySystem/Parry/SpyGameplayAbility_Parry.cpp"
git commit -m "[Feature] SpyGameplayAbility_Parry 추가 - 홀드형 패링 GA"
```

---

## Task 4: SKGameplayAbility_SkillAction 패링 체크 삽입

**Files:**
- Modify: `SkillProject/Source/SKGAS/Ability/SKGameplayAbility_SkillAction.cpp`

`SendTagToTargetByWeapon`과 `SendTagToTargetBySphere` 두 함수에 패링 체크를 추가한다.
패링 성공 조건: 타겟 ASC에 `Character_State_Parry` 태그 있음 + 공격자가 방어자의 정면 90도 이내.
성공 시: `Skill_Parry_Hit` 이벤트를 방어자에게 전송하고 데미지 이벤트 스킵.

- [ ] **Step 1: SendTagToTargetByWeapon에 패링 체크 추가**

`SKGameplayAbility_SkillAction.cpp`의 `SendTagToTargetByWeapon` 함수에서 기존 death check 블록을 다음으로 교체:

**기존 코드 (151~196번째 줄 근처):**
```cpp
            if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetCharacter))
            {
                if (TargetASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Death))
                    continue;
            }

            bInvalidCharacter = true;
```

**변경 후:**
```cpp
            if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetCharacter))
            {
                if (TargetASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Death))
                    continue;

                if (!bIsHeal && TargetASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Parry))
                {
                    FVector DefenderForward = TargetCharacter->GetActorForwardVector();
                    FVector ToAttacker = (OwnerCharacter->GetActorLocation() - TargetCharacter->GetActorLocation()).GetSafeNormal();
                    if (FVector::DotProduct(DefenderForward, ToAttacker) > 0.0f)
                    {
                        FGameplayEventData ParryPayload;
                        ParryPayload.Instigator = OwnerCharacter;
                        ParryPayload.Target = TargetCharacter;
                        UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetCharacter, SKGameplayTags::Skill_Parry_Hit, ParryPayload);
                        continue;
                    }
                }
            }

            bInvalidCharacter = true;
```

- [ ] **Step 2: SendTagToTargetBySphere에 패링 체크 추가**

`SendTagToTargetBySphere` 함수에서 기존 TargetASC + 각도 체크 부분을 다음으로 교체.

**기존 코드 (280~295번째 줄 근처):**
```cpp
            if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetCharacter))
            {
                if (TargetASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Death))
                    continue;
            }

            FVector TargetVector = TargetCharacter->GetActorLocation() - OwnerCharacter->GetActorLocation();
            float TargetDegree = FMath::RadiansToDegrees(FMath::Acos(OwnerCharacter->GetActorForwardVector().CosineAngle2D(TargetVector)));

            UE_LOG(LogTemp, Log, TEXT("# [GA_SkillAction] TargetDegree %f"), TargetDegree);
            if (Degree < TargetDegree)
                continue;

            bInvalidCharacter = true;
```

**변경 후:**
```cpp
            UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetCharacter);
            if (TargetASC && TargetASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Death))
                continue;

            FVector TargetVector = TargetCharacter->GetActorLocation() - OwnerCharacter->GetActorLocation();
            float TargetDegree = FMath::RadiansToDegrees(FMath::Acos(OwnerCharacter->GetActorForwardVector().CosineAngle2D(TargetVector)));

            UE_LOG(LogTemp, Log, TEXT("# [GA_SkillAction] TargetDegree %f"), TargetDegree);
            if (Degree < TargetDegree)
                continue;

            if (!bIsHeal && TargetASC && TargetASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Parry))
            {
                FVector DefenderForward = TargetCharacter->GetActorForwardVector();
                FVector ToAttacker = (OwnerCharacter->GetActorLocation() - TargetCharacter->GetActorLocation()).GetSafeNormal();
                if (FVector::DotProduct(DefenderForward, ToAttacker) > 0.0f)
                {
                    FGameplayEventData ParryPayload;
                    ParryPayload.Instigator = OwnerCharacter;
                    ParryPayload.Target = TargetCharacter;
                    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetCharacter, SKGameplayTags::Skill_Parry_Hit, ParryPayload);
                    continue;
                }
            }

            bInvalidCharacter = true;
```

- [ ] **Step 3: 빌드하여 컴파일 오류 없는지 확인**

Unreal Editor에서 **Tools > Compile**.  
예상 결과: 컴파일 성공.

- [ ] **Step 4: 커밋**

```bash
git add SkillProject/Source/SKGAS/Ability/SKGameplayAbility_SkillAction.cpp
git commit -m "[Feature] SKGameplayAbility_SkillAction에 패링 체크 추가 - 정면 공격 차단 및 Skill_Parry_Hit 이벤트 전송"
```

---

## Task 5: Blueprint 및 에디터 세팅

UE Editor에서 수행. C++ 코드 수정 없음.

- [ ] **Step 1: GA_Parry 블루프린트 생성**

Content Browser에서 `Content/Spy/Abilities/` 경로에 블루프린트 생성:
- 부모 클래스: `SpyGameplayAbility_Parry`
- 이름: `GA_Parry`
- `KnockbackForce` = 500.0 (기본값, 테스트 후 조정)
- `ParryMontage` = 파링 아이들 몽타주 에셋 지정 (없으면 None으로 두어도 동작)

- [ ] **Step 2: SpyCharacterAssetData에 GA_Parry 등록**

`Content/Spy/Data/SpyCharacterAssetData`를 열고 플레이어 캐릭터 항목의 AbilityData에 `GA_Parry` 추가. 입력 태그를 `Input.Ability.Parry`로 설정.

- [ ] **Step 3: Enhanced Input에 파링 입력 바인딩 추가**

`SpyInputConfig` DataAsset에서 파링 액션에 `Input.Ability.Parry` 태그를 바인딩. 실제 키는 프로젝트 Input Action 에셋에서 지정 (예: 마우스 우클릭, Q 키 등).

- [ ] **Step 4: 인에디터 동작 확인**

에디터 PIE(Play In Editor)에서:
1. 플레이어 캐릭터로 플레이 시작
2. 파링 키 홀드 → Output Log에 `[SKGameplayAbility] ActivateAbility` 로그 확인
3. AI가 공격 → Output Log에 `[SKGameplayAbility] CommitAbility` 로그 + 플레이어 캐릭터가 뒤로 밀림 확인
4. 키 해제 → `[SKGameplayAbility] CancelAbility` 로그 확인
5. 파링 키 없이 AI 공격 → 정상적으로 데미지 적용되는지 확인 (파링 미발동 케이스)
6. 옆/뒤에서 오는 공격 → 패링 되지 않고 데미지 적용되는지 확인 (정면 체크 케이스)

- [ ] **Step 5: 커밋**

```bash
git add SkillProject/Content/Spy/
git commit -m "[Data] GA_Parry 블루프린트 및 캐릭터 어빌리티 등록"
```

---

## 완료 기준

- [ ] 파링 키 홀드 중 정면에서 오는 공격이 차단되고 플레이어가 뒤로 밀림
- [ ] 파링 키 없이 공격받으면 데미지 정상 적용
- [ ] 정면이 아닌 공격(90도 밖)은 파링 윈도우 중에도 데미지 정상 적용
- [ ] 파링 키 해제 후 공격받으면 데미지 정상 적용
- [ ] 힐 스킬(bIsHeal=true)이 파링에 영향받지 않음
