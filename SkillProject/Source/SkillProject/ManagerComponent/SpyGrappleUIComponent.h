// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpyGrappleUIComponent.generated.h"

class UUserWidget;
class USpyGrappleTargetingComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SKILLPROJECT_API USpyGrappleUIComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USpyGrappleUIComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UFUNCTION()
    void OnTargetChanged(AActor* NewTarget);

    void SetMeshCustomDepth(AActor* Actor, bool bEnable) const;

private:
    UPROPERTY()
    TObjectPtr<UUserWidget> PromptWidget;

    TWeakObjectPtr<AActor> OldTarget;
};
