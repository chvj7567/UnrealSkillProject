// Fill out your copyright notice in the Description page of Project Settings.


#include "SKCueManager.h"
#include "SKAbilitySystemGlobals.h"
#include "Engine/AssetManager.h"
#include "GameplayCueSet.h"
#include "GameplayTagsManager.h"
#include "UObject/UObjectThreadContext.h"
#include "Async/Async.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKCueManager)

USKCueManager::USKCueManager()
{
}

USKCueManager* USKCueManager::Get()
{
	return Cast<USKCueManager>(USKAbilitySystemGlobals::Get().GetGameplayCueManager());
}

void USKCueManager::OnCreated()
{
    Super::OnCreated();

    UpdateDelayLoadDelegateListeners();
}

bool USKCueManager::ShouldAsyncLoadRuntimeObjectLibraries() const
{
	//# 클라이언트만 비동기 로드 사용(서버는 사용 X)
	return IsRunningDedicatedServer() == false;
}

bool USKCueManager::ShouldSyncLoadMissingGameplayCues() const
{
	//# false로 지정하여 동기 로드 사용 거부(Hitch 방지)
	return false;
}

bool USKCueManager::ShouldAsyncLoadMissingGameplayCues() const
{
	//# 클라이언트만 비동기 로드 사용(서버는 사용 X)
	return IsRunningDedicatedServer() == false;
}

void USKCueManager::UpdateDelayLoadDelegateListeners()
{
	//# 기존 등록 제거 (중복 방지)
	UGameplayTagsManager::Get().OnGameplayTagLoadedDelegate.RemoveAll(this);
	FCoreUObjectDelegates::GetPostGarbageCollect().RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	//# 에디터에서는 이런 복잡한 지연 로딩 리스너가 필요 없음
//#if WITH_EDITOR
//	if (GIsEditor)
//		return;
//#endif

	//# 서버도 필요 없음
	if (IsRunningDedicatedServer())
		return;

	//# 클라이언트 실제 게임에서만 참조 시 로딩을 위해 델리게이트 연결
	UGameplayTagsManager::Get().OnGameplayTagLoadedDelegate.AddUObject(this, &ThisClass::OnGameplayTagLoaded);
	FCoreUObjectDelegates::GetPostGarbageCollect().AddUObject(this, &ThisClass::HandlePostGarbageCollect);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::HandlePostLoadMap);
}

void USKCueManager::OnGameplayTagLoaded(const FGameplayTag& Tag)
{
    FScopeLock ScopeLock(&LoadedGameplayTagsToProcessCS);

    //# 현재 태그를 로드 중인 컨텍스트(에셋) 파악
    FUObjectSerializeContext* LoadContext = FUObjectThreadContext::Get().GetSerializeContext();
    UObject* OwningObject = LoadContext ? LoadContext->SerializedObject : nullptr;

    bool bStartTask = LoadedGameplayTagsToProcess.Num() == 0;
    LoadedGameplayTagsToProcess.Emplace(Tag, OwningObject);

    //# 첫 번째 태그가 들어왔을 때만 메인 스레드 Task 생성
    if (bStartTask)
    {
        //# 메인 스레드에서 ProcessLoadedTags 호출
        AsyncTask(ENamedThreads::GameThread, [this]()
            {
                if (GIsRunning)
                {
                    if (IsGarbageCollecting())
                    {
                        bProcessLoadedTagsAfterGC = true;
                    }
                    else
                    {
                        ProcessLoadedTags();
                    }
                }
            });
    }
}

void USKCueManager::HandlePostGarbageCollect()
{
	//# GC 이후 태그 로딩 재시작
    if (bProcessLoadedTagsAfterGC)
    {
        ProcessLoadedTags();
    }

    bProcessLoadedTagsAfterGC = false;
}

void USKCueManager::HandlePostLoadMap(UWorld* NewWorld)
{
    //# 큐들을 엔진 관리 목록에서 제거 요청
    if (RuntimeGameplayCueObjectLibrary.CueSet)
    {
        for (UClass* CueClass : AlwaysLoadedCues)
        {
            RuntimeGameplayCueObjectLibrary.CueSet->RemoveLoadedClass(CueClass);
        }

        for (UClass* CueClass : PreloadedCues)
        {
            RuntimeGameplayCueObjectLibrary.CueSet->RemoveLoadedClass(CueClass);
        }
    }

    //# 프리로드된 큐들은 정리
    for (auto CueIt = PreloadedCues.CreateIterator(); CueIt; ++CueIt)
    {
        TSet<FObjectKey>& ReferencerSet = PreloadedCueReferencers.FindChecked(*CueIt);
        for (auto RefIt = ReferencerSet.CreateIterator(); RefIt; ++RefIt)
        {
            //# 맵이 바뀌면서 객체가 파괴되었다면 명단에서 제거
            if (RefIt->ResolveObjectPtr() == nullptr)
            {
                RefIt.RemoveCurrent();
            }
        }

        //# 참조자가 없다면 제거
        if (ReferencerSet.Num() == 0)
        {
            PreloadedCueReferencers.Remove(*CueIt);
            CueIt.RemoveCurrent();
        }
    }
}

