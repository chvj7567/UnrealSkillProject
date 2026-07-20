// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/SpyUIManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyUIManager)

USpyUIManager* USpyUIManager::Get(const UObject* WorldContextObject)
{
	if (WorldContextObject == nullptr)
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

void USpyUIManager::OpenSubSpyUI(ESpyUIType UIType, UWidgetComponent* WidgetComponent, EWidgetSpace Space)
{
	FString EnumName = StaticEnum<ESpyUIType>()->GetNameStringByValue((int64)UIType);
	OpenSubUI(FName(*EnumName), WidgetComponent, Space);
}
