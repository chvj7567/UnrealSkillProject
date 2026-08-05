// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"

#include "SpyMissionTargetPoint.generated.h"

class USceneComponent;
class UBillboardComponent;

//# Vault/Climb/GrappleHook 전용 경량 구역 마커(design §5-3·§5-4). 레벨 디자이너가 구역
//# 안쪽(NPC 위치가 아닌 지점)에 배치한다 — 특정 오브젝트가 아니라 진입 안내일 뿐이다(§5-3).
UCLASS()
class SKILLPROJECT_API ASpyMissionTargetPoint : public AActor
{
	GENERATED_BODY()

public:
	ASpyMissionTargetPoint();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	//# 대응 미션의 FSpyMissionRow.MatchTag 와 정확히 일치시켜 배치한다(leaf 태그, §8)
	UPROPERTY(EditAnywhere, Category = "Navigation")
	FGameplayTag TargetMissionTag;

private:
	//# RootComponent 필수(code-reviewer BLOCKER) — 없으면 AActor::GetActorLocation() 이
	//# 항상 원점을 반환해 레벨 배치 좌표가 레지스트리에 전혀 반영되지 않는다.
	UPROPERTY(VisibleAnywhere, Category = "Navigation")
	TObjectPtr<USceneComponent> RootScene;

#if WITH_EDITORONLY_DATA
	//# 에디터 뷰포트 가시성 전용(design §5-4 "권장" 항목) — 런타임 렌더링 없음, 자동으로 스트립된다.
	UPROPERTY()
	TObjectPtr<UBillboardComponent> EditorBillboard;
#endif
};
