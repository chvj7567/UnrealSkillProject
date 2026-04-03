// Copyright Epic Games, Inc. All Rights Reserved.

#include "SpyCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "AbilitySystemComponent.h"
#include "Util/SpyGameplayTags.h"
#include "Attribute/SKAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/WidgetComponent.h"
#include "Manager/SpyUIManager.h"
#include "UI/SpyHPBar.h"
#include "Character/AnimInstance/SpyCharacterAnimInstance.h"
#include "System/SpyPlayerState.h"
#include "Character/SpyCharacterMovementComponent.h"
#include "ManagerComponent/SpyParkourManagerComponent.h"
#include "ManagerComponent/SpyAnimManagerComponent.h"
#include "Item/SpyWeapon.h"
#include "Manager/SpyAssetManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "System/SpyPlayerController.h"
#include "Character/SpyCharacterAttributeSet.h"
#include "Character/SpyPawnExtensionComponent.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "Character/SpyHealthComponent.h"
#include "SKGameplayTags.h"
#include "ManagerComponent/SpyTargetingManagerComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyCharacter)

ASpyCharacter::ASpyCharacter(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer.SetDefaultSubobjectClass<USpyCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bReplicates = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	GetCharacterMovement()->RotationRate.Yaw = 1080.f;
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;	
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bDoCollisionTest = false;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	HPBarComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPBarComponent"));
	HPBarComponent->SetupAttachment(GetMesh());
	HPBarComponent->SetRelativeLocation(FVector(0, 0, 200.f));

	SpyPawnExtensionComponent = CreateDefaultSubobject<USpyPawnExtensionComponent>(TEXT("SpyPawnExtensionComponent"));
	SpyPawnExtensionComponent->OnAbilitySystemInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemInitialized));
	SpyPawnExtensionComponent->OnAbilitySystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemUninitialized));

	SpyHealthComponent = CreateDefaultSubobject<USpyHealthComponent>(TEXT("HealthComponent"));
	SpyHealthComponent->OnHealthChanged.AddDynamic(this, &ThisClass::OnHealthChanged);
	SpyHealthComponent->OnDeath.AddDynamic(this, &ThisClass::OnDeath);
}

void ASpyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(WeaponSpawnTimerHandle, this, &ASpyCharacter::SpawnAndAttachWeapon, 1.0f, false);
	}
}

void ASpyCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	SpyPawnExtensionComponent->HandleControllerChanged();

	//# 조종하는 컨트롤러가 AI인지 확인
	if (NewController->IsPlayerController() == false)
	{
		//# AI 전용 회전 설정
		bUseControllerRotationYaw = true;
		GetCharacterMovement()->bOrientRotationToMovement = false;
	}
}

void ASpyCharacter::UnPossessed()
{
	Super::UnPossessed();

	SpyPawnExtensionComponent->HandleControllerChanged();
}

void ASpyCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	USpyUIManager::Get(this)->OpenSubSpyUI(ESpyUIType::HpBar, HPBarComponent, EWidgetSpace::Screen);

	SpyPawnExtensionComponent->HandleControllerChanged();
}

void ASpyCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	SpyPawnExtensionComponent->HandlePlayerStateReplicated();
}

void ASpyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	SpyPawnExtensionComponent->SetupPlayerInputComponent();
}

UAbilitySystemComponent* ASpyCharacter::GetAbilitySystemComponent() const
{
	if (SpyPawnExtensionComponent == nullptr)
		return nullptr;

	return SpyPawnExtensionComponent->GetSpyAbilitySystemComponent();
}

void ASpyCharacter::InitializeGameplayTags()
{
	if (USpyAbilitySystemComponent* SpyASC = GetSpyAbilitySystemComponent())
	{
		for (const TPair<uint8, FGameplayTag>& TagMapping : SpyGameplayTags::MovementModeTagMap)
		{
			if (TagMapping.Value.IsValid())
			{
				SpyASC->SetLooseGameplayTagCount(TagMapping.Value, 0);
			}
		}

		for (const TPair<uint8, FGameplayTag>& TagMapping : SpyGameplayTags::CustomMovementModeTagMap)
		{
			if (TagMapping.Value.IsValid())
			{
				SpyASC->SetLooseGameplayTagCount(TagMapping.Value, 0);
			}
		}

		USpyCharacterMovementComponent* SpyMoveComp = CastChecked<USpyCharacterMovementComponent>(GetCharacterMovement());
		SetMovementModeTag(SpyMoveComp->MovementMode, SpyMoveComp->CustomMovementMode, true);
	}
}

