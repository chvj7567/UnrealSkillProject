#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "SpyAIConfig.generated.h"

UCLASS()
class SKILLPROJECT_API USpyAIConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	//# Sight
	UPROPERTY(EditDefaultsOnly, Category = "Perception|Sight")
	float SightRadius = 500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Perception|Sight")
	float LoseSightRadius = 700.f;

	UPROPERTY(EditDefaultsOnly, Category = "Perception|Sight")
	float PeripheralVisionAngleDegrees = 90.f;

	UPROPERTY(EditDefaultsOnly, Category = "Perception|Sight")
	float SightMaxAge = 5.f;

	//# Hearing
	UPROPERTY(EditDefaultsOnly, Category = "Perception|Hearing")
	float HearingRange = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Perception|Hearing")
	float HearingMaxAge = 5.f;

	//# Damage
	UPROPERTY(EditDefaultsOnly, Category = "Perception|Damage")
	float DamageMaxAge = 5.f;
};
