// Fill out your copyright notice in the Description page of Project Settings.

#include "ManagerComponent/SpyInteractionComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "Manager/SpyUIManager.h"
#include "Util/DefineEnum.h"
#include "System/SpyMissionComponent.h"

//# plugin-skuicore.md 실측 로드 시간(~1.23초)보다 넉넉한 재검증 지연 — InteractPrompt/Dialogue 공용
static constexpr float UICloseFailsafeDelaySec = 2.0f;

USpyInteractionComponent::USpyInteractionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void USpyInteractionComponent::NotifyNPCRangeChanged(AActor* NPCActor, bool bInRange)
{
	if (bInRange)
	{
		NearbyNPC = NPCActor;

		//# 트리거는 현재 NPC 전용이라 동사를 고정한다 — 트리거 구조 일반화는 이번 범위 밖
		CachedInteractVerb = NSLOCTEXT("SpyInteraction", "TalkVerb", "대화하기");

		if (USpyUIManager* UIManager = Cast<USpyUIManager>(USKUIManager::Get(this)))
		{
			UIManager->OpenSpyUI(ESpyUIType::InteractPrompt);
		}
		return;
	}

	//# 범위를 벗어난 NPC 가 현재 대화 상대면, NearbyNPC 조기 return 가드와 무관하게 대화도 닫는다
	//# (ConversingNPC 는 NearbyNPC 와 별개 상태라 아래 가드에 걸려 스킵되면 안 된다)
	if (NPCActor == ConversingNPC.Get())
	{
		CloseDialogue();
	}

	if (NearbyNPC != NPCActor)
		return;

	NearbyNPC = nullptr;

	if (USpyUIManager* UIManager = Cast<USpyUIManager>(USKUIManager::Get(this)))
	{
		UIManager->CloseSpyUI(ESpyUIType::InteractPrompt);
	}

	//# OpenUI 로드가 이 시점에 아직 안 끝났으면 위 CloseSpyUI 는 no-op 이다 —
	//# 지연 뒤 재검증해 한 번 더 닫아 프롬프트가 남는 걸 방지한다
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			InteractPromptCloseFailsafeTimerHandle,
			this,
			&USpyInteractionComponent::HandleInteractPromptCloseFailsafe,
			UICloseFailsafeDelaySec,
			false);
	}
}

void USpyInteractionComponent::HandleInteractPromptCloseFailsafe()
{
	//# 재검증 시점에 다시 NPC 범위 안이면(재진입) 닫지 않는다
	if (NearbyNPC != nullptr)
		return;

	if (USpyUIManager* UIManager = Cast<USpyUIManager>(USKUIManager::Get(this)))
	{
		UIManager->CloseSpyUI(ESpyUIType::InteractPrompt);
	}
}

void USpyInteractionComponent::TryInteract()
{
	if (bDialogueOpen)
	{
		AdvanceOrCloseDialogue();
		return;
	}

	if (NearbyNPC == nullptr)
		return;

	Server_RequestInteract(NearbyNPC);
}

void USpyInteractionComponent::AdvanceOrCloseDialogue()
{
	ISpyNPCRoot* NPCRoot = Cast<ISpyNPCRoot>(ConversingNPC.Get());

	FText NextLine;
	if (NPCRoot != nullptr && NPCRoot->GetDialogueLineAtIndex(CurrentDialogueId, CurrentDialoguePageIndex + 1, NextLine))
	{
		++CurrentDialoguePageIndex;
		OnDialogueLineChangedDelegate.Broadcast(LastDialogueResult.NPCName, NextLine);
		return;
	}

	CloseDialogue();
}

void USpyInteractionComponent::CloseDialogue()
{
	bDialogueOpen = false;
	ConversingNPC = nullptr;

	if (USpyUIManager* UIManager = Cast<USpyUIManager>(USKUIManager::Get(this)))
	{
		UIManager->CloseSpyUI(ESpyUIType::Dialogue);
	}

	//# OpenSpyUI(Dialogue) 로드가 이 시점에 아직 안 끝났으면 위 CloseSpyUI 는 no-op 이다 —
	//# 지연 뒤 재검증해 한 번 더 닫아 위젯이 뒤늦게 나타나 고아로 남는 걸 방지한다
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DialogueCloseFailsafeTimerHandle,
			this,
			&USpyInteractionComponent::HandleDialogueCloseFailsafe,
			UICloseFailsafeDelaySec,
			false);
	}
}

