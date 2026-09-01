# AI 봇 사망 디졸브 연출 + 재활용 리스폰 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.
>
> **이 프로젝트 예외 — 위 기본 안내보다 아래가 우선한다:**
> 이 저장소는 `start-develop` 파이프라인(`.claude/skills/start-develop`)을 표준 실행 경로로 쓴다. 이 플랜은 `superpowers:subagent-driven-development`/`executing-plans` 대신, **game-designer(도메인 확정 불필요 시 스킵 가능) → gameplay-programmer(Task 2~9) → code-reviewer → test-engineer(Task 10)** 로 이어지는 `start-develop` 파이프라인으로 실행되는 것을 전제로 작성됐다. Task 1과 Task 9 후반부(에셋 파트)는 **gameplay-programmer 가 아니라 메인 오케스트레이터가 unreal-mcp 로 직접 수행**한다 (`gameplay-programmer` 에이전트는 MCP 툴 접근 권한이 없음).
> 이 프로젝트에는 CLI 빌드/테스트 러너가 없다(`project.md`: "빌드/테스트는 에디터·VS 에서 사용자 수행"). 각 Task 의 "검증" 스텝은 `pytest` 류 자동 실행이 아니라 **컴파일은 사용자가 에디터/VS 에서 수행, Automation 테스트는 test-engineer 가 작성 후 사용자가 Automation Window 에서 실행**하는 형태로 대체한다.
> 커밋은 `git commit` 을 직접 실행하지 않는다 — 각 Task 의 "Step: 커밋" 은 `git add` + 커밋 메시지(안) 제시로 대체한다 (`git-conventions.md`, `feedback_no_auto_commit` 메모리).

**Goal:** AI 봇이 사망하면 2초간 디졸브 연출로 사라진 뒤, 같은 Character/AIController 인스턴스를 재활용해 자기 원래 스폰 지점에 다시 등장하게 한다.

**Architecture:** 서버가 사망 시 GameplayCue(`GameplayCue.Actor.Death`)로 머티리얼 디졸브 연출을 트리거하고, 동시에 `SpySpawnBotManagerComponent`(서버 전용)가 독립적으로 같은 사망 신호를 구독해 `DissolveDurationSeconds` 후 `ASpyCharacter::OnRespawn()` 이라는 단일 진입점을 호출한다. `OnRespawn()` 이 텔레포트·Health 복구·사망 태그 해제·콜리전 복구·AI 로직 재시작을 전부 캡슐화한다(§13 루트 파사드 — `OnDeath()` 와 대칭).

**Tech Stack:** Unreal Engine 5.7, C++, Gameplay Ability System(GAS), unreal-mcp(Python, 에셋 편집 전용).

## Global Constraints

- 서버 권한: 게임플레이 상태 변경(사망 처리, 리스폰, Health 리셋)은 반드시 `HasAuthority()`/`GetOwnerRole() == ROLE_Authority` 가드 안에서 수행 — 클라이언트 연출(Cue 시각효과)만 Authority 밖.
- `auto` 금지, 가드 절 중괄호 없이, `!` 단항 부정 금지(`== false`/`== nullptr` 명시), 매직넘버 금지(수치는 `SpyAIConfig` 또는 이름 있는 상수).
- 문자열 리터럴 게임플레이 태그 참조 금지 — 전부 `SpyGameplayTags.h`/`.cpp` 에 등록.
- `FindComponentByClass`/`GetAllActorsOfClass` 를 Tick·이벤트 경로에서 반복 호출 금지 — 초기화/스폰 시점 1회 캐싱.
- 커밋 메시지 포맷: `[Tag] ClassName — 요약` (`git-conventions.md`). Task 마다 예시 메시지(안)를 제시하며, 실제 `git commit` 은 사용자가 수행.

---

### Task 1: `M_Mannequin` 디졸브 머티리얼 파라미터 + Cue Notify 에셋 배선

**담당:** 메인 오케스트레이터 (unreal-mcp `execute_python`) — gameplay-programmer 대상 아님. **Task 8(Cue Notify C++ 클래스) 컴파일 완료 후에 후반부(BP 생성)를 수행한다** — 전반부(머티리얼 그래프 편집)는 지금 바로 가능하다.

**Files:**
- Modify (에셋): `/Game/Spy/Characters/Mannequins/Materials/M_Mannequin` (blend mode는 이미 Masked, 변경 불필요)
- Create (에셋, Task 8 완료 후): `/Game/Spy/Blueprints/GameplayAbilities/Cue/GC_Notify_Character_Death` (부모 = `USpyGameplayCueNotify_Death`)

**Interfaces:**
- Produces: 머티리얼 스칼라 파라미터 이름 `"Dissolve"` (0.0=완전 불투명/생존, 1.0=완전 투명/디졸브 완료) — Task 8의 Cue Notify C++ 코드가 이 정확한 문자열로 `SetScalarParameterValue` 를 호출한다. **이름이 다르면 아무 효과도 나지 않으므로 Task 8 작성자와 문자열이 반드시 일치해야 한다.**

- [ ] **Step 1: 확인된 사실 재확인 (조사 완료, 실행 시 재검증만)**

`M_Mannequin` 의 `blend_mode` 는 이미 `BLEND_MASKED`, `MP_OPACITY_MASK` 입력은 비어있음(연결된 노드 없음) — 블렌드 모드 변경 없이 파라미터 추가만으로 작업 가능함을 조사 단계에서 확인함(2026-08-12).

- [ ] **Step 2: Dissolve 스칼라 파라미터 + 노이즈 텍스처 기반 OpacityMask 배선**

