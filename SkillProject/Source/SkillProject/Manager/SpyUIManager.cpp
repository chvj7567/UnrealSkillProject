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

void USpyUIManager::OpenUI(ESpyUIType UIType)
{
	USpyAssetManager& AssetManager = USpyAssetManager::Get();

	if (USpyUIDataAsset* UIDataAsset = AssetManager.LoadUI())
	{
		for (FSpyUIData Data : UIDataAsset->UIDatas)
		{
			if (Data.UIType != UIType)
				continue;

			//# 이미 열려있는 UI 확인
			const TObjectPtr<USpyUserWidget>* FindOpenningUI = OpenUIList.FindByPredicate(
				[UIType](TObjectPtr<USpyUserWidget>& UserWidget)
				{
					return UserWidget->GetUIType() == UIType;
				});

			if (FindOpenningUI)
			{
				//# 동일한 UI는 중복해서 띄우지 않음
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
				FindCashingUI->Get()->AddToViewport();
				return;
			}

			//# UI 생성
			if (USpyUserWidget* UserWidget = CreateWidget<USpyUserWidget>(GetWorld(), Data.UIWidgetClass))
			{
				UserWidget->UIType = UIType;
				OpenUIList.Add(UserWidget);
				LastUIType = UIType;

				UserWidget->AddToViewport();
			}
		}
	}
}

void USpyUIManager::CloseUI(ESpyUIType UIType)
{
	if (UIType == LastUIType)
	{
		CloseLastUI();
		return;
	}
	
	//# 이미 열려있는 UI 확인
	const TObjectPtr<USpyUserWidget>* FindOpenningUI = OpenUIList.FindByPredicate(
		[UIType](TObjectPtr<USpyUserWidget>& UserWidget)
		{
			return UserWidget->GetUIType() == UIType;
		});

	if (FindOpenningUI)
	{
		AddCashingUI(FindOpenningUI->Get());
		FindOpenningUI->Get()->RemoveFromViewport();
	}
}

void USpyUIManager::CloseLastUI()
{
	USpyUserWidget* UserWidget = OpenUIList.Last();
	if (UserWidget)
	{
		AddCashingUI(UserWidget);
		UserWidget->RemoveFromViewport();
	}
}

void USpyUIManager::AddCashingUI(USpyUserWidget* UserWidget)
{
	//# 캐싱 수가 Max라면 오래된 UI 제거 후 추가
	if (MaxCashingUICount > 0 && CashingUIList.Num() >= MaxCashingUICount)
	{
		CashingUIList.RemoveAt(0);
		CashingUIList.Add(UserWidget);
	}
}
