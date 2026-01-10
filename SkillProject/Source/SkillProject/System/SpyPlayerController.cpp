// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyPlayerController.h"
#include "Character/SpyCharacter.h"
#include "InputActionValue.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Util/SpyGameplayTags.h"
#include "SKGameplayTags.h"
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
		
		EnhancedInputComponent->BindAction(SkillAAction, ETriggerEvent::Started, this, &ASpyPlayerController::UseSkillA);
		EnhancedInputComponent->BindAction(SkillBAction, ETriggerEvent::Started, this, &ASpyPlayerController::UseSkillB);
		EnhancedInputComponent->BindAction(SkillCAction, ETriggerEvent::Started, this, &ASpyPlayerController::UseSkillC);
		EnhancedInputComponent->BindAction(SkillDAction, ETriggerEvent::Started, this, &ASpyPlayerController::UseSkillD);
		EnhancedInputComponent->BindAction(SkillEAction, ETriggerEvent::Started, this, &ASpyPlayerController::UseSkillE);
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

void ASpyPlayerController::Move(const FInputActionValue& InValue)
{
	FVector2D MovementVector = InValue.Get<FVector2D>();

	if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetPawn()))
	{
		//# Move Lock 확인
		if (USKAbilitySystemComponent* ASC = SpyCharacter->GetSKAbilitySystemComponent())
		{
			if (ASC->HasMatchingGameplayTag(SKGameplayTags::Lock_Move))
				return;
		}

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

void ASpyPlayerController::Look(const FInputActionValue& InValue)
{
	FVector2D LookAxisVector = InValue.Get<FVector2D>();

	if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetPawn()))
	{
		//# Move Lock 확인
		if (USKAbilitySystemComponent* ASC = SpyCharacter->GetSKAbilitySystemComponent())
		{
			if (ASC->HasMatchingGameplayTag(SKGameplayTags::Lock_Look))
				return;
		}

		SpyCharacter->AddControllerYawInput(LookAxisVector.X);
		SpyCharacter->AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ASpyPlayerController::JumpPressed(const FInputActionValue& InValue)
{
	if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetPawn()))
	{
		SpyCharacter->Jump();
	}
}

void ASpyPlayerController::JumpReleased(const FInputActionValue& InValue)
{
	if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetPawn()))
	{
		SpyCharacter->StopJumping();
	}
}

void ASpyPlayerController::UseSkillA(const FInputActionValue& InValue)
{
	if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetPawn()))
	{
		SpyCharacter->Server_UseSkill(SpyGameplayTags::Skill_Action_A);
	}
}

void ASpyPlayerController::UseSkillB(const FInputActionValue& InValue)
{
	if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetPawn()))
	{
		SpyCharacter->Server_UseSkill(SpyGameplayTags::Skill_Action_B);
	}
}

void ASpyPlayerController::UseSkillC(const FInputActionValue& InValue)
{
	if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetPawn()))
	{
		SpyCharacter->Server_UseSkill(SpyGameplayTags::Skill_Action_C);
	}
}

void ASpyPlayerController::UseSkillD(const FInputActionValue& InValue)
{
	if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetPawn()))
	{
		SpyCharacter->Server_UseSkill(SpyGameplayTags::Skill_Action_D);
	}
}

void ASpyPlayerController::UseSkillE(const FInputActionValue& InValue)
{
	if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetPawn()))
	{
		SpyCharacter->Server_UseSkill(SpyGameplayTags::Skill_Action_E);
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

void ASpyPlayerController::RefreshMappingContext()
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
						SpyMovementComponent->StartWallClimb(ParkourComponent->GetClimbData(), ParkourComponent->GetClimbWallData());
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
