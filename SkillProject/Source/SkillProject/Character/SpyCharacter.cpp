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
#include "AbilitySystem/Attribute/SpyAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "Components/WidgetComponent.h"
#include "Manager/SpyUIManager.h"
#include "UI/SpyHPBar.h"
#include "ManagerComponent/SpyAnimManagerComponent.h"
#include "ManagerComponent/SpyParkourManagerComponent.h"
#include "Character/AnimInstance/SpyCharacterAnimInstance.h"
#include "System/SpyPlayerState.h"
#include "Components/BoxComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyCharacter)

ASpyCharacter::ASpyCharacter()
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

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

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
	ParkourManagerComponent = CreateDefaultSubobject<USpyParkourManagerComponent>(TEXT("ParkourManagerComponent"));
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

	//# 남은 마나 로그
	UE_LOG(LogTemp, Warning, TEXT("Mana : %f"), CharacterAttributeSet->GetMana());
}

void ASpyCharacter::BeginPlay()
{
	Super::BeginPlay();

	RegisterAbility();
	BindCollision();

	USpyUIManager::Get(this)->OpenSubUI(ESpyUIType::HpBar, HPBarComponent, EWidgetSpace::Screen);

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimManagerComponent->Initialize(Cast<USpyCharacterAnimInstance>(AnimInstance));
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
		if (ASpyPlayerState* SpyPlayerState = Cast<ASpyPlayerState>(GetPlayerState()))
		{
			SpyPlayerState->AddState(ESpyPlayerStateFlags::IsAlive);
		}
	}
}

void ASpyCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
}

void ASpyCharacter::RegisterAbility()
{
	if (!AbilitySystemComponent)
		return;

	//# 설정한 AttributeSet 가져옴
	CharacterAttributeSet = AbilitySystemComponent->GetSet<USpyAttributeSet>();
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
		.AddUObject(this, &ASpyCharacter::OnHealthChanged);
}

void ASpyCharacter::BindCollision()
{
	LeftWeaponCollision->OnComponentBeginOverlap.AddDynamic(this, &ASpyCharacter::OnSkillHitOverlap);
	RightWeaponCollision->OnComponentBeginOverlap.AddDynamic(this, &ASpyCharacter::OnSkillHitOverlap);
}

void ASpyCharacter::TestHit()
{
	FGameplayAttribute HealthAttr = USpyAttributeSet::GetHealthAttribute();
	float CurrentHealth = AbilitySystemComponent->GetNumericAttribute(HealthAttr);
	float NewHealth = CurrentHealth - 10.f;

	NewHealth = FMath::Max(NewHealth, 0.f);
	AbilitySystemComponent->SetNumericAttributeBase(HealthAttr, NewHealth);

	if (USpyHPBar* hpBar = Cast<USpyHPBar>(HPBarComponent->GetWidget()))
	{
		hpBar->UpdateHP(NewHealth, CharacterAttributeSet->GetMaxHealth());
	}
}

void ASpyCharacter::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue < 0.f)
	{
		if (HasAuthority())
		{
			if (ASpyPlayerState* SpyPlayerState = Cast<ASpyPlayerState>(GetPlayerState()))
			{
				if (SpyPlayerState->HasState(ESpyPlayerStateFlags::IsAlive))
				{
					SpyPlayerState->Multicast_Death();
				}
			}
		}
	}
	else
	{
		if (USpyHPBar* hpBar = Cast<USpyHPBar>(HPBarComponent->GetWidget()))
		{
			hpBar->UpdateHP(Data.NewValue, CharacterAttributeSet->GetMaxHealth());
		}
	}
}

void ASpyCharacter::OnSkillHitOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (HasAuthority() == false)
		return;

	ASpyCharacter* OtherCharacter = Cast<ASpyCharacter>(OtherActor);
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

FGameplayTagContainer ASpyCharacter::GetActivatableAbilityTags()
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