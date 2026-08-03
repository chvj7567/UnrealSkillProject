// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "CommonInterface.Interactable.generated.h"

class APlayerController;

//# 상호작용 오브젝트 도메인의 유일한 외부 진입점 (cpp-style §13, 하위 컴포넌트 1개라 §13
//# 예외 대상이나 소비자(USpyInteractionComponent)를 위해 인터페이스는 미리 뺀다).
UINTERFACE(MinimalAPI)
class USpyInteractableRoot : public UInterface
{
	GENERATED_BODY()
};

class ISpyInteractableRoot
{
	GENERATED_BODY()

public:
	//# 서버 권한에서만 유효. 상호작용 처리 + AddProgress + 소진 처리까지 이 안에서 끝낸다.
	virtual void RequestInteract(APlayerController* Requester) = 0;

	//# 서버 재검증 전용 — 트리거(오버랩)와 동일한 기하로 재확인한다
	//# (point-distance로 재구현 시 불일치 구간 발생, NPC 패턴과 동일 이유).
	virtual bool IsPawnInRange(const AActor* RequesterPawn) const = 0;

	virtual FText GetInteractVerb() const = 0;
};
