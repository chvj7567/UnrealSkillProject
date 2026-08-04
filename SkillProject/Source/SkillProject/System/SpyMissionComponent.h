// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/GameFrameworkComponent.h"
#include "GameplayTagContainer.h"

#include "SpyMissionComponent.generated.h"

class USpyAbilitySystemComponent;
class USpyMissionConfig;
struct FSpyMissionRow;
struct FSpyMission_TargetLocationRow;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FSpyMission_ProgressChanged, USpyMissionComponent*, MissionComponent, int32, MissionIndex, int32, Count, int32, TargetCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSpyMission_Completed, USpyMissionComponent*, MissionComponent, int32, CompletedIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSpyMission_AllCompleted, USpyMissionComponent*, MissionComponent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSpyMission_Accepted, USpyMissionComponent*, MissionComponent, int32, MissionIndex);

//# 재진입 가드에 걸린 진행 이벤트를 보관한다 (버리지 않고 순차 처리)
USTRUCT()
struct FSpyMissionPendingEvent
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FGameplayTag EventTag;

	UPROPERTY()
	int32 Amount = 0;
};

//# 복제 단위 — 인덱스와 카운트를 한 구조체로 묶어 둘 중 하나만 바뀌어도 OnRep 이 발화하게 한다
USTRUCT()
struct FSpyMissionState
{
	GENERATED_BODY()

public:
	//# 1-based. 플레이어가 시작할 때 배정되는 첫 미션의 MissionId(=1)
	UPROPERTY()
	int32 MissionIndex = 1;

	UPROPERTY()
	int32 Count = 0;

	//# 현재 미션을 수락했는가. false면 AddProgress가 진행 신호를 전부 무시한다.
	//# Dialogue 타입 미션은 인덱스 진입과 동시에 자동으로 true가 된다 (ProcessProgress)
	UPROPERTY()
	bool bAccepted = false;
};

UCLASS()
class SKILLPROJECT_API USpyMissionComponent : public UGameFrameworkComponent
{
	GENERATED_BODY()

public:
	USpyMissionComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure)
	static USpyMissionComponent* FindMissionComponent(const AActor* Actor)
	{
		return (Actor ? Actor->FindComponentByClass<USpyMissionComponent>() : nullptr);
	}

	void InitializeByAbilitySystem(USpyAbilitySystemComponent* InASC);
	void UnInitializeByAbilitySystem();

	//# 모든 진행 신호의 단일 진입점. 서버 권한에서만 동작한다.
	//# 신호원은 전부 "실제로 수행된 지점"에서 명시적으로 호출한다 —
	//# 어빌리티 활성화 콜백 같은 범용 훅은 쓰지 않는다 (활성화와 실행을 구분할 수 없어
	//# 벽이 없는데 키만 눌러도 카운트되는 문제가 있었다)
	void AddProgress(FGameplayTag InEventTag, int32 InAmount);

	//# NPC 상호작용(서버에서 거리·인덱스 검증 완료)에서만 호출한다.
	//# 이미 수락된 상태에서 재호출하면 멱등하게 true를 반환한다.
	//# 수락 직후 현재 미션이 레벨 기반 Threshold 미션이면 레벨을 즉시 재평가한다 (spec §5-6).
	bool AcceptCurrentMission();

	UFUNCTION(BlueprintPure)
	bool IsCurrentAccepted() const { return MissionState.bAccepted; }

	//# 인덱스로 임의 미션 엔트리를 조회한다. MissionConfig가 없거나 범위 밖이면 nullptr.
	//# NPC의 RequestInteract가 카드(Offer)용 Description을 채울 때 쓴다.
	const FSpyMissionRow* GetMissionEntry(int32 InMissionId) const;

	//# 인덱스로 목표 좌표를 조회한다(§14-1 선택적 관계). 없으면 nullptr — 데이터는 MissionConfig
	//# 가 소유하고 이 컴포넌트는 위임만 한다(cpp-style §8), GetMissionEntry 와 동일한 패턴
	const FSpyMission_TargetLocationRow* GetMissionTargetLocation(int32 InMissionId) const;

	UFUNCTION(BlueprintPure)
	int32 GetMissionIndex() const { return MissionState.MissionIndex; }

	UFUNCTION(BlueprintPure)
	int32 GetCount() const { return MissionState.Count; }

	UFUNCTION(BlueprintPure)
	int32 GetTargetCount() const;

	UFUNCTION(BlueprintPure)
	FText GetDisplayName() const;

	//# 현재 미션의 담당 NPCId(범위 밖/미설정이면 9999). 이름 조회는 하지 않는다 —
	//# 이 컴포넌트는 NPC 시스템을 모른다(cpp-style §8). 이름 조합은 HUD 몫이다.
	UFUNCTION(BlueprintPure)
	int32 GetCurrentNPCId() const;

	UFUNCTION(BlueprintPure)
	bool IsAllCompleted() const;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnUnregister() override;

	//# 실제 판정 1회. AddProgress 가 가드와 큐를 관리하고 이 함수를 호출한다
	void ProcessProgress(FGameplayTag InEventTag, int32 InAmount);

	//# 완료 보상 지급 (서버)
	void GrantReward(int32 InCompletedIndex);

	UFUNCTION()
	void OnRep_MissionState(FSpyMissionState OldMissionState);

public:
	UPROPERTY(BlueprintAssignable)
	FSpyMission_ProgressChanged OnMissionProgressChanged;

	UPROPERTY(BlueprintAssignable)
	FSpyMission_Completed OnMissionCompleted;

	UPROPERTY(BlueprintAssignable)
	FSpyMission_AllCompleted OnAllMissionsCompleted;

	UPROPERTY(BlueprintAssignable)
	FSpyMission_Accepted OnMissionAccepted;

protected:
	//# PlayerState BP 기본값에서 지정한다
	UPROPERTY(EditDefaultsOnly, Category = "Mission")
	TObjectPtr<USpyMissionConfig> MissionConfig;

	UPROPERTY(ReplicatedUsing = OnRep_MissionState)
	FSpyMissionState MissionState;

	UPROPERTY()
	TObjectPtr<USpyAbilitySystemComponent> AbilitySystemComponent;

	//# 재진입 가드 — 보상 XP 가 레벨업을 유발하고 그것이 다시 AddProgress 를 부르는 경로가 실재한다
	bool bProcessingProgress = false;

	//# 설정 누락 경고는 진행 신호마다 반복 호출되므로 1회만 남긴다 (로그 스팸 방지)
	bool bWarnedMissingConfig = false;
	bool bWarnedMissingAbilitySystem = false;

	//# 가드에 걸린 이벤트 (유실 방지)
	UPROPERTY()
	TArray<FSpyMissionPendingEvent> PendingEvents;
};
