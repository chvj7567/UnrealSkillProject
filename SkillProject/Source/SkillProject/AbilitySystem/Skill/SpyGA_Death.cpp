// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyGA_Death.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Character/CommonInterface.Character.h"

void USpyGA_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
    {
        if (UCapsuleComponent* CapsuleComp = OwnerCharacter->GetCapsuleComponent())
        {
            CapsuleComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
        }

		ISpyCharacterRoot* RootPtr = Cast<ISpyCharacterRoot>(OwnerCharacter);
		if (ISpyTargetProvider* TargetingComp = RootPtr ? RootPtr->GetTargetProvider().GetInterface() : nullptr)
		{
			TargetingComp->FindTarget(0.f);
		}
    }
}
