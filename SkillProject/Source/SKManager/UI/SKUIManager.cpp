// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SKUIManager.h"
#include "UI/SKUserWidget.h"
#include "Asset/SKAssetManager.h"
#include "Components/WidgetComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKUIManager)

void USKUIManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void USKUIManager::Deinitialize()
{
	Super::Deinitialize();
}

USKUIManager* USKUIManager::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
		return nullptr;

	if (const UWorld* World = WorldContextObject->GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			return GI->GetSubsystem<USKUIManager>();
		}
	}

	return nullptr;
}

void USKUIManager::OpenUI(FName InUIName)
{
	if (TSubclassOf<USKUserWidget> UI = USKAssetManager::GetSubclassByName<USKUserWidget>(InUIName))
	{
		//# 이미 열려있는 UI 확인
		const TObjectPtr<USKUserWidget>* FindOpenningUI = OpenUIList.FindByPredicate(
			[InUIName](TObjectPtr<USKUserWidget>& UserWidget)
			{
				return UserWidget->GetUIName() == InUIName;
			});

		if (FindOpenningUI)
		{
			//# 동일한 UI는 중복해서 띄우지 않음
			UE_LOG(LogTemp, Warning, TEXT("Already Opening UI: %s"), *InUIName.ToString());
			return;
		}

		//# 캐싱 중인 UI 확인
		const TObjectPtr<USKUserWidget>* FindCashingUI = CashingUIList.FindByPredicate(
			[InUIName](TObjectPtr<USKUserWidget>& UserWidget)
			{
				return UserWidget->GetUIName() == InUIName;
			});

		if (FindCashingUI)
		{
			//# 캐싱 중인 UI이면 Open
			OpenUIList.Add(FindCashingUI->Get());
			FindCashingUI->Get()->AddToViewport();

			UE_LOG(LogTemp, Warning, TEXT("Cashing Opening UI: %s"), *InUIName.ToString());
			return;
		}

		//# UI 생성
		if (USKUserWidget* UserWidget = CreateWidget<USKUserWidget>(GetWorld(), UI))
		{
			UserWidget->SetUIName(InUIName);
			OpenUIList.Add(UserWidget);

			UserWidget->AddToViewport();

			UE_LOG(LogTemp, Warning, TEXT("New Opening UI: %s"), *InUIName.ToString());
		}
	}
}

void USKUIManager::CloseUI(FName InUIName)
{
	UE_LOG(LogTemp, Warning, TEXT("CloseUI: %s"), *InUIName.ToString());

	if (OpenUIList.IsEmpty())
		return;

	//# 이미 열려있는 UI 확인
	const TObjectPtr<USKUserWidget>* FindOpenningUI = OpenUIList.FindByPredicate(
		[InUIName](TObjectPtr<USKUserWidget>& UserWidget)
		{
			return UserWidget->GetUIName() == InUIName;
		});

	if (FindOpenningUI)
	{
		OpenUIList.Remove(FindOpenningUI->Get());
		AddCashingUI(FindOpenningUI->Get());
		FindOpenningUI->Get()->RemoveFromParent();
	}
}

void USKUIManager::CloseLastUI()
{
	if (OpenUIList.IsEmpty())
		return;

	if (USKUserWidget* UserWidget = OpenUIList.Last())
	{
		OpenUIList.Pop();
		CloseUI(UserWidget->GetUIName());
	}
}

void USKUIManager::OpenSubUI(FName InUIName, UWidgetComponent* WidgetComponent, EWidgetSpace Space)
{
	USKAssetManager& AssetManager = USKAssetManager::Get();
	const USKAssetData& AssetData = AssetManager.GetAssetData();

	if (WidgetComponent == nullptr)
		return;

	if (TSubclassOf<USKUserWidget> UI = USKAssetManager::GetSubclassByName<USKUserWidget>(InUIName))
	{
		WidgetComponent->SetWidgetClass(UI);
		WidgetComponent->SetWidgetSpace(Space);
		WidgetComponent->InitWidget();
		WidgetComponent->SetVisibility(true);

	}
}

void USKUIManager::AddCashingUI(USKUserWidget* UserWidget)
{
	//# 캐싱 수가 Max라면 오래된 UI 제거 후 추가
	if (MaxCashingUICount > 0)
	{
		for (USKUserWidget* CashingUserWIdget : CashingUIList)
		{
			if (CashingUserWIdget->GetUIName() == UserWidget->GetUIName())
				return;
		}

		CashingUIList.Add(UserWidget);

		if (CashingUIList.Num() >= MaxCashingUICount)
		{
			CashingUIList.RemoveAt(0);
		}
	}
}
