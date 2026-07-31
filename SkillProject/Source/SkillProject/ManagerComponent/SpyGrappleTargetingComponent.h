// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ManagerComponent/CommonInterface.h"
#include "SpyGrappleTargetingComponent.generated.h"

class USpyMovementConfig;
class UMaterialInterface;

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SKILLPROJECT_API USpyGrappleTargetingComponent : public UActorComponent, public ISpyGrappleHost
{
    GENERATED_BODY()

public:
    USpyGrappleTargetingComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    //# ISpyGrappleHost
    UFUNCTION(BlueprintCallable, Category = "Grapple")
    virtual AActor* GetCurrentGrappleTarget() const override
    {
        return CurrentGrappleTarget;
    }

    virtual AActor* GetLocalCachedTarget() const override
    {
        return LocalCachedTarget.Get();
    }

    virtual FOnGrappleTargetChanged& OnGrappleTargetChanged() override
    {
        return OnGrappleTargetChangedDelegate;
    }

    void ClearGrappleTarget() { CurrentGrappleTarget = nullptr; }

    UPROPERTY(BlueprintAssignable, Category = "Grapple")
    FOnGrappleTargetChanged OnGrappleTargetChangedDelegate;

protected:
    UPROPERTY(EditDefaultsOnly, Category = "Config")
    TObjectPtr<USpyMovementConfig> MovementConfig;

    UPROPERTY(EditDefaultsOnly, Category = "Highlight")
    TObjectPtr<UMaterialInterface> TargetHighlightMaterial;

private:
    UFUNCTION(Server, Reliable)
    void Server_SetGrappleTarget(AActor* NewTarget);

    AActor* FindBestTarget() const;

    void ApplyHighlight(AActor* Actor);
    void ClearHighlight();

    UPROPERTY(Replicated)
    TObjectPtr<AActor> CurrentGrappleTarget;

    TWeakObjectPtr<AActor> LocalCachedTarget;
    TWeakObjectPtr<AActor> HighlightedActor;

};
