// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SpySkillSlotWidget.h"
#include "AbilitySystemComponent.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "GameplayEffect.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Attribute/SKAttributeSet.h"
#include "UI/SpyHUDMath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpySkillSlotWidget)

void USpySkillSlotWidget::Setup(UAbilitySystemComponent* InASC, FGameplayTag InInputTag, FGameplayTagContainer InCooldownTags, FText InKeyHint, float InManaCost, UTexture2D* InIcon)
{
	AbilitySystemComponent = InASC;
	InputTag = InInputTag;
	CooldownTags = InCooldownTags;
	ManaCost = InManaCost;

	//# 아이콘이 있으면 브러시로 설정(플레이스홀더 더미색 대체). null 이면 기존 브러시 유지.
	//# 마나부족 적색 틴트(SetColorAndOpacity)는 브러시 위 곱연산이라 그대로 동작한다
	if (InIcon != nullptr && Img_Icon != nullptr)
	{
		Img_Icon->SetBrushFromTexture(InIcon);
	}

	//# 쿨다운 레이디얼 MID 준비(브러시 머티리얼 기반, 경로 하드코딩 없음).
	//# 아이콘이 있으면 오버레이도 그 텍스처를 어둡게 쓴다
	if (Img_Cooldown != nullptr)
	{
		CooldownMID = Img_Cooldown->GetDynamicMaterial();
		if (CooldownMID != nullptr && InIcon != nullptr)
		{
			CooldownMID->SetTextureParameterValue(TEXT("Icon"), InIcon);
		}
	}

	if (Txt_KeyHint)
	{
		Txt_KeyHint->SetText(InKeyHint);
	}

	if (Txt_Cost)
	{
		//# 코스트 0(Parry 등)은 숨긴다(기획 §6-1)
		if (ManaCost > 0.f)
		{
			Txt_Cost->SetText(FText::AsNumber(FMath::RoundToInt(ManaCost)));
			Txt_Cost->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Txt_Cost->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void USpySkillSlotWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (ASC == nullptr)
		return;

	//# 쿨다운 태그 컨테이너로 활성 쿨다운 GE 의 잔여/지속을 조회한다.
	//# GetActiveEffectsTimeRemainingAndDuration 은 Key=잔여, Value=지속 순서(엔진 UGameplayAbility 경로와 동일)
	float Remaining = 0.f;
	float Duration = 0.f;
	if (CooldownTags.IsEmpty() == false)
	{
		const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(CooldownTags);
		const TArray<TPair<float, float>> TimeRemainingAndDuration = ASC->GetActiveEffectsTimeRemainingAndDuration(Query);
		for (const TPair<float, float>& Entry : TimeRemainingAndDuration)
		{
			//# 여러 쿨다운이 겹치면 가장 오래 남은 것을 표시한다
			if (Entry.Key > Remaining)
			{
				Remaining = Entry.Key;
				Duration = Entry.Value;
			}
		}
	}

	const float Normalized = SpyHUDMath::CooldownNormalized(Remaining, Duration);
	const bool bOnCooldown = Normalized > 0.f;

	//# 레이디얼 언와인드 — Percent=CooldownNormalized(1=방금발동→0=준비). 각도 마스크는 머티리얼이 처리
	if (Img_Cooldown)
	{
		Img_Cooldown->SetVisibility(bOnCooldown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (CooldownMID != nullptr)
		{
			CooldownMID->SetScalarParameterValue(TEXT("Percent"), Normalized);
		}
	}

	//# 잔여 초 — X.X, 준비 시 숨김
	if (Txt_Cooldown)
	{
		if (bOnCooldown)
		{
			Txt_Cooldown->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), Remaining)));
			Txt_Cooldown->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Txt_Cooldown->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	//# 마나 부족 적색 틴트 — 쿨다운이 우선(§6-3)이라 쿨다운 중이 아닐 때만 판정한다
	if (Img_Icon)
	{
		const float CurMana = ASC->GetNumericAttribute(USKAttributeSet::GetManaAttribute());
		const bool bManaShort = (bOnCooldown == false) && (ManaCost > 0.f) && (CurMana < ManaCost);
		Img_Icon->SetColorAndOpacity(bManaShort ? FLinearColor(1.f, 0.3f, 0.3f, 1.f) : FLinearColor::White);
	}
}
