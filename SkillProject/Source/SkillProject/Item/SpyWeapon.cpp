#include "SpyWeapon.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Engine/EngineTypes.h"

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

void ASpyWeapon::ActivateTrail()
{
	if (!IsValid(TrailEffect) || GetNetMode() == NM_DedicatedServer)
		return;

	if (IsValid(ActiveTrailComponent))
	{
		ActiveTrailComponent->EndTrails();
		ActiveTrailComponent->Deactivate();
		ActiveTrailComponent = nullptr;
	}

	ActiveTrailComponent = UGameplayStatics::SpawnEmitterAttached(
		TrailEffect,
		WeaponSkeletalMeshComponent
	);

	if (IsValid(ActiveTrailComponent))
	{
		ActiveTrailComponent->BeginTrails(
			FName("Trail_Start"),
			FName("Trail_End"),
			ETrailWidthMode_FromCentre,
			1.0f
		);
	}
}

void ASpyWeapon::DeactivateTrail()
{
	if (IsValid(ActiveTrailComponent))
	{
		ActiveTrailComponent->EndTrails();
		ActiveTrailComponent->Deactivate();
		ActiveTrailComponent = nullptr;
	}
}

void ASpyWeapon::Multicast_ActivateTrail_Implementation()
{
	ActivateTrail();
}

void ASpyWeapon::Multicast_DeactivateTrail_Implementation()
{
	DeactivateTrail();
}
