// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NativeGameplayTags.h"

#include "SpyWeapon.generated.h"

class UBoxComponent;
class USkeletalMeshComponent;

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
	void SetCurrentSkillTag(FGameplayTag InCurrentSkillTag);

public:
	UFUNCTION()
	void EquipWeapon();

	UFUNCTION()
	void UnEquipWeapon();

private:
	UFUNCTION()
	void OnHit(UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UBoxComponent> WeaponCollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USkeletalMeshComponent> WeaponSkeletalMeshComponent;

protected:
	FGameplayTag CurrentSkillTag;
};
