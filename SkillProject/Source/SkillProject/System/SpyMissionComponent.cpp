// Fill out your copyright notice in the Description page of Project Settings.


#include "System/SpyMissionComponent.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystem/Effect/SpyGE_ExperienceGain.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "Data/SpyMissionConfig.h"
#include "Util/SpyGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyMissionComponent)

USpyMissionComponent::USpyMissionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
}

void USpyMissionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//# 자기 진행도만 보면 되므로 소유 클라이언트에만 복제한다
	DOREPLIFETIME_CONDITION(USpyMissionComponent, MissionState, COND_OwnerOnly);
}

void USpyMissionComponent::InitializeByAbilitySystem(USpyAbilitySystemComponent* InASC)
{
	if (InASC == nullptr)
		return;

	//# 보상 GE 적용 대상. 진행 신호는 각 신호원이 AddProgress 로 직접 밀어 넣으므로
	//# 여기서 어빌리티 콜백을 구독하지 않는다
	AbilitySystemComponent = InASC;
}

void USpyMissionComponent::UnInitializeByAbilitySystem()
{
	AbilitySystemComponent = nullptr;
}

void USpyMissionComponent::OnUnregister()
{
	UnInitializeByAbilitySystem();

	Super::OnUnregister();
}

int32 USpyMissionComponent::GetTargetCount() const
{
	if (MissionConfig == nullptr)
		return 0;

	const FSpyMissionEntry* Entry = MissionConfig->GetMission(MissionState.MissionIndex);

	return (Entry ? Entry->TargetCount : 0);
}

FText USpyMissionComponent::GetDisplayName() const
{
	if (MissionConfig == nullptr)
		return FText::GetEmpty();

	const FSpyMissionEntry* Entry = MissionConfig->GetMission(MissionState.MissionIndex);

	return (Entry ? Entry->DisplayName : FText::GetEmpty());
}

bool USpyMissionComponent::IsAllCompleted() const
{
	if (MissionConfig == nullptr)
		return false;

	return (MissionConfig->IsValidMissionIndex(MissionState.MissionIndex) == false);
}

void USpyMissionComponent::OnRep_MissionState()
{
	//# 클라이언트 표시 갱신
	OnMissionProgressChanged.Broadcast(this, MissionState.MissionIndex, MissionState.Count, GetTargetCount());

	if (IsAllCompleted())
	{
		OnAllMissionsCompleted.Broadcast(this);
	}
}

void USpyMissionComponent::AddProgress(FGameplayTag InEventTag, int32 InAmount)
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr || Owner->HasAuthority() == false)
		return;

	if (MissionConfig == nullptr)
	{
		//# 무증상 실패 1순위 — PlayerState BP 의 MissionComponent 에 Config 를 지정하지 않으면
		//# 미션이 전혀 진행되지 않는다 (mission-system.md §6-5 조건 2)
		if (bWarnedMissingConfig == false)
		{
			bWarnedMissingConfig = true;

			UE_LOG(LogTemp, Warning, TEXT("# [SpyMissionComponent] MissionConfig가 지정되지 않아 미션이 진행되지 않습니다: %s"), *GetNameSafe(Owner));
		}

		return;
	}

	//# 처리 중에 들어온 이벤트는 버리지 않고 큐에 쌓는다.
	//# (보상 XP → 레벨업 → 레벨 신호 → AddProgress 경로가 실재한다)
	if (bProcessingProgress)
	{
		FSpyMissionPendingEvent Pending;
		Pending.EventTag = InEventTag;
		Pending.Amount = InAmount;
		PendingEvents.Add(Pending);

		return;
	}

	TGuardValue<bool> ReentryGuard(bProcessingProgress, true);

	ProcessProgress(InEventTag, InAmount);

	//# 가드 안에서 쌓인 이벤트를 순차 처리한다
	while (PendingEvents.Num() > 0)
	{
		const FSpyMissionPendingEvent Next = PendingEvents[0];
		PendingEvents.RemoveAt(0);

		ProcessProgress(Next.EventTag, Next.Amount);
	}
}

void USpyMissionComponent::ProcessProgress(FGameplayTag InEventTag, int32 InAmount)
{
	if (MissionConfig == nullptr)
		return;

	const FSpyMissionProgressResult Result = MissionConfig->ResolveMissionProgress(
		MissionState.MissionIndex, MissionState.Count, InEventTag, InAmount);

	const bool bChanged = (Result.MissionIndex != MissionState.MissionIndex) || (Result.Count != MissionState.Count);
	if (bChanged == false)
		return;

	const int32 CompletedIndex = MissionState.MissionIndex;

	MissionState.MissionIndex = Result.MissionIndex;
	MissionState.Count = Result.Count;

	//# 서버 브로드캐스트 (클라이언트는 OnRep_MissionState 에서 처리)
	OnMissionProgressChanged.Broadcast(this, MissionState.MissionIndex, MissionState.Count, GetTargetCount());

	if (Result.bCompletedNow)
	{
		OnMissionCompleted.Broadcast(this, CompletedIndex);

		GrantReward(CompletedIndex);

		UE_LOG(LogTemp, Log, TEXT("# [SpyMissionComponent] Mission %d completed by %s"), CompletedIndex, *GetNameSafe(GetOwner()));
	}

	if (Result.bAllCompleted)
	{
		OnAllMissionsCompleted.Broadcast(this);
	}
}

void USpyMissionComponent::GrantReward(int32 InCompletedIndex)
{
	if (MissionConfig == nullptr)
		return;

	if (AbilitySystemComponent == nullptr)
	{
		//# 진행도는 오르는데 보상 XP만 안 들어오는 상태라 원인 추적이 어렵다 — 반드시 남긴다
		if (bWarnedMissingAbilitySystem == false)
		{
			bWarnedMissingAbilitySystem = true;

			UE_LOG(LogTemp, Warning, TEXT("# [SpyMissionComponent] ASC가 연결되지 않아 미션 보상을 지급하지 못했습니다 (InitializeByAbilitySystem 확인): %s"), *GetNameSafe(GetOwner()));
		}

		return;
	}

	const FSpyMissionEntry* Entry = MissionConfig->GetMission(InCompletedIndex);
	if (Entry == nullptr || Entry->ExperienceReward <= 0.f)
		return;

	//# 기존 경험치 이펙트를 재사용한다 (새 GE 를 만들지 않는다)
	FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
	ContextHandle.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
		USpyGE_ExperienceGain::StaticClass(), 1.f, ContextHandle);
	if (SpecHandle.IsValid() == false)
		return;

	SpecHandle.Data->SetSetByCallerMagnitude(SpyGameplayTags::Data_Experience_Gain, Entry->ExperienceReward);
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
}
