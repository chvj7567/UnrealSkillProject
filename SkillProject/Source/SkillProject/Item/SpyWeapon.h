// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "SpyWeapon.generated.h"

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
	void EquipWeapon();
	void UnEquipWeapon();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USkeletalMeshComponent> WeaponSkeletalMeshComponent;
};
