// Copyright Epic Games, Inc. All Rights Reserved.

#include "SKGAS.h"

#define LOCTEXT_NAMESPACE "FSKGASModule"

void FSKGASModule::StartupModule()
{
	//# 모듈 로드 직후 실행 — 정확한 시점은 .uplugin 의 LoadingPhase 로 결정
}

void FSKGASModule::ShutdownModule()
{
	//# 셧다운 시 모듈 정리 — 동적 리로드 지원 모듈은 언로드 직전에 호출됨
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSKGASModule, SKGAS)