// Copyright Epic Games, Inc. All Rights Reserved.

#include "SpyAbilityData.h"
#include "SKAbilitySystemComponent.h"
#include "Attribute/SKAttributeSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyAbilityData)

void FSpyAbilitySet_GrantedHandles::AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle)
{
	if (Handle.IsValid())
	{
		AbilitySpecHandles.Add(Handle);
	}
}

void FSpyAbilitySet_GrantedHandles::AddGameplayEffectHandle(const FActiveGameplayEffectHandle& Handle)
{
	if (Handle.IsValid())
	{
		GameplayEffectHandles.Add(Handle);
	}
}

void FSpyAbilitySet_GrantedHandles::AddAttributeSet(UAttributeSet* Set)
{
	GrantedAttributeSets.Add(Set);
}

void FSpyAbilitySet_GrantedHandles::TakeFromAbilitySystem(UAbilitySystemComponent* SpyASC)
{
	check(SpyASC);

	if (SpyASC->IsOwnerActorAuthoritative() == false)
	{
		return;
	}

	for (const FGameplayAbilitySpecHandle& Handle : AbilitySpecHandles)
	{
		if (Handle.IsValid())
		{
			SpyASC->ClearAbility(Handle);
		}
	}

	for (const FActiveGameplayEffectHandle& Handle : GameplayEffectHandles)
	{
		if (Handle.IsValid())
		{
			SpyASC->RemoveActiveGameplayEffect(Handle);
		}
	}

	for (UAttributeSet* Set : GrantedAttributeSets)
	{
		SpyASC->RemoveSpawnedAttribute(Set);
	}

	AbilitySpecHandles.Reset();
	GameplayEffectHandles.Reset();
	GrantedAttributeSets.Reset();
}

USpyAbilityData::USpyAbilityData(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void USpyAbilityData::GiveToAbilitySystem(UAbilitySystemComponent* SpyASC, FSpyAbilitySet_GrantedHandles* OutGrantedHandles, UObject* SourceObject) const
{
	check(SpyASC);

	if (SpyASC->IsOwnerActorAuthoritative() == false)
		return;

	//# 1. AttributeSet 부여
	for (int32 SetIndex = 0; SetIndex < GrantedAttributes.Num(); ++SetIndex)
	{
		const FSpyAbilitySet_AttributeSet& SetToGrant = GrantedAttributes[SetIndex];

		//# 유효 확인
		if (IsValid(SetToGrant.AttributeSet) == false)
			continue;

		//# 이미 받은 Attribute인지 확인
		if (SpyASC->GetAttributeSet(SetToGrant.AttributeSet))
			continue;

		UAttributeSet* NewSet = NewObject<UAttributeSet>(SpyASC->GetOwner(), SetToGrant.AttributeSet);
		SpyASC->AddAttributeSetSubobject(NewSet);

		UE_LOG(LogTemp, Log, TEXT("[SpyAbilityData] Grant AttributeSet %s"), *NewSet->GetName());

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddAttributeSet(NewSet);
		}
	}

	//# 2. GameplayEffect 부여
	for (int32 EffectIndex = 0; EffectIndex < GrantedGameplayEffects.Num(); ++EffectIndex)
	{
		const FSpyAbilitySet_GameplayEffect& EffectToGrant = GrantedGameplayEffects[EffectIndex];

		if (IsValid(EffectToGrant.GameplayEffect) == false)
			continue;

		const UGameplayEffect* GameplayEffect = EffectToGrant.GameplayEffect->GetDefaultObject<UGameplayEffect>();
		const FActiveGameplayEffectHandle GameplayEffectHandle = SpyASC->ApplyGameplayEffectToSelf(GameplayEffect, EffectToGrant.EffectLevel, SpyASC->MakeEffectContext());

		UE_LOG(LogTemp, Log, TEXT("[SpyAbilityData] Grant Effect %s"), *GameplayEffect->GetName());

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddGameplayEffectHandle(GameplayEffectHandle);
		}
	}

	//# 3. GameplayAbility 부여
	for (int32 AbilityIndex = 0; AbilityIndex < GrantedGameplayAbilities.Num(); ++AbilityIndex)
	{
		const FSpyAbilitySet_GameplayAbility& AbilityToGrant = GrantedGameplayAbilities[AbilityIndex];

		if (IsValid(AbilityToGrant.Ability) == false)
			continue;

		UGameplayAbility* AbilityCDO = AbilityToGrant.Ability->GetDefaultObject<UGameplayAbility>();

		FGameplayAbilitySpec AbilitySpec(AbilityCDO, AbilityToGrant.AbilityLevel);
		AbilitySpec.SourceObject = SourceObject;
		AbilitySpec.DynamicAbilityTags.AddTag(AbilityToGrant.InputTag);

		const FGameplayAbilitySpecHandle AbilitySpecHandle = SpyASC->GiveAbility(AbilitySpec);

		UE_LOG(LogTemp, Log, TEXT("[SpyAbilityData] Grant Ability %s"), *AbilityCDO->GetName());

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddAbilitySpecHandle(AbilitySpecHandle);
		}
	}
}