USpyAbilitySystemComponent* ASpyCharacter::GetSpyAbilitySystemComponent() const
{
	return Cast<USpyAbilitySystemComponent>(GetAbilitySystemComponent());
}

void ASpyCharacter::OnHealthChanged(USpyHealthComponent* InHealthComponent, float InOldValue, float InNewValue, AActor* InInstigator)
{
	if (HasAuthority())
		return;

	if (USpyHPBar* HpBar = Cast<USpyHPBar>(HPBarComponent->GetWidget()))
	{
		HpBar->UpdateHP(InNewValue, InHealthComponent->GetMaxHealth());
		UE_LOG(LogTemp, Log, TEXT("# [SpyCharacter] OnHealthChanged: %f -> %f"), InOldValue, InNewValue);
	}
}

void ASpyCharacter::OnDeath(AActor* InOwningActor, AActor* InCauserActor)
{
	if (UCapsuleComponent* CapsuleComp = GetCapsuleComponent())
	{
		CapsuleComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	}

	if (USpyTargetingManagerComponent* TargetingComp = FindComponentByClass<USpyTargetingManagerComponent>())
	{
		TargetingComp->SetCurrentTarget(nullptr);
	}

	UE_LOG(LogTemp, Log, TEXT("# [SpyCharacter] OnDeath: %s"), *GetName());
}

void ASpyCharacter::OnAbilitySystemInitialized()
{
	USpyAbilitySystemComponent* SpyASC = GetSpyAbilitySystemComponent();
	check(SpyASC);

	SpyHealthComponent->InitializeByAbilitySystem(SpyASC);

	InitializeGameplayTags();
}

void ASpyCharacter::OnAbilitySystemUninitialized()
{
	SpyHealthComponent->UnInitializeByAbilitySystem();
}

void ASpyCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	USpyCharacterMovementComponent* SpyMoveComp = CastChecked<USpyCharacterMovementComponent>(GetCharacterMovement());

	//# 이전 상태는 TagCount 0으로 설정
	SetMovementModeTag(PrevMovementMode, PreviousCustomMode, false);

	//# 신규 상태는 TagCount 1로 설정
	SetMovementModeTag(SpyMoveComp->MovementMode, SpyMoveComp->CustomMovementMode, true);
}

void ASpyCharacter::SetMovementModeTag(EMovementMode MovementMode, uint8 CustomMovementMode, bool bTagEnabled)
{
	if (USpyAbilitySystemComponent* SpyASC = GetSpyAbilitySystemComponent())
	{
		const FGameplayTag* MovementModeTag = nullptr;
		if (MovementMode == MOVE_Custom)
		{
			MovementModeTag = SpyGameplayTags::CustomMovementModeTagMap.Find(CustomMovementMode);
		}
		else
		{
			MovementModeTag = SpyGameplayTags::MovementModeTagMap.Find(MovementMode);
		}

		if (MovementModeTag && MovementModeTag->IsValid())
		{
			SpyASC->SetLooseGameplayTagCount(*MovementModeTag, (bTagEnabled ? 1 : 0));
		}
	}
}

void ASpyCharacter::SpawnAndAttachWeapon()
{
	//# Temp
	const USpyAssetData& AssetData = USpyAssetManager::Get().GetAssetData();
	const FSoftObjectPath& AssetPath = AssetData.GetAssetPathByName(FName("OneHandSword"));

	FSpyAssetAndDelegate LoadDelegate;
	LoadDelegate.BindLambda([this](UObject* LoadedAsset)
		{
			if (LoadedAsset == nullptr)
				return;

			if (TSubclassOf<ASpyWeapon> SpyWeaponClass = USpyAssetManager::GetSubclassByName<ASpyWeapon>(FName("OneHandSword")))
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = this;
				SpawnParams.Instigator = this;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

				SpyWeapon = GetWorld()->SpawnActor<ASpyWeapon>(SpyWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
				SpyWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("weapon_socket"));
			}

		});

	USpyAssetManager::LoadAssetAsync(AssetPath, LoadDelegate);
}
