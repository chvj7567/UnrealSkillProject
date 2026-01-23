// Fill out your copyright notice in the Description page of Project Settings.


#include "System/SpyPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Character/SpyCharacter.h"
#include "UI/SpyHPBar.h"
#include "UI/SpyUserWidget.h"
#include "Util/SpyGameplayTags.h"
#include "Character/SpyCharacterAttributeSet.h"
#include "SKAbilitySystemComponent.h"
#include "Components/GameFrameworkComponentManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyPlayerState)

const FName ASpyPlayerState::NAME_AbilityReady("AbilitiesReady");

ASpyPlayerState::ASpyPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	//# 업데이트 주기를 빠르게 설정
	NetUpdateFrequency = 100.0f;

	//# UActorComponent 를 상속 받기에 ObjectInitializer 사용(Actor가 필요함)
	//# 하위 클래스에서 ObjectInitializer.SetDefaultSubobjectClass<>를 통해 타입 교체 가능
	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<USKAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	//# UObject 를 상속 받기에 ObjectInitializer 미사용
	CharacterAttributeSet = CreateDefaultSubobject<USpyCharacterAttributeSet>(TEXT("CharacterAttributeSet"));
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
}

void ASpyPlayerState::Multicast_Death_Implementation()
{
	RemoveState(SpyGameplayTags::Character_State_Survival_Alive);
	OwnerCharacter->Death();
}

void ASpyPlayerState::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void ASpyPlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	check(AbilitySystemComponent);
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

void ASpyPlayerState::Initialize()
{
	if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetPawn()))
	{
		OwnerCharacter = SpyCharacter;
	}
}

bool ASpyPlayerState::HasState(FGameplayTag Tag)
{
	if (OwnerCharacter == nullptr)
		return false;

	if (OwnerCharacter->GetSKAbilitySystemComponent()->HasMatchingGameplayTag(Tag))
		return true;

	return false;
}

void ASpyPlayerState::AddState(FGameplayTag Tag)
{
	if (OwnerCharacter == nullptr)
		return;

	if (OwnerCharacter->HasAuthority() == false)
		return;

	if (HasState(Tag))
		return;

	UE_LOG(LogTemp, Warning, TEXT("# Server AddState %s"), *Tag.ToString());
	OwnerCharacter->GetSKAbilitySystemComponent()->AddReplicatedLooseGameplayTag(Tag);
}

void ASpyPlayerState::RemoveState(FGameplayTag Tag)
{
	if (OwnerCharacter == nullptr)
		return;

	if (OwnerCharacter->HasAuthority() == false)
		return;

	if (HasState(Tag) == false)
		return;

	UE_LOG(LogTemp, Warning, TEXT("# Server RemoveState %s"), * Tag.ToString());
	OwnerCharacter->GetSKAbilitySystemComponent()->RemoveReplicatedLooseGameplayTag(Tag);
}

void ASpyPlayerState::ToggleState(FGameplayTag Tag)
{
	if (HasState(Tag))
	{
		RemoveState(Tag);
	}
	else
	{
		AddState(Tag);
	}
}

void ASpyPlayerState::SetCharacterAssetData(const USpyCharacterAssetData* InCharacterAssetData)
{
	//# 서버에서만 세팅
	if (GetLocalRole() != ROLE_Authority)
		return;

	CharacterAssetData = InCharacterAssetData;

	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, NAME_AbilityReady);
}

void ASpyPlayerState::OnRep_CharacterAssetData()
{
}
