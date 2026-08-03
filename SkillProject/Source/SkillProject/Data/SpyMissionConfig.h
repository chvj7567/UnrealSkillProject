// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Util/DefineEnum.h"

#include "SpyMissionConfig.generated.h"

//# 진행도 집계 방식
UENUM(BlueprintType)
enum class ESpyMissionMode : uint8
{
	//# 이벤트 수량을 누적한다 (파쿠르 N회, 처치 N회 등)
	Accumulate,

	//# 이벤트가 전달한 값으로 대치한다 (레벨 N 도달 등)
	Threshold,
};

//# 미션 1개의 정의
USTRUCT(BlueprintType)
struct FSpyMissionEntry
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Mission")
	ESpyMissionType MissionType = ESpyMissionType::Gameplay;

	//# 이 미션이 반응할 이벤트 태그. Dialogue 타입은 전부 공용 Event_Mission_Report 를 쓴다
	UPROPERTY(EditDefaultsOnly, Category = "Mission")
	FGameplayTag MatchTag;

	UPROPERTY(EditDefaultsOnly, Category = "Mission")
	ESpyMissionMode Mode = ESpyMissionMode::Accumulate;

	UPROPERTY(EditDefaultsOnly, Category = "Mission", meta = (ClampMin = "1"))
	int32 TargetCount = 1;

	//# HUD 상시 표시 이름. Dialogue 타입은 이 값 자체가 "시스템 메시지"다
	UPROPERTY(EditDefaultsOnly, Category = "Mission")
	FText DisplayName;

	//# 수락 카드 서술문. Gameplay 타입만 사용 — Dialogue 타입은 카드가 없으므로 빈 문자열로 둔다
	UPROPERTY(EditDefaultsOnly, Category = "Mission")
	FText Description;
};

//# Mission 의 선택적 관계(§14-1) — Dialogue 타입 미션에만 행이 존재한다.
//# 명명 규칙(§14-1-5): Mission_Reward
USTRUCT(BlueprintType)
struct FSpyMissionRewardRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	int32 MissionId = 0;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0"))
	float ExperienceReward = 0.f;
};

//# 진행 판정 결과 — 부수효과 없는 계산의 출력
USTRUCT(BlueprintType)
struct FSpyMissionProgressResult
{
	GENERATED_BODY()

public:
	//# 판정 후 진행 중인 미션 인덱스. 배열 범위를 벗어나면 전체 완료
	UPROPERTY(BlueprintReadOnly)
	int32 MissionIndex = 0;

	//# 판정 후 현재 미션의 누적치
	UPROPERTY(BlueprintReadOnly)
	int32 Count = 0;

	//# 이번 판정으로 미션 하나가 완료됐는가
	UPROPERTY(BlueprintReadOnly)
	bool bCompletedNow = false;

	//# 마지막 미션까지 전부 끝났는가
	UPROPERTY(BlueprintReadOnly)
	bool bAllCompleted = false;
};

UCLASS()
class SKILLPROJECT_API USpyMissionConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	//# 배열 인덱스가 곧 진행 순서다
	//# 값은 DA_SpyMissionConfig 에디터에서 입력한다 (코드 기본값 없음)
	UPROPERTY(EditDefaultsOnly, Category = "Mission")
	TArray<FSpyMissionEntry> Missions;

	//# Dialogue 타입 미션의 보상 관계 테이블. RowStruct = FSpyMissionRewardRow
	UPROPERTY(EditDefaultsOnly, Category = "Mission")
	TObjectPtr<UDataTable> MissionRewardTable;

public:
	UFUNCTION(BlueprintPure, Category = "Mission")
	int32 GetMissionCount() const;

	UFUNCTION(BlueprintPure, Category = "Mission")
	bool IsValidMissionIndex(int32 InIndex) const;

	//# 범위 밖이면 nullptr
	const FSpyMissionEntry* GetMission(int32 InIndex) const;

	//# 진행 판정 — 부수효과 없음. 한 번의 호출로 최대 1개 미션만 완료한다
	UFUNCTION(BlueprintPure, Category = "Mission")
	FSpyMissionProgressResult ResolveMissionProgress(int32 InIndex, int32 InCount, FGameplayTag InEventTag, int32 InAmount) const;

	//# MissionId 로 보상을 조회한다. 행이 없으면(Gameplay 타입) 0.f — sentinel 이 아니라
	//# "관계 없음"의 정상적인 부재 결과다 (spec §4-3)
	UFUNCTION(BlueprintPure, Category = "Mission")
	float GetMissionReward(int32 InMissionId) const;
};
