// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyPlayerController.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Character/SpyCharacter.h"
#include "Data/SpyCharacterConfig.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
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

	bShowMouseCursor = false;

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);

	//# 데디 서버의 원격 PC 는 뷰포트가 없다 — HUD 는 로컬 컨트롤러에서만 연다
	if (IsLocalPlayerController())
	{
		if (USpyUIManager* UIMgr = USpyUIManager::Get(this))
		{
			UIMgr->OpenSpyUI(ESpyUIType::MainHUD);
		}
	}
}

void ASpyPlayerController::ToggleCursorMode()
{
	SetCursorMode(bCursorMode == false);
}

void ASpyPlayerController::SetCursorMode(bool bEnabled)
{
	bCursorMode = bEnabled;
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
	Super::OnPossess(InPawn);
}

void ASpyPlayerController::AcknowledgePossession(APawn* InPawn)
{
	Super::AcknowledgePossession(InPawn);

	if (PlayerCameraManager)
	{
		if (ASpyCharacter* SpyChar = Cast<ASpyCharacter>(InPawn))
		{
			if (USpyCharacterConfig* Config = SpyChar->GetCharacterConfig())
			{
				PlayerCameraManager->ViewPitchMin = Config->ViewPitchMin;
				PlayerCameraManager->ViewPitchMax = Config->ViewPitchMax;
			}
		}
	}
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

	//# 엔진 표준 경로 — RotationInput 소비 + PlayerCameraManager 의 ViewPitchMin/Max 클램프(LimitViewPitch)
	Super::UpdateRotation(DeltaTime);
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

void ASpyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	//# 시스템 UI 토글이라 게임플레이 어빌리티 입력 경로 대신 레거시 키 바인딩을 직접 사용
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ASpyPlayerController::HandleEscapePressed);
}

void ASpyPlayerController::HandleEscapePressed()
{
	USpyUIManager* UIMgr = USpyUIManager::Get(this);
	if (UIMgr == nullptr)
		return;

	if (UIMgr->IsSpyUIOpen(ESpyUIType::QuitConfirm))
	{
		UIMgr->CloseSpyUI(ESpyUIType::QuitConfirm);
		HandleQuitConfirmClosed();
	}
	else
	{
		UIMgr->OpenSpyUI(ESpyUIType::QuitConfirm);

		//# 마우스 클릭으로 Yes/No 를 눌러야 한다 — 커서가 꺼진 채로는 Slate 로 입력이 안 들어간다
		//# (SetMissionCardCursorMode 와 동일 사유, SpyInteractionComponent.cpp:329-330)
		SetCursorMode(true);
	}
}

void ASpyPlayerController::HandleQuitConfirmClosed()
{
	USpyUIManager* UIMgr = USpyUIManager::Get(this);

	//# 미션카드 등 다른 UI 가 여전히 커서를 요구하면 그 상태를 덮어쓰지 않는다
	if (UIMgr != nullptr && UIMgr->IsSpyUIOpen(ESpyUIType::MissionOffer))
		return;

	SetCursorMode(false);
}

USpyAbilitySystemComponent* ASpyPlayerController::GetSpyAbilitySystemComponent() const
{
	const ASpyPlayerState* SpyPS = CastChecked<ASpyPlayerState>(PlayerState, ECastCheckedType::NullAllowed);
	return (SpyPS ? SpyPS->GetSpyAbilitySystemComponent() : nullptr);
}

void ASpyPlayerController::Client_TriggerShake_Implementation(bool bCritical, bool bFromReceivedHit)
{
	TriggerShakeLocal(bCritical, bFromReceivedHit);
}

void ASpyPlayerController::TriggerShakeLocal(bool bCritical, bool bFromReceivedHit)
{
	const TCHAR* Source = bFromReceivedHit ? TEXT("피격") : TEXT("공격");
	const TCHAR* Intensity = bCritical ? TEXT("Heavy") : TEXT("Light");
	UE_LOG(LogTemp, Warning, TEXT("[CameraShake] %s — %s | CameraManager=%d ShakeLight=%d ShakeHeavy=%d"),
		Source, Intensity, PlayerCameraManager != nullptr, HitShakeLight != nullptr, HitShakeHeavy != nullptr);

	if (PlayerCameraManager == nullptr) return;

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