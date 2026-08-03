// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Util/DefineEnum.h"

#include "CommonInterface.NPC.generated.h"

class APlayerController;

//# 서버가 상호작용 판정 결과로 클라이언트에 돌려주는 값.
USTRUCT()
struct FSpyNPCDialogueResult
{
	GENERATED_BODY()

public:
	UPROPERTY()
	ESpyNPCDialogueState State = ESpyNPCDialogueState::Default;

	//# 현재 상태(Default/Offer/InProgress/Report)에 대응하는 DialogueId — 클라이언트가
	//# F로 페이지를 넘길 때 이 Id 로 다음 인덱스를 조회한다
	UPROPERTY()
	int32 DialogueId = 0;

	UPROPERTY()
	FText NPCName;

	UPROPERTY()
	FText Line;

	//# Offer 상태일 때만 true — 미션 수락 카드를 띄운다. Report 는 카드가 없다
	UPROPERTY()
	bool bShowMissionCard = false;

	//# Offer 상태일 때만 채운다. 보상 텍스트는 없다 — Gameplay 타입은 보상이 없다
	UPROPERTY()
	FText MissionTitle;

	UPROPERTY()
	FText MissionDescription;
};

//# NPC 도메인의 유일한 외부 진입점 (cpp-style §13, 하위 컴포넌트 1개라 §13 예외 대상이나
//# 소비자를 위해 인터페이스는 미리 뺀다).
UINTERFACE(MinimalAPI)
class USpyNPCRoot : public UInterface
{
	GENERATED_BODY()
};

class ISpyNPCRoot
{
	GENERATED_BODY()

public:
	//# 서버 권한에서만 유효한 결과를 반환한다. 상태가 Report 면 이 호출 안에서
	//# AddProgress(Event_Mission_Report, 1) 까지 함께 처리한다.
	virtual FSpyNPCDialogueResult RequestInteract(APlayerController* Requester) = 0;

	virtual int32 GetNPCId() const = 0;

	//# 서버 재검증 전용 — 클라이언트가 신고한 상호작용 요청을 트리거(오버랩)와 동일한
	//# 기하로 재확인한다(point-distance 로 재구현 시 불일치 구간 발생, spec §5-1).
	virtual bool IsPawnInRange(const AActor* RequesterPawn) const = 0;

	//# 부수효과 없음. InDialogueId 그룹의 InPageIndex 번째 대사를 찾아 OutLine 에 채우고 true,
	//# 없으면 false. 클라이언트에서도 안전하게 호출 가능(DialogueTable 은 정적 콘텐츠 — 서버 재검증 불필요).
	virtual bool GetDialogueLineAtIndex(int32 InDialogueId, int32 InPageIndex, FText& OutLine) const = 0;
};
