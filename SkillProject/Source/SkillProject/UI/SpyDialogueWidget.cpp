// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SpyDialogueWidget.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "Character/CommonInterface.Character.h"
#include "ManagerComponent/CommonInterface.Manager.h"

void USpyDialogueWidget::NativeConstruct()
{
	Super::NativeConstruct();

	//# OpenSpyUI 는 비동기(위젯 로드) 이므로 이 위젯은 서버 결과가 도착한 뒤에 생성된다 —
	//# InteractionHost 에 캐싱된 마지막 결과를 1회 읽어(pull) 채운다 (레이스 없음)
	TScriptInterface<ISpyInteractionHost> Host = ResolveInteractionHost();
	if (Host.GetObject() == nullptr)
		return;

	const FSpyNPCDialogueResult& Result = Host->GetLastDialogueResult();
	ShowLine(Result.NPCName, Result.Line);

	Host->OnDialogueLineChanged().AddUniqueDynamic(this, &USpyDialogueWidget::HandleDialogueLineChanged);
}

void USpyDialogueWidget::HandleDialogueLineChanged(FText InNPCName, FText InLine)
{
	ShowLine(InNPCName, InLine);
}

TScriptInterface<ISpyInteractionHost> USpyDialogueWidget::ResolveInteractionHost() const
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

void USpyDialogueWidget::ShowLine(FText InNPCName, FText InLine)
{
	if (Txt_NPCName != nullptr)
	{
		Txt_NPCName->SetText(InNPCName);
	}

	if (Txt_Line != nullptr)
	{
		Txt_Line->SetText(InLine);
	}
}
