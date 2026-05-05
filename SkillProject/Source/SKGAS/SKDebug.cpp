// Fill out your copyright notice in the Description page of Project Settings.

#include "SKDebug.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarSpyDebugDraw(
    TEXT("spy.DebugDraw"),
    0,
    TEXT("Spy 디버그 시각화 토글 (0=off, 1=on). DrawDebug / OnScreenMessage 일괄 제어."),
    ECVF_Cheat
);

bool SpyDebugDrawEnabled()
{
    return CVarSpyDebugDraw.GetValueOnAnyThread() != 0;
}
