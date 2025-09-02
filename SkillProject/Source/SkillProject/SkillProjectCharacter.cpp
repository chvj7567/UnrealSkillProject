// Copyright Epic Games, Inc. All Rights Reserved.

#include "SkillProjectCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "AbilitySystemComponent.h"
#include "SkillGameplayTags.h"
#include "CharacterAttributeSet.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SkillProjectCharacter)

ASkillProjectCharacter::ASkillProjectCharacter()
{
	bReplicates = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
}

void ASkillProjectCharacter::Server_UseSkill_Implementation(FGameplayTag SkillTag)
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

	//# 남은 마나 로그
	UE_LOG(LogTemp, Warning, TEXT("Mana : %f"), CharacterAttributeSet->GetMana());
}

void ASkillProjectCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	if (HasAuthority() == false)
		return;

	if (IsValid(AbilitySystemComponent))
	{
		//# 설정한 AttributeSet 가져옴
		CharacterAttributeSet = AbilitySystemComponent->GetSet<UCharacterAttributeSet>();
		if (CharacterAttributeSet == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("CharacterAttributeSet is nullptr"));
		}
		else
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
}

UAbilitySystemComponent* ASkillProjectCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}