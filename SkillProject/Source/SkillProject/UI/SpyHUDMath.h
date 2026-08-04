// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

//# 8방위 열거 — N 중심=0도, 시계방향으로 45도씩
enum class ESpyCardinal : uint8
{
	N,
	NE,
	E,
	SE,
	S,
	SW,
	W,
	NW
};

//# HUD 표현 로직 순수함수 모음 — 위젯/월드 없이 테스트 가능하게 분리한다
namespace SpyHUDMath {
//# yaw(도)를 8방위로. N 중심=0, 각 방위 폭 45도(±22.5)
SKILLPROJECT_API ESpyCardinal HeadingToCardinal(float YawDegrees);

//# 쿨다운 진행 0(준비)~1(방금발동). Duration<=0 이면 0
SKILLPROJECT_API float CooldownNormalized(float Remaining, float Duration);

//# 매핑된 키 중 첫 번째의 표시 이름. 매핑이 없으면 빈 텍스트 —
//# 폴백 문구(어떤 키를 보여줄지)는 호출부가 결정한다 (위젯마다 다름)
SKILLPROJECT_API FText ResolveKeyDisplayName(const TArray<FKey>& MappedKeys);
} //namespace SpyHUDMath
