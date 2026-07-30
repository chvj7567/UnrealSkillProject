// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SKOnlineTypes.h"

#include "SKSessionOpRules.generated.h"

//# 세션 작업 진입 규칙 — 엔진 상태에 의존하지 않는 순수 판정이라 단위 테스트가 가능하다
UCLASS()
class SKONLINE_API USKSessionOpRules : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	//# 현재 작업이 없을 때만 새 작업을 시작한다. RequestedOp 가 None 이면 항상 거부
	UFUNCTION(BlueprintCallable, Category = "SKOnline")
	static bool CanStartOp(ESKSessionOp CurrentOp, ESKSessionOp RequestedOp);

	//# 방 만들기는 진행 중인 "검색"만 선점한다. 조인·파괴·호스팅 중에는 선점하지 않는다(세션 상태가 꼬인다)
	UFUNCTION(BlueprintCallable, Category = "SKOnline")
	static bool ShouldPreemptFindForHost(ESKSessionOp CurrentOp);
};
