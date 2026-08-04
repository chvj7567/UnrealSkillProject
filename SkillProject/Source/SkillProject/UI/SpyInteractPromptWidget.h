// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/SpyUserWidget.h"

#include "SpyInteractPromptWidget.generated.h"

class UTextBlock;
class UInputAction;
class UEnhancedInputLocalPlayerSubsystem;
class ISpyInteractionHost;
class ISpyCharacterRoot;

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
	virtual void NativeDestruct() override;

	//# EnhancedInput 이 매핑을 재구성할 때마다(리바인딩·컨텍스트 교체 등) 호출된다
	UFUNCTION()
	void HandleControlMappingsRebuilt();

private:
	//# 소유 폰 → ISpyCharacterRoot 를 1회 조회한다. InteractionHost/입력 액션 조회가 모두
	//# 같은 폰 참조를 공유하도록 NativeConstruct 에서 한 번만 캐스팅한다 (§8 캐싱과 동일 원칙)
	ISpyCharacterRoot* ResolveCharacterRoot() const;

	//# CachedInteractInputAction 기준으로 현재 바인딩된 키를 조회해 키캡 텍스트를 갱신한다
	void RefreshKeyCapText();

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Verb;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_KeyCap;

private:
	//# NativeConstruct 시점 1회 캐싱 (§8) — 대화 진행 입력 액션 자체는 위젯 생존 동안 바뀌지 않는다
	UPROPERTY(Transient)
	TObjectPtr<const UInputAction> CachedInteractInputAction;

	//# ControlMappingsRebuiltDelegate 구독 해제용. TWeakObjectPtr — 서브시스템이 위젯보다 먼저
	//# 파괴될 일은 없지만 방어적으로 IsValid 체크 후 해제한다
	TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> CachedInputSubsystem;
};
