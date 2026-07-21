// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyCharacterAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

void USpyCharacterAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USpyCharacterAttributeSet, MoveNormalSpeed);
	DOREPLIFETIME(USpyCharacterAttributeSet, Experience);
	DOREPLIFETIME(USpyCharacterAttributeSet, MaxExperience);
	DOREPLIFETIME(USpyCharacterAttributeSet, Level);
}

void USpyCharacterAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	AActor* InstigatorActor = Data.EffectSpec.GetContext().GetInstigator();
	AActor* EffectCauser = Data.EffectSpec.GetContext().GetEffectCauser();
	const float DeltaValue = Data.EvaluatedData.Magnitude;

	//# 경험치는 음수로 내려가지 않는다
	if (Data.EvaluatedData.Attribute == GetExperienceAttribute())
	{
		SetExperience(FMath::Max(0.f, GetExperience()));

		//# 서버에서 브로드캐스트 (클라이언트는 OnRep_Experience 에서 처리)
		const float NewExperience = GetExperience();
		OnExperienceChanged.Broadcast(InstigatorActor, EffectCauser, &Data.EffectSpec, DeltaValue, NewExperience - DeltaValue, NewExperience);
	}
	else if (Data.EvaluatedData.Attribute == GetMaxExperienceAttribute())
	{
		const float NewMaxExperience = GetMaxExperience();
		OnMaxExperienceChanged.Broadcast(InstigatorActor, EffectCauser, &Data.EffectSpec, DeltaValue, NewMaxExperience - DeltaValue, NewMaxExperience);
	}
	else if (Data.EvaluatedData.Attribute == GetLevelAttribute())
	{
		const float NewLevel = GetLevel();
		OnLevelChanged.Broadcast(InstigatorActor, EffectCauser, &Data.EffectSpec, DeltaValue, NewLevel - DeltaValue, NewLevel);
	}
}

void USpyCharacterAttributeSet::OnRep_MoveNormalSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USpyCharacterAttributeSet, MoveNormalSpeed, OldValue);

	OnMoveNormalSpeedChanged.Broadcast(nullptr, nullptr, nullptr, GetMoveNormalSpeed() - OldValue.GetCurrentValue(), OldValue.GetCurrentValue(), GetMoveNormalSpeed());
}

void USpyCharacterAttributeSet::OnRep_Experience(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USpyCharacterAttributeSet, Experience, OldValue);

	OnExperienceChanged.Broadcast(nullptr, nullptr, nullptr, GetExperience() - OldValue.GetCurrentValue(), OldValue.GetCurrentValue(), GetExperience());
}

void USpyCharacterAttributeSet::OnRep_MaxExperience(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USpyCharacterAttributeSet, MaxExperience, OldValue);

	OnMaxExperienceChanged.Broadcast(nullptr, nullptr, nullptr, GetMaxExperience() - OldValue.GetCurrentValue(), OldValue.GetCurrentValue(), GetMaxExperience());
}

void USpyCharacterAttributeSet::OnRep_Level(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USpyCharacterAttributeSet, Level, OldValue);

	OnLevelChanged.Broadcast(nullptr, nullptr, nullptr, GetLevel() - OldValue.GetCurrentValue(), OldValue.GetCurrentValue(), GetLevel());
}
