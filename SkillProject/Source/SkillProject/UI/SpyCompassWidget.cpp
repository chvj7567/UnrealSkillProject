// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SpyCompassWidget.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "UI/SpyHUDMath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyCompassWidget)

//# ESpyCardinal enum 식별자와 글자 그대로 일치하는 영문 약자 라벨(기획 §5)
static FText CardinalToText(ESpyCardinal Cardinal)
{
	switch (Cardinal)
	{
	case ESpyCardinal::N:
		return FText::FromString(TEXT("N"));
	case ESpyCardinal::NE:
		return FText::FromString(TEXT("NE"));
	case ESpyCardinal::E:
		return FText::FromString(TEXT("E"));
	case ESpyCardinal::SE:
		return FText::FromString(TEXT("SE"));
	case ESpyCardinal::S:
		return FText::FromString(TEXT("S"));
	case ESpyCardinal::SW:
		return FText::FromString(TEXT("SW"));
	case ESpyCardinal::W:
		return FText::FromString(TEXT("W"));
	case ESpyCardinal::NW:
		return FText::FromString(TEXT("NW"));
	default:
		return FText::GetEmpty();
	}
}

void USpyCompassWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	APlayerController* OwningController = GetOwningPlayer();
	if (OwningController == nullptr)
		return;

	//# 카메라가 아닌 컨트롤 회전 = 플레이어 조준/이동 의도 방향(기획 §5)
	const float Yaw = static_cast<float>(OwningController->GetControlRotation().Yaw);
	const ESpyCardinal Cardinal = SpyHUDMath::HeadingToCardinal(Yaw);

	if (Txt_Heading)
	{
		Txt_Heading->SetText(CardinalToText(Cardinal));
	}
}
