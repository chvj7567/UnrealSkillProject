// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyPlayerController.h"
#include "Camera/CameraShakeBase.h"
#include "Character/SpyHealthComponent.h"
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
#include "ManagerComponent/SpyTargetingManagerComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyPlayerController)

ASpyPlayerController::ASpyPlayerController()
{
}

void ASpyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = false;

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);

	USpyUIManager::Get(this)->OpenSpyUI(ESpyUIType::MainHUD);
}

void ASpyPlayerController::ToggleCursorMode()
{
	bCursorMode = !bCursorMode;
	bShowMouseCursor = bCursorMode;

	if (bCursorMode)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
	}
	else
	{
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
	}
}

void ASpyPlayerController::OnPossess(APawn* InPawn)
{
	if (BoundHealthComponent.IsValid())
	{
		BoundHealthComponent->OnHit.RemoveDynamic(this, &ASpyPlayerController::HandleReceivedHit);
		BoundHealthComponent = nullptr;
	}

	Super::OnPossess(InPawn);

	TargetingComp = GetPawn()->FindComponentByClass<USpyTargetingManagerComponent>();

	if (USpyHealthComponent* HC = USpyHealthComponent::FindHealthComponent(InPawn))
	{
		HC->OnHit.AddDynamic(this, &ASpyPlayerController::HandleReceivedHit);
		BoundHealthComponent = HC;
	}
}

void ASpyPlayerController::OnUnPossess()
{
	if (BoundHealthComponent.IsValid())
	{
		BoundHealthComponent->OnHit.RemoveDynamic(this, &ASpyPlayerController::HandleReceivedHit);
		BoundHealthComponent = nullptr;
	}

	Super::OnUnPossess();
}

void ASpyPlayerController::AcknowledgePossession(APawn* InPawn)
{
	Super::AcknowledgePossession(InPawn);

	TargetingComp = GetPawn()->FindComponentByClass<USpyTargetingManagerComponent>();
}

void ASpyPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
}

void ASpyPlayerController::UpdateRotation(float DeltaTime)
{
	if (USpyAbilitySystemComponent* ASC = GetSpyAbilitySystemComponent())
	{
		if (ASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Death))
			return;
	}

	if (TargetingComp != nullptr && TargetingComp->GetTarget().IsValid())
	{
		FVector LookDir = TargetingComp->GetTarget()->GetActorLocation() - GetPawn()->GetActorLocation();
		LookDir.Z -= 100.f;

		FRotator TargetRot = LookDir.Rotation();
		FRotator CurrentRot = GetControlRotation();
		FRotator SoftRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, 10.f);

		SetControlRotation(SoftRot);
	}
	else
	{
		Super::UpdateRotation(DeltaTime);
	}
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

void ASpyPlayerController::HandleReceivedHit(float Damage, bool bCritical, AActor* DamageCauser)
{
	Client_TriggerShake(bCritical, true);
}

void ASpyPlayerController::HandleDealtHit(bool bCritical)
{
	Client_TriggerShake(bCritical, false);
}

void ASpyPlayerController::Client_TriggerShake_Implementation(bool bCritical, bool bFromReceivedHit)
{
	const TCHAR* Source = bFromReceivedHit ? TEXT("피격") : TEXT("공격");
	const TCHAR* Intensity = bCritical ? TEXT("Heavy") : TEXT("Light");
	UE_LOG(LogTemp, Warning, TEXT("[CameraShake] %s — %s | CameraManager=%d ShakeLight=%d ShakeHeavy=%d"),
		Source, Intensity, PlayerCameraManager != nullptr, HitShakeLight != nullptr, HitShakeHeavy != nullptr);

	if (!PlayerCameraManager) return;

	TSubclassOf<UCameraShakeBase> ShakeClass = bCritical ? HitShakeHeavy : HitShakeLight;
	if (ShakeClass)
	{
		PlayerCameraManager->StartCameraShake(ShakeClass);
		UE_LOG(LogTemp, Warning, TEXT("[CameraShake] StartCameraShake 완료 (%s %s)"), Source, Intensity);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[CameraShake] ShakeClass null (%s %s) — BP 에셋 할당 확인"), Source, Intensity);
	}
}