void USpyInteractionComponent::HandleDialogueCloseFailsafe()
{
	//# 재검증 시점에 이미 새 대화가 열려 있으면(재진입) 닫지 않는다
	if (bDialogueOpen)
		return;

	if (USpyUIManager* UIManager = Cast<USpyUIManager>(USKUIManager::Get(this)))
	{
		UIManager->CloseSpyUI(ESpyUIType::Dialogue);
	}
}

void USpyInteractionComponent::Server_RequestInteract_Implementation(AActor* TargetNPC)
{
	if (TargetNPC == nullptr)
		return;

	AActor* Owner = GetOwner();
	if (Owner == nullptr || Owner->HasAuthority() == false)
		return;

	//# GetObject() 널체크는 인터페이스 미구현을 걸러내지 못한다(클라 RPC 파라미터라 임의 액터 가능) —
	//# Cast<Interface> 로 먼저 판정해 서버 크래시를 막는다.
	ISpyNPCRoot* NPCRoot = Cast<ISpyNPCRoot>(TargetNPC);
	if (NPCRoot == nullptr)
		return;

	//# point-distance 대신 NPC 의 상호작용 SphereComponent 오버랩을 그대로 재확인한다 —
	//# 값을 이중으로 두지 않고 트리거와 서버 판정의 기하를 구조적으로 일치시킨다 (spec §5-1)
	const bool bInRange = NPCRoot->IsPawnInRange(Owner);
	if (bInRange == false)
		return;

	APlayerController* PC = Cast<APlayerController>(Owner->GetInstigatorController());
	if (PC == nullptr)
		return;

	const FSpyNPCDialogueResult Result = NPCRoot->RequestInteract(PC);

	Client_ReceiveDialogueResult(Result, TargetNPC);
}

void USpyInteractionComponent::Server_AcceptCurrentMission_Implementation()
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr || Owner->HasAuthority() == false)
		return;

	APlayerController* PC = Cast<APlayerController>(Owner->GetInstigatorController());
	if (PC == nullptr)
		return;

	APawn* Pawn = PC->GetPawn();
	if (Pawn == nullptr)
		return;

	APlayerState* PS = Pawn->GetPlayerState();
	if (PS == nullptr)
		return;

	USpyMissionComponent* MissionComp = USpyMissionComponent::FindMissionComponent(PS);
	if (MissionComp == nullptr)
		return;

	MissionComp->AcceptCurrentMission();
}

void USpyInteractionComponent::Client_ReceiveDialogueResult_Implementation(FSpyNPCDialogueResult Result, AActor* TargetNPC)
{
	//# 위젯은 아직 존재하지 않는다(OpenSpyUI 는 비동기 로드) — 값을 먼저 캐싱해 두면
	//# 위젯이 나중에 생성돼도 NativeConstruct 에서 이 값을 읽어 채울 수 있다 (레이스 없음)
	LastDialogueResult = Result;

	CurrentDialogueId = Result.DialogueId;
	CurrentDialoguePageIndex = 0;
	bDialogueOpen = true;
	ConversingNPC = TargetNPC;

	USpyUIManager* UIManager = Cast<USpyUIManager>(USKUIManager::Get(this));
	if (UIManager == nullptr)
		return;

	UIManager->OpenSpyUI(ESpyUIType::Dialogue);

	//# 캐싱된 위젯 재사용 시 NativeConstruct 재실행이 비결정적이라, 새로 열리든 재사용되든
	//# 항상 이 델리게이트로 최신 텍스트를 push 해 이전 NPC 의 대사가 남지 않게 한다
	OnDialogueLineChangedDelegate.Broadcast(Result.NPCName, Result.Line);

	if (Result.bShowMissionCard)
	{
		UIManager->OpenSpyUI(ESpyUIType::MissionOffer);
	}
}

void USpyInteractionComponent::ConfirmMissionCard()
{
	Server_AcceptCurrentMission();

	if (USpyUIManager* UIManager = Cast<USpyUIManager>(USKUIManager::Get(this)))
	{
		UIManager->CloseSpyUI(ESpyUIType::MissionOffer);
	}
}

void USpyInteractionComponent::DismissMissionCard()
{
	if (USpyUIManager* UIManager = Cast<USpyUIManager>(USKUIManager::Get(this)))
	{
		UIManager->CloseSpyUI(ESpyUIType::MissionOffer);
	}
}
