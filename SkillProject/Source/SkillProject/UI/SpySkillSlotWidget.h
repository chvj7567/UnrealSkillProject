// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UI/SpyUserWidget.h"

#include "SpySkillSlotWidget.generated.h"

class UAbilitySystemComponent;
class UImage;
class UMaterialInstanceDynamic;
class UTextBlock;
class UTexture2D;

//# 스킬바 한 칸 — 키힌트/아이콘/쿨다운 스윕/마나코스트를 표시한다 (순수 로컬 표시, 상태 변경 없음)
UCLASS()
class SKILLPROJECT_API USpySkillSlotWidget : public USpyUserWidget
{
	GENERATED_BODY()

public:
	//# 슬롯 초기화 — 스킬바가 어빌리티 스펙/데이터·config 아이콘을 해석해 넘긴다
	void Setup(UAbilitySystemComponent* InASC, FGameplayTag InInputTag, FGameplayTagContainer InCooldownTags, FText InKeyHint, float InManaCost, UTexture2D* InIcon);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	//# 이 슬롯이 대응하는 입력 태그 (스킬바가 슬롯을 구분하는 키)
	FGameplayTag InputTag;

	//# 쿨다운 GE 를 조회할 태그 컨테이너(Cooldown_Skill_Action_*)
	FGameplayTagContainer CooldownTags;

	//# 어빌리티별 마나 코스트(표시·마나 부족 판정용)
	float ManaCost = 0.f;

protected:
	//# 아이콘(플레이스홀더). 마나 부족 시 적색 틴트 대상
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UImage> Img_Icon;

	//# 쿨다운 레이디얼 오버레이(M_RadialCooldown 머티리얼). 세로 ProgressBar 대체 —
	//# tick 마다 MID 의 Percent(=CooldownNormalized)로 원형 언와인드를 그린다
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UImage> Img_Cooldown;

	//# Img_Cooldown 브러시 머티리얼의 동적 인스턴스 — Percent/Icon 파라미터 세팅용
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> CooldownMID;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_KeyHint;

	//# 쿨다운 잔여 초(X.X)
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Cooldown;

	//# 마나 코스트 숫자
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Cost;
};