```python
import unreal

mat = unreal.EditorAssetLibrary.load_asset("/Game/Spy/Characters/Mannequins/Materials/M_Mannequin.M_Mannequin")

dissolve_param = unreal.MaterialEditingLibrary.create_material_expression(
    mat, unreal.MaterialExpressionScalarParameter, -600, 600)
dissolve_param.set_editor_property("parameter_name", "Dissolve")
dissolve_param.set_editor_property("default_value", 0.0)

noise_tex = unreal.EditorAssetLibrary.load_asset("/Engine/EngineMaterials/T_Perlin_Noise_M.T_Perlin_Noise_M")
noise_sample = unreal.MaterialEditingLibrary.create_material_expression(
    mat, unreal.MaterialExpressionTextureSample, -600, 750)
noise_sample.set_editor_property("texture", noise_tex)

step_node = unreal.MaterialEditingLibrary.create_material_expression(
    mat, unreal.MaterialExpressionStep, -300, 650)

unreal.MaterialEditingLibrary.connect_material_expressions(dissolve_param, "", step_node, "Y")
unreal.MaterialEditingLibrary.connect_material_expressions(noise_sample, "R", step_node, "X")
unreal.MaterialEditingLibrary.connect_material_property(step_node, "", unreal.MaterialProperty.MP_OPACITY_MASK)

unreal.MaterialEditingLibrary.recompile_material(mat)
unreal.EditorAssetLibrary.save_asset("/Game/Spy/Characters/Mannequins/Materials/M_Mannequin", only_if_is_dirty=False)
```

`Step(Y=Dissolve, X=NoiseR)` 는 `NoiseR >= Dissolve` 면 1(보임), 아니면 0(가려짐) — `Dissolve` 가 0→1로 올라갈수록 노이즈 값이 낮은 픽셀부터 순차적으로 가려져 유기적인 디졸브 패턴이 나온다.

`/Engine/EngineMaterials/T_Perlin_Noise_M` 이 없으면(엔진 버전에 따라 경로가 다를 수 있음) `unreal.EditorAssetLibrary.list_assets("/Engine/EngineMaterials", recursive=False)` 로 실제 노이즈 텍스처 이름을 먼저 확인한다.

- [ ] **Step 3: 에디터에서 재확인**

`get_material_instance_scalar_parameter_value` 또는 머티리얼 에디터를 열어 `MI_Manny_01`/`MI_Manny_02` 양쪽에서 `Dissolve` 파라미터가 노출되는지 확인(부모 머티리얼에 추가했으므로 자동 상속됨).

- [ ] **Step 4 (Task 8 완료 후): Cue Notify Blueprint 생성**

```python
import unreal

factory = unreal.BlueprintFactory()
factory.set_editor_property("parent_class", unreal.load_class(None, "/Script/SkillProject.SpyGameplayCueNotify_Death"))
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
bp = asset_tools.create_asset("GC_Notify_Character_Death", "/Game/Spy/Blueprints/GameplayAbilities/Cue", None, factory)

cdo = unreal.get_default_object(bp.generated_class())
tag = unreal.GameplayTagsManager.get().request_gameplay_tag(unreal.Name("GameplayCue.Actor.Death"))
cdo.set_editor_property("gameplay_cue_tag", tag)

unreal.EditorAssetLibrary.save_asset("/Game/Spy/Blueprints/GameplayAbilities/Cue/GC_Notify_Character_Death", only_if_is_dirty=False)
```

**주의 (`ui-workflow.md` §2-3):** 이 Blueprint 에 `compile_blueprint()` 를 호출하지 않는다 — 에디터가 데드락된다. 저장만 하고 컴파일은 사용자가 1회 수행.

- [ ] **Step 5: 리드백 검증**

`get_blueprint_cdo_properties("/Game/Spy/Blueprints/GameplayAbilities/Cue/GC_Notify_Character_Death")` 로 `gameplay_cue_tag` 가 `GameplayCue.Actor.Death` 로 저장됐는지 재조회.

---

### Task 2: `SpyGameplayTags` 에 사망 Cue 태그 추가

**Files:**
- Modify: `SkillProject/Source/SkillProject/Util/SpyGameplayTags.h:109-113`
- Modify: `SkillProject/Source/SkillProject/Util/SpyGameplayTags.cpp:87-90`

**Interfaces:**
- Produces: `SpyGameplayTags::GameplayCue_Actor_Death` (FGameplayTag, 문자열 `"GameplayCue.Actor.Death"`) — Task 7(GA_Death)과 Task 1(Cue Notify BP)이 이 태그를 참조.

- [ ] **Step 1: 헤더에 선언 추가**

`SpyGameplayTags.h` 의 기존 Cue 태그 블록에 추가:

```cpp
	//# Actor 게임플레이큐
	SKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Actor_Attack);
	SKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Actor_Target);
	SKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Actor_Death);
```

- [ ] **Step 2: .cpp 에 정의 추가**

`SpyGameplayTags.cpp` 의 기존 정의 옆에 추가:

```cpp
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Actor_Death, "GameplayCue.Actor.Death");
```

- [ ] **Step 3: 컴파일 확인**

사용자가 Unreal Editor 또는 Visual Studio 에서 컴파일해 태그 등록에 오류가 없는지 확인.

- [ ] **Step 4: 스테이징**

```bash
git add SkillProject/Source/SkillProject/Util/SpyGameplayTags.h SkillProject/Source/SkillProject/Util/SpyGameplayTags.cpp
```

커밋 메시지(안): `[Feature] SpyGameplayTags — 사망 디졸브 GameplayCue 태그 추가`

---

### Task 3: `SpyAIConfig` 에 `DissolveDurationSeconds` 추가

**Files:**
- Modify: `SkillProject/Source/SkillProject/Data/SpyAIConfig.h`

