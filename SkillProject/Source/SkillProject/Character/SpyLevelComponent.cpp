// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/SpyLevelComponent.h"
#include "AbilitySystem/Effect/SpyGE_ExperienceGain.h"
#include "AbilitySystem/Effect/SpyGE_LevelGrowth.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Character/SpyCharacterAttributeSet.h"
#include "Character/SpyHealthComponent.h"
#include "Data/SpyLevelConfig.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "System/SpyMissionComponent.h"
#include "Util/SpyGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyLevelComponent)

void USpyLevelComponent::InitializeByAbilitySystem(USpyAbilitySystemComponent* InASC)
{
	AActor* Owner = GetOwner();
	check(Owner);

	if (InASC == nullptr)
		return;

	AbilitySystemComponent = InASC;
	LevelSet = AbilitySystemComponent->GetSet<USpyCharacterAttributeSet>();

	if (LevelSet == nullptr)
		return;

	LevelSet->OnExperienceChanged.AddUObject(this, &ThisClass::HandleExperienceChanged);
	LevelSet->OnLevelChanged.AddUObject(this, &ThisClass::HandleAttributeLevelChanged);

	//# 사망 보상은 자기 캐릭터의 HealthComponent 사망 이벤트에서 출발한다
	if (USpyHealthComponent* HealthComponent = USpyHealthComponent::FindHealthComponent(Owner))
	{
		HealthComponent->OnDeath.AddDynamic(this, &ThisClass::HandleDeath);
	}

	//# 초기 상태는 서버에서만 세팅한다 (클라이언트는 복제로 받는다)
	if (Owner->HasAuthority() && LevelConfig)
	{
		AbilitySystemComponent->SetNumericAttributeBase(USpyCharacterAttributeSet::GetLevelAttribute(), 1.f);
		AbilitySystemComponent->SetNumericAttributeBase(USpyCharacterAttributeSet::GetExperienceAttribute(), 0.f);
		AbilitySystemComponent->SetNumericAttributeBase(USpyCharacterAttributeSet::GetMaxExperienceAttribute(), LevelConfig->GetExperienceToNextLevel(1));
	}

	bDeathRewardGranted = false;
}

void USpyLevelComponent::UnInitializeByAbilitySystem()
{
	if (LevelSet)
	{
		LevelSet->OnExperienceChanged.RemoveAll(this);
		LevelSet->OnLevelChanged.RemoveAll(this);
	}

	if (USpyHealthComponent* HealthComponent = USpyHealthComponent::FindHealthComponent(GetOwner()))
	{
		HealthComponent->OnDeath.RemoveDynamic(this, &ThisClass::HandleDeath);
	}

	LevelSet = nullptr;
	AbilitySystemComponent = nullptr;
}

void USpyLevelComponent::OnUnregister()
{
	UnInitializeByAbilitySystem();

	Super::OnUnregister();
}

int32 USpyLevelComponent::GetLevel() const
{
	return (LevelSet ? FMath::FloorToInt(LevelSet->GetLevel()) : 1);
}

float USpyLevelComponent::GetExperience() const
{
	return (LevelSet ? LevelSet->GetExperience() : 0.f);
}

float USpyLevelComponent::GetMaxExperience() const
{
	return (LevelSet ? LevelSet->GetMaxExperience() : 0.f);
}

float USpyLevelComponent::GetExperienceNormalized() const
{
	const float MaxExperience = GetMaxExperience();

	return ((MaxExperience > 0.f) ? FMath::Clamp(GetExperience() / MaxExperience, 0.f, 1.f) : 0.f);
}

USpyAbilitySystemComponent* USpyLevelComponent::ResolveKillerAbilitySystem(AActor* InInstigator) const
{
	if (InInstigator == nullptr)
		return nullptr;

	//# Instigator가 PlayerState로 넘어오는 경우가 있어 Pawn으로 폴백한다
	//# (SpyHealthComponent::HandleHealthChanged의 기존 폴백과 동일한 방식)
	AActor* KillerActor = InInstigator;
	if (APlayerState* PlayerState = Cast<APlayerState>(InInstigator))
	{
		if (APawn* KillerPawn = PlayerState->GetPawn())
		{
			KillerActor = KillerPawn;
		}
	}

	//# 자기 자신을 죽인 경우(자살·환경 피해)는 보상 없음
	if (KillerActor == GetOwner())
		return nullptr;

	return Cast<USpyAbilitySystemComponent>(
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(KillerActor));
}

