// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "SpyAnimManagerComponent.generated.h"

class USpyCharacterAnimInstance;

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
	static float CalcDirectionFromVelocity(const FVector& WorldVelocity, const FRotator& ActorRotation);
	
private:
	UPROPERTY()
	TObjectPtr<USpyCharacterAnimInstance> AnimInstance;
};
