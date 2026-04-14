// Fill out your copyright notice in the Description page of Project Settings.


#include "System/SpyAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "SKGameplayTags.h"
#include "SKAbilitySystemGlobals.h"
#include "System/SpyPlayerState.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Attribute/SKAttributeSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyAIController)

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
	SightConfig->SightRadius = 500.f;
	SightConfig->LoseSightRadius = 700.f;
	SightConfig->PeripheralVisionAngleDegrees = 90.f;
	SightConfig->SetMaxAge(5.f);
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = 200.f;
	HearingConfig->SetMaxAge(5.f);
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;

	DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("DamageConfig"));
	DamageConfig->SetMaxAge(5.f);

	AIPerceptionComponent->ConfigureSense(*SightConfig);
	AIPerceptionComponent->ConfigureSense(*HearingConfig);
	AIPerceptionComponent->ConfigureSense(*DamageConfig);

	//# 시야를 우선 순위로 둠
	AIPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());

	TeamID = FGenericTeamId(1);
}

void ASpyAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn == nullptr)
		return;

	FVector Location = ControlledPawn->GetActorLocation();
	FVector Forward = ControlledPawn->GetActorForwardVector();

	if (SightConfig)
	{
		//# 시각 범위
		//DrawDebugSphere(GetWorld(), Location, SightConfig->SightRadius, 32, FColor::Green, false, 0.f, 0, 1.5f);

		//# 시야에 타겟이 보이고 나서 어그로 없어지는 시야 범위
		//DrawDebugSphere(GetWorld(), Location, SightConfig->LoseSightRadius, 32, FColor::Yellow, false, 0.f, 0, 1.f);

		//# 시야각
		float HalfAngleRad = FMath::DegreesToRadians(SightConfig->PeripheralVisionAngleDegrees);

		//DrawDebugCone(GetWorld(), Location, Forward, SightConfig->SightRadius, HalfAngleRad, HalfAngleRad, 32, FColor::Green, false, 0.f, 0, 1.2f);
	}

	if (HearingConfig)
	{
		//# 청각 범위
		//DrawDebugSphere(GetWorld(), Location, HearingConfig->HearingRange, 32, FColor::Blue, false, 0.f, 0, 1.5f );
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

void ASpyAIController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	TeamID = NewTeamID;
}

FGenericTeamId ASpyAIController::GetGenericTeamId() const
{
	return TeamID;
}

ETeamAttitude::Type ASpyAIController::GetTeamAttitudeTowards(const AActor& Other) const
{
	const ASpyPlayerState* TargetPS = Cast<ASpyPlayerState>(&Other);
	if (TargetPS == nullptr)
	{
		const APawn* OtherPawn = Cast<APawn>(&Other);
		if (OtherPawn == nullptr)
			return ETeamAttitude::Neutral;
		
		TargetPS = OtherPawn->GetPlayerState<ASpyPlayerState>();
		if (TargetPS == nullptr)
			return ETeamAttitude::Neutral;
	}
	
	if (TargetPS->GetGenericTeamId() == GetGenericTeamId())
	{
		return ETeamAttitude::Friendly;
	}
	else
	{
		return ETeamAttitude::Hostile;
	}
}

USpyAbilitySystemComponent* ASpyAIController::GetSpyAbilitySystemComponent() const
{
	return Cast<USpyAbilitySystemComponent>(USKAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PlayerState));
}

void ASpyAIController::OnTargetDetected(AActor* Actor, FAIStimulus Stimulus)
{
	UBlackboardComponent* BlackboardComp = GetBlackboardComponent();
	if (BlackboardComp == nullptr)
		return;

	APawn* MyPawn = GetPawn();
	if (MyPawn == nullptr)
		return;

	if (APlayerState* MyPS = Cast<APlayerState>(MyPawn->GetPlayerState()))
	{
		if (UAbilitySystemComponent* MyASC = MyPS->FindComponentByClass<UAbilitySystemComponent>())
		{
			if (MyASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Death))
			{
				UE_LOG(LogTemp, Warning, TEXT("# [SpyAIController] MyActor is Die"));
				return;
			}
		}
	}

	AActor* TargetActor = nullptr;
	if (APlayerState* TargetPS = Cast<APlayerState>(Actor))
	{
		TargetActor = TargetPS->GetPawn();

		if (UAbilitySystemComponent* MyASC = TargetPS->FindComponentByClass<UAbilitySystemComponent>())
		{
			bool bIsDead = MyASC->GetNumericAttribute(USKAttributeSet::GetHealthAttribute()) <= 0.0f;
			if (bIsDead || MyASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Death))
			{
				UE_LOG(LogTemp, Warning, TEXT("# [SpyAIController] TargetActor is Die"));
				return;
			}
		}
	}
	else
	{
		TargetActor = Actor;
	}

	if (TargetActor == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("# [SpyAIController] TargetActor is Null"));
		return;
	}

	ETeamAttitude::Type Attitude = GetTeamAttitudeTowards(*Actor);
	const FName SenseName = Stimulus.Type.Name;

	if (Stimulus.WasSuccessfullySensed())
	{
		//# 시각 감지
		if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
		{
			//# 적일 경우만 타겟팅
			if (Attitude == ETeamAttitude::Hostile)
			{
				UE_LOG(LogTemp, Warning, TEXT("# [SpyAIController] Sight: Detected %s"), *TargetActor->GetName());
				BlackboardComp->SetValueAsObject("TargetActor", TargetActor);
				BlackboardComp->SetValueAsVector("TargetLocation", Stimulus.StimulusLocation);
			}
		}
		//# 청각 감지
		else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
		{
			/*if (Attitude == ETeamAttitude::Hostile)
			{
				UE_LOG(LogTemp, Warning, TEXT("# [SpyAIController] Hearing: Detected %s"), *Stimulus.StimulusLocation.ToString());
				BlackboardComp->SetValueAsVector("TargetLocation", Stimulus.StimulusLocation);
			}*/
		}
		//# 데미지 감지
		else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Damage>())
		{
			if (Attitude == ETeamAttitude::Hostile)
			{
				UE_LOG(LogTemp, Warning, TEXT("# [SpyAIController] Damage: Detected %s"), *TargetActor->GetName());
				BlackboardComp->SetValueAsObject("TargetActor", TargetActor);
			}
		}
	}
	else
	{
		//# 감지되지 않았을 때
		if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
		{
			UE_LOG(LogTemp, Warning, TEXT("# [SpyAIController] Sight: Lost %s"), *TargetActor->GetName());
			//BlackboardComp->ClearValue("TargetActor");
		}
	}
}
