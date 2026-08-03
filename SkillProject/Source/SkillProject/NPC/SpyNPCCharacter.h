// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularCharacter.h"
#include "NPC/CommonInterface.NPC.h"

#include "SpyNPCCharacter.generated.h"

class USphereComponent;
class USpyNPCConfig;

//# NPC 도메인 루트. NPCId 하나로 3개 DataTable(USpyNPCConfig 경유)을 BeginPlay에 1회 스캔해
//# 자신의 Default/Offer/InProgress/Report 대사와 담당 MissionId(Offer/Report) 를 캐싱한다.
UCLASS()
class SKILLPROJECT_API ASpyNPCCharacter : public AModularCharacter, public ISpyNPCRoot
{
	GENERATED_BODY()

public:
	ASpyNPCCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//# ISpyNPCRoot
	virtual FSpyNPCDialogueResult RequestInteract(APlayerController* Requester) override;
	virtual int32 GetNPCId() const override
	{
		return NPCId;
	}
	virtual bool IsPawnInRange(const AActor* RequesterPawn) const override;
	virtual bool GetDialogueLineAtIndex(int32 InDialogueId, int32 InPageIndex, FText& OutLine) const override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
										 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
									   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	void CacheNPCData();

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> InteractionSphere;

	//# NPC 테이블 행 식별자이자 MissionCommunication.NPCId 매칭 키
	UPROPERTY(EditDefaultsOnly, Category = "Dialogue")
	int32 NPCId = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Dialogue")
	TObjectPtr<USpyNPCConfig> NPCConfig;

	//# BeginPlay 1회 캐싱 (§8 — 매 프레임/매 상호작용 조회 금지)
	bool bDataCached = false;
	FText CachedNPCDisplayName;
	int32 CachedDefaultDialogueId = 0;
	FText CachedDefaultLine;
	int32 CachedOfferMissionId = INDEX_NONE;
	int32 CachedOfferDialogueId = 0;
	FText CachedOfferLine;
	int32 CachedInProgressDialogueId = 0;
	FText CachedInProgressLine;
	int32 CachedReportMissionId = INDEX_NONE;
	int32 CachedReportDialogueId = 0;
	FText CachedReportLine;
};
