// Fill out your copyright notice in the Description page of Project Settings.

#include "Navigation/SpyMissionTargetPoint.h"
#include "Components/BillboardComponent.h"
#include "Components/SceneComponent.h"
#include "System/SpyMissionTargetRegistrySubsystem.h"

ASpyMissionTargetPoint::ASpyMissionTargetPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

#if WITH_EDITORONLY_DATA
	//# 에디터 전용 서브오브젝트 — WITH_EDITOR 빌드에서만 생성되고 나머지 빌드에선 자동 스트립된다
	EditorBillboard = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("EditorBillboard"));
	if (EditorBillboard != nullptr)
		EditorBillboard->SetupAttachment(RootScene);
#endif
}

void ASpyMissionTargetPoint::BeginPlay()
{
	Super::BeginPlay();

	if (TargetMissionTag.IsValid() == false)
		return;

	UWorld* World = GetWorld();
	if (World == nullptr)
		return;

	if (USpyMissionTargetRegistrySubsystem* Registry = World->GetSubsystem<USpyMissionTargetRegistrySubsystem>())
		Registry->RegisterMissionTargetLocation(TargetMissionTag, this);
}

void ASpyMissionTargetPoint::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (TargetMissionTag.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			if (USpyMissionTargetRegistrySubsystem* Registry = World->GetSubsystem<USpyMissionTargetRegistrySubsystem>())
				Registry->UnregisterMissionTargetLocation(TargetMissionTag, this);
		}
	}

	Super::EndPlay(EndPlayReason);
}
