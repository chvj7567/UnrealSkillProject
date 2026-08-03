// Fill out your copyright notice in the Description page of Project Settings.

#include "NPC/SpyNPCCharacter.h"
#include "Components/SphereComponent.h"
#include "Character/CommonInterface.Character.h"
#include "ManagerComponent/CommonInterface.Manager.h"
#include "System/SpyMissionComponent.h"
#include "Data/SpyMissionConfig.h"
#include "Data/SpyNPCDialogueRow.h"
#include "Util/SpyGameplayTags.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

ASpyNPCCharacter::ASpyNPCCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(GetRootComponent());
	InteractionSphere->SetSphereRadius(300.f);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void ASpyNPCCharacter::BeginPlay()
{
	Super::BeginPlay();

	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ASpyNPCCharacter::OnInteractionSphereBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &ASpyNPCCharacter::OnInteractionSphereEndOverlap);

	CacheNPCData();
}

void ASpyNPCCharacter::CacheNPCData()
{
	if (NPCConfig == nullptr)
		return;

	if (NPCConfig->NPCTable != nullptr)
	{
		TArray<FSpyNPCRow*> NPCRows;
		NPCConfig->NPCTable->GetAllRows<FSpyNPCRow>(TEXT("ASpyNPCCharacter::CacheNPCData"), NPCRows);

		for (const FSpyNPCRow* Row : NPCRows)
		{
			if (Row == nullptr || Row->NPCId != NPCId)
				continue;

			CachedNPCDisplayName = Row->NPCDisplayName;
			CachedDefaultDialogueId = Row->DefaultDialogueId;
			TryGetDialogueLineAtIndex(NPCConfig->DialogueTable, CachedDefaultDialogueId, 0, CachedDefaultLine);

			break;
		}
	}

	if (NPCConfig->MissionCommunicationTable == nullptr)
		return;

	TArray<FSpyMissionCommunicationRow*> CommRows;
	NPCConfig->MissionCommunicationTable->GetAllRows<FSpyMissionCommunicationRow>(TEXT("ASpyNPCCharacter::CacheNPCData"), CommRows);

	for (const FSpyMissionCommunicationRow* Row : CommRows)
	{
		if (Row == nullptr || Row->NPCId != NPCId)
			continue;

		if (Row->Role == ESpyMissionCommRole::Offer)
		{
			CachedOfferMissionId = Row->MissionId;
			CachedOfferDialogueId = Row->OfferDialogueId;
			CachedInProgressDialogueId = Row->InProgressDialogueId;
			TryGetDialogueLineAtIndex(NPCConfig->DialogueTable, CachedOfferDialogueId, 0, CachedOfferLine);
			TryGetDialogueLineAtIndex(NPCConfig->DialogueTable, CachedInProgressDialogueId, 0, CachedInProgressLine);
		}
		else
		{
			CachedReportMissionId = Row->MissionId;
			CachedReportDialogueId = Row->ReportDialogueId;
			TryGetDialogueLineAtIndex(NPCConfig->DialogueTable, CachedReportDialogueId, 0, CachedReportLine);
		}
	}

	//# spec §9 — 이 NPC가 Offer/Report 행을 정확히 1개씩 갖지 못하면 대화가 절대 Offer/Report 로
	//# 판정될 수 없다(항상 Default). 조용히 실패하면 원인 추적이 어려우므로 데이터 오류로 남긴다
	bDataCached = (CachedOfferMissionId != INDEX_NONE && CachedReportMissionId != INDEX_NONE);

	if (bDataCached == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("# [ASpyNPCCharacter] NPCId %d 의 MissionCommunication Offer/Report 행이 정확히 1개씩 없습니다 (에디터 데이터 오류 의심): %s"), NPCId, *GetName());
	}
}

void ASpyNPCCharacter::OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
													   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const APawn* OtherPawn = Cast<APawn>(OtherActor);
	if (OtherPawn == nullptr || OtherPawn->IsLocallyControlled() == false)
		return;

	//# TScriptInterface(RawPtr) 생성자는 인터페이스 미구현이어도 ObjectPointer 를 그대로 저장한다 —
	//# GetObject() 널체크로는 구현 여부를 걸러낼 수 없다. Cast<Interface> 로 먼저 판정한다.
	ISpyCharacterRoot* CharRoot = Cast<ISpyCharacterRoot>(OtherActor);
	if (CharRoot == nullptr)
		return;

	TScriptInterface<ISpyInteractionHost> Host = CharRoot->GetInteractionHost();
	if (Host.GetObject() == nullptr)
		return;

	Host->NotifyNPCRangeChanged(this, true);
}

