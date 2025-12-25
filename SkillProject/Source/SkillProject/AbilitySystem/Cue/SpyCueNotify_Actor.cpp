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
	Super::OnActive_Implementation(MyTarget, Parameters);

	//# 서버는 연출 스킵
	if (GetNetMode() == NM_DedicatedServer)
		return true;

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
	return Super::WhileActive_Implementation(MyTarget, Parameters);
}

bool ASpyCueNotify_Actor::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (ParticleSystemComponent == nullptr)
		return false;

	ParticleSystemComponent->DeactivateSystem();

	if (bLooping == false)
	{
		ParticleSystemComponent->OnSystemFinished.RemoveDynamic(this, &ASpyCueNotify_Actor::OnParticleSystemFinished);
	}

	//# 호출해줘야 풀링 재사용 정상 동작
	K2_EndGameplayCue();

	return Super::OnRemove_Implementation(MyTarget, Parameters);
}

void ASpyCueNotify_Actor::GameplayCueFinishedCallback()
{
	Super::GameplayCueFinishedCallback();
}

bool ASpyCueNotify_Actor::Recycle()
{
	return Super::Recycle();
}

void ASpyCueNotify_Actor::OnParticleSystemFinished(UParticleSystemComponent* FinishedComponent)
{
	if (FinishedComponent != ParticleSystemComponent)
		return;

	ParticleSystemComponent->Deactivate();
	ParticleSystemComponent->OnSystemFinished.Clear();
}