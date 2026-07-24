// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

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
} //namespace SpyHUDMath
