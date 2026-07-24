// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "SpyLoadingConfig.generated.h"

//# 로딩 씬 설정 — 전환 대상 맵·최소 표시 시간·1단계 가중치
UCLASS()
class SKILLPROJECT_API USpyLoadingConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	//# 로딩 완료 후 전환할 게임플레이 맵
	UPROPERTY(EditDefaultsOnly, Category = "Loading")
	TSoftObjectPtr<UWorld> GameplayMap;

	//# 로딩 화면 최소 표시 시간(초). 0 이하이면 시간 클램프를 건너뛴다
	UPROPERTY(EditDefaultsOnly, Category = "Loading")
	float MinDisplaySeconds = 2.5f;

	//# 1단계(에셋 프리로드) 가중치. 2단계(맵 스트리밍) 가중치는 1 - AssetPhaseWeight
	UPROPERTY(EditDefaultsOnly, Category = "Loading", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AssetPhaseWeight = 0.99f;

	//# 자동 접속 대상 서버 주소 (예: "127.0.0.1:7777"). 비어 있으면 오프라인 폴백(NM_Standalone 에서만 OpenLevel)
	UPROPERTY(EditDefaultsOnly, Category = "Loading|Network")
	FString ServerAddress;

	//# 접속 타임아웃(초). 이 시간 안에 도착하지 못하면 실패로 간주. 0 이하이면 타임아웃 없음
	UPROPERTY(EditDefaultsOnly, Category = "Loading|Network")
	float ConnectTimeoutSeconds = 15.f;
};