**Interfaces:**
- Produces: `USpyAIConfig::DissolveDurationSeconds` (float, `EditDefaultsOnly`, 기본값 `2.0f`) — Task 4(`ASpyAIController::GetDissolveDurationSeconds`)와 Task 9(`SpySpawnBotManagerComponent`)가 소비.
- 기존 에셋 `/Game/Spy/Data/DA/Config/DA_SpyAIConfig` 는 새 필드를 자동으로 C++ 기본값(2.0f)으로 표시한다 — 별도 MCP 값 세팅 불필요(신규 프로퍼티라 직렬화된 값이 없으므로 CDO 기본값을 그대로 씀).

- [ ] **Step 1: 필드 추가**

```cpp
	//# Death / Respawn
	UPROPERTY(EditDefaultsOnly, Category = "Death")
	float DissolveDurationSeconds = 2.0f;
```

`Perception|Damage` 카테고리 블록 다음에 추가.

- [ ] **Step 2: 컴파일 확인 후 스테이징**

```bash
git add SkillProject/Source/SkillProject/Data/SpyAIConfig.h
```

커밋 메시지(안): `[Feature] SpyAIConfig — 디졸브 지속시간 수치 추가`

---

### Task 4: `ASpyAIController` — 디졸브 시간 게터 + AI 로직 재시작 API

**Files:**
- Modify: `SkillProject/Source/SkillProject/System/SpyAIController.h`
- Modify: `SkillProject/Source/SkillProject/System/SpyAIController.cpp`

**Interfaces:**
- Consumes: `USpyAIConfig::DissolveDurationSeconds` (Task 3), `BehaviorTreeAsset`(기존 private 멤버, `SpyAIController.cpp:151`), `AIConfig`(기존 protected 멤버).
- Produces:
  - `float ASpyAIController::GetDissolveDurationSeconds() const` — `AIConfig` 가 유효하면 그 값, 아니면 기본값 `DefaultDissolveDurationSeconds`(2.0f, `static constexpr`) 반환.
  - `void ASpyAIController::RestartAILogic()` — Task 9(`SpySpawnBotManagerComponent::RespawnBot`)이 리스폰 시 호출.

- [ ] **Step 1: 헤더에 선언 추가**

`SpyAIController.h` 의 `public:` 섹션(`SetBehaviorTree` 옆)에:

```cpp
	float GetDissolveDurationSeconds() const;
	void RestartAILogic();

private:
	static constexpr float DefaultDissolveDurationSeconds = 2.0f;
```

- [ ] **Step 2: .cpp 구현**

`SetBehaviorTree` 구현부(`SpyAIController.cpp:177` 부근) 옆에 추가:

```cpp
float ASpyAIController::GetDissolveDurationSeconds() const
{
	if (AIConfig == nullptr)
		return DefaultDissolveDurationSeconds;

	return AIConfig->DissolveDurationSeconds;
}

void ASpyAIController::RestartAILogic()
{
	//# OnPossess 는 최초 1회만 RunBehaviorTree 를 호출한다 — 리스폰 시 재활용된
	//# 컨트롤러는 재빙의되지 않으므로 여기서 직접 재시작한다.
	if (BehaviorTreeAsset != nullptr)
	{
		RunBehaviorTree(BehaviorTreeAsset);
	}

	if (AIPerceptionComponent != nullptr)
	{
		AIPerceptionComponent->RequestStimuliListenerUpdate();
	}
}
```

`BehaviorTreeAsset` 은 이미 `private` 멤버로 존재(§`SpyAIController.h:90`)하므로 클래스 내부에서 바로 접근 가능.

- [ ] **Step 3: 컴파일 확인 후 스테이징**

```bash
git add SkillProject/Source/SkillProject/System/SpyAIController.h SkillProject/Source/SkillProject/System/SpyAIController.cpp
```

커밋 메시지(안): `[Feature] SpyAIController — 리스폰용 AI 로직 재시작 API 추가`

---

### Task 5: `SKAttributeSet` — Health 음수 클램프 버그 수정

**Files:**
- Modify: `SkillProject/Plugins/SKGAS/Source/SKGAS/Attribute/SKAttributeSet.cpp:71-82`

**Interfaces:**
- 없음(내부 버그 수정, 외부 시그니처 변화 없음). Task 9(`RespawnBot`)이 `SetNumericAttributeBase(Health, MaxHealth)` 로 복구할 때, 클램프된 하한(0) 이후 상태에서 시작한다는 전제만 성립하면 됨.

- [ ] **Step 1: 사망 분기에 클램프 추가**

`Data.Target.HandleGameplayEvent(Payload.EventTag, &Payload);` 호출 앞뒤 어디든, `GetHealth() <= 0.0f` 분기에 들어가기 **전에** 클램프하면 사망 판정 자체(`<= 0.0f`)에는 영향 없이 값만 바로잡을 수 있다. 기존 코드:

```cpp
            //# 사망 확인
            if (GetHealth() <= 0.0f)
            {
```

를 다음으로 교체:

```cpp
            //# Health 를 [0, MaxHealth] 로 클램프 — 클램프 없이 음수로 내려가는 버그 수정.
            //# 사망 판정(<= 0.0f)은 클램프 전 값 기준으로 이미 참이므로 순서를 바꿔도 무관.
            SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));

            //# 사망 확인
            if (GetHealth() <= 0.0f)
            {
```

Mana 분기(`SKAttributeSet.cpp:105-107`)의 기존 클램프 패턴과 동일한 스타일.

- [ ] **Step 2: 컴파일 확인 후 스테이징**

```bash
git add SkillProject/Plugins/SKGAS/Source/SKGAS/Attribute/SKAttributeSet.cpp
```

커밋 메시지(안): `[Fix] SKAttributeSet — Health 음수 클램프 누락 버그 수정`

---

