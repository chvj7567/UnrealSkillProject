// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"

#include "SpyMissionTargetRegistrySubsystem.generated.h"

class AActor;

//# 미션 목표 좌표의 순수 로컬 조회 서비스(design §5-4). 액터를 약참조로만 저장하고
//# 조회 시점에 GetActorLocation() 을 읽는다 — 스냅샷 캐싱이 아니라 재배치가 자동 반영된다.
//# 서버 레플리케이션 없음 — 각 프로세스가 자기 월드의 액터를 로컬로 자기등록한다(§5-4).
UCLASS()
class SKILLPROJECT_API USpyMissionTargetRegistrySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	//# Dialogue 미션 키 공간 — NPCId. Dialogue 6행의 MatchTag 는 전부 공용이라 구분자가 못 된다(§5-4)
	void RegisterNPCLocation(int32 NPCId, AActor* InActor);
	void UnregisterNPCLocation(int32 NPCId, AActor* InActor);
	bool FindNPCLocation(int32 NPCId, FVector& OutLocation) const;

	//# Gameplay(구역형)·Interact 키 공간 — MatchTag/MissionEventTag. Gameplay 6종은 leaf 태그로 고유하다(§5-4)
	void RegisterMissionTargetLocation(FGameplayTag InTag, AActor* InActor);
	void UnregisterMissionTargetLocation(FGameplayTag InTag, AActor* InActor);
	bool FindMissionTargetLocation(FGameplayTag InTag, FVector& OutLocation) const;

private:
	UPROPERTY(Transient)
	TMap<int32, TWeakObjectPtr<AActor>> NPCLocations;

	UPROPERTY(Transient)
	TMap<FGameplayTag, TWeakObjectPtr<AActor>> MissionTargetLocations;
};
