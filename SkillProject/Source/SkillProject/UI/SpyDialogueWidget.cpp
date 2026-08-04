// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SpyDialogueWidget.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "Character/CommonInterface.Character.h"
#include "ManagerComponent/CommonInterface.Manager.h"
#include "UI/SpyHUDMath.h"

void USpyDialogueWidget::NativeConstruct()
{
	Super::NativeConstruct();

	//# 계속 힌트는 ISpyInteractionHost 와 무관하게 OwningPlayerPawn 만 있으면 되므로,
	//# 아래 Host 널 가드보다 먼저 세팅한다 — 대화 결과가 없어도 힌트는 항상 채워져야 한다
	CachedInteractInputAction = ResolveInteractInputAction();

	APlayerController* PC = GetOwningPlayer();
	const ULocalPlayer* LP = (PC != nullptr) ? PC->GetLocalPlayer() : nullptr;
	CachedInputSubsystem = (LP != nullptr) ? LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;

	RefreshContinueHintText();

	if (CachedInputSubsystem.IsValid())
	{
		CachedInputSubsystem->ControlMappingsRebuiltDelegate.AddUniqueDynamic(this, &USpyDialogueWidget::HandleControlMappingsRebuilt);
	}

	//# OpenSpyUI 는 비동기(위젯 로드) 이므로 이 위젯은 서버 결과가 도착한 뒤에 생성된다 —
	//# InteractionHost 에 캐싱된 마지막 결과를 1회 읽어(pull) 채운다 (레이스 없음)
	TScriptInterface<ISpyInteractionHost> Host = ResolveInteractionHost();
	if (Host.GetObject() == nullptr)
		return;

	const FSpyNPCDialogueResult& Result = Host->GetLastDialogueResult();
	ShowLine(Result.NPCName, Result.Line);

	Host->OnDialogueLineChanged().AddUniqueDynamic(this, &USpyDialogueWidget::HandleDialogueLineChanged);
}

void USpyDialogueWidget::NativeDestruct()
{
	if (CachedInputSubsystem.IsValid())
	{
		CachedInputSubsystem->ControlMappingsRebuiltDelegate.RemoveDynamic(this, &USpyDialogueWidget::HandleControlMappingsRebuilt);
	}

	Super::NativeDestruct();
}

void USpyDialogueWidget::HandleDialogueLineChanged(FText InNPCName, FText InLine)
{
	ShowLine(InNPCName, InLine);
}

void USpyDialogueWidget::HandleControlMappingsRebuilt()
{
	RefreshContinueHintText();
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

const UInputAction* USpyDialogueWidget::ResolveInteractInputAction() const
{
	APawn* OwningPawn = GetOwningPlayerPawn();
	if (OwningPawn == nullptr)
		return nullptr;

	//# CharacterAssetData/InputConfig 체인은 루트가 대신 파헤친다 — 위젯은 탐색하지 않는다 (§13)
	ISpyCharacterRoot* CharRoot = Cast<ISpyCharacterRoot>(OwningPawn);
	if (CharRoot == nullptr)
		return nullptr;

	return CharRoot->GetInteractInputAction();
}

void USpyDialogueWidget::RefreshContinueHintText()
{
	if (Txt_ContinueHint == nullptr)
		return;

	if (CachedInteractInputAction == nullptr || CachedInputSubsystem.IsValid() == false)
	{
		Txt_ContinueHint->SetText(BuildContinueHintText(TArray<FKey>()));
		return;
	}

	const TArray<FKey> MappedKeys = CachedInputSubsystem->QueryKeysMappedToAction(CachedInteractInputAction);
	Txt_ContinueHint->SetText(BuildContinueHintText(MappedKeys));
}

FText USpyDialogueWidget::BuildContinueHintText(const TArray<FKey>& MappedKeys)
{
	//# 실제 바인딩을 못 찾은 경우 임의의 키를 단정하지 않는다 — 키 이름 없이 중립 문구로 폴백
	static const FText FallbackHint = NSLOCTEXT("SpyDialogue", "ContinueHintFallback", "계속");

	const FText KeyDisplayName = SpyHUDMath::ResolveKeyDisplayName(MappedKeys);
	if (KeyDisplayName.IsEmpty())
		return FallbackHint;

	return FText::Format(NSLOCTEXT("SpyDialogue", "ContinueHintFormat", "{0} 계속"), KeyDisplayName);
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
