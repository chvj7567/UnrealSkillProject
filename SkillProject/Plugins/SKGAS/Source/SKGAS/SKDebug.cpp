// Fill out your copyright notice in the Description page of Project Settings.

#include "SKDebug.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarSKDebugDraw(
    TEXT("sk.DebugDraw"),
    0,
    TEXT("SK 디버그 시각화 토글 (0=off, 1=on). DrawDebug / OnScreenMessage 일괄 제어."),
    ECVF_Cheat
);

bool SKDebugDrawEnabled()
{
    return CVarSKDebugDraw.GetValueOnAnyThread() != 0;
}
