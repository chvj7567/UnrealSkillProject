// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyWeapon.h"

ASpyWeapon::ASpyWeapon()
{
	WeaponSkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponSkeletalMeshComponent"));
	RootComponent = WeaponSkeletalMeshComponent;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Game/Weapon/Sword/TwoHandSword/TwoHandSword_001/SKM_TwoHandSword_001.SKM_TwoHandSword_001"));

	if (MeshAsset.Succeeded())
	{
		WeaponSkeletalMeshComponent->SetSkeletalMesh(MeshAsset.Object);
	}
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

