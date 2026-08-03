// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SpyInteractPromptWidget.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "Character/CommonInterface.Character.h"
#include "ManagerComponent/CommonInterface.Manager.h"

void USpyInteractPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();

	//# OpenSpyUI 는 비동기(위젯 로드) 이므로 이 위젯은 NotifyNPCRangeChanged 가 값을
	//# 캐싱한 뒤에 생성된다 — Host 에 캐싱된 동사 텍스트를 1회 읽어(pull) 채운다 (레이스 없음)
	TScriptInterface<ISpyInteractionHost> Host = ResolveInteractionHost();
	if (Host.GetObject() == nullptr)
		return;

	ShowPrompt(Host->GetInteractVerb());
}

TScriptInterface<ISpyInteractionHost> USpyInteractPromptWidget::ResolveInteractionHost() const
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

void USpyInteractPromptWidget::ShowPrompt(FText InVerb)
{
	if (Txt_Verb != nullptr)
	{
		Txt_Verb->SetText(InVerb);
	}
}
