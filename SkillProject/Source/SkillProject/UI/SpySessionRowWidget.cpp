// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SpySessionRowWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpySessionRowWidget)

void USpySessionRowWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(JoinButton))
	{
		JoinButton->OnClicked.AddDynamic(this, &USpySessionRowWidget::OnJoinClicked);
	}
}

void USpySessionRowWidget::NativeDestruct()
{
	if (IsValid(JoinButton))
	{
		JoinButton->OnClicked.RemoveDynamic(this, &USpySessionRowWidget::OnJoinClicked);
	}

	OnRowClicked.Clear();

	Super::NativeDestruct();
}

FString USpySessionRowWidget::MakePlayersText(int32 CurrentPlayers, int32 MaxPlayers)
{
	return FString::Printf(TEXT("%d / %d"), FMath::Max(CurrentPlayers, 0), FMath::Max(MaxPlayers, 0));
}

FString USpySessionRowWidget::MakePingText(int32 PingMs)
{
	//# 아직 측정되지 않은 핑은 0/음수로 오므로 숫자 대신 자리표시자를 쓴다
	if (PingMs <= 0)
		return TEXT("-- ms");

	return FString::Printf(TEXT("%d ms"), PingMs);
}

void USpySessionRowWidget::SetSessionInfo(const FSKSessionInfo& InInfo)
{
	SearchResultIndex = InInfo.SearchResultIndex;

	if (IsValid(RoomNameText))
	{
		RoomNameText->SetText(FText::FromString(InInfo.RoomName));
	}

	if (IsValid(PlayersText))
	{
		PlayersText->SetText(FText::FromString(MakePlayersText(InInfo.CurrentPlayers, InInfo.MaxPlayers)));
	}

	if (IsValid(PingText))
	{
		PingText->SetText(FText::FromString(MakePingText(InInfo.PingMs)));
	}

	//# 꽉 찬 방은 누를 수 없게 한다
	if (IsValid(JoinButton))
	{
		const bool bFull = (InInfo.MaxPlayers > 0) && (InInfo.CurrentPlayers >= InInfo.MaxPlayers);
		JoinButton->SetIsEnabled(bFull == false);
	}
}

void USpySessionRowWidget::OnJoinClicked()
{
	if (SearchResultIndex == INDEX_NONE)
		return;

	OnRowClicked.Broadcast(SearchResultIndex);
}
