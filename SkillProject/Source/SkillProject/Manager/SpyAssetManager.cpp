// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/SpyAssetManager.h"
#include "Blueprint/UserWidget.h"
#include "Data/SpyCharacterAssetData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyAssetManager)

USpyAssetManager::USpyAssetManager()
{
}

USpyAssetManager& USpyAssetManager::Get()
{
	check(GEngine);

	if (USpyAssetManager* Singleton = Cast<USpyAssetManager>(GEngine->AssetManager))
	{
		return *Singleton;
	}

	UE_LOG(LogTemp, Fatal, TEXT("Invalid AssetManagerClassName in DefaultEngine.ini. It must be set to USpyAssetManager!"));

	return *NewObject<USpyAssetManager>();
}

const USpyCharacterAssetData& USpyAssetManager::GetCharacterAssetData()
{
    FPrimaryAssetId AssetId = FPrimaryAssetId(TEXT("SpyCharacterAssetData"), TEXT("SpyCharacterAssetData"));
    FSoftObjectPtr AssetPtr(Get().GetPrimaryAssetPath(AssetId));

    if (AssetPtr.IsPending())
    {
        AssetPtr.LoadSynchronous();
    }

    return *CastChecked<USpyCharacterAssetData>(AssetPtr.Get());
}

FName USpyAssetManager::GetSkillAssetNameByType(ESpyCharacterType InCharacterType, FGameplayTag InSkillTag)
{
    return Get().GetCharacterAssetData().GetSkillAssetNameByType(InCharacterType, InSkillTag);
}
