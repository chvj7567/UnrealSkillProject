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
#include "SKAbilitySystemComponent.h"
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
#include "MotionWarpingComponent.h"
#include "System/SpyPlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyCharacter)

ASpyCharacter::ASpyCharacter(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer.SetDefaultSubobjectClass<USpyCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	bReplicates = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

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

	AbilitySystemComponent = CreateDefaultSubobject<USKAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	HPBarComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPBarComponent"));
	HPBarComponent->SetupAttachment(GetMesh());
	HPBarComponent->SetRelativeLocation(FVector(0, 0, 200.f));

	SpyAnimManagerComponent = CreateDefaultSubobject<USpyAnimManagerComponent>(TEXT("SpyAnimManagerComponent"));
	SpyParkourManagerComponent = CreateDefaultSubobject<USpyParkourManagerComponent>(TEXT("SpyParkourManagerComponent"));
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));
}

void ASpyCharacter::BeginPlay()
{
	Super::BeginPlay();

	RegisterAbility();

	USpyUIManager::Get(this)->OpenSubUI(ESpyUIType::HpBar, HPBarComponent, EWidgetSpace::Screen);

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		SpyAnimManagerComponent->Initialize(Cast<USpyCharacterAnimInstance>(AnimInstance));
	}

	if (HasAuthority())
	{
		if (TSubclassOf<ASpyWeapon> SpyWeaponClass = USpyAssetManager::GetSubclassByName<ASpyWeapon>(FName("TwoHandSword"), false))
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = this;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			SpyWeapon = GetWorld()->SpawnActor<ASpyWeapon>(SpyWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
			SpyWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("LeftHandSocket"));
		}
	}
}

void ASpyCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void ASpyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ASpyCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (HasAuthority())
	{
		if (ASpyPlayerState* SpyPlayerState = GetPlayerState<ASpyPlayerState>())
		{
			SpyPlayerState->Initialize();
			SpyPlayerState->AddState(SpyGameplayTags::Character_State_Survival_Alive);
		}
	}
}

void ASpyCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (HasAuthority() == false)
	{
		if (ASpyPlayerState* SpyPlayerState = GetPlayerState<ASpyPlayerState>())
		{
			SpyPlayerState->Initialize();
		}
	}
}

UAbilitySystemComponent* ASpyCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ASpyCharacter::RegisterAbility()
{
	if (AbilitySystemComponent == nullptr)
	{
		UE_LOG(LogTemp, Fatal, TEXT("AbilitySystemComponent is nullptr"));
		UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, true);
		return;
	}

	//# 설정한 AttributeSet 가져옴
	CharacterAttributeSet = AbilitySystemComponent->GetSet<USKAttributeSet>();
	if (CharacterAttributeSet == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("CharacterAttributeSet is nullptr"));
		UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, true);
		return;
	}
	else
	{
		if (HasAuthority())
		{
			//# 캐릭터에 등록된 스킬 부여
			for (TSubclassOf<UGameplayAbility> AbilityClass : AbilityClasses)
			{
				if (AbilityClass)
				{
					FGameplayAbilitySpec AbilitySpec(AbilityClass, 1, INDEX_NONE);
					AbilitySystemComponent->GiveAbility(AbilitySpec);
				}
			}
		}
	}

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(CharacterAttributeSet->GetHealthAttribute())
		.AddUObject(this, &ASpyCharacter::OnHealthChanged);

	/*AbilitySystemComponent->RegisterGameplayTagEvent(SpyGameplayTags::Character_State_Movement_Climb, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ASpyCharacter::OnClimbTagChanged);*/
}

void ASpyCharacter::OnClimbTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (IsLocallyControlled())
	{
		if (ASpyPlayerController* PC = Cast<ASpyPlayerController>(GetController()))
		{
			UE_LOG(LogTemp, Warning, TEXT("# Local on: %s, Count: %d"), *GetName(), NewCount);

			PC->RefreshMappingContext();
		}
	}
}

void ASpyCharacter::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue <= 0.f)
	{
		if (HasAuthority())
		{
			if (ASpyPlayerState* SpyPlayerState = Cast<ASpyPlayerState>(GetPlayerState()))
			{
				if (SpyPlayerState->HasState(SpyGameplayTags::Character_State_Survival_Alive))
				{
					SpyPlayerState->Multicast_Death();
				}
			}
		}
	}
	else
	{
		if (USpyHPBar* HpBar = Cast<USpyHPBar>(HPBarComponent->GetWidget()))
		{
			HpBar->UpdateHP(Data.NewValue, CharacterAttributeSet->GetMaxHealth());
		}
	}
}

void ASpyCharacter::Death()
{
	if (USpyHPBar* HpBar = Cast<USpyHPBar>(HPBarComponent->GetWidget()))
	{
		HpBar->UpdateHP(0, 0);
	}
}

FGameplayTagContainer ASpyCharacter::GetActivatableAbilityTags()
{
	TArray<FGameplayAbilitySpec> Abilities;
	Abilities = AbilitySystemComponent->GetActivatableAbilities();

	FGameplayTagContainer ActivatableTags;
	for (FGameplayAbilitySpec Spec : Abilities)
	{
		if (Spec.IsActive())
		{
			ActivatableTags.AppendTags(Spec.Ability->AbilityTags);
		}
	}

	return ActivatableTags;
}

void ASpyCharacter::Server_UseSkill_Implementation(FGameplayTag SkillTag)
{
	if (HasAuthority() == false)
		return;

	//# 사용할 스킬 태그 등록
	FGameplayTagContainer TagContaingers;
	TagContaingers.AddTag(SkillTag);

	//# 태그를 통해 ASC에 등록된 능력 핸들 가져옴
	TArray<FGameplayAbilitySpecHandle> AbilityHandles;
	AbilitySystemComponent->FindAllAbilitiesWithTags(AbilityHandles, TagContaingers);

	//# 가져온 능력 실행
	for (const FGameplayAbilitySpecHandle& AbilityHandle : AbilityHandles)
	{
		if (AbilitySystemComponent->TryActivateAbility(AbilityHandle))
		{
			UE_LOG(LogTemp, Warning, TEXT("Ability Success %s"), *SkillTag.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Ability Failed %s"), *SkillTag.ToString());
		}
	}
}