// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Character/AnimInstance/SpyCharacterAnimInstance.h"

#include "SpyAnimManagerComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SKILLPROJECT_API USpyAnimManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USpyAnimManagerComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	void Initialize(USpyCharacterAnimInstance* InAnimInstance);
	
private:
	UPROPERTY()
	TObjectPtr<USpyCharacterAnimInstance> AnimInstance;
};
