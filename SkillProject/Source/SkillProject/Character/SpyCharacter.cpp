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
#include "Character/SpyCharacterAttributeSet.h"
#include "Character/SpyPawnExtensionComponent.h"

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
}

void ASpyCharacter::BeginPlay()
{
	Super::BeginPlay();

	USpyUIManager::Get(this)->OpenSubSpyUI(ESpyUIType::HpBar, HPBarComponent, EWidgetSpace::Screen);

	/*if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		SpyAnimManagerComponent->Initialize(Cast<USpyCharacterAnimInstance>(AnimInstance));
	}*/

	if (HasAuthority())
	{
		const USpyAssetData& AssetData = USpyAssetManager::Get().GetAssetData();
		const FSoftObjectPath& AssetPath = AssetData.GetAssetPathByName(FName("TwoHandSword"));

		FSpyAssetAndDelegate LoadDelegate;
		LoadDelegate.BindLambda([this](UObject* LoadedAsset)
			{
				if (LoadedAsset == nullptr)
					return;

				if (TSubclassOf<ASpyWeapon> SpyWeaponClass = USpyAssetManager::GetSubclassByName<ASpyWeapon>(FName("TwoHandSword")))
				{
					FActorSpawnParameters SpawnParams;
					SpawnParams.Owner = this;
					SpawnParams.Instigator = this;
					SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

					SpyWeapon = GetWorld()->SpawnActor<ASpyWeapon>(SpyWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
					SpyWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("LeftHandSocket"));
				}
				
			});

		USpyAssetManager::LoadAssetAsync(AssetPath, LoadDelegate);
	}
}

void ASpyCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	SpyPawnExtensionComponent->HandleControllerChanged();
}

void ASpyCharacter::UnPossessed()
{
	Super::UnPossessed();

	SpyPawnExtensionComponent->HandleControllerChanged();
}

void ASpyCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

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

void ASpyCharacter::InitializeGameplayTags()
{
}

void ASpyCharacter::Death()
{
	if (USpyHPBar* HpBar = Cast<USpyHPBar>(HPBarComponent->GetWidget()))
	{
		HpBar->UpdateHP(0, 0);
	}
}

void ASpyCharacter::OnAbilitySystemInitialized()
{
}

void ASpyCharacter::OnAbilitySystemUninitialized()
{
}

void ASpyCharacter::UseSkill(FGameplayTag SkillTag)
{
	FGameplayEventData EventData;
	EventData.Instigator = GetPlayerState();
	EventData.EventTag = SkillTag;

	FGameplayAbilityTargetData_LocationInfo* LocData = new FGameplayAbilityTargetData_LocationInfo();
	LocData->TargetLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
	LocData->TargetLocation.LiteralTransform = GetActorTransform();
	EventData.TargetData.Add(LocData);

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetPlayerState(), SkillTag, EventData);
}

void ASpyCharacter::Server_UseSkill_Implementation(FGameplayTag SkillTag)
{
	//if (HasAuthority() == false)
	//	return;

	////# 사용할 스킬 태그 등록
	//FGameplayTagContainer TagContaingers;
	//TagContaingers.AddTag(SkillTag);

	////# 태그를 통해 ASC에 등록된 능력 핸들 가져옴
	//TArray<FGameplayAbilitySpecHandle> AbilityHandles;
	//AbilitySystemComponent->FindAllAbilitiesWithTags(AbilityHandles, TagContaingers);

	////# 가져온 능력 실행
	//for (const FGameplayAbilitySpecHandle& AbilityHandle : AbilityHandles)
	//{
	//	if (AbilitySystemComponent->TryActivateAbility(AbilityHandle))
	//	{
	//		UE_LOG(LogTemp, Warning, TEXT("Ability Success %s"), *SkillTag.ToString());
	//	}
	//	else
	//	{
	//		UE_LOG(LogTemp, Warning, TEXT("Ability Failed %s"), *SkillTag.ToString());
	//	}
	//}
}