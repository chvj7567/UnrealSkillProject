#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Components/WidgetComponent.h"

#include "SKUIManager.generated.h"

class UWidgetComponent;
class USKUserWidget;

UCLASS()
class SKUICORE_API USKUIManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//# 파생 서브클래스(예: USpyUIManager)가 있으면 base 는 생성하지 않음 → leaf 인스턴스 1개만
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	//# base·leaf 어디서든 동일한 leaf 인스턴스를 반환
	static USKUIManager* Get(const UObject* WorldContextObject);

public:
	UFUNCTION(BlueprintCallable)
	void OpenUI(FName InUIName);

	UFUNCTION(BlueprintCallable)
	void CloseUI(FName InUIName);

	UFUNCTION(BlueprintCallable)
	void CloseLastUI();

	UFUNCTION(BlueprintCallable)
	void OpenSubUI(FName InUIName, UWidgetComponent* WidgetComponent, EWidgetSpace Space);

	UFUNCTION(BlueprintCallable)
	void AddCashingUI(USKUserWidget* UserWidget);

protected:
	const int MaxCashingUICount = 5;

protected:
	UPROPERTY()
	TArray<TObjectPtr<USKUserWidget>> OpenUIList;

	UPROPERTY()
	TArray<TObjectPtr<USKUserWidget>> CashingUIList;
};
