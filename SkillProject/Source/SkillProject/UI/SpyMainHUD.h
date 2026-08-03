// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/SpyUserWidget.h"

#include "SpyMainHUD.generated.h"

class UProgressBar;
class UTextBlock;
class USpyLevelComponent;
class USpyCharacterAttributeSet;
class USpyNPCConfig;
class AActor;
struct FGameplayEffectSpec;

UCLASS()
class SKILLPROJECT_API USpyMainHUD : public USpyUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	//# 아직 WBP에 배치되지 않았을 수 있어 Optional로 바인딩한다
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> PB_Exp;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> PB_Mana;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> PB_HP;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Level;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_MissionName;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_MissionProgress;

protected:
	//# 로컬 폰의 LevelComponent를 찾아 구독한다. 아직 없으면 false
	bool TryBindLevelComponent();

	void UnbindLevelComponent();

	UFUNCTION()
	void HandleExperienceChanged(USpyLevelComponent* InLevelComponent, float InOldValue, float InNewValue);

	UFUNCTION()
	void HandleLevelChanged(USpyLevelComponent* InLevelComponent, int32 InOldLevel, int32 InNewLevel);

	void RefreshAll();

	//# 로컬 PlayerState 의 MissionComponent 를 찾아 구독한다. 아직 없으면 false
	bool TryBindMissionComponent();

	void UnbindMissionComponent();

	UFUNCTION()
	void HandleMissionProgressChanged(class USpyMissionComponent* InMissionComponent, int32 InMissionIndex, int32 InCount, int32 InTargetCount);

	UFUNCTION()
	void HandleAllMissionsCompleted(class USpyMissionComponent* InMissionComponent);

	void RefreshMission();

	//# 미수락 상태 미션 문구 조립 — NPCConfig 로 이름을 찾아 "{이름} 찾아가세요"를 만든다.
	//# NPCId 9999(시스템 퀘스트) 또는 조회 실패면 빈 텍스트
	FText ResolveNPCNameHintText(int32 InNPCId) const;

	//# 로컬 폰의 ASC 에서 AttributeSet 을 찾아 마나 변경을 구독한다. 아직 준비 안 됐으면 false
	bool TryBindManaAttribute();

	void UnbindManaAttribute();

	//# FSKAttributeEvent(비동적 6-param) 시그니처 — AddUObject 로 연결한다
	void HandleManaChanged(AActor* Instigator, AActor* Causer, const FGameplayEffectSpec* Spec, float Magnitude, float OldValue, float NewValue);

	void HandleMaxManaChanged(AActor* Instigator, AActor* Causer, const FGameplayEffectSpec* Spec, float Magnitude, float OldValue, float NewValue);

	void RefreshMana();

	//# Mana 와 동일 AttributeSet 를 공유하므로 Health 도 같은 바인딩 경로에서 구독한다(FSKAttributeEvent 6-param)
	void HandleHealthChanged(AActor* Instigator, AActor* Causer, const FGameplayEffectSpec* Spec, float Magnitude, float OldValue, float NewValue);

	void HandleMaxHealthChanged(AActor* Instigator, AActor* Causer, const FGameplayEffectSpec* Spec, float Magnitude, float OldValue, float NewValue);

	void RefreshHealth();

protected:
	UPROPERTY()
	TObjectPtr<USpyLevelComponent> BoundLevelComponent;

	UPROPERTY()
	TObjectPtr<const USpyCharacterAttributeSet> BoundManaSet;

	UPROPERTY()
	TObjectPtr<class USpyMissionComponent> BoundMissionComponent;

	//# 미수락 미션 문구의 NPC 이름 조회용. WBP_MainHUD 에서 DA_SpyNPCHub 를 지정한다
	UPROPERTY(EditDefaultsOnly, Category = "Mission")
	TObjectPtr<USpyNPCConfig> NPCConfig;

	FTimerHandle BindRetryTimerHandle;

	//# 바인딩 재시도 횟수. 상한(BindRetryMaxCount)에 도달하면 경고 1회 후 타이머를 멈춘다
	int32 BindRetryCount = 0;

	//# 0.25초 × 40회 = 10초. 이 시간 안에 바인딩되지 않으면 데이터 미설정으로 판단한다
	static constexpr int32 BindRetryMaxCount = 40;
};