### Task 6: `SpyHealthComponent` — `bDead` 래치 추가 (중복 `OnDeath` 발화 방지)

**Files:**
- Modify: `SkillProject/Source/SkillProject/Character/SpyHealthComponent.h`
- Modify: `SkillProject/Source/SkillProject/Character/SpyHealthComponent.cpp:78-83`

**Interfaces:**
- Produces: `void USpyHealthComponent::ResetDeathState()` — 리스폰 완료 시 Task 9가 호출해 래치를 해제한다. 해제 전까지는 `HandleHealthChanged` 가 `NewValue <= 0` 이어도 `OnDeath` 를 재발화하지 않는다.

- [ ] **Step 1: 헤더에 래치 필드 + 리셋 메서드 선언**

```cpp
	//# 사망 후 추가 피해로 OnDeath 가 중복 발화되는 것을 막는 래치. 리스폰 완료 시 ResetDeathState() 로 해제.
	UFUNCTION()
	void ResetDeathState();

protected:
	bool bDead = false;
```

`OnUnregister()` 선언 위/아래 적절한 위치에 추가 (public 섹션에 `ResetDeathState`, protected 섹션에 `bDead`).

- [ ] **Step 2: `HandleHealthChanged` 에 래치 적용**

```cpp
void USpyHealthComponent::HandleHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	if (NewValue <= 0 && bDead == false)
	{
		bDead = true;
		OnDeath.Broadcast(DamageInstigator, DamageCauser);
	}

	OnHealthChanged.Broadcast(this, OldValue, NewValue, DamageInstigator);
	...
```

이 함수의 나머지 로직(카메라 쉐이크 등, `SpyHealthComponent.cpp:87-152`)은 변경하지 않는다.

- [ ] **Step 3: `ResetDeathState` 구현**

```cpp
void USpyHealthComponent::ResetDeathState()
{
	bDead = false;
}
```

- [ ] **Step 4: 컴파일 확인 후 스테이징**

```bash
git add SkillProject/Source/SkillProject/Character/SpyHealthComponent.h SkillProject/Source/SkillProject/Character/SpyHealthComponent.cpp
```

커밋 메시지(안): `[Fix] SpyHealthComponent — OnDeath 중복 발화 방지 bDead 래치 추가`

---

### Task 7: `USpyGA_Death` — 서버에서 디졸브 GameplayCue 실행

**Files:**
- Modify: `SkillProject/Source/SkillProject/AbilitySystem/Skill/SpyGA_Death.cpp`

**Interfaces:**
- Consumes: `SpyGameplayTags::GameplayCue_Actor_Death`(Task 2), `ASpyAIController::GetDissolveDurationSeconds()`(Task 4).
- Produces: 서버가 실행하는 `GameplayCue.Actor.Death` 이벤트, `Parameters.RawMagnitude` 에 디졸브 지속시간(초)을 실어 클라이언트로 리플리케이트 — Task 8(Cue Notify)이 `Parameters.RawMagnitude` 를 읽어 타이밍을 맞춘다 (AIController 는 클라이언트에 존재하지 않을 수 있어 컨트롤러 재조회 대신 Cue 파라미터로 값을 전달).

- [ ] **Step 1: `ActivateAbility` 에 서버 전용 Cue 실행 추가**

기존:

```cpp
    if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
    {
        if (UCapsuleComponent* CapsuleComp = OwnerCharacter->GetCapsuleComponent())
        {
            CapsuleComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
        }

		ISpyCharacterRoot* RootPtr = Cast<ISpyCharacterRoot>(OwnerCharacter);
		TScriptInterface<ISpyTargetProvider> TargetProviderHandle = RootPtr ? RootPtr->GetTargetProvider() : TScriptInterface<ISpyTargetProvider>();
		if (ISpyTargetProvider* TargetingComp = IsValid(TargetProviderHandle.GetObject()) ? TargetProviderHandle.GetInterface() : nullptr)
		{
			TargetingComp->FindTarget(0.f);
		}
    }
```

다음으로 교체(Cue 실행 블록 추가, 기존 로직 유지):

```cpp
    if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
    {
        if (UCapsuleComponent* CapsuleComp = OwnerCharacter->GetCapsuleComponent())
        {
            CapsuleComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
        }

		ISpyCharacterRoot* RootPtr = Cast<ISpyCharacterRoot>(OwnerCharacter);
		TScriptInterface<ISpyTargetProvider> TargetProviderHandle = RootPtr ? RootPtr->GetTargetProvider() : TScriptInterface<ISpyTargetProvider>();
		if (ISpyTargetProvider* TargetingComp = IsValid(TargetProviderHandle.GetObject()) ? TargetProviderHandle.GetInterface() : nullptr)
		{
			TargetingComp->FindTarget(0.f);
		}

		if (HasAuthority(&ActivationInfo))
		{
			//# 디졸브 지속시간을 Cue 파라미터에 실어 보낸다 — 클라이언트에는 AIController 가 없을 수 있어
			//# Cue 가 직접 컨트롤러를 재조회하지 않고 이 값을 그대로 쓴다.
			float DissolveDurationSeconds = 2.0f;
			if (AAIController* AIController = Cast<AAIController>(OwnerCharacter->GetController()))
			{
				if (ASpyAIController* SpyAIController = Cast<ASpyAIController>(AIController))
				{
					DissolveDurationSeconds = SpyAIController->GetDissolveDurationSeconds();
				}
			}

			FGameplayCueParameters CueParameters;
			CueParameters.RawMagnitude = DissolveDurationSeconds;
			K2_ExecuteGameplayCue(SpyGameplayTags::GameplayCue_Actor_Death, CueParameters);
		}
    }
```

- [ ] **Step 2: include 추가**

`SpyGA_Death.cpp` 상단에 추가:

