// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "SKOnlineSettings.generated.h"

namespace SKOnlineKeys
{
	//# 세션 커스텀 세팅 키 — 방 이름을 광고에 실어 목록에 표시한다.
	//# Steam AppID 480 은 전 세계 공용이라 이 키로 남의 테스트 방을 걸러낼 수 있다.
	inline const FName RoomName = TEXT("SK_ROOMNAME");
}

//# 세션 백엔드 프로필 — Null↔Steam 전환을 ini 한 곳에서 끝내기 위한 이음매.
//# UDeveloperSettings 를 고른 이유는 spec §4-4 참고.
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "SK Online"))
class SKONLINE_API USKOnlineSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	//# 프로젝트 설정 > 플러그인 > SK Online 으로 노출
	virtual FName GetCategoryName() const override;

public:
	//# LAN 브로드캐스트 검색 — OSS Null 은 true 여야 방이 보인다. Steam 은 false
	UPROPERTY(config, EditAnywhere, Category = "Backend")
	bool bIsLanMatch = true;

	//# Steam 전용 — presence 기반 검색. Null 에서 켜면 검색이 깨진다
	UPROPERTY(config, EditAnywhere, Category = "Backend")
	bool bUsesPresence = false;

	//# Steam 전용 — 로비 기반 검색
	UPROPERTY(config, EditAnywhere, Category = "Backend")
	bool bUseLobbiesIfAvailable = false;

	//# 세션을 목록에 광고할지. 끄면 방이 뜨지 않는다
	UPROPERTY(config, EditAnywhere, Category = "Session")
	bool bShouldAdvertise = true;

	//# 게임 시작 후 난입 허용
	UPROPERTY(config, EditAnywhere, Category = "Session")
	bool bAllowJoinInProgress = true;

	//# 방 최대 인원(호스트 포함)
	UPROPERTY(config, EditAnywhere, Category = "Session", meta = (ClampMin = "2"))
	int32 MaxPlayers = 4;

	//# 한 번 검색에서 받아올 최대 방 수
	UPROPERTY(config, EditAnywhere, Category = "Session", meta = (ClampMin = "1"))
	int32 MaxSearchResults = 20;

	//# 방 이름 자동 생성 포맷. {0} 에 호스트명이 들어간다 (옵션 입력 화면이 없으므로 필수)
	UPROPERTY(config, EditAnywhere, Category = "Session")
	FString DefaultRoomNameFormat = TEXT("{0}의 방");
};
