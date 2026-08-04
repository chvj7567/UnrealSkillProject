// Fill out your copyright notice in the Description page of Project Settings.

#include "System/SpyNavPathMath.h"

TArray<TPair<FVector, FVector>> SpyNavPathMath::BuildSplineSegments(const TArray<FVector>& PathPoints)
{
	TArray<TPair<FVector, FVector>> Segments;

	if (PathPoints.Num() < 2)
		return Segments;

	for (int32 Index = 0; Index < PathPoints.Num() - 1; ++Index)
	{
		Segments.Add(TPair<FVector, FVector>(PathPoints[Index], PathPoints[Index + 1]));
	}

	return Segments;
}

TArray<FVector> SpyNavPathMath::TrimLeadingDistance(const TArray<FVector>& PathPoints, float TrimDistanceCm)
{
	if (PathPoints.Num() < 2 || TrimDistanceCm <= 0.f)
		return PathPoints;

	float RemainingTrim = TrimDistanceCm;
	int32 StartIndex = PathPoints.Num() - 1;
	FVector StartPoint = PathPoints.Last();

	for (int32 Index = 0; Index < PathPoints.Num() - 1; ++Index)
	{
		const float SegLength = FVector::Dist(PathPoints[Index], PathPoints[Index + 1]);

		if (SegLength >= RemainingTrim)
		{
			const float Alpha = (SegLength > 0.f ? RemainingTrim / SegLength : 0.f);
			StartPoint = FMath::Lerp(PathPoints[Index], PathPoints[Index + 1], Alpha);
			StartIndex = Index + 1;
			break;
		}

		RemainingTrim -= SegLength;
	}

	TArray<FVector> Trimmed;
	Trimmed.Add(StartPoint);

	for (int32 Index = StartIndex; Index < PathPoints.Num(); ++Index)
	{
		if (PathPoints[Index].Equals(StartPoint))
			continue;

		Trimmed.Add(PathPoints[Index]);
	}

	return Trimmed;
}

float SpyNavPathMath::ComputePathLength(const TArray<FVector>& PathPoints)
{
	float Length = 0.f;

	for (int32 Index = 0; Index < PathPoints.Num() - 1; ++Index)
	{
		Length += FVector::Dist(PathPoints[Index], PathPoints[Index + 1]);
	}

	return Length;
}

bool SpyNavPathMath::EvaluateHysteresisVisibility(float PathLengthCm, float HideThresholdCm, float ReshowThresholdCm, bool bPreviouslyVisible)
{
	if (bPreviouslyVisible)
		return (PathLengthCm >= HideThresholdCm);

	return (PathLengthCm >= ReshowThresholdCm);
}