```cpp
#include "System/SpyAIController.h"
#include "Util/SpyGameplayTags.h"
```

- [ ] **Step 3: 컴파일 확인 후 스테이징**

```bash
git add SkillProject/Source/SkillProject/AbilitySystem/Skill/SpyGA_Death.cpp
```

커밋 메시지(안): `[Feature] SpyGA_Death — 사망 디졸브 GameplayCue 실행`

---

### Task 8: `USpyGameplayCueNotify_Death` — 디졸브 머티리얼 타임라인 재생

**Files:**
- Create: `SkillProject/Source/SkillProject/AbilitySystem/Cue/SpyGameplayCueNotify_Death.h`
- Create: `SkillProject/Source/SkillProject/AbilitySystem/Cue/SpyGameplayCueNotify_Death.cpp`

**Interfaces:**
- Consumes: `Parameters.RawMagnitude`(Task 7이 채움), 머티리얼 파라미터 이름 `"Dissolve"`(Task 1이 배선).
- Produces: 이 클래스는 Task 1-Step4 에서 Blueprint 부모 클래스로 참조된다(`/Script/SkillProject.SpyGameplayCueNotify_Death`).

- [ ] **Step 1: 헤더 작성**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "SpyGameplayCueNotify_Death.generated.h"

UCLASS()
class SKILLPROJECT_API USpyGameplayCueNotify_Death : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	USpyGameplayCueNotify_Death();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

private:
	static constexpr float DefaultDissolveDurationSeconds = 2.0f;
	static const FName DissolveParameterName;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterials;

	float DissolveDurationSeconds = DefaultDissolveDurationSeconds;
	float ElapsedSeconds = 0.f;
	bool bIsDissolving = false;
};
```

**주의:** `AGameplayCueNotify_Actor` 는 `UGameplayCueNotify_Actor` 가 아니라 액터 베이스(`GameplayCueNotify_Actor.h`) 이름이 헷갈리기 쉽다 — 실제 엔진 클래스명은 `AGameplayCueNotify_Actor` (Actor 파생)이다. 구현 시 정확한 엔진 헤더/클래스명을 IDE 자동완성으로 재확인할 것.

- [ ] **Step 2: .cpp 작성**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Cue/SpyGameplayCueNotify_Death.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

const FName USpyGameplayCueNotify_Death::DissolveParameterName(TEXT("Dissolve"));

USpyGameplayCueNotify_Death::USpyGameplayCueNotify_Death()
{
	PrimaryActorTick.bCanEverTick = false;
	bAutoDestroyOnRemove = false;
}

bool USpyGameplayCueNotify_Death::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	return false;
}

bool USpyGameplayCueNotify_Death::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	ACharacter* TargetCharacter = Cast<ACharacter>(MyTarget);
	if (TargetCharacter == nullptr)
		return false;

	USkeletalMeshComponent* Mesh = TargetCharacter->GetMesh();
	if (Mesh == nullptr)
		return false;

	DissolveDurationSeconds = (Parameters.RawMagnitude > 0.f) ? Parameters.RawMagnitude : DefaultDissolveDurationSeconds;
	ElapsedSeconds = 0.f;
	bIsDissolving = true;

	DynamicMaterials.Reset();
	const int32 NumMaterials = Mesh->GetNumMaterials();
	for (int32 SlotIndex = 0; SlotIndex < NumMaterials; ++SlotIndex)
	{
		UMaterialInstanceDynamic* MID = Mesh->CreateDynamicMaterialInstance(SlotIndex);
		if (MID != nullptr)
		{
			MID->SetScalarParameterValue(DissolveParameterName, 0.f);
			DynamicMaterials.Add(MID);
		}
	}

	PrimaryActorTick.bCanEverTick = true;
	SetActorTickEnabled(true);

	return false;
}

bool USpyGameplayCueNotify_Death::WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	return false;
}

void USpyGameplayCueNotify_Death::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDissolving == false)
		return;

	ElapsedSeconds += DeltaTime;
	const float Alpha = (DissolveDurationSeconds > 0.f) ? FMath::Clamp(ElapsedSeconds / DissolveDurationSeconds, 0.f, 1.f) : 1.f;

	for (const TObjectPtr<UMaterialInstanceDynamic>& MID : DynamicMaterials)
	{
		if (IsValid(MID))
		{
			MID->SetScalarParameterValue(DissolveParameterName, Alpha);
		}
	}

	if (Alpha >= 1.f)
	{
		bIsDissolving = false;
		SetActorTickEnabled(false);
	}
}

bool USpyGameplayCueNotify_Death::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	//# 리스폰 시 Character.State.Death 태그가 해제되면(ASpyCharacter::OnRespawn) 이 Cue 도 함께 제거된다.
	//# 다음 사망을 위해 파라미터를 0으로 되돌려 둔다 — 인스턴스가 재사용되는 캐릭터이므로 필수.
	for (const TObjectPtr<UMaterialInstanceDynamic>& MID : DynamicMaterials)
	{
		if (IsValid(MID))
		{
			MID->SetScalarParameterValue(DissolveParameterName, 0.f);
		}
	}

	DynamicMaterials.Reset();
	bIsDissolving = false;
	SetActorTickEnabled(false);

	return false;
}
```

**설계 근거 — Cue 종료 타이밍**: `ActivationOwnedTags = Character.State.Death` 가 GA_Death 의 활성 상태를 유지하는 한 이 Cue 도 `WhileActive` 상태로 유지된다(GAS 표준: Cue 는 태그 카운트에 연동). Task 9의 `ASpyCharacter::OnRespawn()` 이 `CancelAbilitiesByTag(Skill.Util.Death)` 를 호출해 GA_Death 를 종료시키면 `Character.State.Death` 태그가 사라지고, 그 결과 이 Cue 의 `OnRemove` 가 자동 호출된다 — 리스폰 로직이 Cue 를 직접 참조하거나 정지시킬 필요가 없다.

