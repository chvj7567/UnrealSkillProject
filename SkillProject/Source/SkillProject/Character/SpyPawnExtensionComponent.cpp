// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/SpyPawnExtensionComponent.h"
#include "Util/SpyGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "Data/SpyCharacterAssetData.h"
#include "Components/GameFrameworkComponentManager.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "System/SpyPlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyPawnExtensionComponent)

const FName USpyPawnExtensionComponent::NAME_ActorFeatureName("PawnExtension");

USpyPawnExtensionComponent::USpyPawnExtensionComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
}

void USpyPawnExtensionComponent::OnRegister()
{
	Super::OnRegister();

	//# ���� �ӽ� ���
	RegisterInitStateFeature();
}

void USpyPawnExtensionComponent::BeginPlay()
{
	Super::BeginPlay();

	//# ���� ��ȭ �˸� ���
	BindOnActorInitStateChanged(NAME_None, FGameplayTag(), false);
	
	//# InitState_Spawned ���·� ��ȯ �õ�
	TryToChangeInitState(SpyGameplayTags::InitState_Spawned);

	CheckDefaultInitialization();

	//# CharacterAssetData가 아직 없으면 주기적으로 재시도 (비동기 로드 지연 대비)
	if (CharacterAssetData == nullptr)
	{
		GetWorld()->GetTimerManager().SetTimer(
			AssetDataRetryTimerHandle,
			this,
			&USpyPawnExtensionComponent::CheckDefaultInitialization,
			0.5f,
			true);
	}

	if (UGameFrameworkComponentManager* Manager =
		UGameFrameworkComponentManager::GetForActor(GetOwner()))
	{
		ExtensionHandle = Manager->AddExtensionHandler(
			TSoftClassPtr<AActor>(APawn::StaticClass()),
			UGameFrameworkComponentManager::FExtensionHandlerDelegate::CreateUObject(this,&USpyPawnExtensionComponent::HandleExtensionEvent));
	}
}

void USpyPawnExtensionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	//# ���� �ӽ� ����
	UnregisterInitStateFeature();

	ExtensionHandle.Reset();

	Super::EndPlay(EndPlayReason);
}

void USpyPawnExtensionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, CharacterAssetData);
}

bool USpyPawnExtensionComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const
{
	check(Manager);

	APawn* Pawn = GetPawn<APawn>();

	//# 1��° �ܰ�
	if (CurrentState.IsValid() == false && DesiredState == SpyGameplayTags::InitState_Spawned)
	{
		//# ���� �����Ǿ����� Ȯ��
		if (Pawn)
			return true;
	}
	//# 2��° �ܰ�
	else if (CurrentState == SpyGameplayTags::InitState_Spawned && DesiredState == SpyGameplayTags::InitState_DataAvailable)
	{
		//# �����Ͱ� ��ȿ���� Ȯ��
		if (CharacterAssetData == nullptr)
			return false;

		const bool bHasAuthority = Pawn->HasAuthority();
		const bool bIsLocallyControlled = Pawn->IsLocallyControlled();

		//# �ڱ� ��Ʈ�ѷ� Ȥ�� ������ �ִ� ��Ʈ�ѷ��� Ȯ��
		if (bHasAuthority || bIsLocallyControlled)
		{
			//# ��Ʈ�ѷ� ���ǰ� �Ǿ����� Ȯ��
			if (GetController<AController>() == nullptr)
				return false;
		}

		return true;
	}
	//# 3��° �ܰ�
	else if (CurrentState == SpyGameplayTags::InitState_DataAvailable && DesiredState == SpyGameplayTags::InitState_DataInitialized)
	{
		//# �Ŵ����� ��ϵ� ��� �������� �ʱ�ȭ�� �Ϸ�Ǿ����� Ȯ��
		return Manager->HaveAllFeaturesReachedInitState(Pawn, SpyGameplayTags::InitState_DataAvailable);
	}
	//# ������ �ܰ�
	else if (CurrentState == SpyGameplayTags::InitState_DataInitialized && DesiredState == SpyGameplayTags::InitState_GameplayReady)
	{
		return true;
	}

	return false;
}

void USpyPawnExtensionComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState)
{
	if (CurrentState == SpyGameplayTags::InitState_Spawned)
	{
		//# DataAvailable로 진입 성공 — 재시도 타이머 해제
		GetWorld()->GetTimerManager().ClearTimer(AssetDataRetryTimerHandle);

		APawn* Pawn = GetPawn<APawn>();
		if (Pawn == nullptr || CharacterAssetData == nullptr)
			return;

		//# ������ ������ ��ϵ� Get
		TArray<TSubclassOf<UActorComponent>> ComponentClasses = CharacterAssetData->GetAllComponentClasses(SpyGameplayTags::Character_Class_Normal);
		for (const TSubclassOf<UActorComponent> ComponentClass : ComponentClasses)
		{
			if (ComponentClass == nullptr)
				continue;

			//# �ߺ� ������Ʈ ����
			if (Pawn->GetComponentByClass(ComponentClass) != nullptr)
				continue;
			
			//# ������Ʈ ����
			UActorComponent* NewComponent = NewObject<UActorComponent>(Pawn, ComponentClass);

			//# ������ ������ â�� �߰�
			Pawn->AddInstanceComponent(NewComponent);

			//# ��Ÿ�� ������Ʈ ���
			NewComponent->RegisterComponent();

			if (NewComponent->HasBeenInitialized() == false)
			{
				NewComponent->InitializeComponent();
			}

			UE_LOG(LogTemp, Log, TEXT("# Success Attach Component: %s"), *ComponentClass->GetName());
		}
	}
}

void USpyPawnExtensionComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	//# �ٸ� ���ĵ��� ���°� ������ ��
	if (Params.FeatureName != NAME_ActorFeatureName)
	{
		CheckDefaultInitialization();
	}
}

