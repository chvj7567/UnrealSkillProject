// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Cue/SpyCueNotify_Actor.h"
#include "Particles/ParticleSystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyCueNotify_Actor)

ASpyCueNotify_Actor::ASpyCueNotify_Actor()
{
	PrimaryActorTick.bCanEverTick = false;

	ParticleSystemComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleSystemComponent"));
	ParticleSystemComponent->SetupAttachment(RootComponent);
	ParticleSystemComponent->bAutoActivate = false;

	bAutoDestroyOnRemove = false;
	bAllowMultipleOnActiveEvents = false;
}

void ASpyCueNotify_Actor::BeginPlay()
{
	Super::BeginPlay();
}

bool ASpyCueNotify_Actor::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (ParticleSystemComponent == nullptr)
		return false;

	ParticleSystemComponent->Activate(true);

	if (bLooping == false)
	{
		ParticleSystemComponent->OnSystemFinished.AddDynamic(this, &ASpyCueNotify_Actor::OnParticleSystemFinished);
	}

	return true;
}

bool ASpyCueNotify_Actor::WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	return false;
}

bool ASpyCueNotify_Actor::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (ParticleSystemComponent && ParticleSystemComponent->IsActive())
	{
		ParticleSystemComponent->Deactivate();
	}

	ReturnToPool();

	return true;
}

void ASpyCueNotify_Actor::ResetCue()
{
	if (ParticleSystemComponent == nullptr)
		return;

	ParticleSystemComponent->Deactivate();
	ParticleSystemComponent->OnSystemFinished.Clear();
}

void ASpyCueNotify_Actor::ReturnToPool()
{
	ResetCue();

	// 실제 풀 반환은 GameplayCueManager가 관리
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void ASpyCueNotify_Actor::OnParticleSystemFinished(UParticleSystemComponent* FinishedComponent)
{
	if (FinishedComponent != ParticleSystemComponent)
		return;

	ReturnToPool();
}