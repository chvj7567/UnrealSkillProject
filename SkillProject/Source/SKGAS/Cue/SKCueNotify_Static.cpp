// Fill out your copyright notice in the Description page of Project Settings.


#include "SKCueNotify_Static.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "SKGameplayEffectContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKCueNotify_Static)

USKCueNotify_Static::USKCueNotify_Static()
{
}

bool USKCueNotify_Static::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	bool Result = Super::OnExecute_Implementation(MyTarget, Parameters);

	ACharacter* TargetCharacter = Cast<ACharacter>(MyTarget);
	if (TargetCharacter == nullptr)
		return false;

	if (IsRunningDedicatedServer())
		return true;

    UParticleSystem* Particle = nullptr;
    if (FSKGameplayEffectContext* Context = FSKGameplayEffectContext::ExtractEffectContext(Parameters.EffectContext))
    {
        if (Context->IsCritical())
        {
            Particle = CriticalParticle;
        }
        else
        {
            Particle = NormalParticle;
        }
    }

	if (Particle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Particle, TargetCharacter->GetActorLocation(), TargetCharacter->GetActorRotation());
	}

	return Result;
}
