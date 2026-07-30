// Fill out your copyright notice in the Description page of Project Settings.

#include "SKOnlineTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKOnlineTypes)

FSKSessionInfo FSKSessionInfo::Make(
	const FString& InRoomName,
	const FString& InHostName,
	int32 InMaxConnections,
	int32 InOpenConnections,
	int32 InPingMs,
	int32 InIndex)
{
	FSKSessionInfo Info;

	//# 방 이름이 비면 목록에 빈 줄이 뜨므로 호스트명으로 대체한다
	Info.RoomName = InRoomName.IsEmpty() ? InHostName : InRoomName;
	Info.HostName = InHostName;

	//# 백엔드가 비정상 값을 줘도 표시가 깨지지 않게 바닥을 잡는다
	Info.MaxPlayers = FMath::Max(InMaxConnections, 0);
	Info.CurrentPlayers = FMath::Clamp(Info.MaxPlayers - InOpenConnections, 0, Info.MaxPlayers);
	Info.PingMs = FMath::Max(InPingMs, 0);
	Info.SearchResultIndex = InIndex;

	return Info;
}
