// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "Util/DefineEnum.h"

#include "SpyNPCDialogueRow.generated.h"

//# NPC 1명 — 핵심 엔티티, 관계 없음 (DefaultDialogueId 는 필수 1:1 관계라 관계 테이블 없이 직접 참조, §14-1-3)
USTRUCT(BlueprintType)
struct FSpyNPCRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	//# MissionCommunication.NPCId 매칭 키 — 다른 관계 테이블과 동일하게 "전체 스캔 + 필드 비교"로 조회한다
	UPROPERTY(EditAnywhere)
	int32 NPCId = 0;

	UPROPERTY(EditAnywhere)
	FText NPCDisplayName;

	UPROPERTY(EditAnywhere)
	int32 DefaultDialogueId = 0;
};

//# 대사 한 줄(또는 여러 줄로 이어지는 한 그룹) — 핵심 엔티티, 복합 키(§14-1-4)
USTRUCT(BlueprintType)
struct FSpyDialogueRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	int32 DialogueId = 0;

	UPROPERTY(EditAnywhere)
	int32 DialogueIndex = 0;

	UPROPERTY(EditAnywhere)
	FText Text;
};

//# DialogueId 그룹에서 InPageIndex(0-base, DialogueIndex 오름차순) 번째 대사를 찾아 OutLine 에
//# 채우고 true, 없으면(범위 밖 = 더 이어질 대사 없음) false 를 반환한다. 부수효과 없음 — 테스트 대상.
SKILLPROJECT_API bool TryGetDialogueLineAtIndex(const UDataTable* InDialogueTable, int32 InDialogueId, int32 InPageIndex, FText& OutLine);

//# MissionCommunication 의 판별자 — Offer 행/Report 행에서 사용하는 필드가 다르다
UENUM(BlueprintType)
enum class ESpyMissionCommRole : uint8
{
	Offer,
	Report,
};

//# Mission 의 필수 관계 — 모든 미션 행이 정확히 1개씩 갖는다. NPC/Dialogue 로의 FK.
//# 명명 규칙(§14-1-5): Mission_Communication
USTRUCT(BlueprintType)
struct FSpyMissionCommunicationRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	int32 MissionId = 0;

	UPROPERTY(EditAnywhere)
	int32 NPCId = 0;

	UPROPERTY(EditAnywhere)
	ESpyMissionCommRole Role = ESpyMissionCommRole::Offer;

	//# Role == Offer 일 때만 사용
	UPROPERTY(EditAnywhere)
	int32 OfferDialogueId = 0;

	//# Role == Offer 일 때만 사용
	UPROPERTY(EditAnywhere)
	int32 InProgressDialogueId = 0;

	//# Role == Report 일 때만 사용
	UPROPERTY(EditAnywhere)
	int32 ReportDialogueId = 0;
};

//# NPC 도메인의 DataTable 3개를 묶는 허브. NPC 블루프린트 6종은 이것 하나만 참조한다
UCLASS()
class SKILLPROJECT_API USpyNPCConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "NPC")
	TObjectPtr<UDataTable> NPCTable; //# RowStruct = FSpyNPCRow

	UPROPERTY(EditDefaultsOnly, Category = "NPC")
	TObjectPtr<UDataTable> DialogueTable; //# RowStruct = FSpyDialogueRow

	UPROPERTY(EditDefaultsOnly, Category = "NPC")
	TObjectPtr<UDataTable> MissionCommunicationTable; //# RowStruct = FSpyMissionCommunicationRow
};

//# 부수효과 없음 — 4상태 판정. 이 NPC의 Offer/Report 대상 MissionId 를 미리 알고 있다고 가정한다
//# (ASpyNPCCharacter가 BeginPlay에 MissionCommunicationTable 스캔으로 캐싱, Task 4)
SKILLPROJECT_API ESpyNPCDialogueState ResolveNPCDialogueState(int32 CurrentMissionId, bool bAccepted, int32 OfferMissionId, int32 ReportMissionId);
