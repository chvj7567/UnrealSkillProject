// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyPlayerController.h"
#include "Character/SpyCharacter.h"
#include "InputActionValue.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Util/SpyGameplayTags.h"
#include "UI/SpyUIDataAsset.h"
#include "UI/SpyUserWidget.h"
#include "Manager/SpyAssetManager.h"
#include "Manager/SpyUIManager.h"
#include "Util/DefineEnum.h"
#include "System/SpyPlayerState.h"
#include "Character/SpyCharacterMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyPlayerController)

ASpyPlayerController::ASpyPlayerController()
{
}

void ASpyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = true;
	USpyUIManager::Get(this)->OpenUI(ESpyUIType::MainHUD);

	SetMappingContext(DefaultMappingContext);
}

void ASpyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASpyPlayerController::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &ASpyPlayerController::Move);

		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASpyPlayerController::Look);

		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ASpyPlayerController::JumpPressed);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ASpyPlayerController::JumpReleased);
		
		EnhancedInputComponent->BindAction(SkillAction, ETriggerEvent::Started, this, &ASpyPlayerController::UseSkill);
	}
}

void ASpyPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
}

void ASpyPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
}

void ASpyPlayerController::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetPawn()))
	{
		if (ASpyPlayerState* SpyPlayerState = SpyCharacter->GetPlayerState<ASpyPlayerState>())
		{
			const FRotator Rotation = SpyCharacter->GetControlRotation();
			const FRotator YawRotation(0, Rotation.Yaw, 0);

			const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
			const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

			SpyCharacter->AddMovementInput(ForwardDirection, MovementVector.Y);
			SpyCharacter->AddMovementInput(RightDirection, MovementVector.X);

			if (USpyCharacterMovementComponent* SpyParkrourComponent = SpyCharacter->GetSpyCharacterMovementComponent())
			{
				SpyParkrourComponent->SetInputVector(MovementVector);
			}
		}
	}
}

void ASpyPlayerController::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (APawn* ControlledPawn = GetPawn())
	{
		ControlledPawn->AddControllerYawInput(LookAxisVector.X);
		ControlledPawn->AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ASpyPlayerController::JumpPressed(const FInputActionValue& Value)
{
	if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetPawn()))
	{
		SpyCharacter->Jump();
	}
}

void ASpyPlayerController::JumpReleased(const FInputActionValue& Value)
{
	if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetPawn()))
	{
		SpyCharacter->StopJumping();
	}
}

void ASpyPlayerController::UseSkill(const FInputActionValue& Value)
{
	if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetPawn()))
	{
		SpyCharacter->Server_UseSkill(SpyGameplayTags::Skill_A);
	}
}

void ASpyPlayerController::SetMappingContext(UInputMappingContext* InMappingContext)
{
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsystem->AddMappingContext(InMappingContext, 0);
		}
	}
}

void ASpyPlayerController::SetMappingContext()
{
	if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetPawn()))
	{
		if (ASpyPlayerState* SpyPlayerState = SpyCharacter->GetPlayerState<ASpyPlayerState>())
		{
			if (SpyPlayerState->HasState(ESpyPlayerStateFlags::IsClimb))
			{
				if (USpyCharacterMovementComponent* SpyMovementComponent = Cast<USpyCharacterMovementComponent>(SpyCharacter->GetCharacterMovement()))
				{
					if (USpyParkourManagerComponent* ParkourComponent = SpyCharacter->GetSpyParkourManagerComponent())
					{
						SpyMovementComponent->StartWallClimb(ParkourComponent->GetHitNormalVector());
					}
				}

				SetMappingContext(WallClimbMappingContext);
			}
			else
			{
				if (USpyCharacterMovementComponent* SpyMovementComponent = Cast<USpyCharacterMovementComponent>(SpyCharacter->GetCharacterMovement()))
				{
					SpyMovementComponent->EndWallClimb();
				}

				SetMappingContext(DefaultMappingContext);
			}
		}
	}
}
