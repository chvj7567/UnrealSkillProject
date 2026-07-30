// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SKUserWidget.h"
#include "SKOnlineTypes.h"

#include "SpySessionRowWidget.generated.h"

class UTextBlock;
class UButton;

//# 행 클릭 — 브라우저가 구독해 조인을 시작한다
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSpySessionRowClicked, int32);

//# 방 목록의 한 줄. 표시만 하고 조인은 브라우저가 수행한다.
UCLASS()
class SKILLPROJECT_API USpySessionRowWidget : public USKUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
	//# 표시 내용 갱신 — 브라우저가 검색 결과마다 호출한다
	void SetSessionInfo(const FSKSessionInfo& InInfo);

	//# 인원 표시 문자열 — "2 / 4"
	static FString MakePlayersText(int32 CurrentPlayers, int32 MaxPlayers);

	//# 핑 표시 문자열 — "30 ms". 음수/미측정은 "-- ms"
	static FString MakePingText(int32 PingMs);

public:
	FOnSpySessionRowClicked OnRowClicked;

protected:
	UFUNCTION()
	void OnJoinClicked();

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UTextBlock> RoomNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UTextBlock> PlayersText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = true))
	TObjectPtr<UTextBlock> PingText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UButton> JoinButton;

	//# 이 행이 가리키는 검색 결과 인덱스
	int32 SearchResultIndex = INDEX_NONE;
};
