// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueManager.h"
#include "Cue/SKCueActorPool.h"

#include "SKCueManager.generated.h"

class UWorld;

UCLASS()
class SKGAS_API USKCueManager : public UGameplayCueManager
{
	GENERATED_BODY()
	
public:
	USKCueManager();

	static USKCueManager* Get();

public:
	//~UGameplayCueManager interface
	virtual void OnCreated() override;
	virtual bool ShouldAsyncLoadRuntimeObjectLibraries() const override;
	virtual bool ShouldSyncLoadMissingGameplayCues() const override;
	virtual bool ShouldAsyncLoadMissingGameplayCues() const override;
	//~End of UGameplayCueManager interface

protected:
	void UpdateDelayLoadDelegateListeners();

	void OnGameplayTagLoaded(const FGameplayTag& Tag);
	void HandlePostGarbageCollect();
	void HandlePostLoadMap(UWorld* NewWorld);
	void ProcessLoadedTags();

	//# 실제 프리로딩 핵심 로직
	void ProcessTagToPreload(const FGameplayTag& Tag, UObject* OwningObject);

	//# 로딩 완료 후 호출될 콜백
	void OnPreloadCueComplete(FSoftObjectPath BundleAssetPath, TWeakObjectPtr<UObject> OwningObject, bool bAlwaysLoadedCue);

	//# 로드된 큐를 관리 리스트에 등록 (중복 방지 및 참조 유지)
	void RegisterPreloadedCue(UClass* LoadedGameplayCueClass, UObject* OwningObject);

private:
	struct FLoadedTag {
		FGameplayTag Tag;
		TWeakObjectPtr<UObject> OwningObject;
		FLoadedTag(FGameplayTag InTag, UObject* InObj) : Tag(InTag), OwningObject(InObj) {}
	};

	FCriticalSection LoadedGameplayTagsToProcessCS;

	//# 실제 로드된 태그 배열
	TArray<FLoadedTag> LoadedGameplayTagsToProcess;
	bool bProcessLoadedTagsAfterGC = false;

	UPROPERTY(transient)
	TSet<TObjectPtr<UClass>> PreloadedCues;
	TMap<FObjectKey, TSet<FObjectKey>> PreloadedCueReferencers;

	//# 자주 쓰이는 큐들은 GC 대상이 되지 않게 참조 유지
	UPROPERTY(transient)
	TSet<TObjectPtr<UClass>> AlwaysLoadedCues;
};
