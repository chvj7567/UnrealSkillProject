// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillProjectPlayerController.h"
#include "SkillProjectCharacter.h"
#include "InputActionValue.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SkillProjectPlayerController)

void ASkillProjectPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ASkillProjectPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASkillProjectPlayerController::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASkillProjectPlayerController::Look);

		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ASkillProjectPlayerController::JumpPressed);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ASkillProjectPlayerController::JumpReleased);
		
		EnhancedInputComponent->BindAction(SkillAction, ETriggerEvent::Started, this, &ASkillProjectPlayerController::UseSkill);
	}
}

void ASkillProjectPlayerController::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (APawn* ControlledPawn = GetPawn())
	{
		const FRotator Rotation = ControlledPawn->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		ControlledPawn->AddMovementInput(ForwardDirection, MovementVector.Y);
		ControlledPawn->AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ASkillProjectPlayerController::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (APawn* ControlledPawn = GetPawn())
	{
		ControlledPawn->AddControllerYawInput(LookAxisVector.X);
		ControlledPawn->AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ASkillProjectPlayerController::JumpPressed(const FInputActionValue& Value)
{
	if (ASkillProjectCharacter* SkillProjectCharacter = Cast<ASkillProjectCharacter>(GetPawn()))
	{
		SkillProjectCharacter->Jump();
	}
}

void ASkillProjectPlayerController::JumpReleased(const FInputActionValue& Value)
{
	if (ASkillProjectCharacter* SkillProjectCharacter = Cast<ASkillProjectCharacter>(GetPawn()))
	{
		SkillProjectCharacter->StopJumping();
	}
}

void ASkillProjectPlayerController::UseSkill(const FInputActionValue& Value)
{
	if (ASkillProjectCharacter* SkillProjectCharacter = Cast<ASkillProjectCharacter>(GetPawn()))
	{
		SkillProjectCharacter->Server_UseSkill();
	}
}