#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "GameplayTagContainer.h"
#include "BTService_CheckCooldown.generated.h"

UCLASS()
class SKILLPROJECT_API UBTService_CheckCooldown : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_CheckCooldown();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Config")
	FGameplayTagContainer CooldownTags;

	UPROPERTY(EditAnywhere, Category = "Config")
	FBlackboardKeySelector IsOnCooldownKey;
};