- [ ] **Step 3: 컴파일 확인 후 스테이징**

```bash
git add SkillProject/Source/SkillProject/AbilitySystem/Cue/SpyGameplayCueNotify_Death.h SkillProject/Source/SkillProject/AbilitySystem/Cue/SpyGameplayCueNotify_Death.cpp
```

커밋 메시지(안): `[Feature] SpyGameplayCueNotify_Death — 디졸브 머티리얼 타임라인 재생`

---

### Task 9: `ASpyCharacter::OnRespawn` + `SpySpawnBotManagerComponent` 리스폰 오케스트레이션

**Files:**
- Modify: `SkillProject/Source/SkillProject/Character/SpyCharacter.h:124-126`
- Modify: `SkillProject/Source/SkillProject/Character/SpyCharacter.cpp:242-266`
- Modify: `SkillProject/Source/SkillProject/ManagerComponent/SpySpawnBotManagerComponent.h`
- Modify: `SkillProject/Source/SkillProject/ManagerComponent/SpySpawnBotManagerComponent.cpp`

**Interfaces:**
- Consumes: `USpyHealthComponent::ResetDeathState()`(Task 6), `ASpyAIController::RestartAILogic()`(Task 4), `ASpyAIController::GetDissolveDurationSeconds()`(Task 4), `SpyGameplayTags::Skill_Util_Death`(기존 태그, `SpyGameplayTags.h:98`).
- Produces: `void ASpyCharacter::OnRespawn(const FTransform& RespawnTransform)` — 리스폰 시 필요한 모든 상태 복구를 캡슐화하는 단일 진입점(`OnDeath()` 와 대칭, §13 루트 파사드). `SpySpawnBotManagerComponent` 를 포함한 모든 외부 호출자는 이 메서드만 호출하고 캡슐 콜리전·ASC·태그를 직접 건드리지 않는다.

**설계 근거 — 캡슐화 개선**: 승인된 스펙 문서(§4)는 리스폰 절차(텔레포트·Health 복구·태그 제거·콜리전 복구·AI 재시작)를 `SpySpawnBotManagerComponent` 가 직접 수행하는 것으로 서술했으나, 이는 `cpp-style.md` §8(컴포넌트 탐색/내부 상태 직접 조작 지양)·§13(루트 파사드) 위반이다. `OnDeath()` 가 이미 캐릭터 자신의 메서드로 캡슐 콜리전·타겟팅·AI 정지를 처리하므로, 대칭되는 `OnRespawn()` 도 캐릭터 자신에 두고 `SpySpawnBotManagerComponent` 는 "언제 부를지"(타이머)만 결정한다.

- [ ] **Step 1: `ASpyCharacter` 헤더에 `OnRespawn` 선언**

`SpyCharacter.h:125` (`OnDeath` 선언) 바로 아래:

```cpp
	UFUNCTION(BlueprintCallable)
	virtual void OnRespawn(const FTransform& RespawnTransform);
```

- [ ] **Step 2: `ASpyCharacter::OnRespawn` 구현**

`SpyCharacter.cpp` 의 `OnDeath` 구현(라인 242-266) 바로 아래에 추가:

```cpp
void ASpyCharacter::OnRespawn(const FTransform& RespawnTransform)
{
	SetActorLocationAndRotation(RespawnTransform.GetLocation(), RespawnTransform.GetRotation());

	if (UCapsuleComponent* CapsuleComp = GetCapsuleComponent())
	{
		CapsuleComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
	}

	if (USpyAbilitySystemComponent* SpyASC = GetSpyAbilitySystemComponent())
	{
		//# GA_Death 를 종료시키면 ActivationOwnedTags(Character.State.Death) 가 자동 해제된다.
		SpyASC->CancelAbilitiesByTag(FGameplayTagContainer(SpyGameplayTags::Skill_Util_Death));

		if (USpyCharacterAttributeSet* AttributeSet = const_cast<USpyCharacterAttributeSet*>(SpyASC->GetSet<USpyCharacterAttributeSet>()))
		{
			SpyASC->SetNumericAttributeBase(USpyCharacterAttributeSet::GetHealthAttribute(), AttributeSet->GetMaxHealth());
		}
	}

	if (SpyHealthComponent != nullptr)
	{
		SpyHealthComponent->ResetDeathState();
	}

	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		if (ASpyAIController* SpyAIController = Cast<ASpyAIController>(AIController))
		{
			SpyAIController->RestartAILogic();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("# [SpyCharacter] OnRespawn: %s"), *GetName());
}
```

`GetSet<USpyCharacterAttributeSet>()` 이 `const USpyCharacterAttributeSet*` 를 반환하는 기존 패턴은 `SpyHealthComponent.cpp:27`과 동일. `SetNumericAttributeBase` 사용 패턴은 `SpyLevelComponent.cpp:247`(레벨업 시 풀힐)과 동일.

**필요 include** (`SpyCharacter.cpp` 상단에 이미 없다면 추가): `System/SpyAIController.h`.

- [ ] **Step 3: `SpySpawnBotManagerComponent` 헤더 — 봇별 트래킹 구조 확장**

`SpySpawnBotManagerComponent.h` 의 `SpawnedBotList` 를 대체:

```cpp
USTRUCT()
struct FSpySpawnedBotInfo
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AAIController> Controller;

	UPROPERTY()
	FTransform SpawnTransform;
};
```

(구조체는 클래스 선언 위, `.generated.h` include 아래에 추가)

`protected:` 섹션의 `SpawnedBotList` 선언을 교체:

