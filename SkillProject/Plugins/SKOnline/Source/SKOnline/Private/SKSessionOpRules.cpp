// Fill out your copyright notice in the Description page of Project Settings.

#include "SKSessionOpRules.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKSessionOpRules)

bool USKSessionOpRules::CanStartOp(ESKSessionOp CurrentOp, ESKSessionOp RequestedOp)
{
	//# 아무 작업도 요청하지 않은 것은 유효한 명령이 아니다
	if (RequestedOp == ESKSessionOp::None)
		return false;

	//# 세션 작업은 비동기 콜백으로 끝나므로 동시에 하나만 허용한다
	return CurrentOp == ESKSessionOp::None;
}

bool USKSessionOpRules::ShouldPreemptFindForHost(ESKSessionOp CurrentOp)
{
	//# 검색은 취소해도 세션 상태가 남지 않는다 — 사용자가 누른 방 만들기를 우선한다
	return CurrentOp == ESKSessionOp::Finding;
}