void USpyLevelComponent::HandleDeath(AActor* OwningActor, AActor* CauserActor)
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr || Owner->HasAuthority() == false)
		return;

	//# OnDeath는 Health가 0 이하로 여러 번 갱신되면 반복 발화할 수 있다 — 1회만 지급
	if (bDeathRewardGranted)
		return;

	if (LevelConfig == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("# [SpyLevelComponent] LevelConfig가 지정되지 않아 경험치를 지급하지 않습니다: %s"), *GetNameSafe(Owner));
		return;
	}

	//# OnDeath의 첫 인자는 DamageInstigator다 (SpyHealthComponent::HandleHealthChanged 참고)
	USpyAbilitySystemComponent* KillerASC = ResolveKillerAbilitySystem(OwningActor);
	if (KillerASC == nullptr)
		return;

	bDeathRewardGranted = true;

	//# 보상 = 처치당한 쪽(자신)의 레벨 × 계수
	const float RewardAmount = GetLevel() * LevelConfig->ExperienceRewardPerLevel;
	if (RewardAmount <= 0.f)
		return;

	FGameplayEffectContextHandle ContextHandle = KillerASC->MakeEffectContext();
	ContextHandle.AddSourceObject(Owner);

	const FGameplayEffectSpecHandle SpecHandle = KillerASC->MakeOutgoingSpec(USpyGE_ExperienceGain::StaticClass(), 1.f, ContextHandle);
	if (SpecHandle.IsValid() == false)
		return;

	SpecHandle.Data->SetSetByCallerMagnitude(SpyGameplayTags::Data_Experience_Gain, RewardAmount);
	KillerASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);

	//# 미션 진행 — 킬러의 미션 컴포넌트는 PlayerState 에 있다.
	//# GetOwnerActor()가 ASC 소유 액터(= ASpyPlayerState)이고, GetAvatarActor()(= Pawn)가 아니다
	if (USpyMissionComponent* KillerMission = USpyMissionComponent::FindMissionComponent(KillerASC->GetOwnerActor()))
	{
		KillerMission->AddProgress(SpyGameplayTags::Event_Mission_Kill, 1);
	}
}

void USpyLevelComponent::HandleExperienceChanged(AActor* Instigator, AActor* Causer, const FGameplayEffectSpec* Spec, float Magnitude, float OldValue, float NewValue)
{
	//# UI·연출용 브로드캐스트는 서버/클라이언트 양쪽에서 발생한다
	OnExperienceChanged.Broadcast(this, OldValue, NewValue);

	AActor* Owner = GetOwner();
	if (Owner == nullptr || Owner->HasAuthority() == false)
		return;

	TryLevelUp();
}

void USpyLevelComponent::HandleAttributeLevelChanged(AActor* Instigator, AActor* Causer, const FGameplayEffectSpec* Spec, float Magnitude, float OldValue, float NewValue)
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr || Owner->HasAuthority())
		return;

	//# 클라이언트 전용 경로.
	//# 서버는 SetNumericAttributeBase로 Level을 바꾸므로 AttributeSet의 OnLevelChanged가 발화하지 않고,
	//# TryLevelUp이 직접 브로드캐스트한다. 클라이언트는 OnRep_Level만 오므로 여기서 이어 붙인다.
	//# (레벨업 프레임에 Experience가 20→0처럼 같은 값으로 되돌아오면 OnRep_Experience가 발화하지 않아
	//#  이 경로가 없으면 클라이언트 HUD가 다음 킬까지 갱신되지 않는다)
	OnLevelChanged.Broadcast(this, FMath::FloorToInt(OldValue), FMath::FloorToInt(NewValue));
}

