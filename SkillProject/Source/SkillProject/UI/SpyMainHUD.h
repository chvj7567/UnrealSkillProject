// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/SpyUserWidget.h"

#include "SpyMainHUD.generated.h"

class UProgressBar;
class UButton;
class UTextBlock;
class USpyLevelComponent;

UCLASS()
class SKILLPROJECT_API USpyMainHUD : public USpyUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UButton> Btn_Menu;

	//# 아직 WBP에 배치되지 않았을 수 있어 Optional로 바인딩한다
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> PB_Exp;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Level;

protected:
	UFUNCTION(BlueprintCallable)
	void ShowMenu();

	//# 로컬 폰의 LevelComponent를 찾아 구독한다. 아직 없으면 false
	bool TryBindLevelComponent();

	void UnbindLevelComponent();

	UFUNCTION()
	void HandleExperienceChanged(USpyLevelComponent* InLevelComponent, float InOldValue, float InNewValue);

	UFUNCTION()
	void HandleLevelChanged(USpyLevelComponent* InLevelComponent, int32 InOldLevel, int32 InNewLevel);

	void RefreshAll();

protected:
	UPROPERTY()
	TObjectPtr<USpyLevelComponent> BoundLevelComponent;

	FTimerHandle BindRetryTimerHandle;

	//# 바인딩 재시도 횟수. 상한(BindRetryMaxCount)에 도달하면 경고 1회 후 타이머를 멈춘다
	int32 BindRetryCount = 0;

	//# 0.25초 × 40회 = 10초. 이 시간 안에 바인딩되지 않으면 데이터 미설정으로 판단한다
	static constexpr int32 BindRetryMaxCount = 40;
};
