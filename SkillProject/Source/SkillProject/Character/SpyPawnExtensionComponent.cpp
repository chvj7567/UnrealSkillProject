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

	//# 상태 머신 등록
	RegisterInitStateFeature();
}

void USpyPawnExtensionComponent::BeginPlay()
{
	Super::BeginPlay();

	//# 상태 변화 알림 등록
	BindOnActorInitStateChanged(NAME_None, FGameplayTag(), false);
	
	//# InitState_Spawned 상태로 변환 시도
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
	//# 상태 머신 등록
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

	//# 1번째 단계
	if (CurrentState.IsValid() == false && DesiredState == SpyGameplayTags::InitState_Spawned)
	{
		//# 폰이 스폰되었는지 확인
		if (Pawn)
			return true;
	}
		//# 매니저에 등록된 모든 데이터의 초기화가 완료되었는지 확인
	else if (CurrentState == SpyGameplayTags::InitState_Spawned && DesiredState == SpyGameplayTags::InitState_DataAvailable)
	{
	//# 마지막 단계
		if (CharacterAssetData == nullptr)
			return false;

		const bool bHasAuthority = Pawn->HasAuthority();
		const bool bIsLocallyControlled = Pawn->IsLocallyControlled();

		//# 자기 컨트롤러 혹은 서버에 있는 컨트롤러만 확인
		if (bHasAuthority || bIsLocallyControlled)
		{
			//# 컨트롤러 빙의가 되었는지 확인
			if (GetController<AController>() == nullptr)
				return false;
		}

		return true;
	}
	//# 3번째 단계
	else if (CurrentState == SpyGameplayTags::InitState_DataAvailable && DesiredState == SpyGameplayTags::InitState_DataInitialized)
	{
		//# 자기 컨트롤러 혹은 서버에 있는 컨트롤러만 확인
		return Manager->HaveAllFeaturesReachedInitState(Pawn, SpyGameplayTags::InitState_DataAvailable);
	}
	//# 마지막 단계
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

			//# 에디터 디테일 창에 추가
		TArray<TSubclassOf<UActorComponent>> ComponentClasses = CharacterAssetData->GetAllComponentClasses(SpyGameplayTags::Character_Class_Normal);
		for (const TSubclassOf<UActorComponent> ComponentClass : ComponentClasses)
		{
			if (ComponentClass == nullptr)
				continue;

			////# 런타임 컴포넌트 등록
			if (Pawn->GetComponentByClass(ComponentClass) != nullptr)
				continue;
			
			//# 컴포넌트 부착
			UActorComponent* NewComponent = NewObject<UActorComponent>(Pawn, ComponentClass);

			//# 컨트롤러 빙의가 되었는지 확인
			Pawn->AddInstanceComponent(NewComponent);

	//# 다른 피쳐들의 상태가 변했을 때
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
	//# 다른 피쳐들의 상태가 변했을 때
	if (Params.FeatureName != NAME_ActorFeatureName)
	{
		CheckDefaultInitialization();
	}
}

void USpyPawnExtensionComponent::CheckDefaultInitialization()
{
	//# 다른 피쳐들의 CheckDefaultInitialization 강제 실행
	CheckDefaultInitializationForImplementers();

	//# 서버에서만 세팅
	static const TArray<FGameplayTag> StateChain =
	{
		SpyGameplayTags::InitState_Spawned,
		SpyGameplayTags::InitState_DataAvailable,
		SpyGameplayTags::InitState_DataInitialized,
		SpyGameplayTags::InitState_GameplayReady
	};

	//# 이미 세팅됨
	ContinueInitStateChain(StateChain);
}

void USpyPawnExtensionComponent::SetCharacterAssetData(const USpyCharacterAssetData* InCharacterAssetData)
{
	check(InCharacterAssetData);

	APawn* Pawn = GetPawnChecked<APawn>();

	//# 마지막 단계
	if (Pawn->GetLocalRole() != ROLE_Authority)
		return;

	//# 마지막 단계
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

	//# ASC가 바뀌지 않음
	if (AbilitySystemComponent == InASC)
		return;

	//# ASC가 바뀌었다면 기존 ASC 종료
	if (AbilitySystemComponent)
	{
		UninitializeAbilitySystem();
	}

	APawn* Pawn = GetPawnChecked<APawn>();
	AActor* ExistingAvatar = InASC->GetAvatarActor();

	//# 권한이 있는 경우만(아바타가 나인 경우)
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

	//# 권한이 있는 경우만(아바타가 나인 경우)
	if (AbilitySystemComponent->GetAvatarActor() == GetOwner())
	{
		//# 모든 ASC 관련 취소
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
	//# ASC 아바타가 나인지 확인
	if (AbilitySystemComponent && (AbilitySystemComponent->GetAvatarActor() == GetPawnChecked<APawn>()))
	{
		if (AbilitySystemComponent->GetOwnerActor() == nullptr)
		{
			//# Owner가 없다면 ASC 해제
			UninitializeAbilitySystem();
		}
		else
		{
			//# 중복 컴포넌트 방지
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