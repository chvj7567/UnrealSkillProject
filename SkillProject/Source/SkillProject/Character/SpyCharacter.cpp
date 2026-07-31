// Copyright Epic Games, Inc. All Rights Reserved.

#include "SpyCharacter.h"
#include "AIController.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "BrainComponent.h"
#include "Camera/CameraComponent.h"
#include "Character/SpyHealthComponent.h"
#include "Character/SpyLevelComponent.h"
#include "Character/SpyPawnExtensionComponent.h"
#include "Components/CapsuleComponent.h"
#include "Data/SpyCharacterAssetData.h"
#include "Data/SpyCharacterConfig.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Item/SpyWeapon.h"
#include "Manager/SpyAssetManager.h"
#include "ManagerComponent/SpyGrappleTargetingComponent.h"
#include "ManagerComponent/SpyGrappleUIComponent.h"
#include "ManagerComponent/SpyParkourManagerComponent.h"
#include "ManagerComponent/SpyTargetingManagerComponent.h"
#include "MotionWarpingComponent.h"
#include "Util/SpyGameplayTags.h"

#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyCharacter)

void ASpyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASpyCharacter, SpyWeapon);
}

ASpyCharacter::ASpyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<USpyCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
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
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bDoCollisionTest = true;

	//# Pawn(자신/타 캐릭터)은 카메라 SpringArm 트레이스를 막지 않음 — 벽(WorldStatic 등)만 가림 처리
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	SpyPawnExtensionComponent = CreateDefaultSubobject<USpyPawnExtensionComponent>(TEXT("SpyPawnExtensionComponent"));
	SpyPawnExtensionComponent->OnAbilitySystemInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemInitialized));
	SpyPawnExtensionComponent->OnAbilitySystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemUninitialized));

	SpyHealthComponent = CreateDefaultSubobject<USpyHealthComponent>(TEXT("HealthComponent"));
	SpyHealthComponent->OnHealthChanged.AddDynamic(this, &ThisClass::OnHealthChanged);
	SpyHealthComponent->OnDeath.AddDynamic(this, &ThisClass::OnDeath);

	SpyLevelComponent = CreateDefaultSubobject<USpyLevelComponent>(TEXT("LevelComponent"));
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
	}
}

void ASpyCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ASpyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	//# 부착만으로는 소유자와 수명이 묶이지 않는다 — 정리하지 않으면 무기 메시가 그 자리에 남는다.
	//# 서버에서 파괴해야 클라이언트로 전파된다(SpyWeapon 은 Replicated 프로퍼티다).
	if (HasAuthority() && IsValid(SpyWeapon))
	{
		SpyWeapon->Destroy();
	}

	SpyWeapon = nullptr;

	Super::EndPlay(EndPlayReason);
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

void ASpyCharacter::PushCameraCollisionSuppress()
{
	++CameraCollisionSuppressCount;

	//# 첫 요청에서만 원래 값을 저장하고 끈다. 이후 중첩 요청은 카운트만 올린다.
	if (CameraCollisionSuppressCount == 1 && IsValid(CameraBoom))
	{
		//# bDoCollisionTest 는 uint32:1 비트필드라 bool 로 명시 변환해 보관한다.
		bCachedDoCollisionTest = (CameraBoom->bDoCollisionTest != 0);
		CameraBoom->bDoCollisionTest = false;
	}
}

