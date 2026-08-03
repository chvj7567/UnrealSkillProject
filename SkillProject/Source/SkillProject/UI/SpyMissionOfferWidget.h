// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/SpyUserWidget.h"

#include "SpyMissionOfferWidget.generated.h"

class UTextBlock;
class UButton;
class ISpyInteractionHost;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMissionCardChoice);

//# 미션 수락/거절 카드. Offer 전용 — Report 는 카드가 없으므로 겸용하지 않는다.
UCLASS()
class SKILLPROJECT_API USpyMissionOfferWidget : public USpyUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void ShowMission(FText InTitle, FText InDescription);

	UPROPERTY(BlueprintAssignable)
	FOnMissionCardChoice OnAcceptClicked;

	UPROPERTY(BlueprintAssignable)
	FOnMissionCardChoice OnDeclineClicked;

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleAcceptButtonClicked();

	UFUNCTION()
	void HandleDeclineButtonClicked();

private:
	//# 소유 폰 → ISpyCharacterRoot → ISpyInteractionHost 로 1회 조회한다 (§8 캐싱과 동일 원칙)
	TScriptInterface<ISpyInteractionHost> ResolveInteractionHost() const;

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Title;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Description;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Accept;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Decline;
};
