// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Attribute/SKAttributeSet.h"

#include "SpyCharacterAttributeSet.generated.h"

UCLASS()
class SKILLPROJECT_API USpyCharacterAttributeSet : public USKAttributeSet
{
	GENERATED_BODY()
	
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	UPROPERTY(ReplicatedUsing = OnRep_Health, BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData MoveNormalSpeed;
	ATTRIBUTE_ACCESSORS(USpyCharacterAttributeSet, MoveNormalSpeed);

protected:
	UFUNCTION()
	void OnRep_MoveNormalSpeed(const FGameplayAttributeData& OldValue);

public:
	mutable FSKAttributeEvent OnMoveNormalSpeedChanged;
};
