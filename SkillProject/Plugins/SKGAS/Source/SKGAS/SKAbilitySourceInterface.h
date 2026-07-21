// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/Interface.h"

#include "SKAbilitySourceInterface.generated.h"

class UObject;
class UPhysicalMaterial;
struct FGameplayTagContainer;

//# 어빌리티 계산 소스 역할을 하는 오브젝트의 베이스 인터페이스
UINTERFACE()
class USKAbilitySourceInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class ISKAbilitySourceInterface
{
	GENERATED_IINTERFACE_BODY()

	//# 거리에 따른 이펙트 감쇠 배율 계산
	//# Distance   — 소스에서 타겟까지의 거리 (총알이 이동한 거리 등)
	//# SourceTags — 소스에서 취합된 태그
	//# TargetTags — 타겟이 현재 보유한 태그
	//# return     — 거리로 인해 base 어트리뷰트 값에 곱할 배율
	virtual float GetDistanceAttenuation(float Distance, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr) const = 0;

	virtual float GetPhysicalMaterialAttenuation(const UPhysicalMaterial* PhysicalMaterial, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr) const = 0;
};
