// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/SpyUIManager.h"
#include "Manager/SpyAssetManager.h"
#include "Blueprint/UserWidget.h"
#include "UI/SpyUserWidget.h"
#include "Components/WidgetComponent.h"
#include "Asset/SKAssetData.h"
#include "Character/SpyCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyUIManager)

void USpyUIManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void USpyUIManager::Deinitialize()
{
	Super::Deinitialize();
}

USpyUIManager* USpyUIManager::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
		return nullptr;

	if (const UWorld* World = WorldContextObject->GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			return GI->GetSubsystem<USpyUIManager>();
		}
	}

	return nullptr;
}

void USpyUIManager::OpenSpyUI(ESpyUIType UIType)
{
	FString EnumName = StaticEnum<ESpyUIType>()->GetNameStringByValue((int64)UIType);
	OpenUI(FName(*EnumName));
}

void USpyUIManager::CloseSpyUI(ESpyUIType UIType)
{
	FString EnumName = StaticEnum<ESpyUIType>()->GetNameStringByValue((int64)UIType);
	CloseUI(FName(*EnumName));
}

void USpyUIManager::CloseLastSpyUI()
{
	CloseLastUI();
}

void USpyUIManager::OpenSubSpyUI(ESpyUIType UIType, UWidgetComponent* WidgetComponent, EWidgetSpace Space)
{
	FString EnumName = StaticEnum<ESpyUIType>()->GetNameStringByValue((int64)UIType);
	OpenSubUI(FName(*EnumName), WidgetComponent, Space);
}
