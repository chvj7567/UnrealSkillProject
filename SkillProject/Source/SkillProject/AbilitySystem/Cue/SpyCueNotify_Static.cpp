// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Cue/SpyCueNotify_Static.h"
#include "Character/SpyCharacter.h"
#include "Kismet/GameplayStatics.h"

USpyCueNotify_Static::USpyCueNotify_Static()
{
}

bool USpyCueNotify_Static::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	bool Result = Super::OnExecute_Implementation(MyTarget, Parameters);

	ACharacter* TargetCharacter = Cast<ACharacter>(MyTarget);
	if (TargetCharacter == nullptr)
		return false;

	if (IsRunningDedicatedServer())
		return true;

	if (AnimMontage)
	{
		if (UAnimInstance* AnimInstance = TargetCharacter->GetMesh()->GetAnimInstance())
		{
			AnimInstance->Montage_Play(AnimMontage, 1.0f);
		}
	}

	if (Particle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Particle, TargetCharacter->GetActorLocation(), TargetCharacter->GetActorRotation());
	}

	return Result;
}
