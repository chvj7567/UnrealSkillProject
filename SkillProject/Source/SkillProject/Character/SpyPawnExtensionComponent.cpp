// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/SpyPawnExtensionComponent.h"
#include "Util/SpyGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "Data/SpyCharacterAssetData.h"
#include "Components/GameFrameworkComponentManager.h"

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
}

void USpyPawnExtensionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	//# 상태 머신 해제
	UnregisterInitStateFeature();

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
	//# 2번째 단계
	else if (CurrentState == SpyGameplayTags::InitState_Spawned && DesiredState == SpyGameplayTags::InitState_DataAvailable)
	{
		//# 데이터가 유효한지 확인
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
		//# 매니저에 등록된 모든 데이터의 초기화가 완료되었는지 확인
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
	//# 3번째 단계 시 실행
	if (DesiredState == SpyGameplayTags::InitState_DataInitialized)
	{
		APawn* Pawn = GetPawn<APawn>();
		if (Pawn == nullptr || CharacterAssetData == nullptr)
			return;

		//# 부착할 데이터 목록들 Get
		TArray<TSubclassOf<UActorComponent>> ComponentClasses = CharacterAssetData->GetAllComponentClasses(SpyGameplayTags::Character_Class_Normal);
		for (const TSubclassOf<UActorComponent> ComponentClass : ComponentClasses)
		{
			if (ComponentClass == nullptr)
				continue;

			//# 중복 컴포넌트 방지
			if (Pawn->GetComponentByClass(ComponentClass) != nullptr)
				continue;
			
			//# 컴포넌트 부착
			UActorComponent* NewComponent = NewObject<UActorComponent>(Pawn, ComponentClass);
			NewComponent->RegisterComponent();

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

	//# 단계 순서도
	static const TArray<FGameplayTag> StateChain =
	{
		SpyGameplayTags::InitState_Spawned,
		SpyGameplayTags::InitState_DataAvailable,
		SpyGameplayTags::InitState_DataInitialized,
		SpyGameplayTags::InitState_GameplayReady
	};

	//# 다음 단계 진행
	ContinueInitStateChain(StateChain);
}

void USpyPawnExtensionComponent::SetCharacterAssetData(const USpyCharacterAssetData& InCharacterAssetData)
{
	CharacterAssetData = InCharacterAssetData;
}

void USpyPawnExtensionComponent::OnRep_CharacterAssetData()
{
	UE_LOG(LogTemp, Log, TEXT("PawnExtension: OnRep_CharacterAssetData"));
}