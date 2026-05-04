// Fill out your copyright notice in the Description page of Project Settings.


#include "System/SpyAIController.h"
#include "Data/SpyAIConfig.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "Util/SpyGameplayTags.h"
#include "SKAbilitySystemGlobals.h"
#include "System/SpyPlayerState.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Damage.h"
#include "Attribute/SKAttributeSet.h"
#include "GameFramework/PlayerState.h"

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
}

void ASpyAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn == nullptr)
		return;

	//# Perception은 현재 타겟이 죽어도 새 이벤트를 안 쏘므로 Tick에서 주기적으로 재검증
	TargetRefreshAccumulator += DeltaSeconds;
	if (TargetRefreshAccumulator >= TargetRefreshInterval)
	{
		TargetRefreshAccumulator = 0.f;
		RefreshBlackboardTarget();
	}

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

	//# OnPossess 시점엔 BP 기본값이 이미 적용되어 AIConfig를 신뢰할 수 있음
	if (AIConfig && SightConfig && HearingConfig && DamageConfig)
	{
		SightConfig->SightRadius = AIConfig->SightRadius;
		SightConfig->LoseSightRadius = AIConfig->LoseSightRadius;
		SightConfig->PeripheralVisionAngleDegrees = AIConfig->PeripheralVisionAngleDegrees;
		SightConfig->SetMaxAge(AIConfig->SightMaxAge);

		HearingConfig->HearingRange = AIConfig->HearingRange;
		HearingConfig->SetMaxAge(AIConfig->HearingMaxAge);

		DamageConfig->SetMaxAge(AIConfig->DamageMaxAge);

		if (AIPerceptionComponent)
		{
			AIPerceptionComponent->ConfigureSense(*SightConfig);
			AIPerceptionComponent->ConfigureSense(*HearingConfig);
			AIPerceptionComponent->ConfigureSense(*DamageConfig);
			AIPerceptionComponent->RequestStimuliListenerUpdate();
		}
	}

	if (BehaviorTreeAsset)
	{
		RunBehaviorTree(BehaviorTreeAsset);
	}

	if (AIPerceptionComponent)
	{
		AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &ASpyAIController::OnTargetDetected);
	}

	//# BB.TargetActor 변경을 옵저버로 직접 추적 — 누가 클리어하는지 잡기 위함
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		const FBlackboard::FKey TargetKeyId = BB->GetKeyID(TEXT("TargetActor"));
		if (TargetKeyId != FBlackboard::InvalidKey)
		{
			BB->RegisterObserver(TargetKeyId, this,
				FOnBlackboardChangeNotification::CreateWeakLambda(this,
					[this](const UBlackboardComponent& InBB, FBlackboard::FKey InKey) -> EBlackboardNotificationResult
					{
						UObject* NewVal = InBB.GetValueAsObject(InBB.GetKeyName(InKey));
						const FString BotName = GetPawn() ? GetPawn()->GetName() : GetName();
						UE_LOG(LogTemp, Warning, TEXT("[SpyAI %s] BB.TargetActor 변경됨: %s"),
							*BotName,
							NewVal ? *NewVal->GetName() : TEXT("NULL"));
						return EBlackboardNotificationResult::ContinueObserving;
					}));
		}
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
	if (ASpyPlayerState* PS = GetPlayerState<ASpyPlayerState>())
	{
		PS->SetGenericTeamId(NewTeamID);
	}
}

FGenericTeamId ASpyAIController::GetGenericTeamId() const
{
	if (const ASpyPlayerState* PS = GetPlayerState<ASpyPlayerState>())
	{
		return PS->GetGenericTeamId();
	}
	return FGenericTeamId::NoTeam;
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

	//# 내가 죽었으면 타겟 갱신 중단
	if (APlayerState* MyPS = Cast<APlayerState>(MyPawn->GetPlayerState()))
	{
		if (UAbilitySystemComponent* MyASC = MyPS->FindComponentByClass<UAbilitySystemComponent>())
		{
			if (MyASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Death))
				return;
		}
	}

	//# 데미지는 즉시 그 가해자를 타겟으로 (등 뒤 피격 대응)
	if (Stimulus.WasSuccessfullySensed() && Stimulus.Type == UAISense::GetSenseID<UAISense_Damage>())
	{
		if (APawn* DamagerPawn = ResolvePawnFromActor(Actor))
		{
			if (IsHostileAndAlive(DamagerPawn))
			{
				BlackboardComp->SetValueAsObject("TargetActor", DamagerPawn);
				BlackboardComp->SetValueAsVector("TargetLocation", DamagerPawn->GetActorLocation());
				return;
			}
		}
	}

	//# 그 외 시각/청각 감지는 통합 재평가 (현재 타겟이 살아있으면 유지, 죽었으면 가장 가까운 살아있는 적으로 교체)
	RefreshBlackboardTarget();
}

