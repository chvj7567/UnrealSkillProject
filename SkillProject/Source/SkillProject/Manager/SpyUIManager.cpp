// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/SpyUIManager.h"
#include "Manager/SpyAssetManager.h"
#include "Blueprint/UserWidget.h"
#include "UI/SpyUIDataAsset.h"
#include "UI/SpyUserWidget.h"
#include "Components/WidgetComponent.h"
#include "Data/SpyAssetData.h"

#include "Character/SkillProjectCharacter.h"

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

void USpyUIManager::OpenUI(ESpyUIType UIType)
{
	USpyAssetManager& AssetManager = USpyAssetManager::Get();
	const USpyAssetData& AssetData = AssetManager.GetAssetData();

	FString EnumName = StaticEnum<ESpyUIType>()->GetNameStringByValue((int64)UIType);
	UClass* UI = AssetManager.GetAssetByName<UClass>(*EnumName);
	if (UI)
	{
		//# 이미 열려있는 UI 확인
		const TObjectPtr<USpyUserWidget>* FindOpenningUI = OpenUIList.FindByPredicate(
			[UIType](TObjectPtr<USpyUserWidget>& UserWidget)
			{
				return UserWidget->GetUIType() == UIType;
			});

		if (FindOpenningUI)
		{
			//# 동일한 UI는 중복해서 띄우지 않음
			UE_LOG(LogTemp, Warning, TEXT("Already Opening UI: %s"), *EnumName);
			return;
		}

		//# 캐싱 중인 UI 확인
		const TObjectPtr<USpyUserWidget>* FindCashingUI = CashingUIList.FindByPredicate(
			[UIType](TObjectPtr<USpyUserWidget>& UserWidget)
			{
				return UserWidget->GetUIType() == UIType;
			});

		if (FindCashingUI)
		{
			//# 캐싱 중인 UI이면 Open
			OpenUIList.Add(FindCashingUI->Get());
			FindCashingUI->Get()->AddToViewport();

			UE_LOG(LogTemp, Warning, TEXT("Cashing Opening UI: %s"), *EnumName);
			return;
		}

		//# UI 생성
		if (USpyUserWidget* UserWidget = CreateWidget<USpyUserWidget>(GetWorld(), UI))
		{
			UserWidget->UIType = UIType;
			OpenUIList.Add(UserWidget);

			UserWidget->AddToViewport();

			UE_LOG(LogTemp, Warning, TEXT("New Opening UI: %s"), *EnumName);
		}
	}
}

void USpyUIManager::CloseUI(ESpyUIType UIType)
{
	FString EnumName = StaticEnum<ESpyUIType>()->GetNameStringByValue((int64)UIType);
	UE_LOG(LogTemp, Warning, TEXT("CloseUI: %s"), *EnumName);

	if (OpenUIList.IsEmpty())
		return;
	
	//# 이미 열려있는 UI 확인
	const TObjectPtr<USpyUserWidget>* FindOpenningUI = OpenUIList.FindByPredicate(
		[UIType](TObjectPtr<USpyUserWidget>& UserWidget)
		{
			return UserWidget->GetUIType() == UIType;
		});

	if (FindOpenningUI)
	{
		OpenUIList.Remove(FindOpenningUI->Get());
		AddCashingUI(FindOpenningUI->Get());
		FindOpenningUI->Get()->RemoveFromParent();
	}
}

void USpyUIManager::CloseLastUI()
{
	if (OpenUIList.IsEmpty())
		return;

	if (USpyUserWidget* UserWidget = OpenUIList.Last())
	{
		OpenUIList.Pop();
		CloseUI(UserWidget->UIType);
	}
}

void USpyUIManager::OpenSubUI(ESpyUIType UIType, UWidgetComponent* WidgetComponent, EWidgetSpace Space)
{
	USpyAssetManager& AssetManager = USpyAssetManager::Get();
	const USpyAssetData& AssetData = AssetManager.GetAssetData();

	if (!WidgetComponent)
		return;

	FString EnumName = StaticEnum<ESpyUIType>()->GetNameStringByValue((int64)UIType);
	UClass* UI = AssetManager.GetAssetByName<UClass>(*EnumName);
	if (UI)
	{
		WidgetComponent->SetWidgetClass(UI);
		WidgetComponent->SetWidgetSpace(Space);
		WidgetComponent->InitWidget();
		WidgetComponent->SetVisibility(true);

	}
}

void USpyUIManager::AddCashingUI(USpyUserWidget* UserWidget)
{
	//# 캐싱 수가 Max라면 오래된 UI 제거 후 추가
	if (MaxCashingUICount > 0)
	{
		for (USpyUserWidget* CashingUserWIdget : CashingUIList)
		{
			if (CashingUserWIdget->UIType == UserWidget->UIType)
				return;
		}

		CashingUIList.Add(UserWidget);

		if (CashingUIList.Num() >= MaxCashingUICount)
		{
			CashingUIList.RemoveAt(0);
		}
	}
}
