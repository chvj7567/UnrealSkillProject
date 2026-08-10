// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "CommonInterface.System.generated.h"

class UPrimitiveComponent;

//# 미션 타겟 액터(3종)가 선택적으로 노출하는 네비 숨김 트리거 볼륨(design 2026-08-10 §4) —
//# USpyNavigationComponent 는 이 인터페이스로만 접근하고 구체 타겟 타입을 알지 않는다(cpp-style §8·§10).
UINTERFACE(MinimalAPI)
class USpyMissionTargetHideVolume : public UInterface
{
	GENERATED_BODY()
};

class ISpyMissionTargetHideVolume
{
	GENERATED_BODY()

public:
	//# 트리거 비활성 인스턴스는 nullptr 반환 — 호출부는 이를 "거리 히스테리시스로 폴백" 신호로 해석한다.
	virtual UPrimitiveComponent* GetHideTriggerComponent() const = 0;
};
