// Fill out your copyright notice in the Description page of Project Settings.


#include "System/SpyAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "SKAbilitySystemGlobals.h"
#include "GameFramework/PlayerState.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Damage.h"

ASpyAIController::ASpyAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	//# 봇에게도 PlayerState를 부여하여 플레이어와 동일한 데이터 구조를 갖게 함
	bWantsPlayerState = true;

	//# 폰에서 분리되어도 즉시 AI 로직을 중단하지 않음
	bStopAILogicOnUnposses = false;

	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
	SetPerceptionComponent(*AIPerceptionComponent);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 1500.f;
	SightConfig->LoseSightRadius = 1700.f;
	SightConfig->PeripheralVisionAngleDegrees = 90.f;
	SightConfig->SetMaxAge(5.f);
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = 1200.f;
	HearingConfig->SetMaxAge(5.f);
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;

	DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("DamageConfig"));
	DamageConfig->SetMaxAge(5.f);

	AIPerceptionComponent->ConfigureSense(*SightConfig);
	AIPerceptionComponent->ConfigureSense(*HearingConfig);
	AIPerceptionComponent->ConfigureSense(*DamageConfig);
	AIPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
}

void ASpyAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) return;

	FVector Location = ControlledPawn->GetActorLocation();
	FVector Forward = ControlledPawn->GetActorForwardVector();

	if (SightConfig)
	{
		// Sight Radius
		DrawDebugSphere(
			GetWorld(),
			Location,
			SightConfig->SightRadius,
			32,
			FColor::Green,
			false,     // ? persistent 아님
			0.f,
			0,
			1.5f
		);

		// Lose Sight Radius
		DrawDebugSphere(
			GetWorld(),
			Location,
			SightConfig->LoseSightRadius,
			32,
			FColor::Yellow,
			false,
			0.f,
			0,
			1.f
		);

		// 시야각 콘
		float HalfAngleRad =
			FMath::DegreesToRadians(
				SightConfig->PeripheralVisionAngleDegrees * 0.5f
			);

		DrawDebugCone(
			GetWorld(),
			Location,
			Forward,
			SightConfig->SightRadius,
			HalfAngleRad,
			HalfAngleRad,
			32,
			FColor::Green,
			false,
			0.f,
			0,
			1.2f
		);
	}

	if (HearingConfig)
	{
		DrawDebugSphere(
			GetWorld(),
			Location,
			HearingConfig->HearingRange,
			32,
			FColor::Blue,
			false,
			0.f,
			0,
			1.5f
		);
	}
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

void ASpyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (BehaviorTreeAsset)
	{
		RunBehaviorTree(BehaviorTreeAsset);
	}

	if (AIPerceptionComponent)
	{
		AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &ASpyAIController::OnTargetDetected);
	}
}

void ASpyAIController::SetBehaviorTree(UBehaviorTree* InBehaviorTreeAsset)
{
	BehaviorTreeAsset = InBehaviorTreeAsset;
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

void ASpyAIController::OnTargetDetected(AActor* Actor, FAIStimulus Stimulus)
{
	if (!GetBlackboardComponent()) return;

	const FName SenseName = Stimulus.Type.Name;

	if (Stimulus.WasSuccessfullySensed())
	{
		UE_LOG(LogTemp, Warning, TEXT("# Detected %s by %s"),
			*Actor->GetName(),
			*SenseName.ToString());

		GetBlackboardComponent()->SetValueAsObject("TargetActor", Actor);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("# Lost %s by %s"),
			*Actor->GetName(),
			*SenseName.ToString());

		GetBlackboardComponent()->ClearValue("TargetActor");
	}
}
