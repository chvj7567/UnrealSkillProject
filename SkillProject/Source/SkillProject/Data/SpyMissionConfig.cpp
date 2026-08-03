// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/SpyMissionConfig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyMissionConfig)

int32 USpyMissionConfig::GetMissionCount() const
{
	return Missions.Num();
}

bool USpyMissionConfig::IsValidMissionIndex(int32 InIndex) const
{
	return Missions.IsValidIndex(InIndex);
}

const FSpyMissionEntry* USpyMissionConfig::GetMission(int32 InIndex) const
{
	if (Missions.IsValidIndex(InIndex) == false)
		return nullptr;

	return &Missions[InIndex];
}

FSpyMissionProgressResult USpyMissionConfig::ResolveMissionProgress(int32 InIndex, int32 InCount, FGameplayTag InEventTag, int32 InAmount) const
{
	FSpyMissionProgressResult Result;
	Result.MissionIndex = FMath::Max(0, InIndex);
	Result.Count = FMath::Max(0, InCount);
	Result.bCompletedNow = false;
	Result.bAllCompleted = false;

	const FSpyMissionEntry* Entry = GetMission(Result.MissionIndex);

	//# 인덱스가 범위 밖 = 전체 완료 상태. 추가 이벤트에 반응하지 않는다
	if (Entry == nullptr)
	{
		Result.bAllCompleted = true;

		return Result;
	}

	//# 계층 매칭 — 부모 태그 미션이 자식 태그 이벤트를 받아들인다
	if (InEventTag.IsValid() == false || InEventTag.MatchesTag(Entry->MatchTag) == false)
		return Result;

	if (Entry->Mode == ESpyMissionMode::Accumulate)
	{
		Result.Count += FMath::Max(0, InAmount);
	}
	else
	{
		//# Threshold — 누적하지 않고 대치한다
		Result.Count = FMath::Max(0, InAmount);
	}

	if (Result.Count >= Entry->TargetCount)
	{
		Result.bCompletedNow = true;
		Result.MissionIndex += 1;

		//# 초과분은 다음 미션으로 이월하지 않는다 (미션 종류가 서로 달라 의미가 없다)
		Result.Count = 0;

		if (IsValidMissionIndex(Result.MissionIndex) == false)
		{
			Result.bAllCompleted = true;
		}
	}

	return Result;
}

float USpyMissionConfig::GetMissionReward(int32 InMissionId) const
{
	if (MissionRewardTable == nullptr)
		return 0.f;

	TArray<FSpyMissionRewardRow*> Rows;
	MissionRewardTable->GetAllRows<FSpyMissionRewardRow>(TEXT("USpyMissionConfig::GetMissionReward"), Rows);

	for (const FSpyMissionRewardRow* Row : Rows)
	{
		if (Row != nullptr && Row->MissionId == InMissionId)
			return Row->ExperienceReward;
	}

	return 0.f;
}
