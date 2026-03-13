// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyGA_Targeting.h"
#include "Character/SpyCharacter.h"
#include "ManagerComponent/SpyTargetingManagerComponent.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGA_Targeting)

void USpyGA_Targeting::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	do
	{
		if (HasAuthority(&CurrentActivationInfo) == false)
			break;

		if (TargetingEffectClass == nullptr)
			break;

		ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
		if (OwnerCharacter == nullptr)
			break;

		USpyTargetingManagerComponent* TargetingComp = OwnerCharacter->FindComponentByClass<USpyTargetingManagerComponent>();
		if (TargetingComp == nullptr)
			break;

		bool bFindTarget = TargetingComp->FindTarget(500.f);
		if (bFindTarget == false)
			break;

		TWeakObjectPtr<AActor> Target = TargetingComp->GetTarget();
		if (Target.IsValid() == false)
			break;

		ASpyCharacter* TargetCharacter = Cast<ASpyCharacter>(Target.Get());
		if (TargetCharacter == nullptr)
			break;

		USpyAbilitySystemComponent* TargetASC = TargetCharacter->GetSpyAbilitySystemComponent();
		if (TargetASC == nullptr)
			break;

		FGameplayEffectContextHandle EffectContext = TargetASC->MakeEffectContext();
		EffectContext.AddInstigator(GetOwningActorFromActorInfo(), GetAvatarActorFromActorInfo());

		FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(TargetingEffectClass, 1.0f, EffectContext);
		if (SpecHandle.IsValid() == false)
			break;

		FActiveGameplayEffectHandle AppliedHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		if (AppliedHandle.WasSuccessfullyApplied())
		{
			UE_LOG(LogTemp, Warning, TEXT("# [SpyGA_Targeting] GE Successfully Applied! Effect: %s"), *SpecHandle.Data.Get()->Def->GetName());
		}

	} while (false);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
