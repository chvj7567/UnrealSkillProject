// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyAbilitySystemComponent.h"
#include "Util/SpyGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyAbilitySystemComponent)

USpyAbilitySystemComponent::USpyAbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void USpyAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (InputTag.IsValid())
	{
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			if (AbilitySpec.Ability && (AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag)))
			{
				InputPressedSpecHandles.AddUnique(AbilitySpec.Handle);
				InputHeldSpecHandles.AddUnique(AbilitySpec.Handle);
			}
		}
	}
}

void USpyAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (InputTag.IsValid())
	{
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			if (AbilitySpec.Ability && (AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag)))
			{
				InputReleasedSpecHandles.AddUnique(AbilitySpec.Handle);
				InputHeldSpecHandles.Remove(AbilitySpec.Handle);
			}
		}
	}
}

void USpyAbilitySystemComponent::ProcessAbilityInput(float DeltaTime, bool bGamePaused)
{
	//# 입력 Lock 확인
	if (HasMatchingGameplayTag(SpyGameplayTags::Lock_Input_All))
	{
		ClearAbilityInput();
		return;
	}

	TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;

    //# 이번 프레임에 눌린(Pressed) 어빌리티 처리
    for (const FGameplayAbilitySpecHandle& SpecHandle : InputPressedSpecHandles)
    {
        if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
        {
			AbilitySpec->InputPressed = true;

			//# 실행 중이면 "눌렀다"는 이벤트 전달
			if (AbilitySpec->IsActive())
			{
				AbilitySpecInputPressed(*AbilitySpec);
			}
			else
			{
				AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
			}
        }
    }

	//# 이번 프레임에 꾹 누르고 있는(Held) 어빌리티 처리
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputHeldSpecHandles)
	{
		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability && !AbilitySpec->IsActive())
			{
				AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
			}
		}
	}

	//# 실제 능력 실행
	for (const FGameplayAbilitySpecHandle& AbilitySpecHandle : AbilitiesToActivate)
	{
		TryActivateAbility(AbilitySpecHandle);
	}

    //# 이번 프레임에 뗀(Released) 어빌리티 처리
    for (const FGameplayAbilitySpecHandle& SpecHandle : InputReleasedSpecHandles)
    {
        if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
        {
            if (AbilitySpec->Ability)
            {
                AbilitySpec->InputPressed = false;

                if (AbilitySpec->IsActive())
                {
                    //# 실행 중이면 "뗐다"는 이벤트 전달
                    AbilitySpecInputReleased(*AbilitySpec);
                }
            }
        }
    }

	ClearAbilityInput();
}

void USpyAbilitySystemComponent::ClearAbilityInput()
{
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();
}

bool USpyAbilitySystemComponent::HasAbilityByTag(const FGameplayTag& AbilityTag) const
{
	//# 모든 태그 중에서 현재 태그를 가지고 있는지 확인
	TArray<FGameplayAbilitySpecHandle> SpecHandles;
	GetAllAbilities(SpecHandles);

	for (FGameplayAbilitySpecHandle SpecHandle : SpecHandles)
	{
		const FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(SpecHandle);
		if (Spec == nullptr || Spec->Ability == nullptr)
			continue;

		if (Spec->Ability->AbilityTags.HasTag(AbilityTag))
			return true;
	}

	return false;
}

bool USpyAbilitySystemComponent::IsActiveAbilityByTag(const FGameplayTag& AbilityTag) const
{
	//# 활성화 중인 태그 중에서 현재 태그가 있는지 확인
	const TArray<FGameplayAbilitySpec>& Specs = GetActivatableAbilities();
	for (const FGameplayAbilitySpec& Spec : Specs)
	{
		if (Spec.Ability->AbilityTags.HasTag(AbilityTag))
		{
			if (Spec.IsActive())
				return true;
		}
	}

	return false;
}