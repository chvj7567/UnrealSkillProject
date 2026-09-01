// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/SpyHealthComponent.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Character/SpyCharacterAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "SKGameplayEffectContext.h"
#include "SKGameplayTags.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "System/SpyPlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyHealthComponent)

void USpyHealthComponent::InitializeByAbilitySystem(USpyAbilitySystemComponent* InASC)
{
	AActor* Owner = GetOwner();
	check(Owner);

	if (InASC == nullptr)
		return;

	AbilitySystemComponent = InASC;

	HealthSet = AbilitySystemComponent->GetSet<USpyCharacterAttributeSet>();

	HealthSet->OnHealthChanged.AddUObject(this, &ThisClass::HandleHealthChanged);
	HealthSet->OnMaxHealthChanged.AddUObject(this, &ThisClass::HandleMaxHealthChanged);

	OnHealthChanged.Broadcast(this, HealthSet->GetHealth(), HealthSet->GetHealth(), nullptr);
	OnMaxHealthChanged.Broadcast(this, HealthSet->GetHealth(), HealthSet->GetHealth(), nullptr);
}

void USpyHealthComponent::UnInitializeByAbilitySystem()
{
	if (HealthSet)
	{
		HealthSet->OnHealthChanged.RemoveAll(this);
		HealthSet->OnMaxHealthChanged.RemoveAll(this);
	}

	HealthSet = nullptr;
	AbilitySystemComponent = nullptr;
}

float USpyHealthComponent::GetHealth() const
{
	return (HealthSet ? HealthSet->GetHealth() : 0.0f);
}

float USpyHealthComponent::GetMaxHealth() const
{
	return (HealthSet ? HealthSet->GetMaxHealth() : 0.0f);
}

float USpyHealthComponent::GetHealthNormalized() const
{
	if (HealthSet)
	{
		const float Health = HealthSet->GetHealth();
		const float MaxHealth = HealthSet->GetMaxHealth();

		return ((MaxHealth > 0.0f) ? (Health / MaxHealth) : 0.0f);
	}

	return 0.0f;
}

void USpyHealthComponent::OnUnregister()
{
	UnInitializeByAbilitySystem();

	Super::OnUnregister();
}

void USpyHealthComponent::HandleHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	if (NewValue <= 0 && bDead == false)
	{
		bDead = true;
		OnDeath.Broadcast(DamageInstigator, DamageCauser);
	}

	OnHealthChanged.Broadcast(this, OldValue, NewValue, DamageInstigator);

	UE_LOG(LogTemp, Warning, TEXT("[CameraShake] HealthComp Enter — Mag=%.2f Spec=%d Owner=%s Instigator=%s"),
		DamageMagnitude, DamageEffectSpec != nullptr,
		*GetNameSafe(GetOwner()), *GetNameSafe(DamageInstigator));

	//# 컨텍스트에서 피격/크리티컬 정보 추출 — GA 차단과 무관하게 발화하기 위해 GA가 아닌 이 경로에서 처리
	if (DamageEffectSpec == nullptr)
		return;

	FSKGameplayEffectContext* Ctx = FSKGameplayEffectContext::ExtractEffectContext(DamageEffectSpec->GetContext());
	if (Ctx == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CameraShake] HealthComp — Ctx null"));
		return;
	}

	const FGameplayTag HitDirTag = Ctx->GetHitDirectionTag();
	const bool bIsHit = HitDirTag.IsValid() && HitDirTag.MatchesTag(SKGameplayTags::Skill_Hit);
	const bool bCritical = Ctx->IsCritical();

	UE_LOG(LogTemp, Warning, TEXT("[CameraShake] HealthComp Ctx — HitDir=%s bIsHit=%d bCritical=%d"),
		*HitDirTag.ToString(), bIsHit, bCritical);

	//# BP 호환 OnHit (실제 피해가 발생한 경우만)
	if (DamageMagnitude < 0.0f)
	{
		OnHit.Broadcast(FMath::Abs(DamageMagnitude), bCritical, DamageCauser);
	}

	if (bIsHit == false)
		return;

	//# 피격자 카메라 쉐이크 — 로컬 컨트롤러는 RPC 우회 (ProcessEvent 경로 BP 이슈 회피)
	ASpyPlayerController* DefenderPC = nullptr;
	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		DefenderPC = Cast<ASpyPlayerController>(OwnerPawn->GetController());
		if (DefenderPC)
		{
			if (DefenderPC->IsLocalController())
				DefenderPC->TriggerShakeLocal(bCritical, true);
			else
				DefenderPC->Client_TriggerShake(bCritical, true);
		}
	}

	//# 공격자 카메라 쉐이크 — DamageInstigator는 PlayerState일 수 있어 Pawn 변환 폴백
	APawn* InstigatorPawn = Cast<APawn>(DamageInstigator);
	if (InstigatorPawn == nullptr)
	{
		if (APlayerState* PS = Cast<APlayerState>(DamageInstigator))
		{
			InstigatorPawn = PS->GetPawn();
		}
	}
	ASpyPlayerController* AttackerPC = InstigatorPawn ? Cast<ASpyPlayerController>(InstigatorPawn->GetController()) : nullptr;
	if (AttackerPC)
	{
		if (AttackerPC->IsLocalController())
			AttackerPC->TriggerShakeLocal(bCritical, false);
		else
			AttackerPC->Client_TriggerShake(bCritical, false);
	}

	UE_LOG(LogTemp, Warning, TEXT("[CameraShake] HealthComp RPC — DefenderPC=%d AttackerPC=%d"),
		DefenderPC != nullptr, AttackerPC != nullptr);
}

void USpyHealthComponent::HandleMaxHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	OnMaxHealthChanged.Broadcast(this, OldValue, NewValue, DamageInstigator);
}

void USpyHealthComponent::ResetDeathState()
{
	bDead = false;
}