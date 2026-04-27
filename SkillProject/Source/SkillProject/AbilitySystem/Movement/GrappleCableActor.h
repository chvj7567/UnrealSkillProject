// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrappleCableActor.generated.h"

class UCableComponent;

UCLASS()
class SKILLPROJECT_API AGrappleCableActor : public AActor
{
    GENERATED_BODY()

public:
    AGrappleCableActor();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void InitCable(ACharacter* InOwnerCharacter, const FVector& InTargetLocation, const FName& InHandBoneName);

    FORCEINLINE FVector GetTargetLocation() const { return TargetLocation; }

private:
    void UpdateCableTransform();

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UCableComponent> CableComponent;

    UPROPERTY(Replicated)
    FVector TargetLocation = FVector::ZeroVector;

    UPROPERTY(Replicated)
    FName HandBoneName;

    UPROPERTY(Replicated)
    TObjectPtr<ACharacter> OwnerCharacter;
};
