// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SpyMissionOfferWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "GameFramework/Pawn.h"
#include "Character/CommonInterface.Character.h"
#include "ManagerComponent/CommonInterface.Manager.h"

void USpyMissionOfferWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Accept != nullptr)
	{
		Btn_Accept->OnClicked.AddDynamic(this, &USpyMissionOfferWidget::HandleAcceptButtonClicked);
	}

	if (Btn_Decline != nullptr)
	{
		Btn_Decline->OnClicked.AddDynamic(this, &USpyMissionOfferWidget::HandleDeclineButtonClicked);
	}

	//# OpenSpyUI 는 비동기(위젯 로드) 이므로 이 위젯은 서버 결과가 도착한 뒤에 생성된다 —
	//# InteractionHost 에 캐싱된 마지막 결과를 1회 읽어(pull) 채운다 (레이스 없음)
	TScriptInterface<ISpyInteractionHost> Host = ResolveInteractionHost();
	if (Host.GetObject() == nullptr)
		return;

	const FSpyNPCDialogueResult& Result = Host->GetLastDialogueResult();
	ShowMission(Result.MissionTitle, Result.MissionDescription);
}

TScriptInterface<ISpyInteractionHost> USpyMissionOfferWidget::ResolveInteractionHost() const
{
	APawn* OwningPawn = GetOwningPlayerPawn();
	if (OwningPawn == nullptr)
		return nullptr;

	//# TScriptInterface(RawPtr) 생성자는 인터페이스 미구현이어도 ObjectPointer 를 그대로 저장한다 —
	//# GetObject() 널체크로는 구현 여부를 걸러낼 수 없다. Cast<Interface> 로 먼저 판정한다.
	ISpyCharacterRoot* CharRoot = Cast<ISpyCharacterRoot>(OwningPawn);
	if (CharRoot == nullptr)
		return nullptr;

	return CharRoot->GetInteractionHost();
}

void USpyMissionOfferWidget::ShowMission(FText InTitle, FText InDescription)
{
	if (Txt_Title != nullptr)
	{
		Txt_Title->SetText(InTitle);
	}

	if (Txt_Description != nullptr)
	{
		Txt_Description->SetText(InDescription);
	}
}

void USpyMissionOfferWidget::HandleAcceptButtonClicked()
{
	OnAcceptClicked.Broadcast();

	TScriptInterface<ISpyInteractionHost> Host = ResolveInteractionHost();
	if (Host.GetObject() == nullptr)
		return;

	Host->ConfirmMissionCard();
}

void USpyMissionOfferWidget::HandleDeclineButtonClicked()
{
	OnDeclineClicked.Broadcast();

	TScriptInterface<ISpyInteractionHost> Host = ResolveInteractionHost();
	if (Host.GetObject() == nullptr)
		return;

	Host->DismissMissionCard();
}
