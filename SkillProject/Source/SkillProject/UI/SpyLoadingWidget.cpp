// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SpyLoadingWidget.h"

#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Manager/SpyLoadingSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyLoadingWidget)

void USpyLoadingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	//# 에러 UI 는 평소 숨김
	if (IsValid(ErrorText))
	{
		ErrorText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (IsValid(RetryButton))
	{
		RetryButton->SetVisibility(ESlateVisibility::Collapsed);
		RetryButton->OnClicked.AddDynamic(this, &USpyLoadingWidget::OnRetryClicked);
	}
	if (IsValid(EnterButton))
	{
		EnterButton->SetVisibility(ESlateVisibility::Collapsed);
		EnterButton->OnClicked.AddDynamic(this, &USpyLoadingWidget::OnEnterClicked);
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance == nullptr)
	{
		return;
	}

	USpyLoadingSubsystem* LoadingSubsystem = GameInstance->GetSubsystem<USpyLoadingSubsystem>();
	if (LoadingSubsystem == nullptr)
	{
		return;
	}

	ProgressChangedHandle = LoadingSubsystem->OnProgressChanged.AddUObject(this, &USpyLoadingWidget::HandleProgressChanged);
	ConnectFailedHandle = LoadingSubsystem->OnConnectionFailed.AddUObject(this, &USpyLoadingWidget::HandleConnectionFailed);
	ReadyToEnterHandle = LoadingSubsystem->OnReadyToEnter.AddUObject(this, &USpyLoadingWidget::HandleReadyToEnter);

	//# 구독 이전에 진행된 분량을 즉시 반영한다
	HandleProgressChanged(LoadingSubsystem->GetDisplayedProgress());
}

void USpyLoadingWidget::NativeDestruct()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USpyLoadingSubsystem* LoadingSubsystem = GameInstance->GetSubsystem<USpyLoadingSubsystem>())
		{
			LoadingSubsystem->OnProgressChanged.Remove(ProgressChangedHandle);
			LoadingSubsystem->OnConnectionFailed.Remove(ConnectFailedHandle);
			LoadingSubsystem->OnReadyToEnter.Remove(ReadyToEnterHandle);
		}
	}

	ProgressChangedHandle.Reset();
	ConnectFailedHandle.Reset();
	ReadyToEnterHandle.Reset();

	if (IsValid(RetryButton))
	{
		RetryButton->OnClicked.RemoveDynamic(this, &USpyLoadingWidget::OnRetryClicked);
	}
	if (IsValid(EnterButton))
	{
		EnterButton->OnClicked.RemoveDynamic(this, &USpyLoadingWidget::OnEnterClicked);
	}

	Super::NativeDestruct();
}

void USpyLoadingWidget::HandleProgressChanged(float InDisplayed)
{
	const float Clamped = FMath::Clamp(InDisplayed, 0.f, 1.f);

	if (IsValid(LoadingBar))
	{
		LoadingBar->SetPercent(Clamped);
	}

	if (IsValid(PercentText))
	{
		const int32 Percent = FMath::RoundToInt(Clamped * 100.f);

		//# 기획서 §5-1 — 3자리 zero-pad + % 기호 (예: 007%)
		PercentText->SetText(FText::FromString(FString::Printf(TEXT("%03d%%"), Percent)));
	}
}

void USpyLoadingWidget::HandleConnectionFailed(const FString& Reason)
{
	//# 실패 시 바·퍼센트를 숨긴다 — "실패했는데 100%" 모순 제거(사용자 결정).
	//# 타임아웃 시 DisplayedProgress 가 0.995(텍스트 100%)에 동결되므로 반드시 감춘다.
	if (IsValid(LoadingBar))
	{
		LoadingBar->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (IsValid(PercentText))
	{
		PercentText->SetVisibility(ESlateVisibility::Collapsed);
	}

	//# 에러 메시지 + 재시도 버튼 노출. 문구는 기획서 §9-3 확정값.
	if (IsValid(ErrorText))
	{
		ErrorText->SetText(FText::FromString(TEXT("서버에 연결하지 못했습니다")));
		ErrorText->SetVisibility(ESlateVisibility::Visible);
	}
	if (IsValid(RetryButton))
	{
		RetryButton->SetVisibility(ESlateVisibility::Visible);
	}
}

void USpyLoadingWidget::OnRetryClicked()
{
	//# 에러 UI 숨기고 바·퍼센트 복원 후 재접속
	if (IsValid(ErrorText))
	{
		ErrorText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (IsValid(RetryButton))
	{
		RetryButton->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (IsValid(LoadingBar))
	{
		LoadingBar->SetVisibility(ESlateVisibility::Visible);
	}
	if (IsValid(PercentText))
	{
		PercentText->SetVisibility(ESlateVisibility::Visible);
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USpyLoadingSubsystem* LoadingSubsystem = GameInstance->GetSubsystem<USpyLoadingSubsystem>())
		{
			LoadingSubsystem->RetryConnect();
		}
	}
}

void USpyLoadingWidget::HandleReadyToEnter()
{
	//# 로딩 완료 — 바·퍼센트를 숨기고 "접속" 버튼만 보인다
	if (IsValid(LoadingBar))
	{
		LoadingBar->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (IsValid(PercentText))
	{
		PercentText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (IsValid(EnterButton))
	{
		EnterButton->SetVisibility(ESlateVisibility::Visible);
	}

	//# 버튼을 누를 수 있도록 커서 표시 + UI 입력 모드 (패키지 standalone 은 기본이 GameOnly·커서숨김이라 클릭이 UI 로 안 감)
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (APlayerController* PC = GameInstance->GetFirstLocalPlayerController())
		{
			PC->bShowMouseCursor = true;
			FInputModeGameAndUI InputMode;
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			InputMode.SetHideCursorDuringCapture(false);
			PC->SetInputMode(InputMode);
		}
	}
}

void USpyLoadingWidget::OnEnterClicked()
{
	//# 버튼 숨기고 바·퍼센트 복원 — 맵 로딩바가 다시 차오른다
	if (IsValid(EnterButton))
	{
		EnterButton->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (IsValid(LoadingBar))
	{
		LoadingBar->SetVisibility(ESlateVisibility::Visible);
	}
	if (IsValid(PercentText))
	{
		PercentText->SetVisibility(ESlateVisibility::Visible);
	}

	//# 게임플레이 입력 모드로 복원 (DevMap 의 SpyPlayerController 도 GameOnly 로 재설정하지만, 전환 전 커서 깜빡임 방지)
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (APlayerController* PC = GameInstance->GetFirstLocalPlayerController())
		{
			PC->bShowMouseCursor = false;
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
		}
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USpyLoadingSubsystem* LoadingSubsystem = GameInstance->GetSubsystem<USpyLoadingSubsystem>())
		{
			LoadingSubsystem->EnterGameplay();
		}
	}
}
