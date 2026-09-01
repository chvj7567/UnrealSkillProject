// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SpyQuitConfirmWidget.h"
#include "Components/Button.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Manager/SpyUIManager.h"
#include "System/SpyPlayerController.h"
#include "Util/DefineEnum.h"

void USpyQuitConfirmWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(Btn_Yes))
	{
		Btn_Yes->OnClicked.AddDynamic(this, &USpyQuitConfirmWidget::HandleYesClicked);
	}

	if (IsValid(Btn_No))
	{
		Btn_No->OnClicked.AddDynamic(this, &USpyQuitConfirmWidget::HandleNoClicked);
	}
}

void USpyQuitConfirmWidget::HandleYesClicked()
{
	//# 로컬 클라이언트 자신의 게임 종료 — 서버 권한 체크 불필요 (unreal-infra §2)
	UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
}

void USpyQuitConfirmWidget::HandleNoClicked()
{
	USpyUIManager* UIMgr = USpyUIManager::Get(this);
	if (UIMgr == nullptr)
		return;

	UIMgr->CloseSpyUI(ESpyUIType::QuitConfirm);

	//# 팝업 스스로 닫히는 경로(No 버튼)도 ESC 재입력 경로와 동일하게 커서를 정리해야 한다
	if (ASpyPlayerController* PC = GetOwningPlayer<ASpyPlayerController>())
	{
		PC->HandleQuitConfirmClosed();
	}
}