void ASpyCharacter::PopCameraCollisionSuppress()
{
	//# 짝이 맞지 않는 해제로 카운트가 음수로 내려가지 않게 막는다.
	if (CameraCollisionSuppressCount <= 0)
		return;

	--CameraCollisionSuppressCount;

	//# 마지막 요청이 풀렸을 때만 원래 값으로 되돌린다.
	if (CameraCollisionSuppressCount == 0 && IsValid(CameraBoom))
	{
		CameraBoom->bDoCollisionTest = bCachedDoCollisionTest;
	}
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
	//# 머리 위 HP바는 제거됨 — HP 표시는 MainHUD 가 담당한다.
	//# 델리게이트 바인딩은 유지하되(향후 연출 훅 여지) 현재 캐릭터 측 처리는 없다.
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
	SpyLevelComponent->InitializeByAbilitySystem(SpyASC);

	InitializeGameplayTags();

	if (HasAuthority())
	{
		SpawnAndAttachWeapon();
	}

	//# 하위 컴포넌트 캐싱 + 형제 간 주입 — 루트가 조립점이다 (cpp-style §13)
	AssembleComponents();
}

void ASpyCharacter::OnAbilitySystemUninitialized()
{
	SpyHealthComponent->UnInitializeByAbilitySystem();
	SpyLevelComponent->UnInitializeByAbilitySystem();
}

void ASpyCharacter::AssembleComponents()
{
	//# 컴포넌트가 없는 캐릭터도 있다 — 널 핸들은 정상 상태다.
	CachedParkourHost = TScriptInterface<ISpyParkourHost>(FindComponentByClass<USpyParkourManagerComponent>());
	CachedTargetProvider = TScriptInterface<ISpyTargetProvider>(FindComponentByClass<USpyTargetingManagerComponent>());
	CachedGrappleHost = TScriptInterface<ISpyGrappleHost>(FindComponentByClass<USpyGrappleTargetingComponent>());
	CachedMotionWarping = FindComponentByClass<UMotionWarpingComponent>();

	//# 형제끼리 서로 찾아가지 않게 루트가 주입한다.
	if (USpyCharacterMovementComponent* SpyMovement = GetSpyCharacterMovementComponent())
	{
		SpyMovement->InjectTargetProvider(CachedTargetProvider);
	}

	if (USpyGrappleUIComponent* GrappleUI = FindComponentByClass<USpyGrappleUIComponent>())
	{
		GrappleUI->InjectGrappleHost(CachedGrappleHost);
	}
}

void ASpyCharacter::AddMotionWarpTarget(FName WarpName, const FVector& Loc, const FRotator& Rot)
{
	if (IsValid(CachedMotionWarping) == false)
		return;

	CachedMotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(WarpName, Loc, Rot);
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

	//# 패키지 빌드에서 BP 오브젝트(BP_X.BP_X)는 cook 시 stripped되므로 generated class(BP_X.BP_X_C) 경로로 로드
	const USKAssetData& SKAssetData = USpyAssetManager::Get().GetAssetData();
	const FSoftObjectPath& AssetPath = SKAssetData.GetAssetPathByName(WeaponAssetName);
	FString ClassPathString = AssetPath.GetAssetPathString();
	if (ClassPathString.EndsWith(TEXT("_C")) == false)
	{
		ClassPathString.Append(TEXT("_C"));
	}
	FSoftObjectPath ClassPath(ClassPathString);

	FSKAssetAndDelegate LoadDelegate;
	TWeakObjectPtr<ASpyCharacter> WeakThis = this;
	LoadDelegate.BindLambda([WeakThis, WeaponAssetName, WeaponSocketName](UObject* LoadedAsset) {
		if (WeakThis.IsValid() == false || LoadedAsset == nullptr)
			return;

		if (TSubclassOf<ASpyWeapon> SpyWeaponClass = USpyAssetManager::GetSubclassByName<ASpyWeapon>(WeaponAssetName))
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = WeakThis.Get();
			SpawnParams.Instigator = WeakThis.Get();
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			WeakThis->SpyWeapon = WeakThis->GetWorld()->SpawnActor<ASpyWeapon>(SpyWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
			if (WeakThis->SpyWeapon)
			{
				WeakThis->SpyWeapon->AttachToComponent(WeakThis->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, WeaponSocketName);
			}
		}
	});

	USpyAssetManager::LoadAssetAsync(ClassPath, LoadDelegate);
}
