// Fill out your copyright notice in the Description page of Project Settings.


#include "Input/SpyInputComponent.h"
#include "Util/SpyGameplayTags.h"
#include "System/SpyPlayerState.h"
#include "System/SpyPlayerController.h"
#include "Character/SpyPawnExtensionComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Input/SpyInputConfig.h"
#include "Input/SpyEnhancedInputComponent.h"
#include "Data/SpyCharacterAssetData.h"
#include "Character/SpyCharacterMovementComponent.h"
#include "Character/SpyCharacter.h"

const FName USpyInputComponent::NAME_ActorFeatureName("Input");

USpyInputComponent::USpyInputComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBindingInput = false;
}

void USpyInputComponent::Input_AbilityInputTagPressed(FGameplayTag InputTag)
{
	if (const APawn* Pawn = GetPawn<APawn>())
	{
		if (const USpyPawnExtensionComponent* PawnExtComp = USpyPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			if (USpyAbilitySystemComponent* SpyASC = PawnExtComp->GetSpyAbilitySystemComponent())
			{
				SpyASC->AbilityInputTagPressed(InputTag);
			}
		}
	}
}

void USpyInputComponent::Input_AbilityInputTagReleased(FGameplayTag InputTag)
{
	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	if (const USpyPawnExtensionComponent* PawnExtComp = USpyPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (USpyAbilitySystemComponent* SpyASC = PawnExtComp->GetSpyAbilitySystemComponent())
		{
			SpyASC->AbilityInputTagReleased(InputTag);
		}
	}
}

void USpyInputComponent::InitializePlayerInput(UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);

	const APawn* Pawn = GetPawn<APawn>();
	if (Pawn == nullptr)
		return;

	const APlayerController* PC = GetController<APlayerController>();
	check(PC);

	const ULocalPlayer* LP = Cast<ULocalPlayer>(PC->GetLocalPlayer());
	check(LP);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	check(Subsystem);

	Subsystem->ClearAllMappings();

	if (const USpyPawnExtensionComponent* PawnExtComp = USpyPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (const USpyCharacterAssetData* PawnData = PawnExtComp->GetCharacterAssetData())
		{
			if (const USpyInputConfig* InputConfig = PawnData->CharacterAssets.AssetEntries[0].InputConfig)
			{
				for (auto& Pair : DefaultInputMappings)
				{
					UInputMappingContext* IMC = Pair.Key;
					int32 Priority = Pair.Value;

					if (IMC)
					{
						//# IMC 변경 시 눌려져 있다면 해당 키를 그대로 바뀐 IMC 입력에 사용
						FModifyContextOptions Options = {};
						Options.bIgnoreAllPressedKeysUntilRelease = false;

						Subsystem->AddMappingContext(IMC, Priority, Options);
					}
				}

				if (USpyEnhancedInputComponent* SpyIC = Cast<USpyEnhancedInputComponent>(PlayerInputComponent))
				{
					SpyIC->AddInputMappings(InputConfig, Subsystem);

					TArray<uint32> BindHandles;
					SpyIC->BindAbilityActions(InputConfig, this, &ThisClass::Input_AbilityInputTagPressed, &ThisClass::Input_AbilityInputTagReleased, BindHandles);

					SpyIC->BindNativeAction(InputConfig, SpyGameplayTags::Input_Native_Move, ETriggerEvent::Triggered, this, &ThisClass::Move);
					SpyIC->BindNativeAction(InputConfig, SpyGameplayTags::Input_Native_Look, ETriggerEvent::Triggered, this, &ThisClass::Look);
				}
			}
		}
	}

	bIsBindingInput = true;

	/*UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(const_cast<APlayerController*>(PC), NAME_BindInputsNow);
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(const_cast<APawn*>(Pawn), NAME_BindInputsNow);*/
}

