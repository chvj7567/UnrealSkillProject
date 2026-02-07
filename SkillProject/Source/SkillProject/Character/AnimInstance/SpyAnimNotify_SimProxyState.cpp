// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyAnimNotify_SimProxyState.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Util/SpyGameplayTags.h"

void USpyAnimNotify_SimProxyState::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (ACharacter* OwnerCharacter = Cast<ACharacter>(MeshComp->GetOwner()))
	{
		if (UCharacterMovementComponent* CMC = OwnerCharacter->GetCharacterMovement())
		{
			if (bIsFreeState)
			{
				CMC->SetMovementMode(EMovementMode::MOVE_Flying);
			}
			else
			{
				CMC->SetMovementMode(EMovementMode::MOVE_Walking);
			}
		}

		if (UCapsuleComponent* CC = OwnerCharacter->GetCapsuleComponent())
		{
			if (bIsFreeState)
			{
				CC->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Ignore);
			}
			else
			{
				CC->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
			}
		}
	}
	
}
