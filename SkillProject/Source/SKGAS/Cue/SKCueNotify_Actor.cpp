// Fill out your copyright notice in the Description page of Project Settings.


#include "SKCueNotify_Actor.h"
#include "Particles/ParticleSystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKCueNotify_Actor)

ASKCueNotify_Actor::ASKCueNotify_Actor()
{
	PrimaryActorTick.bCanEverTick = false;

	ParticleSystemComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleSystemComponent"));
	ParticleSystemComponent->SetupAttachment(RootComponent);
	ParticleSystemComponent->bAutoActivate = false;
	RootComponent = ParticleSystemComponent;

	bAutoDestroyOnRemove = false;
	bAllowMultipleOnActiveEvents = false;
}

void ASKCueNotify_Actor::BeginPlay()
{
	Super::BeginPlay();
}

bool ASKCueNotify_Actor::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
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
		ParticleSystemComponent->OnSystemFinished.AddDynamic(this, &ASKCueNotify_Actor::OnParticleSystemFinished);
	}

	return true;
}

bool ASKCueNotify_Actor::WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	return Super::WhileActive_Implementation(MyTarget, Parameters);
}

bool ASKCueNotify_Actor::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (ParticleSystemComponent == nullptr)
		return false;

	ParticleSystemComponent->DeactivateSystem();

	if (bLooping == false)
	{
		ParticleSystemComponent->OnSystemFinished.RemoveDynamic(this, &ASKCueNotify_Actor::OnParticleSystemFinished);
	}

	//# 호출해줘야 풀링 재사용 정상 동작
	K2_EndGameplayCue();

	return Super::OnRemove_Implementation(MyTarget, Parameters);
}

void ASKCueNotify_Actor::GameplayCueFinishedCallback()
{
	Super::GameplayCueFinishedCallback();
}

bool ASKCueNotify_Actor::Recycle()
{
	return Super::Recycle();
}

void ASKCueNotify_Actor::OnParticleSystemFinished(UParticleSystemComponent* FinishedComponent)
{
	if (FinishedComponent != ParticleSystemComponent)
		return;

	ParticleSystemComponent->Deactivate();
	ParticleSystemComponent->OnSystemFinished.Clear();
}