void USpyPawnExtensionComponent::CheckDefaultInitialization()
{
	//# �ٸ� ���ĵ��� CheckDefaultInitialization ���� ����
	CheckDefaultInitializationForImplementers();

	//# �ܰ� ������
	static const TArray<FGameplayTag> StateChain =
	{
		SpyGameplayTags::InitState_Spawned,
		SpyGameplayTags::InitState_DataAvailable,
		SpyGameplayTags::InitState_DataInitialized,
		SpyGameplayTags::InitState_GameplayReady
	};

	//# ���� �ܰ� ����
	ContinueInitStateChain(StateChain);
}

void USpyPawnExtensionComponent::SetCharacterAssetData(const USpyCharacterAssetData* InCharacterAssetData)
{
	check(InCharacterAssetData);

	APawn* Pawn = GetPawnChecked<APawn>();

	//# ���������� ����
	if (Pawn->GetLocalRole() != ROLE_Authority)
		return;

	//# �̹� ���õ�
	if (CharacterAssetData)
		return;

	CharacterAssetData = InCharacterAssetData;

	//# 데이터가 설정되었으므로 재시도 타이머 해제
	GetWorld()->GetTimerManager().ClearTimer(AssetDataRetryTimerHandle);

	//# 다른 클라이언트들에게 바뀐 데이터를 전달하도록 복제 요청
	Pawn->ForceNetUpdate();

	CheckDefaultInitialization();
}

void USpyPawnExtensionComponent::InitializeAbilitySystem(USpyAbilitySystemComponent* InASC, AActor* InOwnerActor)
{
	check(InASC);
	check(InOwnerActor);

	//# ASC�� �ٲ��� ����
	if (AbilitySystemComponent == InASC)
		return;

	//# ASC�� �ٲ���ٸ� ���� ASC ����
	if (AbilitySystemComponent)
	{
		UninitializeAbilitySystem();
	}

	APawn* Pawn = GetPawnChecked<APawn>();
	AActor* ExistingAvatar = InASC->GetAvatarActor();

	//# �ٸ� �ƹ�Ÿ�� ASC�� �����ϰ� ������ ���� ���� ��Ŵ
	if ((ExistingAvatar != nullptr) && (ExistingAvatar != Pawn))
	{
		ensure(!ExistingAvatar->HasAuthority());

		if (USpyPawnExtensionComponent* OtherExtensionComponent = FindPawnExtensionComponent(ExistingAvatar))
		{
			OtherExtensionComponent->UninitializeAbilitySystem();
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("# PawnExtension: InitAbilityActorInfo"));
	AbilitySystemComponent = InASC;
	AbilitySystemComponent->InitAbilityActorInfo(InOwnerActor, Pawn);

	OnAbilitySystemInitialized.Broadcast();
}

void USpyPawnExtensionComponent::UninitializeAbilitySystem()
{
	if (AbilitySystemComponent == nullptr)
		return;

	//# ������ �ִ� ��츸(�ƹ�Ÿ�� ���� ���)
	if (AbilitySystemComponent->GetAvatarActor() == GetOwner())
	{
		//# ��� ASC ���� ���
		AbilitySystemComponent->CancelAbilities();
		AbilitySystemComponent->ClearAbilityInput();
		AbilitySystemComponent->RemoveAllGameplayCues();

		if (AbilitySystemComponent->GetOwnerActor() != nullptr)
		{
			AbilitySystemComponent->SetAvatarActor(nullptr);
		}
		else
		{
			AbilitySystemComponent->ClearActorInfo();
		}

		OnAbilitySystemUninitialized.Broadcast();
	}

	AbilitySystemComponent = nullptr;
}

void USpyPawnExtensionComponent::HandleControllerChanged()
{
	//# ASC �ƹ�Ÿ�� ������ Ȯ��
	if (AbilitySystemComponent && (AbilitySystemComponent->GetAvatarActor() == GetPawnChecked<APawn>()))
	{
		if (AbilitySystemComponent->GetOwnerActor() == nullptr)
		{
			//# Owner�� ���ٸ� ASC ����
			UninitializeAbilitySystem();
		}
		else
		{
			//# ��Ʈ�ѷ��� �ٲ������ ����
			AbilitySystemComponent->RefreshAbilityActorInfo();
		}
	}

	CheckDefaultInitialization();
}

void USpyPawnExtensionComponent::HandlePlayerStateReplicated()
{
	CheckDefaultInitialization();
}

void USpyPawnExtensionComponent::SetupPlayerInputComponent()
{
	CheckDefaultInitialization();
}

void USpyPawnExtensionComponent::OnAbilitySystemInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate Delegate)
{
	if (OnAbilitySystemInitialized.IsBoundToObject(Delegate.GetUObject()) == false)
	{
		OnAbilitySystemInitialized.Add(Delegate);
	}

	if (AbilitySystemComponent)
	{
		Delegate.Execute();
	}
}

void USpyPawnExtensionComponent::OnAbilitySystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate Delegate)
{
	if (OnAbilitySystemUninitialized.IsBoundToObject(Delegate.GetUObject()) == false)
	{
		OnAbilitySystemUninitialized.Add(Delegate);
	}
}

void USpyPawnExtensionComponent::HandleExtensionEvent(AActor* OwnerActor, FName EventName)
{
	UE_LOG(LogTemp, Log, TEXT("# [PawnExtension] %s received by %s"), *EventName.ToString(), *GetName());
}

void USpyPawnExtensionComponent::OnRep_CharacterAssetData()
{
	CheckDefaultInitialization();
	UE_LOG(LogTemp, Log, TEXT("# [PawnExtension] OnRep_CharacterAssetData"));
}