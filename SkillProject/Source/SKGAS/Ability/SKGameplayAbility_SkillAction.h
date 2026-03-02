// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Ability/SKGameplayAbility.h"

#include "SKGameplayAbility_SkillAction.generated.h"

UENUM(BlueprintType)
enum class EDetectRangeType : uint8
{
    None = 0,
    Weapon = 1,
    Sphere = 2,
};

UENUM(BlueprintType)
enum class EDetectTargetType : uint8
{
    None = 0,
    IncludeMe = 1,
    ExcludeMe = 2,
};

UCLASS()
class SKGAS_API USKGameplayAbility_SkillAction : public USKGameplayAbility
{
	GENERATED_BODY()
	
public:
    USKGameplayAbility_SkillAction();

public:
    virtual void OnWaitGameplayEvent(FGameplayEventData Payload) override;

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

protected:
    UFUNCTION()
    void CheckDetect();

    UFUNCTION()
    void ScheduleServerDetect();

    UFUNCTION()
    void SendTagToTargetByWeapon(ACharacter* OwnerCharacter, FGameplayTag EffectSkillActionTag);
    
    UFUNCTION()
    void SendTagToTargetBySphere(ACharacter* OwnerCharacter, FGameplayTag EffectSkillActionTag);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UAnimMontage> AbilityMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    EDetectRangeType DetectRangeType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (EditCondition = "DetectRangeType != EDetectRangeType::None", EditConditionHides))
    EDetectTargetType DetectTargetType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (EditCondition = "DetectRangeType != EDetectRangeType::None", EditConditionHides))
    float Radius;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (EditCondition = "DetectRangeType != EDetectRangeType::None", EditConditionHides))
    FGameplayTag WaitEffectSkillTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (EditCondition = "DetectRangeType != EDetectRangeType::None", EditConditionHides))
    TSubclassOf<UGameplayEffect> DamageEffectClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (EditCondition = "DetectRangeType != EDetectRangeType::None", EditConditionHides))
    TArray<float> DetectTimes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (EditCondition = "DetectRangeType == EDetectRangeType::Weapon", EditConditionHides))
    FName StartWeaponSocketName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (EditCondition = "DetectRangeType == EDetectRangeType::Weapon", EditConditionHides))
    FName EndWeaponSocketName;
};
