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
	if (const APawn* Pawn = GetPawn<APawn>())
	{
		if (const USpyPawnExtensionComponent* PawnExtComp = USpyPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			if (USpyAbilitySystemComponent* SpyASC = PawnExtComp->GetSpyAbilitySystemComponent())
			{
				SpyASC->AbilityInputTagReleased(InputTag);
			}
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
			for (auto& Pair : PawnData->CharacterAssets.AssetEntries[0].InputMappingContexts)
			{
				UInputMappingContext* IMC = Pair.Key;
				int32 Priority = Pair.Value;

				if (IMC)
				{
					//# IMC ���� �� ������ �ִٸ� �ش� Ű�� �״�� �ٲ� IMC �Է¿� ���
					FModifyContextOptions Options = {};
					Options.bIgnoreAllPressedKeysUntilRelease = false;

					Subsystem->AddMappingContext(IMC, Priority, Options);
				}
			}
			
			if (const USpyInputConfig* InputConfig = PawnData->CharacterAssets.AssetEntries[0].InputConfig)
			{
				if (USpyEnhancedInputComponent* SpyIC = Cast<USpyEnhancedInputComponent>(PlayerInputComponent))
				{
					SpyIC->AddInputMappings(InputConfig, Subsystem);

					TArray<uint32> BindHandles;
					SpyIC->BindAbilityActions(InputConfig, this, &ThisClass::Input_AbilityInputTagPressed, &ThisClass::Input_AbilityInputTagReleased, BindHandles);

					SpyIC->BindNativeAction(InputConfig, SpyGameplayTags::Input_Native_Move, ETriggerEvent::Triggered, this, &ThisClass::Move);
					SpyIC->BindNativeAction(InputConfig, SpyGameplayTags::Input_Native_Move, ETriggerEvent::Completed, this, &ThisClass::MoveEnd);
					SpyIC->BindNativeAction(InputConfig, SpyGameplayTags::Input_Native_Look, ETriggerEvent::Triggered, this, &ThisClass::Look);
					SpyIC->BindNativeAction(InputConfig, SpyGameplayTags::Input_Native_CursorToggle, ETriggerEvent::Started, this, &ThisClass::CursorToggle);
				}
			}
		}
	}

	bIsBindingInput = true;

	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(const_cast<APlayerController*>(PC), NAME_ActorFeatureName);
}

void USpyInputComponent::ReplaceMappingContext(UInputMappingContext* InMappingContext, int32 Priority)
{
	const APawn* Pawn = GetPawn<APawn>();
	if (Pawn == nullptr)
		return;

	const APlayerController* PC = GetController<APlayerController>();
	if (PC == nullptr)
		return;

	const ULocalPlayer* LP = Cast<ULocalPlayer>(PC->GetLocalPlayer());
	if (LP == nullptr)
		return;

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		Subsystem->ClearAllMappings();
		Subsystem->AddMappingContext(InMappingContext, Priority);
	}
}

void USpyInputComponent::AddMappingContext(UInputMappingContext* InMappingContext, int32 Priority)
{
	const APawn* Pawn = GetPawn<APawn>();
	if (Pawn == nullptr)
		return;

	const APlayerController* PC = GetController<APlayerController>();
	if (PC == nullptr)
		return;

	const ULocalPlayer* LP = Cast<ULocalPlayer>(PC->GetLocalPlayer());
	if (LP == nullptr)
		return;

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		Subsystem->AddMappingContext(InMappingContext, Priority);
	}
}

void USpyInputComponent::RemoveMappingContext(UInputMappingContext* InMappingContext)
{
	const APawn* Pawn = GetPawn<APawn>();
	if (Pawn == nullptr)
		return;

	const APlayerController* PC = GetController<APlayerController>();
	if (PC == nullptr)
		return;

	const ULocalPlayer* LP = Cast<ULocalPlayer>(PC->GetLocalPlayer());
	if (LP == nullptr)
		return;

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		Subsystem->RemoveMappingContext(InMappingContext);
	}
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
				//# Death Ȯ��
				if (SpyPlayerState->GetAbilitySystemComponent()->HasMatchingGameplayTag(SKGameplayTags::Character_State_Death))
					return;

				//# Move Lock Ȯ��
				if (SpyPlayerState->GetAbilitySystemComponent()->HasMatchingGameplayTag(SpyGameplayTags::Lock_Input_Move))
					return;

				const FRotator Rotation = SpyCharacter->GetControlRotation();
				const FRotator YawRotation(0, Rotation.Yaw, 0);

				const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
				const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

				SpyCharacter->AddMovementInput(ForwardDirection, MovementVector.Y);
				SpyCharacter->AddMovementInput(RightDirection, MovementVector.X);
			}

			if (USpyCharacterMovementComponent* SpyParkrourComponent = SpyCharacter->GetSpyCharacterMovementComponent())
			{
				SpyParkrourComponent->SetWallClimbInput(MovementVector);
			}
		}
	}
}

void USpyInputComponent::MoveEnd(const FInputActionValue& InValue)
{
	if (APawn* Pawn = GetPawn<APawn>())
	{
		if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(Pawn))
		{
			if (USpyCharacterMovementComponent* SpyParkrourComponent = SpyCharacter->GetSpyCharacterMovementComponent())
			{
				SpyParkrourComponent->SetWallClimbInput(FVector2D::Zero());
			}
		}
	}
}

void USpyInputComponent::CursorToggle(const FInputActionValue& InValue)
{
	if (ASpyPlayerController* SpyPC = GetController<ASpyPlayerController>())
	{
		SpyPC->ToggleCursorMode();
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

	//# ���� �ӽ� ���
	RegisterInitStateFeature();
}

void USpyInputComponent::BeginPlay()
{
	Super::BeginPlay();

	//# ���� ��ȭ �˸� ���
	BindOnActorInitStateChanged(USpyPawnExtensionComponent::NAME_ActorFeatureName, FGameplayTag(), false);

	//# InitState_Spawned ���·� ��ȯ �õ�
	TryToChangeInitState(SpyGameplayTags::InitState_Spawned);

	CheckDefaultInitialization();
}

void USpyInputComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	//# ���� �ӽ� ����
	UnregisterInitStateFeature();

	Super::EndPlay(EndPlayReason);
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

		//# ���� �÷��̾� Ȥ�� �������� Ȯ��
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

		//# ���� �ƴ� ���� �÷��̾����� Ȯ��
		if (bIsLocallyControlled && bIsBot == false)
		{
			ASpyPlayerController* SpyPC = GetController<ASpyPlayerController>();
			if (Pawn->InputComponent == nullptr || SpyPC == nullptr || SpyPC->GetLocalPlayer() == nullptr)
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
	if (Params.FeatureName == USpyPawnExtensionComponent::NAME_ActorFeatureName)
	{
		CheckDefaultInitialization();
	}
}

void USpyInputComponent::CheckDefaultInitialization()
{
	static const TArray<FGameplayTag> StateChain =
	{
		SpyGameplayTags::InitState_Spawned,
		SpyGameplayTags::InitState_DataAvailable,
		SpyGameplayTags::InitState_DataInitialized,
		SpyGameplayTags::InitState_GameplayReady
	};

	ContinueInitStateChain(StateChain);
}
