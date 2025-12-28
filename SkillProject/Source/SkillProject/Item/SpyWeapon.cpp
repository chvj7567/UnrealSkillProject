// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyWeapon.h"
#include "Character/SpyCharacter.h"

ASpyWeapon::ASpyWeapon()
{
	bReplicates = true;

	WeaponSkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponSkeletalMeshComponent"));
	RootComponent = WeaponSkeletalMeshComponent;
}

void ASpyWeapon::BeginPlay()
{
	Super::BeginPlay();
}

void ASpyWeapon::EquipWeapon()
{
	
}

void ASpyWeapon::UnEquipWeapon()
{
}