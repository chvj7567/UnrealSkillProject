// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyDamageCalculation.h"
#include "SKGameplayEffectContext.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "AIController.h"
#include "System/SpyPlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyDamageCalculation)

USpyDamageCalculation::USpyDamageCalculation()
{
	CaptureHealth(EGameplayEffectAttributeCaptureSource::Target);
}

float USpyDamageCalculation::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	FAggregatorEvaluateParameters EvaluateParameters;
	GetSourceAndTargetTags(Spec, EvaluateParameters);

	float Health = 0.0f;
	GetCapturedAttributeMagnitude(HealthDef, Spec, EvaluateParameters, Health);

	FGameplayEffectContextHandle ContextHandle = Spec.GetContext();
    FSKGameplayEffectContext* CustomContext = FSKGameplayEffectContext::ExtractEffectContext(ContextHandle);
    if (CustomContext == nullptr)
        return 0.f;

    const bool bIsCritical = FMath::FRand() <= 0.5f;
    CustomContext->SetIsCritical(bIsCritical);

    AActor* Instigator = ContextHandle.GetInstigator();
    if (Instigator == nullptr)
        return 0.f;

    //# 팀 확인
    ASpyPlayerState* SourcePS = nullptr;
    if (ASpyPlayerState* PS = Cast<ASpyPlayerState>(Instigator)) {
        SourcePS = PS;
    }
    else if (APawn* Pawn = Cast<APawn>(Instigator)) {
        SourcePS = Pawn->GetPlayerState<ASpyPlayerState>();
    }
    else if (AController* PC = Cast<AController>(Instigator)) {
        SourcePS = Cast<ASpyPlayerState>(PC->PlayerState);
    }

    AActor* TargetActor = CustomContext->GetActors()[0].Get();
    if (TargetActor == nullptr)
        return 0.f;

    ASpyPlayerState* TargetPS = nullptr;
    if (ASpyPlayerState* PS = Cast<ASpyPlayerState>(TargetActor)) {
        TargetPS = PS;
    }
    else if (APawn* Pawn = Cast<APawn>(TargetActor)) {
        TargetPS = Pawn->GetPlayerState<ASpyPlayerState>();
    }

    if (SourcePS && TargetPS)
    {
        //# 같은 팀이면 데미지 X
        if (SourcePS->GetGenericTeamId() == TargetPS->GetGenericTeamId())
            return 0.f;
    }

    //# AI 여부 확인
    bool bIsSourceAI = false;
    if (Cast<AAIController>(Instigator))
    {
        bIsSourceAI = true;
    }
    else if (APawn* Pawn = Cast<APawn>(Instigator))
    {
        if (Cast<AAIController>(Pawn->GetController()))
        {
            bIsSourceAI = true;
        }
    }
    else if (APlayerState* PS = Cast<APlayerState>(Instigator))
    {
        if (Cast<AAIController>(PS->GetPlayerController()) == nullptr && PS->IsABot())
        {
            bIsSourceAI = true;
        }
    }

    float FinalDamage = 100.f;
    if (bIsSourceAI)
    {
        FinalDamage = 10.f;
        UE_LOG(LogTemp, Warning, TEXT("# [SpyDamageCalc] AI: Damage %f"), FinalDamage);
    }
    else
    {
        FinalDamage = 30.f;
        UE_LOG(LogTemp, Warning, TEXT("# [SpyDamageCalc] Player: Damage %f"), FinalDamage);
    }

	return Health < FinalDamage ? -Health : -FinalDamage;
}
