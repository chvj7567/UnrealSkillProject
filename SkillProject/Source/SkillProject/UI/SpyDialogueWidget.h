// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/SpyUserWidget.h"

#include "SpyDialogueWidget.generated.h"

class UTextBlock;
class ISpyInteractionHost;

//# NPC 대사 한 줄만 표시하는 대화창. 내부 텍스트 위젯은 캡슐화하고
//# 외부는 ShowLine 하나만 호출한다 (cpp-style §9-2)
UCLASS()
class SKILLPROJECT_API USpyDialogueWidget : public USpyUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void ShowLine(FText InNPCName, FText InLine);

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleDialogueLineChanged(FText InNPCName, FText InLine);

private:
	//# 소유 폰 → ISpyCharacterRoot → ISpyInteractionHost 로 1회 조회한다 (§8 캐싱과 동일 원칙)
	TScriptInterface<ISpyInteractionHost> ResolveInteractionHost() const;

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_NPCName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Line;
};
