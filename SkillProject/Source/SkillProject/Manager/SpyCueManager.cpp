// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/SpyCueManager.h"
#include "GameplayTagsManager.h"
#include "AbilitySystem/SpyAbilitySystemGlobals.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "UObject/UObjectThreadContext.h"
#include "Misc/CoreDelegates.h"
#include "Manager/SpyAssetManager.h"
#include "Util/SpyGameplayTags.h"

USpyCueManager::USpyCueManager()
{
	UE_LOG(LogTemp, Warning, TEXT("USpyCueManager Create"));
}

USpyCueManager* USpyCueManager::Get()
{
	return Cast<USpyCueManager>(USpyAbilitySystemGlobals::Get().GetGameplayCueManager());
}

void USpyCueManager::OnCreated()
{
	Super::OnCreated();

	//# 기존 바인딩 제거
	UGameplayTagsManager::Get().OnGameplayTagLoadedDelegate.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
	FCoreUObjectDelegates::GetPostGarbageCollect().RemoveAll(this);

	//# 태그 로드 이벤트 바인딩
	UGameplayTagsManager::Get().OnGameplayTagLoadedDelegate.AddUObject(this,
		&USpyCueManager::OnGameplayTagLoaded);

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this,
		&USpyCueManager::HandlePostLoadMap);

	FCoreUObjectDelegates::GetPostGarbageCollect().AddUObject(this,
		&USpyCueManager::HandlePostGarbageCollect);

	USpyAssetManager& AssetManager = USpyAssetManager::Get();
	const USpyAssetData& AssetData = AssetManager.GetAssetData();

	TSoftClassPtr<AActor> SoftClass(AssetData.GetAssetPathByName(TEXT("SkillA_Static")));
	TSoftClassPtr<AActor> SoftClass2(AssetData.GetAssetPathByName(TEXT("SkillA_Actor")));

	TagToCueActorClass.Add(SpyGameplayTags::GameplayCue_A, SoftClass);
	TagToCueActorClass.Add(SpyGameplayTags::GameplayCue_B, SoftClass2);

	NotifyCueActorPool.Clear();
}

void USpyCueManager::OnEngineInitComplete()
{
	Super::OnEngineInitComplete();
}

bool USpyCueManager::ShouldSuppressGameplayCues(AActor* TargetActor)
{
	return IsGarbageCollecting();
}

void USpyCueManager::HandleGameplayCue(AActor* TargetActor, FGameplayTag GameplayCueTag, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters, EGameplayCueExecutionOptions Options)
{
	Super::HandleGameplayCue(TargetActor, GameplayCueTag, EventType, Parameters, Options);

	TSoftClassPtr<AActor>* CueClassPtr = TagToCueActorClass.Find(GameplayCueTag);
	if (CueClassPtr == nullptr || TargetActor == nullptr)
		return;

	if (EventType == EGameplayCueEvent::WhileActive || EventType == EGameplayCueEvent::Executed)
		return;

	NotifyCueActorPool.Initialize(TargetActor->GetWorld());
	AActor* CueActor = NotifyCueActorPool.RentCueActor(CueClassPtr->Get(), GameplayCueTag, TargetActor->GetActorTransform());
	if (CueActor == nullptr)
		return;

	switch (EventType)
	{
	case EGameplayCueEvent::OnActive:
	{
		CueActor->SetActorLocation(TargetActor->GetActorLocation());
		CueActor->SetActorHiddenInGame(false);
	}
		break;
	case EGameplayCueEvent::Removed:
	{
		NotifyCueActorPool.ReturnCueActor(CueActor);
	}
		break;
	default:
		break;
	}
}

void USpyCueManager::OnGameplayTagLoaded(const FGameplayTag& Tag)
{
	FScopeLock Lock(&LoadedGameplayTagsToProcessCS);
	LoadedGameplayTagsToProcess.Add(Tag);

	// 태그 처리 (Lyra는 Task로 비동기 처리)
	if (IsGarbageCollecting() == false)
	{
		ProcessLoadedTags();
	}
}

void USpyCueManager::HandlePostLoadMap(UWorld*)
{
	ProcessLoadedTags();
}

void USpyCueManager::HandlePostGarbageCollect()
{
	ProcessLoadedTags();
}

void USpyCueManager::ProcessLoadedTags()
{
	FScopeLock Lock(&LoadedGameplayTagsToProcessCS);

	for (const FGameplayTag& Tag : LoadedGameplayTagsToProcess)
	{
		if (TSoftClassPtr<AActor>* FoundClass = TagToCueActorClass.Find(Tag))
		{
			FoundClass->LoadSynchronous();
		}
	}

	LoadedGameplayTagsToProcess.Empty();
}