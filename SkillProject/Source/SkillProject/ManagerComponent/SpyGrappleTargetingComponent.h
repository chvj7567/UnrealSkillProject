// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpyGrappleTargetingComponent.generated.h"

class USpyMovementConfig;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGrappleTargetChanged, AActor*, NewTarget);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SKILLPROJECT_API USpyGrappleTargetingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USpyGrappleTargetingComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Grapple")
    AActor* GetCurrentGrappleTarget() const { return CurrentGrappleTarget; }

    UPROPERTY(BlueprintAssignable, Category = "Grapple")
    FOnGrappleTargetChanged OnGrappleTargetChanged;

protected:
    UPROPERTY(Transient)
    TObjectPtr<USpyMovementConfig> MovementConfig;

private:
    UFUNCTION(Server, Reliable)
    void Server_SetGrappleTarget(AActor* NewTarget);

    AActor* FindBestTarget() const;

    UPROPERTY(Replicated)
    TObjectPtr<AActor> CurrentGrappleTarget;

    TWeakObjectPtr<AActor> LocalCachedTarget;
};
