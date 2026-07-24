// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SKUserWidget.h"

#include "SpyLoadingWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UButton;

//# 로딩 화면 뷰 — 서브시스템 진행률을 구독해 바/퍼센트만 갱신한다. 접속 실패 시 에러 UI 노출.
UCLASS()
class SKILLPROJECT_API USpyLoadingWidget : public USKUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	//# 진행률(0~1) 반영
	void HandleProgressChanged(float InDisplayed);

	//# 접속 실패 — 에러 메시지 + 재시도 버튼 노출
	void HandleConnectionFailed(const FString& Reason);

	//# 로딩 완료 — "접속" 버튼 노출
	void HandleReadyToEnter();

	//# 재시도 버튼 클릭
	UFUNCTION()
	void OnRetryClicked();

	//# 접속 버튼 클릭
	UFUNCTION()
	void OnEnterClicked();

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UProgressBar> LoadingBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UTextBlock> PercentText;

	//# 접속 실패 메시지 — 평소 숨김(BindWidgetOptional 로 안전)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = true))
	TObjectPtr<UTextBlock> ErrorText;

	//# 재시도 버튼 — 평소 숨김
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = true))
	TObjectPtr<UButton> RetryButton;

	//# 로딩 완료 후 게임 진입 버튼 — 평소 숨김
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = true))
	TObjectPtr<UButton> EnterButton;

	FDelegateHandle ProgressChangedHandle;
	FDelegateHandle ConnectFailedHandle;
	FDelegateHandle ReadyToEnterHandle;
};