```cpp
	UPROPERTY()
	TArray<FSpySpawnedBotInfo> SpawnedBotList;
```

`private:` 섹션에 리스폰 콜백 추가:

```cpp
private:
	void HandleBotDeath(AActor* InOwningActor, AActor* InCauserActor, TWeakObjectPtr<AAIController> InController);
	void RespawnBot(TWeakObjectPtr<AAIController> InController);

	UPROPERTY()
	TMap<TObjectPtr<AAIController>, FTimerHandle> RespawnTimers;
```

**Interfaces (구조체):**
- Produces: `FSpySpawnedBotInfo{Controller, SpawnTransform}` — `SpawnOneBot` 이 채우고 `HandleBotDeath`/`RespawnBot` 이 조회.

- [ ] **Step 4: `SpawnOneBot` — 스폰 트랜스폼 기억 + 사망 델리게이트 구독**

`SpySpawnBotManagerComponent.cpp:72` 부근 (`SpawnedBotList.Add(NewController);`) 교체:

```cpp
		FSpySpawnedBotInfo BotInfo;
		BotInfo.Controller = NewController;
		BotInfo.SpawnTransform = FTransform(InRotator, InLocation, FVector::OneVector);
		SpawnedBotList.Add(BotInfo);

		//# 서버 전용 — 사망 신호를 구독해 리스폰 타이밍을 스케줄링한다. Cue(디졸브 연출) 트리거와는
		//# 독립적으로 같은 OnDeath 델리게이트를 각자 구독한다 (cpp-style §8, 이벤트 기반 분리).
		if (APawn* ControlledPawn = NewController->GetPawn())
		{
			if (USpyHealthComponent* HealthComp = USpyHealthComponent::FindHealthComponent(ControlledPawn))
			{
				TWeakObjectPtr<AAIController> WeakController(NewController);
				HealthComp->OnDeath.AddDynamic(this, &ThisClass::HandleBotDeath);
			}
		}

		UE_LOG(LogTemp, Log, TEXT("# [SpawnBotManager]: Spawn %s"), *NewController->GetName());
```

**참고:** `AddDynamic` 은 정적 시그니처 바인딩이라 람다/`TWeakObjectPtr` 캡처를 못 받는다 — `HandleBotDeath` 는 태그된 컨트롤러가 아니라 `DamageInstigator`/`DamageCauser` 만 받으므로, "어느 봇이 죽었는지"는 브로드캐스트한 `HealthComp` 의 `GetOwner()` 로 역추적해야 한다. **Step 5 에서 이 시그니처로 구현.**

- [ ] **Step 5: `HandleBotDeath`/`RespawnBot` 구현**

```cpp
void USpySpawnBotManagerComponent::HandleBotDeath(AActor* InOwningActor, AActor* InCauserActor)
{
	//# 서버 전용 — OnDeath 는 서버/클라 양쪽에서 발화되므로(SKAttributeSet OnRep_Health 포함) 반드시 가드.
	if (GetOwnerRole() < ROLE_Authority)
		return;

	APawn* DeadPawn = Cast<APawn>(InOwningActor);
	if (DeadPawn == nullptr)
		return;

	AAIController* DeadController = Cast<AAIController>(DeadPawn->GetController());
	if (DeadController == nullptr)
		return;

	const FSpySpawnedBotInfo* FoundInfo = SpawnedBotList.FindByPredicate(
		[DeadController](const FSpySpawnedBotInfo& Info) { return Info.Controller == DeadController; });

	if (FoundInfo == nullptr)
		return;

	float DissolveDurationSeconds = 2.0f;
	if (ASpyAIController* SpyAIController = Cast<ASpyAIController>(DeadController))
	{
		DissolveDurationSeconds = SpyAIController->GetDissolveDurationSeconds();
	}

	TWeakObjectPtr<AAIController> WeakController(DeadController);
	FTimerHandle& RespawnTimer = RespawnTimers.FindOrAdd(DeadController);
	FTimerDelegate RespawnDelegate = FTimerDelegate::CreateUObject(this, &ThisClass::RespawnBot, WeakController);
	GetWorld()->GetTimerManager().SetTimer(RespawnTimer, RespawnDelegate, DissolveDurationSeconds, false);
}

void USpySpawnBotManagerComponent::RespawnBot(TWeakObjectPtr<AAIController> InController)
{
	if (InController.IsValid() == false)
		return;

	AAIController* Controller = InController.Get();
	RespawnTimers.Remove(Controller);

	const FSpySpawnedBotInfo* FoundInfo = SpawnedBotList.FindByPredicate(
		[Controller](const FSpySpawnedBotInfo& Info) { return Info.Controller == Controller; });

	if (FoundInfo == nullptr)
		return;

	if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(Controller->GetPawn()))
	{
		SpyCharacter->OnRespawn(FoundInfo->SpawnTransform);
	}
}
```

**필요 include** (`SpySpawnBotManagerComponent.cpp` 상단에 추가): `Character/SpyCharacter.h`, `Character/SpyHealthComponent.h`.

- [ ] **Step 6: `RemoveOneBot` — 새 구조체에 맞춰 갱신**

`SpySpawnBotManagerComponent.cpp:77-98` 을 새 `FSpySpawnedBotInfo` 배열에 맞게 수정 (기존 랜덤 제거 동작은 유지, `SpawnedBotList[Index]` → `SpawnedBotList[Index].Controller`):

```cpp
void USpySpawnBotManagerComponent::RemoveOneBot()
{
	if (SpawnedBotList.Num() > 0)
	{
		const int32 BotToRemoveIndex = FMath::RandRange(0, SpawnedBotList.Num() - 1);
		AAIController* BotToRemove = SpawnedBotList[BotToRemoveIndex].Controller;

		if (BotToRemove)
		{
			if (APawn* ControlledPawn = BotToRemove->GetPawn())
			{
				ControlledPawn->Destroy();
			}

			BotToRemove->Destroy();
		}

		RespawnTimers.Remove(BotToRemove);
		SpawnedBotList.RemoveAtSwap(BotToRemoveIndex);
	}
}
```

