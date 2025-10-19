// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/SpyUIManager.h"
#include "Manager/SpyAssetManager.h"
#include "Blueprint/UserWidget.h"
#include "UI/SpyUIDataAsset.h"
#include "UI/SpyUserWidget.h"

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

void USpyUIManager::OpenWidget(ESpyUIType UIType)
{
	USpyAssetManager& AM = USpyAssetManager::Get();

	if (USpyUIDataAsset* UIDataAsset = AM.LoadUI())
	{
		for (FSpyUIData Data : UIDataAsset->UIDatas)
		{
			if (Data.UIType != UIType)
				continue;

			if (USpyUserWidget* MainHUD = CreateWidget<USpyUserWidget>(GetWorld(), Data.UIWidgetClass))
			{
				MainHUD->AddToViewport();
			}
		}
	}
}
