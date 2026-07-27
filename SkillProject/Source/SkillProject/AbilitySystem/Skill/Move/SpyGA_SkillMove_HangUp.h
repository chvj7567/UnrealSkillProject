// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Ability/SKGameplayAbility_SkillMove.h"

#include "SpyGA_SkillMove_HangUp.generated.h"

class ASpyCharacter;

UCLASS()
class SKILLPROJECT_API USpyGA_SkillMove_HangUp : public USKGameplayAbility_SkillMove
{
	GENERATED_BODY()

public:
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
    void OnSyncMotionWarpingData(FMotionWarpingData InHangUpData);

protected:
    //# FreeMoveMode 를 이 GA 가 실제로 켰는지 여부. 켠 주체만 되돌린다.
    //# SetFreeMoveMode 호출은 전부 HasAuthority 안이라 이 플래그도 서버 인스턴스에서만 의미가 있다.
    bool bFreeMoveEngaged = false;

	//# 이 GA 가 카메라 콜리전 억제를 건 캐릭터. 건 주체만 해제한다.
	//# 약참조로 들고 있는 이유 — 사망·파괴 경로의 EndAbility 에서는 ActorInfo 의 Avatar 가
	//# 이미 비어 있을 수 있어 다시 조회하면 해제를 놓치고 카운트가 샌다.
	TWeakObjectPtr<ASpyCharacter> CameraSuppressedCharacter;

	UPROPERTY(EditAnywhere)
	FName MotionWarpingStartName;

	UPROPERTY(EditAnywhere)
	FName MotionWarpingEndName;
};
