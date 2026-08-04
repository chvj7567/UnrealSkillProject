// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "UI/SpyUserWidget.h"

#include "SpyDialogueWidget.generated.h"

class UTextBlock;
class UInputAction;
class UEnhancedInputLocalPlayerSubsystem;
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

	//# 현재 바인딩된 키 목록으로 "<Key> 계속" 힌트 텍스트를 구성한다. 매핑이 없으면 중립 폴백.
	//# 위젯 상태와 분리된 순수 함수 — 테스트 용이성을 위해 static 으로 노출한다 (§8)
	static FText BuildContinueHintText(const TArray<FKey>& MappedKeys);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void HandleDialogueLineChanged(FText InNPCName, FText InLine);

	//# EnhancedInput 이 매핑을 재구성할 때마다(리바인딩·컨텍스트 교체 등) 호출된다
	UFUNCTION()
	void HandleControlMappingsRebuilt();

private:
	//# 소유 폰 → ISpyCharacterRoot → ISpyInteractionHost 로 1회 조회한다 (§8 캐싱과 동일 원칙)
	TScriptInterface<ISpyInteractionHost> ResolveInteractionHost() const;

	//# 소유 폰 → ISpyCharacterRoot 로 1회 조회한다 — CharacterAssetData/InputConfig 체인은
	//# 루트(ASpyCharacter)가 대신 파헤친다 (cpp-style §13, 하위 컴포넌트 직접 탐색 금지)
	const UInputAction* ResolveInteractInputAction() const;

	//# CachedInteractInputAction 기준으로 현재 바인딩된 키를 조회해 힌트 텍스트를 갱신한다
	void RefreshContinueHintText();

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_NPCName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Line;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_ContinueHint;

private:
	//# NativeConstruct 시점 1회 캐싱 (§8) — 대화 진행 입력 액션 자체는 위젯 생존 동안 바뀌지 않는다
	UPROPERTY(Transient)
	TObjectPtr<const UInputAction> CachedInteractInputAction;

	//# ControlMappingsRebuiltDelegate 구독 해제용. TWeakObjectPtr — 서브시스템이 위젯보다 먼저
	//# 파괴될 일은 없지만 방어적으로 IsValid 체크 후 해제한다
	TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> CachedInputSubsystem;
};
