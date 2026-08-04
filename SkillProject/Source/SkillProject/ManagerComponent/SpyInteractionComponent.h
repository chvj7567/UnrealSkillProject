// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "ManagerComponent/CommonInterface.Manager.h"
#include "NPC/CommonInterface.NPC.h"
#include "Interactable/CommonInterface.Interactable.h"

#include "SpyInteractionComponent.generated.h"

//# 플레이어 측 상호작용 컴포넌트. 근접 NPC 하나를 추적하고,
//# 입력이 들어오면 서버에 상호작용/수락 요청을 보낸다.
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SKILLPROJECT_API USpyInteractionComponent : public UActorComponent, public ISpyInteractionHost
{
	GENERATED_BODY()

public:
	USpyInteractionComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//# ISpyInteractionHost
	virtual void NotifyNPCRangeChanged(AActor* NPCActor, bool bInRange) override;
	virtual void NotifyInteractableRangeChanged(AActor* InteractableActor, bool bInRange) override;
	virtual void TryInteract() override;
	virtual const FSpyNPCDialogueResult& GetLastDialogueResult() const override
	{
		return LastDialogueResult;
	}
	virtual FText GetInteractVerb() const override
	{
		return CachedInteractVerb;
	}
	virtual void ConfirmMissionCard() override;
	virtual void DismissMissionCard() override;
	virtual FOnDialogueLineChanged& OnDialogueLineChanged() override
	{
		return OnDialogueLineChangedDelegate;
	}

protected:
	//# 대화가 열려 있을 때 F 입력을 페이지 넘기기/닫기로 라우팅한다
	void AdvanceOrCloseDialogue();

	//# 대화 종료 정리 — 다음 페이지 없음(AdvanceOrCloseDialogue) / 대화 상대 범위 이탈(NotifyNPCRangeChanged) 양쪽에서 공유
	void CloseDialogue();

	//# OpenUI 는 LoadAssetAsync 콜백 안에서만 위젯을 OpenUIList 에 넣는다 — 로드 완료 전에
	//# CloseSpyUI 가 오면 no-op 이 되고 뒤늦게 위젯이 남는다. 지연 재검증으로 한 번 더 닫는다.
	void HandleInteractPromptCloseFailsafe();

	//# Dialogue 위젯도 동일한 비동기 로드 레이스가 있다 — CloseDialogue 직후 지연 재검증으로 닫는다.
	void HandleDialogueCloseFailsafe();

	//# 미션카드(MissionOffer) 열림/닫힘에만 커서를 묶는다 — 대화만 열려 있을 때는 켜지 않는다.
	//# Owner(폰)→Controller 캐스팅 체인은 Server_RequestInteract_Implementation 과 동일 패턴 재사용 (§8)
	void SetMissionCardCursorMode(bool bEnabled);

	//# 카드 닫기 뒷정리 3단계 — ConfirmMissionCard/DismissMissionCard 양쪽이 공유한다:
	//# CloseSpyUI(MissionOffer) + 대화 열려있으면 CloseDialogue() + 커서 복귀
	void HandleMissionCardClosed();

	//# 범위 이탈이 아니라 대화/카드가 자연 종료된 지점(AdvanceOrCloseDialogue, HandleMissionCardClosed)
	//# 에서만 호출 — 여전히 범위 안이면 프롬프트를 다시 켠다. bInRange=false 경로에서는 호출 금지.
	void ReopenInteractPromptIfStillNearby();

	UFUNCTION(Server, Reliable)
	void Server_RequestInteract(AActor* TargetNPC);

	UFUNCTION(Server, Reliable)
	void Server_RequestInteractObject(AActor* TargetObject);

	UFUNCTION(Server, Reliable)
	void Server_AcceptCurrentMission();

	//# TargetNPC 는 서버가 이미 확정한 상호작용 대상이다 — 로컬 NearbyNPC 는 RPC 왕복 중
	//# EndOverlap 으로 먼저 비워질 수 있어(레이스) ConversingNPC 를 채우는 데 쓰지 않는다.
	UFUNCTION(Client, Reliable)
	void Client_ReceiveDialogueResult(FSpyNPCDialogueResult Result, AActor* TargetNPC);

protected:
	UPROPERTY(Transient)
	TObjectPtr<AActor> NearbyNPC;

	UPROPERTY(Transient)
	TObjectPtr<AActor> NearbyInteractable;

	//# 서버로부터 마지막으로 수신한 대화 결과. Dialogue/MissionOffer 위젯이
	//# NativeConstruct 에서 1회 읽어(pull) 채운다 (ISpyInteractionHost::GetLastDialogueResult 참조)
	FSpyNPCDialogueResult LastDialogueResult;

	//# 근접 상호작용 프롬프트 위젯이 NativeConstruct 에서 1회 읽어(pull) 채운다
	//# (ISpyInteractionHost::GetInteractVerb 참조)
	FText CachedInteractVerb;

	UPROPERTY(Transient)
	FTimerHandle InteractPromptCloseFailsafeTimerHandle;

	UPROPERTY(Transient)
	FTimerHandle DialogueCloseFailsafeTimerHandle;

	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FOnDialogueLineChanged OnDialogueLineChangedDelegate;

	//# 현재 대화 중인 NPC — NearbyNPC(오버랩 범위) 와 별개다. 대화 중 한 걸음 벗어나도
	//# 대화가 끊기지 않게 별도로 들고 있는다.
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> ConversingNPC;

	int32 CurrentDialogueId = INDEX_NONE;
	int32 CurrentDialoguePageIndex = 0;
	bool bDialogueOpen = false;
};
