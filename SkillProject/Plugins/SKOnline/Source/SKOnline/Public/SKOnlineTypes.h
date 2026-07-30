// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "SKOnlineTypes.generated.h"

//# 진행 중인 세션 작업 — 동시에 하나만 허용한다(중복 입력 가드)
UENUM(BlueprintType)
enum class ESKSessionOp : uint8
{
	None,
	Hosting,
	Finding,
	Joining,
	Destroying
};

//# 세션 작업 실패 사유 — 사용자 문구는 게임 모듈이 정한다(플러그인은 사유 코드만)
UENUM(BlueprintType)
enum class ESKSessionError : uint8
{
	None,
	NoOnlineSubsystem,
	Busy,
	CreateFailed,
	FindFailed,
	JoinFailed,
	InvalidIndex,
	ResolveFailed,
	DestroyFailed
};

//# 방 목록 표시용 struct — UI 가 OSS 타입(FOnlineSessionSearchResult)에 직접 물리지 않게 하는 차단막
USTRUCT(BlueprintType)
struct SKONLINE_API FSKSessionInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "SKOnline")
	FString RoomName;

	UPROPERTY(BlueprintReadOnly, Category = "SKOnline")
	FString HostName;

	UPROPERTY(BlueprintReadOnly, Category = "SKOnline")
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "SKOnline")
	int32 MaxPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "SKOnline")
	int32 PingMs = 0;

	//# 조인 시 원본 검색 결과를 되찾는 인덱스
	UPROPERTY(BlueprintReadOnly, Category = "SKOnline")
	int32 SearchResultIndex = INDEX_NONE;

	//# 순수 변환 — OSS 타입에 의존하지 않아 자동화 테스트가 가능하다
	static FSKSessionInfo Make(
		const FString& InRoomName,
		const FString& InHostName,
		int32 InMaxConnections,
		int32 InOpenConnections,
		int32 InPingMs,
		int32 InIndex);
};
