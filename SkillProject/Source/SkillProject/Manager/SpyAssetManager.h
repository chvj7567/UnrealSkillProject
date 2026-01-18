// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Asset/SKAssetManager.h"
#include "Asset/SKAssetData.h"
#include "NativeGameplayTags.h"
#include "Util/DefineEnum.h"

#include "SpyAssetManager.generated.h"

class USpyCharacterAssetData;

UCLASS()
class SKILLPROJECT_API USpyAssetManager : public USKAssetManager
{
	GENERATED_BODY()
	
public:
	USpyAssetManager();

public:
	static USpyAssetManager& Get();

public:
	const USpyCharacterAssetData& GetCharacterAssetData();

public:
	FName GetSkillAssetNameByType(ESpyCharacterType InCharacterType, FGameplayTag InSkillTag);
};