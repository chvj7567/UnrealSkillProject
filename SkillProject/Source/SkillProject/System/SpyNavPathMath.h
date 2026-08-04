// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

//# 내비게이션 경로 표현 로직 순수함수 모음 — 컴포넌트/월드 없이 테스트 가능하게 분리한다
namespace SpyNavPathMath {
//# 경로점을 세그먼트 시작/끝 좌표 쌍으로 변환한다. 점이 2개 미만이면 빈 배열.
SKILLPROJECT_API TArray<TPair<FVector, FVector>> BuildSplineSegments(const TArray<FVector>& PathPoints);

//# 경로점 배열에서 시작(플레이어 위치)으로부터 TrimDistanceCm 만큼을 잘라낸 새 배열을 만든다.
//# 발밑 시야 가림 방지(§4-1) — 잘라낸 지점은 원래 세그먼트를 보간해 정확히 TrimDistanceCm 위치에 놓는다.
//# 점이 2개 미만이거나 TrimDistanceCm<=0 이면 원본을 그대로 반환한다.
SKILLPROJECT_API TArray<FVector> TrimLeadingDistance(const TArray<FVector>& PathPoints, float TrimDistanceCm);

//# 경로점을 잇는 총 길이(cm). 점이 2개 미만이면 0.
SKILLPROJECT_API float ComputePathLength(const TArray<FVector>& PathPoints);

//# 도착 임계값 히스테리시스(§4-3) — 보이는 상태(bPreviouslyVisible=true)에서는 길이가
//# HideThresholdCm 아래로 내려가야 숨고, 숨은 상태에서는 ReshowThresholdCm 위로 올라가야
//# 다시 보인다. 두 임계값 사이(히스테리시스 밴드)에서는 이전 상태를 유지한다.
SKILLPROJECT_API bool EvaluateHysteresisVisibility(float PathLengthCm, float HideThresholdCm, float ReshowThresholdCm, bool bPreviouslyVisible);
} //namespace SpyNavPathMath
