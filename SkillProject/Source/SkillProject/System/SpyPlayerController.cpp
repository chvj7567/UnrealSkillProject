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
}

void ASpyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = true;

	USpyUIManager::Get(this)->OpenSpyUI(ESpyUIType::MainHUD);

	SetMappingContext(DefaultMappingContext);
}

void ASpyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	/*if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASpyPlayerController::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &ASpyPlayerController::Move);

		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASpyPlayerController::Look);

		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ASpyPlayerController::JumpPressed);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ASpyPlayerController::JumpReleased);
		
		EnhancedInputComponent->BindAction(SkillAAction, ETriggerEvent::Started, this, &ASpyPlayerController::UseSkillA);
		EnhancedInputComponent->BindAction(SkillBAction, ETriggerEvent::Started, this, &ASpyPlayerController::UseSkillB);
		EnhancedInputComponent->BindAction(SkillCAction, ETriggerEvent::Started, this, &ASpyPlayerController::UseSkillC);
		EnhancedInputComponent->BindAction(SkillDAction, ETriggerEvent::Started, this, &ASpyPlayerController::UseSkillD);
		EnhancedInputComponent->BindAction(SkillEAction, ETriggerEvent::Started, this, &ASpyPlayerController::UseSkillE);

		EnhancedInputComponent->BindAction(TryVaultAction, ETriggerEvent::Started, this, &ASpyPlayerController::TryVault);
	}*/
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

void ASpyPlayerController::SetMappingContext(UInputMappingContext* InMappingContext)
{
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsystem->ClearAllMappings();
			Subsystem->AddMappingContext(InMappingContext, 0);
		}
	}
}

void ASpyPlayerController::RefreshMappingContext()
{
	if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetPawn()))
	{
		if (ASpyPlayerState* SpyPlayerState = SpyCharacter->GetPlayerState<ASpyPlayerState>())
		{
			if (SpyPlayerState->HasState(SpyGameplayTags::Character_State_Movement_Climb))
			{
				SetMappingContext(WallClimbMappingContext);
			}
			else
			{
				SetMappingContext(DefaultMappingContext);
			}
		}
	}
}

USpyAbilitySystemComponent* ASpyPlayerController::GetSpyAbilitySystemComponent() const
{
	const ASpyPlayerState* SpyPS = CastChecked<ASpyPlayerState>(PlayerState, ECastCheckedType::NullAllowed);
	return (SpyPS ? SpyPS->GetSpyAbilitySystemComponent() : nullptr);
}
