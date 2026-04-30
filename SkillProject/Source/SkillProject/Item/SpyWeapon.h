#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "SpyWeapon.generated.h"

class USkeletalMeshComponent;
class UParticleSystemComponent;
class UParticleSystem;

UCLASS()
class SKILLPROJECT_API ASpyWeapon : public AActor
{
	GENERATED_BODY()

public:
	ASpyWeapon();

protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION()
	void EquipWeapon();

	UFUNCTION()
	void UnEquipWeapon();

	UFUNCTION(BlueprintCallable)
	void ActivateTrail();

	UFUNCTION(BlueprintCallable)
	void DeactivateTrail();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ActivateTrail();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_DeactivateTrail();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USkeletalMeshComponent> WeaponSkeletalMeshComponent;

	UPROPERTY(EditDefaultsOnly, Category = "FX")
	TObjectPtr<UParticleSystem> TrailEffect;

private:
	UPROPERTY(Transient)
	TObjectPtr<UParticleSystemComponent> ActiveTrailComponent;
};
