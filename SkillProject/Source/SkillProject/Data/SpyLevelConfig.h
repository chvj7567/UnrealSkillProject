// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "SpyLevelConfig.generated.h"

//# 레벨업 계산 결과 — 부수효과 없는 계산의 출력
USTRUCT(BlueprintType)
struct FSpyLevelUpResult
{
	GENERATED_BODY()

public:
	//# 계산 후 레벨
	UPROPERTY(BlueprintReadOnly)
	int32 Level = 1;

	//# 계산 후 현재 레벨 구간 내 잔여 경험치
	UPROPERTY(BlueprintReadOnly)
	float Experience = 0.f;

	//# 계산 후 다음 레벨까지 필요 경험치
	UPROPERTY(BlueprintReadOnly)
	float MaxExperience = 0.f;

	//# 이번 계산에서 오른 레벨 수 (0 이면 레벨업 없음)
	UPROPERTY(BlueprintReadOnly)
	int32 LevelsGained = 0;
};

UCLASS()
class SKILLPROJECT_API USpyLevelConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	//# 인덱스 i = 레벨 (i+1) → (i+2) 승급에 필요한 경험치. 배열 길이 + 1 이 최대 레벨
	//# 값은 DA_SpyLevelConfig 에디터에서 입력한다 (코드 기본값 없음)
	UPROPERTY(EditDefaultsOnly, Category = "Level")
	TArray<float> ExperienceToNextLevel;

	//# 레벨업 1회당 MaxHealth 증가량
	UPROPERTY(EditDefaultsOnly, Category = "Level")
	float MaxHealthPerLevel = 10.f;

	//# 레벨업 1회당 MaxMana 증가량
	UPROPERTY(EditDefaultsOnly, Category = "Level")
	float MaxManaPerLevel = 5.f;

	//# 레벨업 시 Health/Mana 를 최대치로 회복할지
	UPROPERTY(EditDefaultsOnly, Category = "Level")
	bool bFullHealOnLevelUp = true;

	//# 처치 보상 경험치 = 처치당한 대상의 Level × 이 값
	UPROPERTY(EditDefaultsOnly, Category = "Level")
	float ExperienceRewardPerLevel = 20.f;

public:
	//# 커브 길이 + 1. 커브가 비었으면 1
	UFUNCTION(BlueprintPure, Category = "Level")
	int32 GetMaxLevel() const;

	//# InLevel 에서 다음 레벨까지 필요한 경험치. 최대 레벨이면 마지막 커브값을 반환(진행도 바 고정용)
	UFUNCTION(BlueprintPure, Category = "Level")
	float GetExperienceToNextLevel(int32 InLevel) const;

	//# 레벨업 판정 — 부수효과 없음. 한 번에 여러 레벨 상승과 잔여 경험치 이월을 함께 처리한다
	UFUNCTION(BlueprintPure, Category = "Level")
	FSpyLevelUpResult ResolveLevelUp(int32 InLevel, float InExperience) const;
};
