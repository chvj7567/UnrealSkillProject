// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/GameFrameworkComponent.h"
#include "CoreMinimal.h"

#include "SpyLevelComponent.generated.h"

class USpyAbilitySystemComponent;
class USpyCharacterAttributeSet;
class USpyLevelConfig;
struct FGameplayEffectSpec;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSpyLevel_ExperienceChanged, USpyLevelComponent*, LevelComponent, float, OldValue, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSpyLevel_LevelChanged, USpyLevelComponent*, LevelComponent, int32, OldLevel, int32, NewLevel);

UCLASS()
class SKILLPROJECT_API USpyLevelComponent : public UGameFrameworkComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure)
	static USpyLevelComponent* FindLevelComponent(const AActor* Actor)
	{
		return (Actor ? Actor->FindComponentByClass<USpyLevelComponent>() : nullptr);
	}

	void InitializeByAbilitySystem(USpyAbilitySystemComponent* InASC);
	void UnInitializeByAbilitySystem();

	UFUNCTION(BlueprintPure)
	int32 GetLevel() const;

	UFUNCTION(BlueprintPure)
	float GetExperience() const;

	UFUNCTION(BlueprintPure)
	float GetMaxExperience() const;

	//# 진행도 0~1. MaxExperience가 0 이하면 0
	UFUNCTION(BlueprintPure)
	float GetExperienceNormalized() const;

protected:
	virtual void OnUnregister() override;

	//# AttributeSet 델리게이트 수신
	void HandleExperienceChanged(AActor* Instigator, AActor* Causer, const FGameplayEffectSpec* Spec, float Magnitude, float OldValue, float NewValue);

	//# AttributeSet 델리게이트 수신 — 클라이언트 OnRep_Level 경로 전용
	void HandleAttributeLevelChanged(AActor* Instigator, AActor* Causer, const FGameplayEffectSpec* Spec, float Magnitude, float OldValue, float NewValue);

	//# 자기 캐릭터 사망 — 서버에서 킬러에게 경험치를 지급한다
	UFUNCTION()
	void HandleDeath(AActor* OwningActor, AActor* CauserActor);

	//# 서버 전용 — 레벨업 판정 후 어트리뷰트·성장 이펙트 적용
	void TryLevelUp();

	//# 킬러 액터에서 ASC를 해석한다 (PlayerState로 넘어온 경우 Pawn 폴백)
	USpyAbilitySystemComponent* ResolveKillerAbilitySystem(AActor* InInstigator) const;

public:
	UPROPERTY(BlueprintAssignable)
	FSpyLevel_ExperienceChanged OnExperienceChanged;

	UPROPERTY(BlueprintAssignable)
	FSpyLevel_LevelChanged OnLevelChanged;

protected:
	//# 캐릭터 BP 기본값에서 지정한다 (SpyMovementConfig 선례와 동일)
	UPROPERTY(EditDefaultsOnly, Category = "Level")
	TObjectPtr<USpyLevelConfig> LevelConfig;

	UPROPERTY()
	TObjectPtr<USpyAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<const USpyCharacterAttributeSet> LevelSet;

	//# 레벨업 처리 중 어트리뷰트 변경으로 재진입하는 것을 막는다
	bool bProcessingLevelUp = false;

	//# 사망 보상 1회 지급 보장 (Health가 0 이하로 여러 번 갱신될 수 있음)
	bool bDeathRewardGranted = false;
};
