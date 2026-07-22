// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Ability/SKGameplayAbility_SkillMove.h"

#include "SpyGA_SkillMove_Vault.generated.h"

UCLASS()
class SKILLPROJECT_API USpyGA_SkillMove_Vault : public USKGameplayAbility_SkillMove
{
	GENERATED_BODY()
	
protected:
    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    virtual void EndAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility,
        bool bWasCancelled) override;

protected:
    UFUNCTION()
    void OnSyncMotionWarpingData(FMotionWarpingData InVaultData);

protected:
    //# FreeMoveMode 를 이 GA 가 실제로 켰는지 여부. 켠 주체만 되돌린다.
    //# SetFreeMoveMode 호출은 전부 HasAuthority 안이라 이 플래그도 서버 인스턴스에서만 의미가 있다.
    bool bFreeMoveEngaged = false;

    UPROPERTY(EditAnywhere)
    FName MotionWarpingStartName;

    UPROPERTY(EditAnywhere)
    FName MotionWarpingEndName;
};
