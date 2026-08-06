// Fill out your copyright notice in the Description page of Project Settings.


#include "System/SpyMissionComponent.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystem/Effect/SpyGE_ExperienceGain.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "Character/SpyCharacterAttributeSet.h"
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

	const FSpyMissionRow* Entry = MissionConfig->GetMission(MissionState.MissionIndex);

	return (Entry ? Entry->TargetCount : 0);
}

FText USpyMissionComponent::GetDisplayName() const
{
	if (MissionConfig == nullptr)
		return FText::GetEmpty();

	const FSpyMissionRow* Entry = MissionConfig->GetMission(MissionState.MissionIndex);

	return (Entry ? Entry->DisplayName : FText::GetEmpty());
}

int32 USpyMissionComponent::GetCurrentNPCId() const
{
	if (MissionConfig == nullptr)
		return FSpyMissionRow::NoNPCId;

	const FSpyMissionRow* Entry = MissionConfig->GetMission(MissionState.MissionIndex);

	return (Entry ? Entry->NPCId : FSpyMissionRow::NoNPCId);
}

bool USpyMissionComponent::IsAllCompleted() const
{
	if (MissionConfig == nullptr)
		return false;

	return (MissionConfig->IsValidMissionIndex(MissionState.MissionIndex) == false);
}

void USpyMissionComponent::OnRep_MissionState(FSpyMissionState OldMissionState)
{
	//# 클라이언트 표시 갱신
	OnMissionProgressChanged.Broadcast(this, MissionState.MissionIndex, MissionState.Count, GetTargetCount());

	//# 원격 클라이언트는 서버 권한 경로(AcceptCurrentMission/ProcessProgress)를 타지 않으므로
	//# 상태-diff 로 같은 신호를 재현한다. ⚠ Completed 를 Accepted 보다 반드시 먼저 판정 — design §2-3 참조.
	if (OldMissionState.MissionIndex != MissionState.MissionIndex)
		OnMissionCompleted.Broadcast(this, OldMissionState.MissionIndex);

	const bool bAcceptedTransition = (OldMissionState.bAccepted == false && MissionState.bAccepted == true) ||
		(OldMissionState.MissionIndex != MissionState.MissionIndex && MissionState.bAccepted == true);

	if (bAcceptedTransition)
		OnMissionAccepted.Broadcast(this, MissionState.MissionIndex);

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

	//# NPC 대화로 수락하기 전에는 어떤 진행 신호도 반영하지 않는다.
	if (MissionState.bAccepted)
		ProcessProgress(InEventTag, InAmount);

	//# 드레인도 매 반복 "지금" 상태로 재검증한다 — 큐잉 당시엔 수락 상태였어도
	//# ProcessProgress가 그 사이 미션을 전진시켜 bAccepted가 false로 바뀌었을 수 있다
	//# (spec §5-2-1 — 보상 GE가 유발한 재진입 레벨업이 아직 미수락인 다음 미션을 완료시키는 경합 수정)
	while (PendingEvents.Num() > 0)
	{
		const FSpyMissionPendingEvent Next = PendingEvents[0];
		PendingEvents.RemoveAt(0);

		if (MissionState.bAccepted)
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

	//# ResolveMissionProgress 는 bCompletedNow 일 때만 MissionIndex 를 전진시킨다(SpyMissionConfig.cpp) —
	//# 별도로 인덱스 변경 여부를 재계산하지 않고 이 신호 하나로 "다음 미션 전환" 전체를 판단한다.
	if (Result.bCompletedNow)
	{
		//# 상태 대입 전에 실행 — 완료 인덱스는 "이전 값"이어야 한다(design §2-3, OnRep 계약과 동일).
		GrantReward(MissionState.MissionIndex); //# Gameplay 타입은 MissionReward 행이 없어 조용히 no-op
		OnMissionCompleted.Broadcast(this, MissionState.MissionIndex);

		//# 새로 진입할 미션이 Dialogue/Interact 타입이면 자동 수락 — 카드를 보여줄 NPC가 없다
		const FSpyMissionRow* NewEntry = GetMissionEntry(Result.MissionIndex);
		MissionState.bAccepted = (NewEntry != nullptr &&
			(NewEntry->MissionType == ESpyMissionType::Dialogue || NewEntry->MissionType == ESpyMissionType::Interact));

		if (MissionState.bAccepted)
			OnMissionAccepted.Broadcast(this, Result.MissionIndex);
	}

	MissionState.MissionIndex = Result.MissionIndex;
	MissionState.Count = Result.Count;

	OnMissionProgressChanged.Broadcast(this, MissionState.MissionIndex, MissionState.Count, GetTargetCount());

	if (IsAllCompleted())
		OnAllMissionsCompleted.Broadcast(this);
}

bool USpyMissionComponent::AcceptCurrentMission()
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr || Owner->HasAuthority() == false)
		return false;

	if (IsAllCompleted())
		return false;

	if (MissionState.bAccepted)
		return true; //# 멱등

	MissionState.bAccepted = true;

	//# 수락 사실 자체를 즉시 알린다 — HUD는 이 델리게이트로만 갱신되므로 진행값 불변이어도 필요
	OnMissionProgressChanged.Broadcast(this, MissionState.MissionIndex, MissionState.Count, GetTargetCount());

	//# 수락 사실 전용 신호 — 진행값이 바뀔 때마다 도는 OnMissionProgressChanged 와 달리
	//# "새 미션을 따라가기 시작해야 한다"를 아는 구독자(내비게이션 등)를 위한 전용 델리게이트
	OnMissionAccepted.Broadcast(this, MissionState.MissionIndex);

	//# 이미 조건을 만족한 상태로 수락하는 경우를 위한 재평가.
	//# 레벨(Threshold) 미션은 승급 이벤트가 이 시점 이전에 이미 지나갔을 수 있다(spec §2-6/§5-6).
	//# 현재는 레벨 미션이 유일한 Threshold 사례이므로 여기서 직접 재주입한다.
	const FSpyMissionRow* CurrentEntry = GetMissionEntry(MissionState.MissionIndex);
	if (CurrentEntry != nullptr && CurrentEntry->MatchTag == SpyGameplayTags::Event_Mission_Level && AbilitySystemComponent != nullptr)
	{
		const float CurrentLevel = AbilitySystemComponent->GetNumericAttribute(USpyCharacterAttributeSet::GetLevelAttribute());
		AddProgress(SpyGameplayTags::Event_Mission_Level, FMath::RoundToInt(CurrentLevel));
	}

	return true;
}

