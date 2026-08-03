// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/SpyUserWidget.h"

#include "SpyInteractPromptWidget.generated.h"

class UTextBlock;
class ISpyInteractionHost;

//# 근접 상호작용 프롬프트. NPC/아이템 등 트리거 대상 타입을 모르는 일반화된 위젯 —
//# 외부는 ShowPrompt(동사 텍스트) 하나만 호출한다 (cpp-style §9-2)
UCLASS()
class SKILLPROJECT_API USpyInteractPromptWidget : public USpyUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void ShowPrompt(FText InVerb);

protected:
	virtual void NativeConstruct() override;

private:
	//# 소유 폰 → ISpyCharacterRoot → ISpyInteractionHost 로 1회 조회한다 (§8 캐싱과 동일 원칙)
	TScriptInterface<ISpyInteractionHost> ResolveInteractionHost() const;

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Verb;
};
