// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SplineComponent.h"
#include "ManagerComponent/SpyNavigationComponent.h"

#include "SpyNavigationComponentTestAccessor.generated.h"

//# ApplyPathPoints()의 BoundHideTrigger.IsValid() 바이패스(design 2026-08-10 §6)는 protected 라
//# 직접 호출 못한다 — World 없이 그 분기만 검증하는 테스트 전용 서브클래스(production 미변경, SpyMissionComponentTestListener.h 선례).
UCLASS()
class USpyNavigationComponentTestAccessor : public USpyNavigationComponent
{
	GENERATED_BODY()

public:
	void Test_ApplyPathPoints(const TArray<FVector>& InPathPoints)
	{
		ApplyPathPoints(InPathPoints);
	}

	void Test_SetPathSpline(USplineComponent* InSpline)
	{
		PathSpline = InSpline;
	}

	void Test_SetBoundHideTrigger(UPrimitiveComponent* InTrigger)
	{
		BoundHideTrigger = InTrigger;
	}

	void Test_SetPathVisible(bool bInVisible)
	{
		bPathVisible = bInVisible;
	}

	//# UnbindHideTrigger() 가 이전 구독을 실제로 RemoveDynamic 했는지 확인하는 훅
	bool Test_IsAnyHideTriggerListenerBound(UPrimitiveComponent* InTrigger) const
	{
		if (InTrigger == nullptr)
			return false;

		return InTrigger->OnComponentBeginOverlap.IsBound() || InTrigger->OnComponentEndOverlap.IsBound();
	}

	//# BeginPlay 활성화 게이트(ShouldActivateForOwningPawn) 는 protected static 이라
	//# World 없이 Pawn+Controller 픽스처만으로 이 정적 래퍼를 통해 검증한다.
	static bool Test_ShouldActivateForOwningPawn(const APawn* InPawn)
	{
		return ShouldActivateForOwningPawn(InPawn);
	}
};
