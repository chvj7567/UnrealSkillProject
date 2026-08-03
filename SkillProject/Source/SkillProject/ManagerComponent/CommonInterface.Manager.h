// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NPC/CommonInterface.NPC.h"

#include "CommonInterface.Manager.generated.h"

//# 아래 3개 USTRUCT 과 3개 델리게이트는 SpyParkourManagerComponent.h /
//# SpyGrappleTargetingComponent.h 에서 이동해 왔다 — 인터페이스가 참조하므로 순환을 피한다.

USTRUCT(BlueprintType)
struct FClimbWallData
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere)
	FVector NormalVector;
	UPROPERTY(VisibleAnywhere)
	FVector HitVector;

	FClimbWallData()
		: NormalVector(FVector::ZeroVector), HitVector(FVector::ZeroVector)
	{
	}

	void Clear()
	{
		NormalVector = FVector::ZeroVector;
		HitVector = FVector::ZeroVector;
	}
};

USTRUCT(BlueprintType)
struct FClimbData
{

	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DistanceOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HandOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FootOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Speed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CheckHangUpHeight;

	FClimbData()
		: DistanceOffset(0.0f), HandOffset(0.0f), FootOffset(0.0f), Speed(0.0f), CheckHangUpHeight(0.0f)
	{
	}

	void Clear()
	{
		DistanceOffset = 0.0f;
		HandOffset = 0.0f;
		FootOffset = 0.0f;
		Speed = 0.0f;
		CheckHangUpHeight = 0.0f;
	}
};

USTRUCT(BlueprintType)
struct FMotionWarpingData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector StartLoc;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FRotator StartRot;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector EndLoc;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FRotator EndRot;

	FMotionWarpingData()
		: StartLoc(FVector::ZeroVector), StartRot(FRotator::ZeroRotator), EndLoc(FVector::ZeroVector), EndRot(FRotator::ZeroRotator)
	{
	}

	void Clear()
	{
		StartLoc = FVector::ZeroVector;
		StartRot = FRotator::ZeroRotator;
		EndLoc = FVector::ZeroVector;
		EndRot = FRotator::ZeroRotator;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSyncMotionWarpingDataDelegate, FMotionWarpingData, InVaultData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSyncClilmbDataDelegate, const FClimbData&, InClimbData, const FClimbWallData&, InClimbWallData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGrappleTargetChanged, AActor*, NewTarget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDialogueLineChanged, FText, NPCName, FText, Line);

//# 파쿠르 프로토콜 제공자 — USpyParkourManagerComponent 가 구현한다.
//# GA 는 이 인터페이스로만 접근하고 구체 컴포넌트 타입을 알지 않는다.
UINTERFACE(MinimalAPI)
class USpyParkourHost : public UInterface
{
	GENERATED_BODY()
};

class ISpyParkourHost
{
	GENERATED_BODY()

public:
	virtual bool CanVaultAction() = 0;
	virtual void SetVaultMotionWarpingData() = 0;
	virtual void SetHangUpMotionWarpingData(const FVector& HitVector) = 0;
	virtual bool TryToggleClimbAction() = 0;
	virtual void SetFreeMoveMode(bool bInFreeMoveMode) = 0;

	virtual FSyncMotionWarpingDataDelegate& OnVaultMotionWarping() = 0;
	virtual FSyncMotionWarpingDataDelegate& OnHangUpMotionWarping() = 0;
	virtual FSyncClilmbDataDelegate& OnClimb() = 0;
};

//# 타깃 제공자 — USpyTargetingManagerComponent 가 구현한다.
UINTERFACE(MinimalAPI)
class USpyTargetProvider : public UInterface
{
	GENERATED_BODY()
};

class ISpyTargetProvider
{
	GENERATED_BODY()

public:
	virtual TWeakObjectPtr<AActor> GetTarget() const = 0;
	virtual bool IsTargetValid() const = 0;
	virtual bool FindTarget(float Radius) = 0;
	virtual void SetCurrentTarget(AActor* NewTarget) = 0;
};

//# 그래플 타깃 제공자 — USpyGrappleTargetingComponent 가 구현한다.
UINTERFACE(MinimalAPI)
class USpyGrappleHost : public UInterface
{
	GENERATED_BODY()
};

class ISpyGrappleHost
{
	GENERATED_BODY()

public:
	virtual AActor* GetLocalCachedTarget() const = 0;
	virtual AActor* GetCurrentGrappleTarget() const = 0;
	virtual FOnGrappleTargetChanged& OnGrappleTargetChanged() = 0;
};

//# 상호작용 호스트 — USpyInteractionComponent 가 구현한다.
//# NPC 는 이 인터페이스로만 "플레이어가 범위에 들어왔다"를 알린다 —
//# 구체 컴포넌트 타입을 알지 않는다 (§8, §10)
UINTERFACE(MinimalAPI)
class USpyInteractionHost : public UInterface
{
	GENERATED_BODY()
};

class ISpyInteractionHost
{
	GENERATED_BODY()

public:
	//# NPCActor 는 상호작용 대상 액터(내부적으로 ISpyNPCRoot 캐스팅은 구현부가 담당).
	//# bInRange가 false면 이 NPCActor를 현재 대상에서 해제한다.
	virtual void NotifyNPCRangeChanged(AActor* NPCActor, bool bInRange) = 0;

	//# InteractableActor 는 오브젝트 상호작용 대상 액터(내부적으로 ISpyInteractableRoot 캐스팅은
	//# 구현부가 담당). NPC 경로와 별개 슬롯으로 추적된다.
	virtual void NotifyInteractableRangeChanged(AActor* InteractableActor, bool bInRange) = 0;

	//# 입력 바인딩(Interact 액션)이 호출한다. 로컬에서 근접 NPC가 없으면 아무 일도 하지 않는다.
	virtual void TryInteract() = 0;

	//# 서버로부터 마지막으로 수신한 대화 결과. OpenSpyUI 비동기 로드라 위젯 생성이 항상 늦다 —
	//# 위젯은 NativeConstruct 에서 이 값을 1회 pull 한다(레이스 없음).
	virtual const FSpyNPCDialogueResult& GetLastDialogueResult() const = 0;

	//# 근접 상호작용 프롬프트에 표시할 동사 텍스트. OpenSpyUI 가 비동기(위젯 로드)라
	//# 위젯은 NativeConstruct 에서 이 값을 1회 읽어(pull) 자신을 채운다(레이스 없음).
	virtual FText GetInteractVerb() const = 0;

	//# WBP_MissionOffer 의 [수락] 버튼이 호출한다. 서버에 수락 RPC를 보내고 카드를 닫는다.
	virtual void ConfirmMissionCard() = 0;

	//# WBP_MissionOffer 의 [거절] 버튼이 호출한다. 카드만 닫는다 (RPC 없음).
	virtual void DismissMissionCard() = 0;

	//# 대화창이 열린 채로 다음 페이지로 넘어갈 때 발화한다. 위젯은 NativeConstruct 에서 1회 구독하고
	//# 이후 재생성 없이 이 델리게이트로 텍스트만 갱신한다(재오픈 시 NativeConstruct 미재실행 대응).
	virtual FOnDialogueLineChanged& OnDialogueLineChanged() = 0;
};
