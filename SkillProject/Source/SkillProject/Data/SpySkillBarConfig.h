// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "SpySkillBarConfig.generated.h"

class UTexture2D;

//# 스킬바 슬롯 1개 정의 — 배열 순서가 곧 화면 좌→우 슬롯 순서
USTRUCT(BlueprintType)
struct FSpySkillBarSlot
{
	GENERATED_BODY()

	//# 이 슬롯이 표시·활성·쿨다운 조회에 쓰는 입력태그
	UPROPERTY(EditAnywhere, meta = (Categories = "Input.Ability"))
	FGameplayTag InputTag;

	//# 슬롯 아이콘. 소프트 참조 — 슬롯 빌드 시 로드
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UTexture2D> Icon;
};

//# 스킬바 슬롯 구성 DataAsset. 에디터 Details 패널에서 편집한다
UCLASS()
class SKILLPROJECT_API USpySkillBarConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	//# 배열 순서 = 슬롯 순서
	UPROPERTY(EditDefaultsOnly, Category = "SkillBar")
	TArray<FSpySkillBarSlot> Slots;
};
