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

	static TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;
	AbilitiesToActivate.Reset();

    //# 이번 프레임에 눌린(Pressed) 어빌리티 처리
    for (const FGameplayAbilitySpecHandle& SpecHandle : InputPressedSpecHandles)
    {
        if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
        {
            if (AbilitySpec->Ability)
            {
                AbilitySpec->InputPressed = true;

                if (AbilitySpec->IsActive())
                {
                    //# 이미 실행 중이면 "눌렸다"는 이벤트만 전달
                    AbilitySpecInputPressed(*AbilitySpec);
                }
                else
                {
                    //# 실행 중이 아니면 즉시 실행 시도
                    TryActivateAbility(AbilitySpec->Handle);
                }
            }
        }
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
}

void USpyAbilitySystemComponent::ClearAbilityInput()
{
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}