void USpyLevelComponent::TryLevelUp()
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr || Owner->HasAuthority() == false)
		return;

	//# 루프 안에서 어트리뷰트를 바꾸면 HandleExperienceChanged가 재진입한다
	if (bProcessingLevelUp)
		return;

	if (LevelConfig == nullptr || LevelSet == nullptr || AbilitySystemComponent == nullptr)
		return;

	if (LevelConfig->ExperienceToNextLevel.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("# [SpyLevelComponent] ExperienceToNextLevel 커브가 비어 레벨업을 건너뜁니다: %s"), *GetNameSafe(Owner));
		return;
	}

	const int32 OldLevel = GetLevel();
	const FSpyLevelUpResult Result = LevelConfig->ResolveLevelUp(OldLevel, GetExperience());

	//# 레벨업이 없어도 최대 레벨 클램프로 경험치가 조정될 수 있다
	const bool bExperienceChanged = FMath::IsNearlyEqual(Result.Experience, GetExperience()) == false;
	const bool bMaxExperienceChanged = FMath::IsNearlyEqual(Result.MaxExperience, GetMaxExperience()) == false;

	if (Result.LevelsGained == 0 && bExperienceChanged == false && bMaxExperienceChanged == false)
		return;

	TGuardValue<bool> ReentryGuard(bProcessingLevelUp, true);

	AbilitySystemComponent->SetNumericAttributeBase(USpyCharacterAttributeSet::GetLevelAttribute(), static_cast<float>(Result.Level));
	AbilitySystemComponent->SetNumericAttributeBase(USpyCharacterAttributeSet::GetExperienceAttribute(), Result.Experience);
	AbilitySystemComponent->SetNumericAttributeBase(USpyCharacterAttributeSet::GetMaxExperienceAttribute(), Result.MaxExperience);

	if (Result.LevelsGained > 0)
	{
		//# 성장 이펙트 — 오른 레벨 수만큼 곱해 한 번에 적용
		FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
		ContextHandle.AddSourceObject(Owner);

		const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(USpyGE_LevelGrowth::StaticClass(), 1.f, ContextHandle);
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(SpyGameplayTags::Data_Level_MaxHealthGrowth, LevelConfig->MaxHealthPerLevel * Result.LevelsGained);
			SpecHandle.Data->SetSetByCallerMagnitude(SpyGameplayTags::Data_Level_MaxManaGrowth, LevelConfig->MaxManaPerLevel * Result.LevelsGained);

			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
		}

		if (LevelConfig->bFullHealOnLevelUp)
		{
			//# 성장 이펙트 적용 뒤이므로 MaxHealth/MaxMana는 이미 증가한 값이다
			AbilitySystemComponent->SetNumericAttributeBase(USpyCharacterAttributeSet::GetHealthAttribute(), LevelSet->GetMaxHealth());
			AbilitySystemComponent->SetNumericAttributeBase(USpyCharacterAttributeSet::GetManaAttribute(), LevelSet->GetMaxMana());
		}

		OnLevelChanged.Broadcast(this, OldLevel, Result.Level);

		//# 미션 진행 — 미션 컴포넌트는 PlayerState 에 있다.
		//# AttributeSet 의 OnLevelChanged 는 서버에서 발화하지 않으므로 이 지점에서 직접 보낸다
		if (APawn* OwnerPawn = Cast<APawn>(Owner))
		{
			if (USpyMissionComponent* OwnerMission = USpyMissionComponent::FindMissionComponent(OwnerPawn->GetPlayerState()))
			{
				//# Threshold 모드이므로 누적이 아니라 도달한 레벨값을 그대로 넘긴다
				OwnerMission->AddProgress(SpyGameplayTags::Event_Mission_Level, Result.Level);
			}
		}

		UE_LOG(LogTemp, Log, TEXT("# [SpyLevelComponent] LevelUp %s: %d -> %d"), *GetNameSafe(Owner), OldLevel, Result.Level);
	}
}
