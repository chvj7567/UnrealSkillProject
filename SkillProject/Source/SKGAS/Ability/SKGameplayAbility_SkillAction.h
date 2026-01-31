// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Ability/SKGameplayAbility.h"

#include "SKGameplayAbility_SkillAction.generated.h"

UENUM(BlueprintType)
enum class EAttackType : uint8
{
    WeaponRange,
    SphereRange,
};

UCLASS()
class SKGAS_API USKGameplayAbility_SkillAction : public USKGameplayAbility
{
	GENERATED_BODY()
	
public:
    USKGameplayAbility_SkillAction();

protected:
    virtual void OnMontageCompleted() override;
    virtual void OnMontageCancelled() override;
    virtual void OnWaitGameplayEvent(FGameplayEventData Payload) override;
    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

protected:
    UFUNCTION()
    void CheckHit();

    UFUNCTION()
    void ScheduleServerHits();

    UFUNCTION()
    void SendTagToTargetByWeaponRange(ACharacter* OwnerCharacter, FGameplayTag EffectSkillActionTag);
    
    UFUNCTION()
    void SendTagToTargetBySphereRange(ACharacter* OwnerCharacter, FGameplayTag EffectSkillActionTag);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    EAttackType AttackType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    float Radius;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    FGameplayTag WaitEffectSkillTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    FGameplayTag WaitNotifyTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    TSubclassOf<UGameplayEffect> DamageEffectClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    TArray<float> HitTimes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (EditCondition = "AttackType == EAttackType::WeaponRange", EditConditionHides))
    FName StartWeaponSocketName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (EditCondition = "AttackType == EAttackType::WeaponRange", EditConditionHides))
    FName EndWeaponSocketName;
};