void USKCueManager::ProcessLoadedTags()
{
    TArray<FLoadedTag> TagsToProcess;
    {
        FScopeLock ScopeLock(&LoadedGameplayTagsToProcessCS);
        
        //# 복사 후 비우기
        TagsToProcess = MoveTemp(LoadedGameplayTagsToProcess);
    }

    if (GIsRunning)
    {
        if (RuntimeGameplayCueObjectLibrary.CueSet)
        {
            for (const FLoadedTag& TagData : TagsToProcess)
            {
                // 이 태그가 매니저가 관리하는 큐 세트에 포함되어 있는지 확인
                if (RuntimeGameplayCueObjectLibrary.CueSet->GameplayCueDataMap.Contains(TagData.Tag))
                {
                    if (TagData.OwningObject.IsValid())
                    {
                        // 포함되어 있다면 실제 로딩 함수 호출
                        ProcessTagToPreload(TagData.Tag, TagData.OwningObject.Get());
                    }
                }
            }
        }
    }
}

void USKCueManager::ProcessTagToPreload(const FGameplayTag& Tag, UObject* OwningObject)
{
    //# 에디터에서는 프리로딩을 스킵함
//#if WITH_EDITOR
//    if (GIsEditor)
//        return;
//#endif

    //# 서버는 로드할 필요 없음
    if (IsRunningDedicatedServer())
        return;

    check(RuntimeGameplayCueObjectLibrary.CueSet);

    //# 인덱스 맵에서 태그에 해당하는 데이터 위치(Index)를 찾음
    int32* DataIdx = RuntimeGameplayCueObjectLibrary.CueSet->GameplayCueDataMap.Find(Tag);
    if (DataIdx && RuntimeGameplayCueObjectLibrary.CueSet->GameplayCueData.IsValidIndex(*DataIdx))
    {
        const FGameplayCueNotifyData& CueData = RuntimeGameplayCueObjectLibrary.CueSet->GameplayCueData[*DataIdx];

        //# 이미 메모리에 로드되어 있는지 먼저 확인
        UClass* LoadedGameplayCueClass = FindObject<UClass>(nullptr, *CueData.GameplayCueNotifyObj.ToString());
        if (LoadedGameplayCueClass)
        {
            //# 이미 있다면 등록만 하고 끝
            RegisterPreloadedCue(LoadedGameplayCueClass, OwningObject);
        }
        else
        {
            //# 없다면 비동기 로딩 시작
            bool bAlwaysLoadedCue = (OwningObject == nullptr);
            TWeakObjectPtr<UObject> WeakOwner = OwningObject;

            FStreamableManager& Streamable = UAssetManager::GetStreamableManager();

            Streamable.RequestAsyncLoad(
                CueData.GameplayCueNotifyObj,
                FStreamableDelegate::CreateUObject(this, &ThisClass::OnPreloadCueComplete, CueData.GameplayCueNotifyObj, WeakOwner, bAlwaysLoadedCue),
                FStreamableManager::DefaultAsyncLoadPriority,
                false,
                false,
                TEXT("SKGameplayCueManager")
            );
        }

        UE_LOG(LogTemp, Log, TEXT("# [SKCueManager] Start Preload Tag: %s, Owner: %s"), *Tag.ToString(), OwningObject ? *OwningObject->GetName() : TEXT("None"));
    }
}

void USKCueManager::OnPreloadCueComplete(FSoftObjectPath BundleAssetPath, TWeakObjectPtr<UObject> OwningObject, bool bAlwaysLoadedCue)
{
    if (bAlwaysLoadedCue || OwningObject.IsValid())
    {
        if (UClass* LoadedGameplayCueClass = Cast<UClass>(BundleAssetPath.ResolveObject()))
        {
            RegisterPreloadedCue(LoadedGameplayCueClass, OwningObject.Get());
        }
    }
}

void USKCueManager::RegisterPreloadedCue(UClass* LoadedGameplayCueClass, UObject* OwningObject)
{
    check(LoadedGameplayCueClass);

    //# 주인이 없이 호출 가능한 자주 쓰이는 큐
    const bool bAlwaysLoadedCue = (OwningObject == nullptr);
    if (bAlwaysLoadedCue)
    {
        AlwaysLoadedCues.Add(LoadedGameplayCueClass);
        PreloadedCues.Remove(LoadedGameplayCueClass);
        PreloadedCueReferencers.Remove(LoadedGameplayCueClass);
    }
    //# 자기 참조 확인
    //# CDO 참조 확인
    //# 자주 쓰이는 큐 확인
    else if ((OwningObject != LoadedGameplayCueClass) && (OwningObject != LoadedGameplayCueClass->GetDefaultObject()) &&
        AlwaysLoadedCues.Contains(LoadedGameplayCueClass) == false)
    {
        PreloadedCues.Add(LoadedGameplayCueClass);
        TSet<FObjectKey>& ReferencerSet = PreloadedCueReferencers.FindOrAdd(LoadedGameplayCueClass);
        ReferencerSet.Add(OwningObject);
    }

    UE_LOG(LogTemp, Warning, TEXT("# [SKCueManager] Register Class: %s (Type: %s)"), *LoadedGameplayCueClass->GetName(), OwningObject ? TEXT("Preloaded") : TEXT("AlwaysLoaded"));
}