void ASpyAIController::RefreshBlackboardTarget()
{
	UBlackboardComponent* BB = GetBlackboardComponent();
	APawn* MyPawn = GetPawn();
	if (BB == nullptr || MyPawn == nullptr || AIPerceptionComponent == nullptr)
		return;

	//# 내가 죽었으면 갱신하지 않음
	if (APlayerState* MyPS = Cast<APlayerState>(MyPawn->GetPlayerState()))
	{
		if (UAbilitySystemComponent* MyASC = MyPS->FindComponentByClass<UAbilitySystemComponent>())
		{
			if (MyASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Death))
				return;
		}
	}

	//# 현재 타겟이 "명확히 사라진 상태"인지 판정 (사망/파괴만 사라짐으로 본다 — 시야 상실은 사라짐 아님)
	AActor* CurrentTarget = Cast<AActor>(BB->GetValueAsObject("TargetActor"));
	bool bCurrentGone = false;
	if (CurrentTarget == nullptr)
	{
		bCurrentGone = true;
	}
	else if (!IsValid(CurrentTarget))
	{
		bCurrentGone = true;
	}
	else
	{
		APlayerState* TgtPS = nullptr;
		if (APawn* TgtPawn = Cast<APawn>(CurrentTarget))
			TgtPS = TgtPawn->GetPlayerState();
		else
			TgtPS = Cast<APlayerState>(CurrentTarget);

		if (TgtPS)
		{
			if (UAbilitySystemComponent* TgtASC = TgtPS->FindComponentByClass<UAbilitySystemComponent>())
			{
				if (TgtASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Death))
					bCurrentGone = true;
				else if (TgtASC->GetNumericAttribute(USKAttributeSet::GetHealthAttribute()) <= 0.f)
					bCurrentGone = true;
			}
		}
	}

	//# 살아있는 타겟이면 위치만 갱신하고 유지 (시야 잃어도 BB는 그대로)
	if (!bCurrentGone)
	{
		BB->SetValueAsVector("TargetLocation", CurrentTarget->GetActorLocation());
		return;
	}

	const FString BotName = MyPawn->GetName();
	UE_LOG(LogTemp, Warning, TEXT("[SpyAI %s] Refresh: bCurrentGone=true (Target=%s, IsValid=%d) — 새 타겟 검색"),
		*BotName,
		CurrentTarget ? *CurrentTarget->GetName() : TEXT("NULL"),
		CurrentTarget ? IsValid(CurrentTarget) : 0);

	//# 현재 타겟이 사라짐 → 시야 내 살아있는 적 중 가장 가까운 적으로 교체
	TArray<AActor*> PerceivedActors;
	AIPerceptionComponent->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);

	APawn* BestPawn = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	const FVector MyLoc = MyPawn->GetActorLocation();

	for (AActor* Perceived : PerceivedActors)
	{
		APawn* CandidatePawn = ResolvePawnFromActor(Perceived);
		if (!IsValid(CandidatePawn))
			continue;
		if (!IsHostileAndAlive(CandidatePawn))
			continue;

		const float DistSq = FVector::DistSquared(MyLoc, CandidatePawn->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestPawn = CandidatePawn;
		}
	}

	if (BestPawn)
	{
		BB->SetValueAsObject("TargetActor", BestPawn);
		BB->SetValueAsVector("TargetLocation", BestPawn->GetActorLocation());

		UE_LOG(LogTemp, Warning, TEXT("[SpyAI %s] Refresh SET: %s (LastLogged=%s)"),
			*BotName,
			*BestPawn->GetName(),
			LastLoggedTarget.IsValid() ? *LastLoggedTarget.Get()->GetName() : TEXT("NULL"));

		LastLoggedTarget = BestPawn;

		//# 새 타겟의 OnDestroyed 델리게이트 바인딩 — destroy 시점 직접 잡음
		if (AActor* PrevTracked = TrackedTargetForDestroyDetection.Get())
		{
			if (PrevTracked != BestPawn)
			{
				PrevTracked->OnDestroyed.RemoveDynamic(this, &ASpyAIController::OnTrackedTargetDestroyed);
				TrackedTargetForDestroyDetection = BestPawn;
				BestPawn->OnDestroyed.AddDynamic(this, &ASpyAIController::OnTrackedTargetDestroyed);
			}
		}
		else
		{
			TrackedTargetForDestroyDetection = BestPawn;
			BestPawn->OnDestroyed.AddDynamic(this, &ASpyAIController::OnTrackedTargetDestroyed);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[SpyAI %s] Refresh CLEAR: bCurrentGone=true, 시야 내 적 없음 — BB 클리어"), *BotName);
		BB->ClearValue("TargetActor");
		LastLoggedTarget = nullptr;
	}
}

void ASpyAIController::OnTrackedTargetDestroyed(AActor* DestroyedActor)
{
	UE_LOG(LogTemp, Error, TEXT("[SpyAI] !!! 타겟 폰 OnDestroyed 발화: %s !!!"),
		DestroyedActor ? *DestroyedActor->GetName() : TEXT("NULL"));
}

bool ASpyAIController::IsHostileAndAlive(AActor* InActor) const
{
	if (!IsValid(InActor))
		return false;

	if (GetTeamAttitudeTowards(*InActor) != ETeamAttitude::Hostile)
		return false;

	APlayerState* PS = nullptr;
	if (APawn* OtherPawn = Cast<APawn>(InActor))
		PS = OtherPawn->GetPlayerState();
	else
		PS = Cast<APlayerState>(InActor);

	if (PS)
	{
		if (UAbilitySystemComponent* ASC = PS->FindComponentByClass<UAbilitySystemComponent>())
		{
			if (ASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Death))
				return false;
			if (ASC->GetNumericAttribute(USKAttributeSet::GetHealthAttribute()) <= 0.f)
				return false;
		}
	}
	return true;
}

APawn* ASpyAIController::ResolvePawnFromActor(AActor* InActor) const
{
	if (InActor == nullptr)
		return nullptr;
	if (APawn* P = Cast<APawn>(InActor))
		return P;
	if (APlayerState* PS = Cast<APlayerState>(InActor))
		return PS->GetPawn();
	return nullptr;
}
