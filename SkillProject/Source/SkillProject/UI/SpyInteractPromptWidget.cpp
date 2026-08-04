// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SpyInteractPromptWidget.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "InputCoreTypes.h"
#include "Character/CommonInterface.Character.h"
#include "ManagerComponent/CommonInterface.Manager.h"
#include "UI/SpyHUDMath.h"

void USpyInteractPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ISpyCharacterRoot* CharRoot = ResolveCharacterRoot();

	//# 키캡 힌트는 ISpyInteractionHost 와 무관하게 CharRoot 만 있으면 되므로,
	//# 아래 Host 널 가드보다 먼저 세팅한다 — 상호작용 대상이 없어도 키캡은 항상 채워져야 한다
	CachedInteractInputAction = (CharRoot != nullptr) ? CharRoot->GetInteractInputAction() : nullptr;

	APlayerController* PC = GetOwningPlayer();
	const ULocalPlayer* LP = (PC != nullptr) ? PC->GetLocalPlayer() : nullptr;
	CachedInputSubsystem = (LP != nullptr) ? LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;

	RefreshKeyCapText();

	if (CachedInputSubsystem.IsValid())
	{
		CachedInputSubsystem->ControlMappingsRebuiltDelegate.AddUniqueDynamic(this, &USpyInteractPromptWidget::HandleControlMappingsRebuilt);
	}

	//# OpenSpyUI 는 비동기(위젯 로드) 이므로 이 위젯은 NotifyNPCRangeChanged 가 값을
	//# 캐싱한 뒤에 생성된다 — Host 에 캐싱된 동사 텍스트를 1회 읽어(pull) 채운다 (레이스 없음)
	TScriptInterface<ISpyInteractionHost> Host = (CharRoot != nullptr) ? CharRoot->GetInteractionHost() : TScriptInterface<ISpyInteractionHost>();
	if (Host.GetObject() == nullptr)
		return;

	ShowPrompt(Host->GetInteractVerb());
}

void USpyInteractPromptWidget::NativeDestruct()
{
	if (CachedInputSubsystem.IsValid())
	{
		CachedInputSubsystem->ControlMappingsRebuiltDelegate.RemoveDynamic(this, &USpyInteractPromptWidget::HandleControlMappingsRebuilt);
	}

	Super::NativeDestruct();
}

void USpyInteractPromptWidget::HandleControlMappingsRebuilt()
{
	RefreshKeyCapText();
}

ISpyCharacterRoot* USpyInteractPromptWidget::ResolveCharacterRoot() const
{
	APawn* OwningPawn = GetOwningPlayerPawn();
	if (OwningPawn == nullptr)
		return nullptr;

	//# TScriptInterface(RawPtr) 생성자는 인터페이스 미구현이어도 ObjectPointer 를 그대로 저장해
	//# GetObject() 널체크로는 구현 여부를 걸러낼 수 없다 — Cast<Interface> 로 직접 판정한다.
	return Cast<ISpyCharacterRoot>(OwningPawn);
}

void USpyInteractPromptWidget::RefreshKeyCapText()
{
	if (Txt_KeyCap == nullptr)
		return;

	if (CachedInteractInputAction == nullptr || CachedInputSubsystem.IsValid() == false)
	{
		//# 실제 바인딩과 다른 키를 보여줄 수 없으므로 임의의 키를 단정하지 않고 비워 둔다
		Txt_KeyCap->SetText(FText::GetEmpty());
		return;
	}

	const TArray<FKey> MappedKeys = CachedInputSubsystem->QueryKeysMappedToAction(CachedInteractInputAction);
	Txt_KeyCap->SetText(SpyHUDMath::ResolveKeyDisplayName(MappedKeys));
}

void USpyInteractPromptWidget::ShowPrompt(FText InVerb)
{
	if (Txt_Verb != nullptr)
	{
		Txt_Verb->SetText(InVerb);
	}
}
