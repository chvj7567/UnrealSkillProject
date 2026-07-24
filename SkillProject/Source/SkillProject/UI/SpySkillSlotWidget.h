// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UI/SpyUserWidget.h"

#include "SpySkillSlotWidget.generated.h"

class UAbilitySystemComponent;
class UImage;
class UProgressBar;
class UTextBlock;

//# 스킬바 한 칸 — 키힌트/아이콘/쿨다운 스윕/마나코스트를 표시한다 (순수 로컬 표시, 상태 변경 없음)
UCLASS()
class SKILLPROJECT_API USpySkillSlotWidget : public USpyUserWidget
{
	GENERATED_BODY()

public:
	//# 슬롯 초기화 — 스킬바(Task 7)가 어빌리티 스펙/데이터를 해석해 넘긴다
	void Setup(UAbilitySystemComponent* InASC, FGameplayTag InInputTag, FGameplayTagContainer InCooldownTags, FText InKeyHint, float InManaCost);

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

	//# 쿨다운 어둠 스윕 — 세로 ProgressBar(하단 앵커, 어둠 fill). SetPercent(CooldownNormalized) 로
	//# 발동=가득(1)→준비=0 으로 높이가 아래로 줄어든다(기획 §6-1 하단 앵커 높이 스윕).
	//# fill 방향(하단 앵커·수직)은 WBP(Task 8) 설정이고 코드는 채움 비율만 구동한다
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> PB_Cooldown;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_KeyHint;

	//# 쿨다운 잔여 초(X.X)
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Cooldown;

	//# 마나 코스트 숫자
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Cost;
};
