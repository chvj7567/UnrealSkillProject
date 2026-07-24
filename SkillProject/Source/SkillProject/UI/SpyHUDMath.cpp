// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SpyHUDMath.h"

ESpyCardinal SpyHUDMath::HeadingToCardinal(float YawDegrees)
{
	//# [0,360) 정규화
	float Yaw = FMath::Fmod(YawDegrees, 360.f);
	if (Yaw < 0.f)
	{
		Yaw += 360.f;
	}

	//# 22.5도 오프셋 후 45도 섹터 인덱스 (0=N)
	const int32 Sector = FMath::FloorToInt((Yaw + 22.5f) / 45.f) % 8;
	return static_cast<ESpyCardinal>(Sector);
}

float SpyHUDMath::CooldownNormalized(float Remaining, float Duration)
{
	if (Duration <= 0.f)
	{
		return 0.f;
	}

	return FMath::Clamp(Remaining / Duration, 0.f, 1.f);
}
