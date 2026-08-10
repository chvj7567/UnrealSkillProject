// Fill out your copyright notice in the Description page of Project Settings.

#include "System/SpyMissionTargetRegistrySubsystem.h"
#include "GameFramework/Actor.h"

void USpyMissionTargetRegistrySubsystem::RegisterNPCLocation(int32 NPCId, AActor* InActor)
{
	if (InActor == nullptr)
		return;

	NPCLocations.Add(NPCId, InActor);
}

void USpyMissionTargetRegistrySubsystem::UnregisterNPCLocation(int32 NPCId, AActor* InActor)
{
	//# 다른 액터가 같은 키로 이미 재등록했다면 그 등록을 지우지 않는다(늦게 실행되는 EndPlay 방지)
	const TWeakObjectPtr<AActor>* Existing = NPCLocations.Find(NPCId);
	if (Existing == nullptr || Existing->Get() != InActor)
		return;

	NPCLocations.Remove(NPCId);
}

bool USpyMissionTargetRegistrySubsystem::FindNPCLocation(int32 NPCId, FVector& OutLocation) const
{
	const TWeakObjectPtr<AActor>* Found = NPCLocations.Find(NPCId);
	if (Found == nullptr || Found->IsValid() == false)
		return false;

	OutLocation = Found->Get()->GetActorLocation();

	return true;
}

void USpyMissionTargetRegistrySubsystem::RegisterMissionTargetLocation(FGameplayTag InTag, AActor* InActor)
{
	if (InActor == nullptr || InTag.IsValid() == false)
		return;

	MissionTargetLocations.Add(InTag, InActor);
}

void USpyMissionTargetRegistrySubsystem::UnregisterMissionTargetLocation(FGameplayTag InTag, AActor* InActor)
{
	const TWeakObjectPtr<AActor>* Existing = MissionTargetLocations.Find(InTag);
	if (Existing == nullptr || Existing->Get() != InActor)
		return;

	MissionTargetLocations.Remove(InTag);
}

bool USpyMissionTargetRegistrySubsystem::FindMissionTargetLocation(FGameplayTag InTag, FVector& OutLocation) const
{
	const TWeakObjectPtr<AActor>* Found = MissionTargetLocations.Find(InTag);
	if (Found == nullptr || Found->IsValid() == false)
		return false;

	OutLocation = Found->Get()->GetActorLocation();

	return true;
}

AActor* USpyMissionTargetRegistrySubsystem::FindNPCActor(int32 NPCId) const
{
	const TWeakObjectPtr<AActor>* Found = NPCLocations.Find(NPCId);
	if (Found == nullptr || Found->IsValid() == false)
		return nullptr;

	return Found->Get();
}

AActor* USpyMissionTargetRegistrySubsystem::FindMissionTargetActor(FGameplayTag InTag) const
{
	const TWeakObjectPtr<AActor>* Found = MissionTargetLocations.Find(InTag);
	if (Found == nullptr || Found->IsValid() == false)
		return nullptr;

	return Found->Get();
}
