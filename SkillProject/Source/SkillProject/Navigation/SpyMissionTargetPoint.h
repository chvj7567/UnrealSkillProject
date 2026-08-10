// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "System/CommonInterface.System.h"

#include "SpyMissionTargetPoint.generated.h"

class USceneComponent;
class UBillboardComponent;
class UBoxComponent;
class UPrimitiveComponent;

//# Vault/Climb/GrappleHook 전용 경량 구역 마커(design §5-3·§5-4). 레벨 디자이너가 구역
//# 안쪽(NPC 위치가 아닌 지점)에 배치한다 — 특정 오브젝트가 아니라 진입 안내일 뿐이다(§5-3).
UCLASS()
class SKILLPROJECT_API ASpyMissionTargetPoint : public AActor, public ISpyMissionTargetHideVolume
{
	GENERATED_BODY()

public:
	ASpyMissionTargetPoint();

	//# ISpyMissionTargetHideVolume
	virtual UPrimitiveComponent* GetHideTriggerComponent() const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	//# 대응 미션의 FSpyMissionRow.MatchTag 와 정확히 일치시켜 배치한다(leaf 태그, §8)
	UPROPERTY(EditAnywhere, Category = "Navigation")
	FGameplayTag TargetMissionTag;

	//# 사용자 요청(2026-08-10) — 기본 true: 트리거를 기본 활성화한다. 인스턴스에서 false로
	//# 끄면 거리 히스테리시스로 폴백한다(design §5의 opt-in 방향을 opt-out으로 반전).
	UPROPERTY(EditAnywhere, Category = "Navigation")
	bool bEnableHideTrigger = true;

private:
	//# RootComponent 필수(code-reviewer BLOCKER) — 없으면 AActor::GetActorLocation() 이
	//# 항상 원점을 반환해 레벨 배치 좌표가 레지스트리에 전혀 반영되지 않는다.
	UPROPERTY(VisibleAnywhere, Category = "Navigation")
	TObjectPtr<USceneComponent> RootScene;

	//# 항상 생성하되 bEnableHideTrigger 로 콜리전 on/off — 디자이너가 에디터에서 Extent/회전을
	//# 직접 드래그해 영역을 그린다(design 2026-08-10 §5)
	UPROPERTY(VisibleAnywhere, Category = "Navigation")
	TObjectPtr<UBoxComponent> HideTriggerVolume;

#if WITH_EDITORONLY_DATA
	//# 에디터 뷰포트 가시성 전용(design §5-4 "권장" 항목) — 런타임 렌더링 없음, 자동으로 스트립된다.
	UPROPERTY()
	TObjectPtr<UBillboardComponent> EditorBillboard;
#endif
};
