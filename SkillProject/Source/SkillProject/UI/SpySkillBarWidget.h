// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UI/SpyUserWidget.h"

#include "SpySkillBarWidget.generated.h"

class UAbilitySystemComponent;
class UPanelWidget;
class USpySkillSlotWidget;

//# 데이터 구동 스킬바 — 슬롯 입력 태그 목록을 순회하며 슬롯 위젯을 동적 생성해 컨테이너에 채운다
UCLASS()
class SKILLPROJECT_API USpySkillBarWidget : public USpyUserWidget
{
	GENERATED_BODY()

public:
	//# 슬롯 순서(Skill_1~6)를 반환하는 순수 정적 함수 — 위젯 없이 테스트한다
	static TArray<FGameplayTag> BuildSlotInputTags();

	//# config 가 유효하고 Slots 가 비어있지 않으면 그 InputTag 순서, 아니면 BuildSlotInputTags() 폴백
	static TArray<FGameplayTag> ExtractSlotInputTags(const class USpySkillBarConfig* Config);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	//# ASC 준비 + 어빌리티 부여를 기다렸다가 슬롯을 1회 생성한다. 준비 전이면 false
	bool TryBuildSlots();

	void BuildSlots(UAbilitySystemComponent* ASC);

	//# SkillBarConfig 인스턴스→CDO 폴백 해석 (SlotWidgetClass 폴백과 동일 패턴). 미지정이면 nullptr
	const class USpySkillBarConfig* ResolveConfig() const;

	//# InputTag 의 실제 Enhanced Input 바인딩 키 표시문자를 유도한다.
	//# CharacterAssetData→InputConfig→InputAction→매핑키 체인. 어느 단계든 실패하면 SlotKeyHint 폴백
	FText ResolveSlotKeyHint(FGameplayTag InputTag) const;

protected:
	//# 슬롯로 인스턴스화할 위젯 클래스(WBP_SkillSlot). 캐릭터/스킬바 BP 기본값에서 지정
	UPROPERTY(EditDefaultsOnly, Category = "SkillBar")
	TSubclassOf<USpySkillSlotWidget> SlotWidgetClass;

	//# 슬롯 구성 DataAsset. WBP 기본값에서 지정. 미지정 시 하드코딩 Skill_1~6 폴백
	UPROPERTY(EditDefaultsOnly, Category = "SkillBar")
	TObjectPtr<class USpySkillBarConfig> SkillBarConfig;

	//# 슬롯을 담는 컨테이너(HorizontalBox 등). 아직 없을 수 있어 Optional
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> Panel_Slots;

	bool bSlotsBuilt = false;

	FTimerHandle BuildRetryTimerHandle;

	int32 BuildRetryCount = 0;

	//# 0.25초 × 40회 = 10초. SpyMainHUD 재시도 상한과 동일
	static constexpr int32 BuildRetryMaxCount = 40;
};
