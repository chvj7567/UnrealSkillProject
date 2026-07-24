// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/SpyUserWidget.h"

#include "SpyCompassWidget.generated.h"

class UTextBlock;

//# 로컬 컨트롤 회전 yaw 를 매 tick 읽어 8방위 라벨을 갱신하는 나침반 위젯 (순수 로컬 표시)
UCLASS()
class SKILLPROJECT_API USpyCompassWidget : public USpyUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	//# 현재 방위 라벨(N/NE/E/...). WBP 에 아직 없을 수 있어 Optional
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Heading;
};