- [ ] **Step 7: 컴파일 확인 후 스테이징**

```bash
git add SkillProject/Source/SkillProject/Character/SpyCharacter.h SkillProject/Source/SkillProject/Character/SpyCharacter.cpp SkillProject/Source/SkillProject/ManagerComponent/SpySpawnBotManagerComponent.h SkillProject/Source/SkillProject/ManagerComponent/SpySpawnBotManagerComponent.cpp
```

커밋 메시지(안): `[Feature] SpySpawnBotManagerComponent — 봇 재활용 리스폰 오케스트레이션`

---

### Task 10: Automation 테스트 (test-engineer 단계)

**Files:**
- Create: `SkillProject/Source/SkillProject/Character/Tests/SpyHealthComponentTests.cpp`
- Create: `SkillProject/Source/SkillProject/ManagerComponent/Tests/SpySpawnBotRespawnTests.cpp`

**Interfaces:**
- Consumes: Task 5(Health 클램프), Task 6(`bDead`/`ResetDeathState`), Task 9(`OnRespawn`/`RespawnBot`/`SpawnedBotList`).

기존 컨벤션(`AI/Tests/SpyAICircleStrafeTests.cpp`, `Character/Tests/SpyLevelTests.cpp`) 을 따른다 — 구조체 `F<Domain><Case>Test`, 등록 문자열 `"SkillProject.도메인.기능.케이스"`, 파일 전체 `#if WITH_DEV_AUTOMATION_TESTS` 로 감싸기.

- [ ] **Step 1: Health 클램프 + bDead 래치 테스트 시나리오 작성** (test-engineer)

검증 대상:
- Health 에 `MaxHealth` 를 초과하는 음수 피해를 입혀도 `GetHealth() >= 0.f`.
- 사망 후 추가 피해가 들어와도 `OnDeath` 델리게이트가 1회만 발화(`bDead` 래치).
- `ResetDeathState()` 호출 후에는 다시 `OnDeath` 가 발화 가능.

- [ ] **Step 2: 리스폰 오케스트레이션 테스트 시나리오 작성** (test-engineer)

검증 대상:
- 봇 사망 후 `DissolveDurationSeconds` 경과 시 `RespawnBot` 이 호출되는지(타이머 검증 — `FTimerManager` 를 테스트 월드에서 직접 Tick 하거나 `AutomationOpenMap` + `ADD_LATENT_AUTOMATION_COMMAND` 로 시간 경과 시뮬레이션).
- `RespawnBot` 이후 `Health == MaxHealth`, 캡슐 콜리전이 `ECR_Block` 로 복구, 봇이 자기 원래 `SpawnTransform` 위치로 돌아오는지.
- `ASpyAIController::RestartAILogic()` 호출 후 `GetBrainComponent()->IsRunning()` (또는 동등한 BT 재개 확인 API)이 참인지.

- [ ] **Step 3: 사용자가 Unreal Automation Window 에서 실행해 결과 확인**

이 프로젝트는 CLI 테스트 러너가 없으므로(`project.md`), test-engineer 가 작성한 테스트는 사용자가 에디터의 Automation Window(`SkillProject.*` 필터)에서 직접 실행해 PASS/FAIL 을 확인한다.

- [ ] **Step 4: 스테이징**

```bash
git add SkillProject/Source/SkillProject/Character/Tests/SpyHealthComponentTests.cpp SkillProject/Source/SkillProject/ManagerComponent/Tests/SpySpawnBotRespawnTests.cpp
```

커밋 메시지(안): `[Chore] SpySpawnBotRespawnTests — 사망 클램프·리스폰 오케스트레이션 테스트 추가`

---

## 셀프 리뷰 메모 (writing-plans 셀프 리뷰 완료, 2026-08-12)

- **스펙 커버리지**: 스펙 §1(버그 픽스)→Task 5,6 / §2(디졸브 Cue)→Task 1,2,7,8 / §3(Config)→Task 3 / §4(리스폰 오케스트레이션)→Task 4,9 / §5(엣지 케이스: `TWeakObjectPtr` 체크→Task 9 Step5, 중복 발화 방지→Task 6, 동시 사망 독립 타이머→Task 9의 `TMap<Controller,FTimerHandle>`) / §6(테스트 관점)→Task 10. 누락 없음.
- **플레이스홀더 스캔**: "TODO"/"TBD" 없음. 모든 코드 스텝에 실제 코드 포함.
- **타입 일관성 재확인**: `GetDissolveDurationSeconds()`(Task 4)↔`SpyAIController->GetDissolveDurationSeconds()`(Task 7, Task 9) 일치. `ResetDeathState()`(Task 6)↔`SpyHealthComponent->ResetDeathState()`(Task 9) 일치. `OnRespawn(const FTransform&)`(Task 9 Step1)↔`SpyCharacter->OnRespawn(FoundInfo->SpawnTransform)`(Task 9 Step5) 일치. 머티리얼 파라미터 이름 `"Dissolve"`(Task 1, Task 8) 일치.
- **스펙 이탈 사항**: 스펙 §4가 리스폰 절차 소유자를 `SpySpawnBotManagerComponent` 로 서술했으나, cpp-style §8/§13 준수를 위해 `ASpyCharacter::OnRespawn()` 으로 캡슐화를 옮김(Task 9 서두에 근거 명시). 기능적 동작은 스펙과 동일 — code-reviewer 단계에서 이 이탈이 스펙 취지에 부합하는지 확인 필요.