void ASpyNPCCharacter::OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
													 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	const APawn* OtherPawn = Cast<APawn>(OtherActor);
	if (OtherPawn == nullptr || OtherPawn->IsLocallyControlled() == false)
		return;

	//# TScriptInterface(RawPtr) 생성자는 인터페이스 미구현이어도 ObjectPointer 를 그대로 저장한다 —
	//# GetObject() 널체크로는 구현 여부를 걸러낼 수 없다. Cast<Interface> 로 먼저 판정한다.
	ISpyCharacterRoot* CharRoot = Cast<ISpyCharacterRoot>(OtherActor);
	if (CharRoot == nullptr)
		return;

	TScriptInterface<ISpyInteractionHost> Host = CharRoot->GetInteractionHost();
	if (Host.GetObject() == nullptr)
		return;

	Host->NotifyNPCRangeChanged(this, false);
}

bool ASpyNPCCharacter::IsPawnInRange(const AActor* RequesterPawn) const
{
	if (RequesterPawn == nullptr || InteractionSphere == nullptr)
		return false;

	//# 트리거(오버랩)와 동일한 기하로 재검증한다 — point-distance 를 별도로 두면
	//# 캡슐 반경만큼 "프롬프트는 뜨는데 서버가 거부하는" 구간이 생긴다 (spec §5-1)
	return InteractionSphere->IsOverlappingActor(RequesterPawn);
}

FSpyNPCDialogueResult ASpyNPCCharacter::RequestInteract(APlayerController* Requester)
{
	FSpyNPCDialogueResult Result;

	//# 게임플레이 상태(미션 진행)를 바꾸는 함수라 호출부 권한 체크에만 기대지 않는다 — 자체 방어
	if (HasAuthority() == false)
		return Result;

	if (Requester == nullptr)
		return Result;

	//# 대화창 빈 값 재현 확인용 — 어느 NPC가 어떤 NPCId로 인식되는지 대조한다
	UE_LOG(LogTemp, Warning, TEXT("# [ASpyNPCCharacter] RequestInteract NPCId=%d CachedNPCDisplayName=%s"), NPCId, *CachedNPCDisplayName.ToString());

	Result.NPCName = CachedNPCDisplayName;

	APawn* RequesterPawn = Requester->GetPawn();
	if (RequesterPawn == nullptr)
		return Result;

	APlayerState* RequesterPS = RequesterPawn->GetPlayerState();
	if (RequesterPS == nullptr)
		return Result;

	USpyMissionComponent* MissionComp = USpyMissionComponent::FindMissionComponent(RequesterPS);
	if (MissionComp == nullptr)
		return Result;

	Result.State = ResolveNPCDialogueState(
		MissionComp->GetMissionIndex(), MissionComp->IsCurrentAccepted(), CachedOfferMissionId, CachedReportMissionId);

	switch (Result.State)
	{
	case ESpyNPCDialogueState::Offer:
		Result.Line = CachedOfferLine;
		Result.DialogueId = CachedOfferDialogueId;
		Result.bShowMissionCard = true;

		if (const FSpyMissionRow* Entry = MissionComp->GetMissionEntry(CachedOfferMissionId))
		{
			Result.MissionTitle = Entry->DisplayName;
			Result.MissionDescription = Entry->Description;
		}
		break;

	case ESpyNPCDialogueState::InProgress:
		Result.Line = CachedInProgressLine;
		Result.DialogueId = CachedInProgressDialogueId;
		break;

	case ESpyNPCDialogueState::Report:
		Result.Line = CachedReportLine;
		Result.DialogueId = CachedReportDialogueId;

		//# 카드 없이 이 자리에서 완료 처리까지 끝낸다 — 서버 재검증(거리)은 호출부(Task 6)가 이미 마쳤다
		MissionComp->AddProgress(SpyGameplayTags::Event_Mission_Report, 1);
		break;

	case ESpyNPCDialogueState::Default:
	default:
		Result.Line = CachedDefaultLine;
		Result.DialogueId = CachedDefaultDialogueId;
		break;
	}

	return Result;
}

bool ASpyNPCCharacter::GetDialogueLineAtIndex(int32 InDialogueId, int32 InPageIndex, FText& OutLine) const
{
	if (NPCConfig == nullptr)
	{
		OutLine = FText::GetEmpty();
		return false;
	}

	return TryGetDialogueLineAtIndex(NPCConfig->DialogueTable, InDialogueId, InPageIndex, OutLine);
}
