// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyWeapon.h"
#include "Net/UnrealNetwork.h"
#include "Components/BoxComponent.h"
#include "Character/SpyCharacter.h"

ASpyWeapon::ASpyWeapon()
{
	bReplicates = true;

	WeaponSkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponSkeletalMeshComponent"));
	RootComponent = WeaponSkeletalMeshComponent;

	WeaponCollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponCollisionComponent"));
	WeaponCollisionComponent->SetupAttachment(WeaponSkeletalMeshComponent);
	//WeaponCollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ASpyWeapon::BeginPlay()
{
	Super::BeginPlay();
}

void ASpyWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASpyWeapon, CurrentSkillTag);
}

void ASpyWeapon::SetCurrentSkillTag(FGameplayTag InCurrentSkillTag)
{
	CurrentSkillTag = InCurrentSkillTag;

	WeaponCollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void ASpyWeapon::EquipWeapon()
{
	
}

void ASpyWeapon::UnEquipWeapon()
{
}