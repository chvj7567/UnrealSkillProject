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
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	UFUNCTION()
	void SetCurrentSkillTag(FGameplayTag InCurrentSkillTag);

public:
	UFUNCTION()
	void EquipWeapon();

	UFUNCTION()
	void UnEquipWeapon();

public:
	UPROPERTY(Replicated)
	FGameplayTag CurrentSkillTag;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UBoxComponent> WeaponCollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USkeletalMeshComponent> WeaponSkeletalMeshComponent;
};
