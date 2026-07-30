// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyPlayerController.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Character/SpyCharacter.h"
#include "Data/SpyCharacterConfig.h"
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
	bCursorMode = (bCursorMode == false);
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

	TargetingComp = GetPawn()->FindComponentByClass<USpyTargetingManagerComponent>();
}

void ASpyPlayerController::AcknowledgePossession(APawn* InPawn)
{
	Super::AcknowledgePossession(InPawn);

	TargetingComp = GetPawn()->FindComponentByClass<USpyTargetingManagerComponent>();

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

	APawn* ControlledPawn = GetPawn();
	AActor* TargetActor = (ControlledPawn != nullptr && TargetingComp != nullptr) ? TargetingComp->GetTarget().Get() : nullptr;

	//# 타겟팅 중에는 Yaw(좌우) 만 타겟이 소유하고, Pitch(상하) 는 플레이어 입력에 맡긴다
	const bool bTargetLocked = (TargetActor != nullptr);

	if (bTargetLocked)
	{
		//# Super 가 만드는 DeltaRot 에서 yaw 성분을 제거 — 컨트롤 회전 Yaw 가 입력으로 움직이지 않게
		//# 전제: 플레이어 폰은 bUseControllerRotationYaw == false. 이를 켜게 되면 Super 이후 보간된 Yaw 로 FaceRotation 을 재적용해야 한다
		RotationInput.Yaw = 0.f;
	}

	//# 엔진 표준 경로 — RotationInput 소비 + PlayerCameraManager 의 ViewPitchMin/Max 클램프(LimitViewPitch)
	Super::UpdateRotation(DeltaTime);

	if (bTargetLocked == false)
		return;

	//# Yaw 만 사용하므로 높이 차(Z) 는 결과에 영향을 주지 않는다
	const FVector LookDir = TargetActor->GetActorLocation() - ControlledPawn->GetActorLocation();

	//# 수직으로 겹친 프레임 — Yaw 를 0 으로 스냅시키지 않고 현재 값을 유지
	if (LookDir.SizeSquared2D() <= KINDA_SMALL_NUMBER)
		return;

	const FRotator CurrentRot = GetControlRotation();
	const FRotator TargetRot = FRotator(0.f, LookDir.Rotation().Yaw, 0.f);

	//# Yaw 스칼라만 보간 — RInterpTo 가 델타를 정규화해 ±180° 경계에서 최단 경로로 돈다
	const float SoftYaw = FMath::RInterpTo(FRotator(0.f, CurrentRot.Yaw, 0.f), TargetRot, DeltaTime, 10.f).Yaw;

	//# Pitch 는 Super 가 클램프한 값 그대로 유지, Roll 은 0
	SetControlRotation(FRotator(CurrentRot.Pitch, SoftYaw, 0.f));
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