// Fill out your copyright notice in the Description page of Project Settings.

#include "Data/SpyLevelConfig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyLevelConfig)

int32 USpyLevelConfig::GetMaxLevel() const
{
	return ExperienceToNextLevel.Num() + 1;
}

float USpyLevelConfig::GetExperienceToNextLevel(int32 InLevel) const
{
	if (ExperienceToNextLevel.Num() == 0)
	{
		return 0.f;
	}

	const int32 Index = InLevel - 1;
	if (ExperienceToNextLevel.IsValidIndex(Index))
	{
		return ExperienceToNextLevel[Index];
	}

	//# 최대 레벨 도달 — 마지막 커브값을 유지해 진행도 바 분모를 고정한다
	return ExperienceToNextLevel.Last();
}

FSpyLevelUpResult USpyLevelConfig::ResolveLevelUp(int32 InLevel, float InExperience) const
{
	FSpyLevelUpResult Result;
	Result.Level = FMath::Max(1, InLevel);
	Result.Experience = FMath::Max(0.f, InExperience);
	Result.LevelsGained = 0;

	//# 커브 미설정 — 레벨업 판정을 하지 않는다
	if (ExperienceToNextLevel.Num() == 0)
	{
		Result.MaxExperience = 0.f;

		return Result;
	}

	const int32 MaxLevel = GetMaxLevel();
	float Required = GetExperienceToNextLevel(Result.Level);

	//# Required > 0 조건은 커브에 0 이 들어갔을 때 무한 루프를 막는다
	while (Result.Level < MaxLevel && Required > 0.f && Result.Experience >= Required)
	{
		Result.Experience -= Required;
		Result.Level += 1;
		Result.LevelsGained += 1;

		Required = GetExperienceToNextLevel(Result.Level);
	}

	Result.MaxExperience = Required;

	if (Result.Level >= MaxLevel)
	{
		Result.Experience = FMath::Min(Result.Experience, Result.MaxExperience);
	}

	return Result;
}
