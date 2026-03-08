// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyDamageCalculation.h"
#include "SKGameplayEffectContext.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "AIController.h"
#include "GameFramework/PlayerState.h"

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
	if (FSKGameplayEffectContext* CustomContext = FSKGameplayEffectContext::ExtractEffectContext(ContextHandle))
	{
		const bool bIsCritical = FMath::FRand() <= 0.5f;
		CustomContext->SetIsCritical(bIsCritical);
	}

    bool bIsSourceAI = false;
    if (AActor* Instigator = Spec.GetContext().GetInstigator())
    {
        // 1. 직접 AIController인지 확인
        if (Cast<AAIController>(Instigator))
        {
            bIsSourceAI = true;
        }
        // 2. 만약 Instigator가 Pawn(캐릭터)이라면 그 컨트롤러가 AIController인지 확인
        else if (APawn* Pawn = Cast<APawn>(Instigator))
        {
            if (Cast<AAIController>(Pawn->GetController()))
            {
                bIsSourceAI = true;
            }
        }
        // 3. 만약 Instigator가 PlayerState라면 그 컨트롤러가 AIController인지 확인
        else if (APlayerState* PS = Cast<APlayerState>(Instigator))
        {
            if (Cast<AAIController>(PS->GetPlayerController()) == nullptr && PS->IsABot())
            {
                bIsSourceAI = true;
            }
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
