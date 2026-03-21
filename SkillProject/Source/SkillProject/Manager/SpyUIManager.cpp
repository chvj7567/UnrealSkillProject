// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/SpyUIManager.h"
#include "Manager/SpyAssetManager.h"
#include "Blueprint/UserWidget.h"
#include "UI/SpyUserWidget.h"

#include "Data/SKAssetData.h"
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

void USpyUIManager::OpenUI(FName InUIName)
{
	const USpyAssetData& AssetData = USpyAssetManager::Get().GetAssetData();
	const FSoftObjectPath& AssetPath = AssetData.GetAssetPathByName(InUIName);

	FSpyAssetAndDelegate LoadDelegate;
	LoadDelegate.BindLambda([this, InUIName](UObject* LoadedAsset)
		{
			if (LoadedAsset == nullptr)
				return;

			if (TSubclassOf<USKUserWidget> UI = USpyAssetManager::GetSubclassByName<USKUserWidget>(InUIName))
			{
				//# 이미 열려있는 UI 확인
				const TObjectPtr<USpyUserWidget>* FindOpenningUI = OpenUIList.FindByPredicate(
					[InUIName](TObjectPtr<USpyUserWidget>& UserWidget)
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
				const TObjectPtr<USpyUserWidget>* FindCashingUI = CashingUIList.FindByPredicate(
					[InUIName](TObjectPtr<USpyUserWidget>& UserWidget)
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
				if (USpyUserWidget* UserWidget = CreateWidget<USpyUserWidget>(GetWorld(), UI))
				{
					UserWidget->SetUIName(InUIName);
					OpenUIList.Add(UserWidget);

					UserWidget->AddToViewport();

					UE_LOG(LogTemp, Warning, TEXT("New Opening UI: %s"), *InUIName.ToString());
				}
			}
		});

	USpyAssetManager::LoadAssetAsync(AssetPath, LoadDelegate);
}

void USpyUIManager::CloseUI(FName InUIName)
{
	UE_LOG(LogTemp, Warning, TEXT("CloseUI: %s"), *InUIName.ToString());

	if (OpenUIList.IsEmpty())
		return;

	//# 이미 열려있는 UI 확인
	const TObjectPtr<USpyUserWidget>* FindOpenningUI = OpenUIList.FindByPredicate(
		[InUIName](TObjectPtr<USpyUserWidget>& UserWidget)
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

void USpyUIManager::CloseLastUI()
{
	if (OpenUIList.IsEmpty())
		return;

	if (USKUserWidget* UserWidget = OpenUIList.Last())
	{
		OpenUIList.Pop();
		CloseUI(UserWidget->GetUIName());
	}
}

void USpyUIManager::OpenSubUI(FName InUIName, UWidgetComponent* WidgetComponent, EWidgetSpace Space)
{
	const USpyAssetData& AssetData = USpyAssetManager::Get().GetAssetData();
	const FSoftObjectPath& AssetPath = AssetData.GetAssetPathByName(InUIName);

	if (WidgetComponent == nullptr)
		return;

	FSpyAssetAndDelegate LoadDelegate;
	LoadDelegate.BindLambda([InUIName, WidgetComponent, Space](UObject* LoadedAsset)
		{
			if (LoadedAsset == nullptr)
				return;

			if (TSubclassOf<USKUserWidget> UI = USpyAssetManager::GetSubclassByName<USKUserWidget>(InUIName))
			{
				WidgetComponent->SetWidgetClass(UI);
				WidgetComponent->SetWidgetSpace(Space);
				WidgetComponent->InitWidget();
				WidgetComponent->SetVisibility(true);
			}
		});

	USpyAssetManager::LoadAssetAsync(AssetPath, LoadDelegate);
}

void USpyUIManager::AddCashingUI(USpyUserWidget* UserWidget)
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
