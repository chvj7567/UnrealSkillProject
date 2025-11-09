// Copyright Epic Games, Inc. All Rights Reserved.

#include "SkillProjectCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "AbilitySystemComponent.h"
#include "Util/SkillGameplayTags.h"
#include "AbilitySystem/Attribute/CharacterAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "Components/WidgetComponent.h"
#include "Manager/SpyUIManager.h"
#include "UI/SpyHPBar.h"
#include "ManagerComponent/SpyAnimManagerComponent.h"
#include "Character/AnimInstance/CharacterAnimInstance.h"

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

	AbilitySystemComponent = CreateDefaultSubobject<USpyAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	LeftWeaponCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftWeaponCollision"));
	RightWeaponCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("RightWeaponCollision"));
	LeftWeaponCollision->SetupAttachment(GetMesh());
	RightWeaponCollision->SetupAttachment(GetMesh());

	HPBarComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPBarComponent"));
	HPBarComponent->SetupAttachment(GetMesh());
	HPBarComponent->SetRelativeLocation(FVector(0, 0, 200.f));

	AnimManagerComponent = CreateDefaultSubobject<USpyAnimManagerComponent>(TEXT("AnimManagerComponent"));
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
	Super::BeginPlay();

	RegisterAbility();
	BindCollision();

	USpyUIManager::Get(this)->OpenSubUI(ESpyUIType::HpBar, HPBarComponent, EWidgetSpace::Screen);

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimManagerComponent->Initialize(Cast<UCharacterAnimInstance>(AnimInstance));
	}
}

void ASkillProjectCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void ASkillProjectCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ASkillProjectCharacter::RegisterAbility()
{
	if (HasAuthority() == false)
		return;

	if (!AbilitySystemComponent)
		return;

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

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(CharacterAttributeSet->GetHealthAttribute())
		.AddUObject(this, &ASkillProjectCharacter::OnHealthChanged);
}

void ASkillProjectCharacter::BindCollision()
{
	LeftWeaponCollision->OnComponentBeginOverlap.AddDynamic(this, &ASkillProjectCharacter::OnSkillHitOverlap);
	RightWeaponCollision->OnComponentBeginOverlap.AddDynamic(this, &ASkillProjectCharacter::OnSkillHitOverlap);
}

void ASkillProjectCharacter::TestHit()
{
	FGameplayAttribute HealthAttr = UCharacterAttributeSet::GetHealthAttribute();
	float CurrentHealth = AbilitySystemComponent->GetNumericAttribute(HealthAttr);
	float NewHealth = CurrentHealth - 10.f;

	NewHealth = FMath::Max(NewHealth, 0.f);
	AbilitySystemComponent->SetNumericAttributeBase(HealthAttr, NewHealth);

	if (USpyHPBar* hpBar = Cast<USpyHPBar>(HPBarComponent->GetWidget()))
	{
		hpBar->UpdateHP(NewHealth, CharacterAttributeSet->GetMaxHealth());
	}
}

void ASkillProjectCharacter::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	UE_LOG(LogTemp, Warning, TEXT("[Struct] Health changed: %f -> %f"), Data.OldValue, Data.NewValue);

	if (Data.NewValue < 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Struct] Die"));
	}
	else
	{
		if (USpyHPBar* hpBar = Cast<USpyHPBar>(HPBarComponent->GetWidget()))
		{
			hpBar->UpdateHP(Data.NewValue, CharacterAttributeSet->GetMaxHealth());
		}
	}
}

void ASkillProjectCharacter::OnSkillHitOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (HasAuthority() == false)
		return;

	ASkillProjectCharacter* OtherCharacter = Cast<ASkillProjectCharacter>(OtherActor);
	if (OtherCharacter == nullptr)
		return;

	FGameplayTagContainer EventTags = GetActivatableAbilityTags();
	for (FGameplayTag EventTag : EventTags.GetGameplayTagArray())
	{
		FGameplayEventData Payload;
		Payload.EventTag = EventTag;
		Payload.Instigator = this;
		Payload.Target = OtherActor;

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, EventTag, Payload);
	}
}

FGameplayTagContainer ASkillProjectCharacter::GetActivatableAbilityTags()
{
	TArray<FGameplayAbilitySpec> Abilities;
	Abilities = AbilitySystemComponent->GetActivatableAbilities();

	FGameplayTagContainer ActivatableTags;
	for (FGameplayAbilitySpec Spec : Abilities)
	{
		if (Spec.IsActive())
		{
			UE_LOG(LogTemp, Warning, TEXT("Ability 실행 중: %s"), *Spec.Ability->GetName());
			ActivatableTags.AppendTags(Spec.Ability->AbilityTags);
		}
	}

	return ActivatableTags;
}