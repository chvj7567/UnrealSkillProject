// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Attribute/SKAttributeSet.h"

#include "SpyCharacterAttributeSet.generated.h"

UCLASS()
class SKILLPROJECT_API USpyCharacterAttributeSet : public USKAttributeSet
{
	GENERATED_BODY()

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;

public:
	UPROPERTY(ReplicatedUsing = OnRep_MoveNormalSpeed, BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData MoveNormalSpeed;
	ATTRIBUTE_ACCESSORS(USpyCharacterAttributeSet, MoveNormalSpeed);

	//# 현재 레벨 구간 내 누적 경험치
	UPROPERTY(ReplicatedUsing = OnRep_Experience, BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Experience;
	ATTRIBUTE_ACCESSORS(USpyCharacterAttributeSet, Experience);

	//# 다음 레벨까지 필요한 경험치 — 클라이언트가 커브를 몰라도 진행도를 계산할 수 있게 복제한다
	UPROPERTY(ReplicatedUsing = OnRep_MaxExperience, BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData MaxExperience;
	ATTRIBUTE_ACCESSORS(USpyCharacterAttributeSet, MaxExperience);

	//# 현재 레벨 (1 시작)
	UPROPERTY(ReplicatedUsing = OnRep_Level, BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Level;
	ATTRIBUTE_ACCESSORS(USpyCharacterAttributeSet, Level);

protected:
	UFUNCTION()
	void OnRep_MoveNormalSpeed(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Experience(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxExperience(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Level(const FGameplayAttributeData& OldValue);

public:
	mutable FSKAttributeEvent OnMoveNormalSpeedChanged;
	mutable FSKAttributeEvent OnExperienceChanged;
	mutable FSKAttributeEvent OnMaxExperienceChanged;
	mutable FSKAttributeEvent OnLevelChanged;
};
