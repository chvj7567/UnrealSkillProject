// Fill out your copyright notice in the Description page of Project Settings.


#include "System/SpyAIController.h"
#include "Data/SpyAIConfig.h"
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

	//# �����Ե� PlayerState�� �ο��Ͽ� �÷��̾�� ������ ������ ������ ���� ��
	bWantsPlayerState = true;

	//# ������ �и��Ǿ ��� AI ������ �ߴ����� ����
	bStopAILogicOnUnposses = false;

	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
	SetPerceptionComponent(*AIPerceptionComponent);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = AIConfig ? AIConfig->SightRadius : 500.f;
	SightConfig->LoseSightRadius = AIConfig ? AIConfig->LoseSightRadius : 700.f;
	SightConfig->PeripheralVisionAngleDegrees = AIConfig ? AIConfig->PeripheralVisionAngleDegrees : 90.f;
	SightConfig->SetMaxAge(AIConfig ? AIConfig->SightMaxAge : 5.f);
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = AIConfig ? AIConfig->HearingRange : 200.f;
	HearingConfig->SetMaxAge(AIConfig ? AIConfig->HearingMaxAge : 5.f);
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;

	DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("DamageConfig"));
	DamageConfig->SetMaxAge(AIConfig ? AIConfig->DamageMaxAge : 5.f);

	AIPerceptionComponent->ConfigureSense(*SightConfig);
	AIPerceptionComponent->ConfigureSense(*HearingConfig);
	AIPerceptionComponent->ConfigureSense(*DamageConfig);

	//# �þ߸� �켱 ������ ��
	AIPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
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
		//# �ð� ����
		//DrawDebugSphere(GetWorld(), Location, SightConfig->SightRadius, 32, FColor::Green, false, 0.f, 0, 1.5f);

		//# �þ߿� Ÿ���� ���̰� ���� ��׷� �������� �þ� ����
		//DrawDebugSphere(GetWorld(), Location, SightConfig->LoseSightRadius, 32, FColor::Yellow, false, 0.f, 0, 1.f);

		//# �þ߰�
		float HalfAngleRad = FMath::DegreesToRadians(SightConfig->PeripheralVisionAngleDegrees);

		//DrawDebugCone(GetWorld(), Location, Forward, SightConfig->SightRadius, HalfAngleRad, HalfAngleRad, 32, FColor::Green, false, 0.f, 0, 1.2f);
	}

	if (HearingConfig)
	{
		//# û�� ����
		//DrawDebugSphere(GetWorld(), Location, HearingConfig->HearingRange, 32, FColor::Blue, false, 0.f, 0, 1.5f );
	}
}

void ASpyAIController::InitPlayerState()
{
	//# ���� �� �ʱ�ȭ
	Super::InitPlayerState();
	BroadcastOnPlayerStateChanged();
}

void ASpyAIController::CleanupPlayerState()
{
	//# ���� �� ����
	BroadcastOnPlayerStateChanged();
	Super::CleanupPlayerState();
}

void ASpyAIController::OnRep_PlayerState()
{
	//# Ŭ���̾�Ʈ �� �ʱ�ȭ
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

	//# ���� PlayerState�� �ִٸ� �ʿ� �� ��������Ʈ ����
	if (LastSeenPlayerState != nullptr)
	{
		
	}

	//# �� PlayerState�� �Ҵ�Ǿ��� �� ��������Ʈ ����
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
	//# ���� ���� ��, ASC�� �ı��� ���� ��� �������� �ʵ��� �ƹ�Ÿ�� ������
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

	if (APlayerState* MyPS = Cast<APlayerState>(GetPawn()->GetPlayerState()))
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
		//# �ð� ����
		if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
		{
			//# ���� ��츸 Ÿ����
			if (Attitude == ETeamAttitude::Hostile)
			{
				UE_LOG(LogTemp, Warning, TEXT("# [SpyAIController] Sight: Detected %s"), *TargetActor->GetName());
				BlackboardComp->SetValueAsObject("TargetActor", TargetActor);
				BlackboardComp->SetValueAsVector("TargetLocation", Stimulus.StimulusLocation);
			}
		}
		//# û�� ����
		else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
		{
			/*if (Attitude == ETeamAttitude::Hostile)
			{
				UE_LOG(LogTemp, Warning, TEXT("# [SpyAIController] Hearing: Detected %s"), *Stimulus.StimulusLocation.ToString());
				BlackboardComp->SetValueAsVector("TargetLocation", Stimulus.StimulusLocation);
			}*/
		}
		//# ������ ����
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
		//# �������� �ʾ��� ��
		if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
		{
			UE_LOG(LogTemp, Warning, TEXT("# [SpyAIController] Sight: Lost %s"), *TargetActor->GetName());
			//BlackboardComp->ClearValue("TargetActor");
		}
	}
}
