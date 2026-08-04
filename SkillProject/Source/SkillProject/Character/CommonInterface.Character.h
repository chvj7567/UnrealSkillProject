// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ManagerComponent/CommonInterface.Manager.h"

#include "CommonInterface.Character.generated.h"

class UInputAction;

//# 캐릭터 도메인의 유일한 외부 진입점 (cpp-style §13).
//# 소비자는 하위 컴포넌트를 탐색하지 않고 루트에서 핸들을 받는다.
UINTERFACE(MinimalAPI)
class USpyCharacterRoot : public UInterface
{
	GENERATED_BODY()
};

class ISpyCharacterRoot
{
	GENERATED_BODY()

public:
	virtual TScriptInterface<ISpyParkourHost> GetParkourHost() const = 0;
	virtual TScriptInterface<ISpyTargetProvider> GetTargetProvider() const = 0;
	virtual TScriptInterface<ISpyGrappleHost> GetGrappleHost() const = 0;
	virtual TScriptInterface<ISpyInteractionHost> GetInteractionHost() const = 0;

	//# 대화 진행(F) 등 상호작용 네이티브 입력에 현재 바인딩된 InputAction.
	//# UI 가 CharacterAssetData/InputConfig 체인을 직접 파헤치지 않게 루트가 대신 조회한다.
	virtual const UInputAction* GetInteractInputAction() const = 0;

	//# 벽 밀착 액션 동안 SpringArm 콜리전 테스트를 억제한다 (참조 카운트).
	virtual void PushCameraCollisionSuppress() = 0;
	virtual void PopCameraCollisionSuppress() = 0;

	//# 엔진 UMotionWarpingComponent 는 우리 인터페이스를 구현할 수 없다.
	//# 탐색을 없애기 위해 루트가 얇은 의도 API 하나만 노출한다.
	virtual void AddMotionWarpTarget(FName WarpName, const FVector& Loc, const FRotator& Rot) = 0;
};
