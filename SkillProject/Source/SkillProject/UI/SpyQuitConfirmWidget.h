// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/SpyUserWidget.h"

#include "SpyQuitConfirmWidget.generated.h"

class UButton;

//# ESC 종료 확인 팝업. 로컬 클라이언트 전용 UI 액션(자기 자신의 게임 종료)이라 서버 권한 체크 대상 아님 (unreal-infra §2)
UCLASS()
class SKILLPROJECT_API USpyQuitConfirmWidget : public USpyUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleYesClicked();

	UFUNCTION()
	void HandleNoClicked();

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Yes;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_No;
};
