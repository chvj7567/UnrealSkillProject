// Copyright Epic Games, Inc. All Rights Reserved.

#include "SpyCharacter.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "Components/WidgetComponent.h"
#include "Util/SpyGameplayTags.h"
#include "Manager/SpyUIManager.h"
#include "UI/SpyHPBar.h"
#include "Item/SpyWeapon.h"
#include "Manager/SpyAssetManager.h"
#include "Character/SpyPawnExtensionComponent.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "Character/SpyHealthComponent.h"
#include "ManagerComponent/SpyTargetingManagerComponent.h"
#include "Data/SpyCharacterConfig.h"
#include "Data/SpyCharacterAssetData.h"

#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyCharacter)

void ASpyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASpyCharacter, SpyWeapon);
}

ASpyCharacter::ASpyCharacter(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer.SetDefaultSubobjectClass<USpyCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bReplicates = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 1080.f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bDoCollisionTest = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	HPBarComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPBarComponent"));
	HPBarComponent->SetupAttachment(GetMesh());
	HPBarComponent->SetRelativeLocation(FVector(0.f, 0.f, 200.f));

	SpyPawnExtensionComponent = CreateDefaultSubobject<USpyPawnExtensionComponent>(TEXT("SpyPawnExtensionComponent"));
	SpyPawnExtensionComponent->OnAbilitySystemInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemInitialized));
	SpyPawnExtensionComponent->OnAbilitySystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemUninitialized));

	SpyHealthComponent = CreateDefaultSubobject<USpyHealthComponent>(TEXT("HealthComponent"));
	SpyHealthComponent->OnHealthChanged.AddDynamic(this, &ThisClass::OnHealthChanged);
	SpyHealthComponent->OnDeath.AddDynamic(this, &ThisClass::OnDeath);
}

void ASpyCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	//# BP 기본값이 적용된 이후 시점이므로 CharacterConfig를 신뢰할 수 있음
	if (CharacterConfig)
	{
		GetCapsuleComponent()->InitCapsuleSize(CharacterConfig->CapsuleRadius, CharacterConfig->CapsuleHalfHeight);

		GetCharacterMovement()->RotationRate = FRotator(0.0f, CharacterConfig->RotationRateYaw, 0.0f);
		GetCharacterMovement()->JumpZVelocity = CharacterConfig->JumpZVelocity;
		GetCharacterMovement()->AirControl = CharacterConfig->AirControl;
		GetCharacterMovement()->MaxWalkSpeed = CharacterConfig->MaxWalkSpeed;
		GetCharacterMovement()->MinAnalogWalkSpeed = CharacterConfig->MinAnalogWalkSpeed;
		GetCharacterMovement()->BrakingDecelerationWalking = CharacterConfig->BrakingDecelerationWalking;
		GetCharacterMovement()->BrakingDecelerationFalling = CharacterConfig->BrakingDecelerationFalling;

		CameraBoom->TargetArmLength = CharacterConfig->CameraArmLength;

		HPBarComponent->SetRelativeLocation(CharacterConfig->HPBarOffset);
	}
}

void ASpyCharacter::BeginPlay()
{
	Super::BeginPlay();
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
	if (GetNetMode() == NM_DedicatedServer)
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

	//# AI 사망 시 BT 정지 + focus 해제 — 사후 회전 방지
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		if (UBrainComponent* BrainComp = AIController->GetBrainComponent())
		{
			BrainComp->StopLogic(SKGameplayTags::Character_State_Death.GetTag().ToString());
		}
		AIController->StopMovement();
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}

	UE_LOG(LogTemp, Log, TEXT("# [SpyCharacter] OnDeath: %s"), *GetName());
}

void ASpyCharacter::OnAbilitySystemInitialized()
{
	USpyAbilitySystemComponent* SpyASC = GetSpyAbilitySystemComponent();
	check(SpyASC);

	SpyHealthComponent->InitializeByAbilitySystem(SpyASC);

	InitializeGameplayTags();

	if (HasAuthority())
	{
		SpawnAndAttachWeapon();
	}
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
	const USpyCharacterAssetData* AssetData = SpyPawnExtensionComponent->GetCharacterAssetData();
	if (AssetData == nullptr || AssetData->CharacterAssets.AssetEntries.IsEmpty())
		return;

	const FCharacterAssetEntry& Entry = AssetData->CharacterAssets.AssetEntries[0];
	if (Entry.WeaponAssetName.IsNone())
		return;

	const FName WeaponAssetName = Entry.WeaponAssetName;
	const FName WeaponSocketName = Entry.WeaponSocketName.IsNone() ? FName(TEXT("weapon_socket")) : Entry.WeaponSocketName;

	const USpyAssetData& SKAssetData = USpyAssetManager::Get().GetAssetData();
	const FSoftObjectPath& AssetPath = SKAssetData.GetAssetPathByName(WeaponAssetName);

	FSpyAssetAndDelegate LoadDelegate;
	TWeakObjectPtr<ASpyCharacter> WeakThis = this;
	LoadDelegate.BindLambda([WeakThis, WeaponAssetName, WeaponSocketName](UObject* LoadedAsset)
		{
			if (WeakThis.IsValid() == false || LoadedAsset == nullptr)
				return;

			if (TSubclassOf<ASpyWeapon> SpyWeaponClass = USpyAssetManager::GetSubclassByName<ASpyWeapon>(WeaponAssetName))
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = WeakThis.Get();
				SpawnParams.Instigator = WeakThis.Get();
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

				WeakThis->SpyWeapon = WeakThis->GetWorld()->SpawnActor<ASpyWeapon>(SpyWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
				WeakThis->SpyWeapon->AttachToComponent(WeakThis->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, WeaponSocketName);
			}
		});

	USpyAssetManager::LoadAssetAsync(AssetPath, LoadDelegate);
}
