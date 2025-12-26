// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyWeapon.h"
#include "Components/BoxComponent.h"
#include "Character/SpyCharacter.h"
#include "AbilitySystemBlueprintLibrary.h"

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

	WeaponCollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ASpyWeapon::OnHit);
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

void ASpyWeapon::OnHit(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//# 데미지 관련 처리는 서버에서만
	if (HasAuthority() == false)
		return;

	ASpyCharacter* OwnerCharacter = Cast<ASpyCharacter>(GetOwner());
	ASpyCharacter* OtherCharacter = Cast<ASpyCharacter>(OtherActor);
	if (OwnerCharacter == nullptr || OtherCharacter == nullptr)
		return;

	if (OwnerCharacter == OtherCharacter)
		return;

	if (CurrentSkillTag.IsValid() == false)
		return;

	FGameplayEventData Payload;
	Payload.EventTag = CurrentSkillTag;
	Payload.Instigator = OwnerCharacter;
	Payload.Target = OtherCharacter;

	UE_LOG(LogTemp, Warning, TEXT("OnHit %s %s %s"), *OwnerCharacter->GetName(), *OtherCharacter->GetName(), *CurrentSkillTag.ToString());
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerCharacter, CurrentSkillTag, Payload);
}


