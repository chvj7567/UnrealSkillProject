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

//# 미션 1개의 정의. 명시적 MissionId(1부터 시작하는 연속 정수)로 식별한다 — 배열 위치 비의존.
//# MissionReward/MissionCommunication 의 MissionId 와 일치해야 한다 (상세: spec §2-1·§2-3)
USTRUCT(BlueprintType)
struct FSpyMissionRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	//# "담당 NPC 없음" sentinel — NPCId 기본값 및 각 조회 실패 폴백에 공용으로 쓴다
	static constexpr int32 NoNPCId = 9999;

	UPROPERTY(EditAnywhere)
	int32 MissionId = 0;

	UPROPERTY(EditAnywhere)
	ESpyMissionType MissionType = ESpyMissionType::Gameplay;

	//# 이 미션이 반응할 이벤트 태그. Dialogue 타입은 전부 공용 Event_Mission_Report 를 쓴다
	UPROPERTY(EditAnywhere)
	FGameplayTag MatchTag;

	UPROPERTY(EditAnywhere)
	ESpyMissionMode Mode = ESpyMissionMode::Accumulate;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "1"))
	int32 TargetCount = 1;

	//# HUD 상시 표시 이름. Dialogue 타입은 이 값 자체가 "시스템 메시지"다
	UPROPERTY(EditAnywhere)
	FText DisplayName;

	//# 수락 카드 서술문. Gameplay 타입만 사용 — Dialogue 타입은 카드가 없으므로 빈 문자열로 둔다
	UPROPERTY(EditAnywhere)
	FText Description;

	//# 이 미션을 담당하는 NPC(NoNPCId=시스템 퀘스트, cpp-style §14-1-3 예외).
	//# GetCurrentNPCId() 가 그대로 반환 — 이름 조회/조립은 HUD 몫이다(cpp-style §8)
	UPROPERTY(EditAnywhere)
	int32 NPCId = NoNPCId;
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

//# Mission 의 선택적 관계(§14-1) — 목표 지점이 정의된 미션에만 행이 존재한다.
//# 명명 규칙(§14-1-5): Mission_TargetLocation. Dialogue 타입도 NPCId 대신 이 좌표를 그대로 쓴다.
USTRUCT(BlueprintType)
struct FSpyMission_TargetLocationRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	int32 MissionId = 0;

	UPROPERTY(EditAnywhere)
	FVector TargetLocation = FVector::ZeroVector;
};

//# 진행 판정 결과 — 부수효과 없는 계산의 출력
USTRUCT(BlueprintType)
struct FSpyMissionProgressResult
{
	GENERATED_BODY()

public:
	//# 판정 후 진행 중인 미션 인덱스(1-based). 범위를 벗어나면 전체 완료
	UPROPERTY(BlueprintReadOnly)
	int32 MissionIndex = 1;

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
	//# 미션 데이터. RowStruct = FSpyMissionRow. MissionId 필드가 진행 순서를 결정한다
	//# (배열 위치가 아니다). 값은 DT_SpyMission 에디터에서 입력한다 (코드 기본값 없음)
	UPROPERTY(EditDefaultsOnly, Category = "Mission")
	TObjectPtr<UDataTable> MissionTable;

	//# Dialogue 타입 미션의 보상 관계 테이블. RowStruct = FSpyMissionRewardRow
	UPROPERTY(EditDefaultsOnly, Category = "Mission")
	TObjectPtr<UDataTable> MissionRewardTable;

	//# 선택적 관계 — 목표 지점이 있는 미션만 행 존재. RowStruct = FSpyMission_TargetLocationRow
	UPROPERTY(EditDefaultsOnly, Category = "Mission")
	TObjectPtr<UDataTable> MissionTargetLocationTable;

public:
	UFUNCTION(BlueprintPure, Category = "Mission")
	int32 GetMissionCount() const;

	UFUNCTION(BlueprintPure, Category = "Mission")
	bool IsValidMissionIndex(int32 InMissionId) const;

	//# MissionId 로 조회한다(배열 인덱스 아님). 없으면 nullptr
	const FSpyMissionRow* GetMission(int32 InMissionId) const;

	//# 진행 판정 — 부수효과 없음. 한 번의 호출로 최대 1개 미션만 완료한다
	UFUNCTION(BlueprintPure, Category = "Mission")
	FSpyMissionProgressResult ResolveMissionProgress(int32 InIndex, int32 InCount, FGameplayTag InEventTag, int32 InAmount) const;

	//# MissionId 로 보상을 조회한다. 행이 없으면(Gameplay 타입) 0.f — sentinel 이 아니라
	//# "관계 없음"의 정상적인 부재 결과다 (spec §4-3)
	UFUNCTION(BlueprintPure, Category = "Mission")
	float GetMissionReward(int32 InMissionId) const;

	//# MissionId 로 목표 좌표를 조회한다. 행이 없으면(목표 미정의) nullptr — sentinel 아님(§14-1)
	const FSpyMission_TargetLocationRow* GetMissionTargetLocation(int32 InMissionId) const;
};