void USpyInputComponent::Move(const FInputActionValue& InValue)
{
	FVector2D MovementVector = InValue.Get<FVector2D>();

	if (APawn* Pawn = GetPawn<APawn>())
	{
		if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(Pawn))
		{
			if (ASpyPlayerState* SpyPlayerState = SpyCharacter->GetPlayerState<ASpyPlayerState>())
			{
				//# Move Lock 확인
				if (SpyPlayerState->GetAbilitySystemComponent()->HasMatchingGameplayTag(SpyGameplayTags::Lock_Input_Move))
					return;

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
}

void USpyInputComponent::Look(const FInputActionValue& InValue)
{
	FVector2D LookAxisVector = InValue.Get<FVector2D>();

	if (APawn* Pawn = GetPawn<APawn>())
	{
		if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(Pawn))
		{
			if (ASpyPlayerState* SpyPlayerState = SpyCharacter->GetPlayerState<ASpyPlayerState>())
			{
				//# Look Lock 확인
				if (SpyPlayerState->GetAbilitySystemComponent()->HasMatchingGameplayTag(SpyGameplayTags::Lock_Input_Look))
					return;
			}

			SpyCharacter->AddControllerYawInput(LookAxisVector.X);
			SpyCharacter->AddControllerPitchInput(LookAxisVector.Y);
		}
	}
}

void USpyInputComponent::OnRegister()
{
	Super::OnRegister();

	//# 상태 머신 등록
	RegisterInitStateFeature();
}

void USpyInputComponent::BeginPlay()
{
}

void USpyInputComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
}

bool USpyInputComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const
{
	check(Manager);

	APawn* Pawn = GetPawn<APawn>();

	if (CurrentState.IsValid() == false && DesiredState == SpyGameplayTags::InitState_Spawned)
	{
		if (Pawn)
			return true;
	}
	else if (CurrentState == SpyGameplayTags::InitState_Spawned && DesiredState == SpyGameplayTags::InitState_DataAvailable)
	{
		if (GetPlayerState<ASpyPlayerState>() == nullptr)
			return false;

		//# 자기 혹은 서버인지 확인
		if (Pawn->GetLocalRole() != ROLE_SimulatedProxy)
		{
			AController* Controller = GetController<AController>();

			const bool bHasControllerPairedWithPS = (Controller != nullptr) && \
				(Controller->PlayerState != nullptr) && \
				(Controller->PlayerState->GetOwner() == Controller);

			if (bHasControllerPairedWithPS == false)
				return false;
		}

		const bool bIsLocallyControlled = Pawn->IsLocallyControlled();
		const bool bIsBot = Pawn->IsBotControlled();
		if (bIsLocallyControlled && bIsBot == false)
		{
			ASpyPlayerController* SpyPC = GetController<ASpyPlayerController>();
			if (Pawn->InputComponent == false || SpyPC == nullptr || SpyPC->GetLocalPlayer() == nullptr)
				return false;
		}

		return true;
	}
	else if (CurrentState == SpyGameplayTags::InitState_DataAvailable && DesiredState == SpyGameplayTags::InitState_DataInitialized)
	{
		ASpyPlayerState* SpyPS = GetPlayerState<ASpyPlayerState>();
		return SpyPS && Manager->HasFeatureReachedInitState(Pawn, USpyPawnExtensionComponent::NAME_ActorFeatureName, SpyGameplayTags::InitState_DataInitialized);
	}
	else if (CurrentState == SpyGameplayTags::InitState_DataInitialized && DesiredState == SpyGameplayTags::InitState_GameplayReady)
	{
		return true;
	}

	return false;
}

void USpyInputComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState)
{
	if (CurrentState == SpyGameplayTags::InitState_DataAvailable && DesiredState == SpyGameplayTags::InitState_DataInitialized)
	{
		APawn* Pawn = GetPawn<APawn>();
		ASpyPlayerState* SpyPS = GetPlayerState<ASpyPlayerState>();
		if (!ensure(Pawn && SpyPS))
		{
			return;
		}

		const USpyCharacterAssetData* CharacterAssetData = nullptr;

		if (USpyPawnExtensionComponent* PawnExtComp = USpyPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			CharacterAssetData = PawnExtComp->GetCharacterAssetData();
			PawnExtComp->InitializeAbilitySystem(SpyPS->GetSpyAbilitySystemComponent(), SpyPS);
		}

		if (ASpyPlayerController* SpyPC = GetController<ASpyPlayerController>())
		{
			if (Pawn->InputComponent != nullptr)
			{
				InitializePlayerInput(Pawn->InputComponent);
			}
		}
	}
}

void USpyInputComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
}

void USpyInputComponent::CheckDefaultInitialization()
{
}
