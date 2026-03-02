// Fill out your copyright notice in the Description page of Project Settings.


#include "System/SpyPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Character/SpyCharacter.h"
#include "UI/SpyHPBar.h"
#include "UI/SpyUserWidget.h"
#include "Util/SpyGameplayTags.h"
#include "Character/SpyCharacterAttributeSet.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Manager/SpyAssetManager.h"
#include "Data/SpyCharacterAssetData.h"
#include "Data/SpyAbilityData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyPlayerState)

const FName ASpyPlayerState::NAME_AbilityReady("AbilitiesReady");

ASpyPlayerState::ASpyPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	//# 업데이트 주기를 빠르게 설정
	NetUpdateFrequency = 100.0f;

	//# UActorComponent 를 상속 받기에 ObjectInitializer 사용(Actor가 필요함)
	//# 하위 클래스에서 ObjectInitializer.SetDefaultSubobjectClass<>를 통해 타입 교체 가능
	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<USpyAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	//# UObject 를 상속 받기에 ObjectInitializer 미사용
	CharacterAttributeSet = CreateDefaultSubobject<USpyCharacterAttributeSet>(TEXT("CharacterAttributeSet"));
}

UAbilitySystemComponent* ASpyPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ASpyPlayerState::SetPlayerConnectionType(EPlayerConnectionType NewType)
{
	//# 수정 마크 표시
	//MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, PlayerConnectionType, this);
	PlayerConnectionType = NewType;
}

void ASpyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, PlayerConnectionType);
	DOREPLIFETIME(ThisClass, CharacterAssetData);
}

void ASpyPlayerState::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void ASpyPlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	check(AbilitySystemComponent);

	UE_LOG(LogTemp, Warning, TEXT("# PlayerState: InitAbilityActorInfo"));
	AbilitySystemComponent->InitAbilityActorInfo(this, GetPawn());
}

void ASpyPlayerState::Reset()
{
	Super::Reset();

	//# TODO
}

void ASpyPlayerState::ClientInitialize(AController* C)
{
	//# 클라이언트가 PS를 인식했을 때
	Super::ClientInitialize(C);

	//# TODO
}

void ASpyPlayerState::CopyProperties(APlayerState* PlayerState)
{
	//# 플레이어 상태가 갱신되거나 맵 이동 시에 이전 정보 남겨두는 용도
	Super::CopyProperties(PlayerState);

	//# TODO
}

void ASpyPlayerState::OnDeactivated()
{
	//# 게임에서 나가거나 연결이 끊겼을 때
	bool bDestory = false;

	switch (GetPlayerConnectionType())
	{
		case EPlayerConnectionType::ActivePlayer:
		case EPlayerConnectionType::DeactivePlayer:
		{
			bDestory = true;
			//# TODO
		}
		break;
		default:
		{
			bDestory = true;
			//# TODO
		}
		break;
	}

	//# PS를 남길지 바로 파괴할지 결정
	if (bDestory)
	{
		Super::OnDeactivated();
	}
}

void ASpyPlayerState::OnReactivated()
{
	//# 게임에 들어오거나 연결이 되었을 때
	Super::OnReactivated();

	if (GetPlayerConnectionType() == EPlayerConnectionType::DeactivePlayer)
	{
		SetPlayerConnectionType(EPlayerConnectionType::ActivePlayer);
	}
}

void ASpyPlayerState::SetCharacterAssetData(const USpyCharacterAssetData* InCharacterAssetData, bool bInIsBot)
{
	//# 서버에서만 세팅
	if (GetLocalRole() != ROLE_Authority)
		return;

	//# 이미 세팅됨
	if (CharacterAssetData)
		return;

	CharacterAssetData = InCharacterAssetData;

	for (const USpyAbilityData* AbilityData : CharacterAssetData->CharacterAssets.CommonSkills)
	{
		if (AbilityData)
		{
			AbilityData->GiveToAbilitySystem(AbilitySystemComponent, nullptr);
		}
	}

	for (const FCharacterAssetEntry& Entry : CharacterAssetData->CharacterAssets.AssetEntries)
	{
		//# AI 데이터 세팅
		if (bInIsBot && Entry.ClassType == SpyGameplayTags::Character_Class_AI)
		{
			for (const USpyAbilityData* AbilityData : Entry.ClassSkills)
			{
				if (AbilityData)
				{
					AbilityData->GiveToAbilitySystem(AbilitySystemComponent, nullptr);
				}
			}
		}
		//# 임시로 우선 한 개 클래스만 있다고 가정
		else
		{
			for (const USpyAbilityData* AbilityData : Entry.ClassSkills)
			{
				if (AbilityData)
				{
					AbilityData->GiveToAbilitySystem(AbilitySystemComponent, nullptr);
				}
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("# PlayerState: AbilityReady"));
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, NAME_AbilityReady);
}

void ASpyPlayerState::OnRep_CharacterAssetData()
{
	UE_LOG(LogTemp, Log, TEXT("PlayerState: OnRep_CharacterAssetData"));
}
