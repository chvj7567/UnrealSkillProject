#pragma once

#include "CoreMinimal.h"
#include "Ability/SKGameplayAbility_SkillAction.h"

#include "SpyGameplayAbility_SkillAction.generated.h"

UCLASS()
class SKILLPROJECT_API USpyGameplayAbility_SkillAction : public USKGameplayAbility_SkillAction
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

	virtual void InputPressed(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	//# 현재 마나가 ManaCost 미만이면 발동을 차단한다 (읽기 전용, 클라·서버 공통)
	virtual bool CheckCost(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	//# 서버 권한에서 ManaCost 만큼 마나를 감산한다
	virtual void ApplyCost(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

	//# 스킬바 슬롯이 코스트 표시·마나 부족 판정에 쓴다
	float GetManaCost() const
	{
		return ManaCost;
	}

protected:
	//# 어빌리티별 마나 코스트. 0 이면 코스트 없음. 수치는 game-designer 설계(에셋에서 지정)
	UPROPERTY(EditDefaultsOnly, Category = "Cost", meta = (ClampMin = "0.0"))
	float ManaCost = 0.f;
};