const FSpyMissionRow* USpyMissionComponent::GetMissionEntry(int32 InMissionId) const
{
	return (MissionConfig != nullptr ? MissionConfig->GetMission(InMissionId) : nullptr);
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

	const FSpyMissionRow* Entry = MissionConfig->GetMission(InCompletedIndex);
	if (Entry == nullptr)
		return;

	const float Reward = MissionConfig->GetMissionReward(InCompletedIndex);

	if (Reward <= 0.f)
	{
		//# Gameplay 타입은 보상이 없는 게 정상이다. Dialogue/Interact 타입인데 0이면
		//# MissionReward 행을 빠뜨린 에디터 데이터 실수일 수밖에 없다 — 경고로 구분한다
		if (Entry->MissionType == ESpyMissionType::Dialogue || Entry->MissionType == ESpyMissionType::Interact)
		{
			UE_LOG(LogTemp, Warning, TEXT("# [SpyMissionComponent] Dialogue/Interact 미션 %d 의 MissionReward 행이 없습니다 (데이터 누락 의심): %s"), InCompletedIndex, *GetNameSafe(GetOwner()));
		}

		return;
	}

	//# 기존 경험치 이펙트를 재사용한다 (새 GE 를 만들지 않는다)
	FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
	ContextHandle.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
		USpyGE_ExperienceGain::StaticClass(), 1.f, ContextHandle);
	if (SpecHandle.IsValid() == false)
		return;

	SpecHandle.Data->SetSetByCallerMagnitude(SpyGameplayTags::Data_Experience_Gain, Reward);
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
}
