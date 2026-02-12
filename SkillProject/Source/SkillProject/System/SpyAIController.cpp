// Fill out your copyright notice in the Description page of Project Settings.


#include "System/SpyAIController.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "SKAbilitySystemGlobals.h"
#include "GameFramework/PlayerState.h"

ASpyAIController::ASpyAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	//# 봇에게도 PlayerState를 부여하여 플레이어와 동일한 데이터 구조를 갖게 함
	bWantsPlayerState = true;

	//# 폰에서 분리되어도 즉시 AI 로직을 중단하지 않음
	bStopAILogicOnUnposses = false;
}

void ASpyAIController::InitPlayerState()
{
	//# 서버 측 초기화
	Super::InitPlayerState();
	BroadcastOnPlayerStateChanged();
}

void ASpyAIController::CleanupPlayerState()
{
	//# 해제 시 정리
	BroadcastOnPlayerStateChanged();
	Super::CleanupPlayerState();
}

void ASpyAIController::OnRep_PlayerState()
{
	//# 클라이언트 측 초기화
	Super::OnRep_PlayerState();
	BroadcastOnPlayerStateChanged();
}

void ASpyAIController::BroadcastOnPlayerStateChanged()
{
	OnPlayerStateChanged();

	//# 이전 PlayerState가 있다면 필요 시 델리게이트 해제
	if (LastSeenPlayerState != nullptr)
	{
		
	}

	//# 새 PlayerState가 할당되었을 때 델리게이트 설정
	if (PlayerState != nullptr)
	{
		
	}

	LastSeenPlayerState = PlayerState;
}

void ASpyAIController::OnPlayerStateChanged()
{
	// 하위 클래스에서 활용할 수 있도록 비워둠
}

void ASpyAIController::OnUnPossess()
{
	//# 빙의 해제 시, ASC가 파괴될 폰을 계속 참조하지 않도록 아바타를 정리함
	if (APawn* PawnBeingUnpossessed = GetPawn())
	{
		if (UAbilitySystemComponent* ASC = GetSpyAbilitySystemComponent())
		{
			if (ASC->GetAvatarActor() == PawnBeingUnpossessed)
			{
				ASC->SetAvatarActor(nullptr);
			}
		}
	}

	Super::OnUnPossess();
}

USpyAbilitySystemComponent* ASpyAIController::GetSpyAbilitySystemComponent() const
{
	return Cast<USpyAbilitySystemComponent>(USKAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PlayerState));
}