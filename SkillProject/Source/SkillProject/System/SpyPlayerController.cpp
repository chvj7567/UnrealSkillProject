// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyPlayerController.h"
#include "Character/SpyCharacter.h"
#include "InputActionValue.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Util/SpyGameplayTags.h"
#include "UI/SpyUserWidget.h"
#include "Manager/SpyAssetManager.h"
#include "Manager/SpyUIManager.h"
#include "Util/DefineEnum.h"
#include "System/SpyPlayerState.h"
#include "Character/SpyCharacterMovementComponent.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyPlayerController)

ASpyPlayerController::ASpyPlayerController()
{
	TeamID = FGenericTeamId(100);
}

void ASpyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = true;

	USpyUIManager::Get(this)->OpenSpyUI(ESpyUIType::MainHUD);
}

void ASpyPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
}

void ASpyPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
}

void ASpyPlayerController::PreProcessInput(const float DeltaTime, const bool bGamePaused)
{
	Super::PreProcessInput(DeltaTime, bGamePaused);
}

void ASpyPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	if (USpyAbilitySystemComponent* SpyASC = GetSpyAbilitySystemComponent())
	{
		SpyASC->ProcessAbilityInput(DeltaTime, bGamePaused);
	}

	Super::PostProcessInput(DeltaTime, bGamePaused);
}

USpyAbilitySystemComponent* ASpyPlayerController::GetSpyAbilitySystemComponent() const
{
	const ASpyPlayerState* SpyPS = CastChecked<ASpyPlayerState>(PlayerState, ECastCheckedType::NullAllowed);
	return (SpyPS ? SpyPS->GetSpyAbilitySystemComponent() : nullptr);
}

void ASpyPlayerController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	TeamID = NewTeamID;
}

FGenericTeamId ASpyPlayerController::GetGenericTeamId() const
{
	return TeamID;
}
