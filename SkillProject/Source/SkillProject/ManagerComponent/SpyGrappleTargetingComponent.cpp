// Fill out your copyright notice in the Description page of Project Settings.

#include "SpyGrappleTargetingComponent.h"
#include "Data/SpyMovementConfig.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DrawDebugHelpers.h"
#include "SKDebug.h"
#include "Components/MeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGrappleTargetingComponent)

USpyGrappleTargetingComponent::USpyGrappleTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void USpyGrappleTargetingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USpyGrappleTargetingComponent, CurrentGrappleTarget);
}

void USpyGrappleTargetingComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr)
		return;

	APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (PC == nullptr || PC->IsLocalController() == false)
		return;

	AActor* NewTarget = FindBestTarget();

	if (NewTarget != LocalCachedTarget.Get())
	{
		ClearHighlight();
		LocalCachedTarget = NewTarget;
		if (NewTarget)
		{
			ApplyHighlight(NewTarget);
		}

		OnGrappleTargetChangedDelegate.Broadcast(NewTarget);

		//# null은 서버에 보내지 않음 — 마지막 유효 타겟을 유지
		if (NewTarget != nullptr)
		{
			Server_SetGrappleTarget(NewTarget);
		}
	}
}

void USpyGrappleTargetingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearHighlight();
	Super::EndPlay(EndPlayReason);
}

void USpyGrappleTargetingComponent::ApplyHighlight(AActor* Actor)
{
	if (Actor == nullptr || TargetHighlightMaterial == nullptr)
		return;

	TArray<UMeshComponent*> Meshes;
	Actor->GetComponents<UMeshComponent>(Meshes);
	for (UMeshComponent* Mesh : Meshes)
	{
		if (Mesh)
		{
			Mesh->SetOverlayMaterial(TargetHighlightMaterial);
		}
	}
	HighlightedActor = Actor;
}

void USpyGrappleTargetingComponent::ClearHighlight()
{
	AActor* Actor = HighlightedActor.Get();
	if (Actor == nullptr)
	{
		HighlightedActor = nullptr;
		return;
	}

	TArray<UMeshComponent*> Meshes;
	Actor->GetComponents<UMeshComponent>(Meshes);
	for (UMeshComponent* Mesh : Meshes)
	{
		if (Mesh)
		{
			Mesh->SetOverlayMaterial(nullptr);
		}
	}
	HighlightedActor = nullptr;
}

AActor* USpyGrappleTargetingComponent::FindBestTarget() const
{
	if (MovementConfig == nullptr)
	{
		if (SKDebugDrawEnabled())
			GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, TEXT("[GrappleTgt] MovementConfig is NULL"));
		return nullptr;
	}

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	APlayerController* PC = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	if (PC == nullptr)
		return nullptr;

	//# 로컬 컨트롤러가 있어도 뷰포트가 없을 수 있다(-nullrhi·커밋릿) — 화면 좌표 기준 탐색 불가
	if (GEngine->GameViewport == nullptr)
		return nullptr;

	FVector2D ViewportSize;
	GEngine->GameViewport->GetViewportSize(ViewportSize);
	const FVector2D ViewportCenter = ViewportSize * 0.5f;

	//# 탐색 반경 구체
	if (SKDebugDrawEnabled())
	{
		DrawDebugSphere(GetWorld(), OwnerPawn->GetActorLocation(),
						MovementConfig->GrapplePromptRange, 16, FColor::Cyan, false, 0.f, 0, 1.f);
	}

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(GetOwner());

	TArray<AActor*> OutActors;
	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		OwnerPawn->GetActorLocation(),
		MovementConfig->GrapplePromptRange,
		ObjectTypes,
		nullptr,
		ActorsToIgnore,
		OutActors);

	if (SKDebugDrawEnabled())
	{
		GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::White,
										 FString::Printf(TEXT("[GrappleTgt] Overlap=%d  Center=%s  Radius=%.0f"),
														 OutActors.Num(), *ViewportCenter.ToString(), MovementConfig->GrappleTargetingScreenRadius));
	}

	AActor* BestTarget = nullptr;
	float BestDistToCenter = MovementConfig->GrappleTargetingScreenRadius;

	for (AActor* Actor : OutActors)
	{
		if (Actor == nullptr)
			continue;

		const bool bHasTag = Actor->Tags.Contains(FName("GrappleAnchor"));

		FVector2D ScreenPos;
		const bool bOnScreen = PC->ProjectWorldLocationToScreen(Actor->GetActorLocation(), ScreenPos, true);
		const float DistToCenter = bOnScreen ? FVector2D::Distance(ScreenPos, ViewportCenter) : -1.f;

		if (bHasTag == false || bOnScreen == false)
			continue;

		if (DistToCenter < BestDistToCenter)
		{
			BestDistToCenter = DistToCenter;
			BestTarget = Actor;
		}
	}

	if (SKDebugDrawEnabled())
	{
		if (BestTarget)
		{
			GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Green,
											 FString::Printf(TEXT("[GrappleTgt] BEST=%s (%.0fpx)"), *BestTarget->GetName(), BestDistToCenter));
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Orange, TEXT("[GrappleTgt] No target"));
		}
	}

	return BestTarget;
}

void USpyGrappleTargetingComponent::Server_SetGrappleTarget_Implementation(AActor* NewTarget)
{
	UE_LOG(LogTemp, Warning, TEXT("[GrappleTgt][Server] Set=%s  NetMode=%d  Time=%.3f  Frame=%llu"),
		   NewTarget ? *NewTarget->GetName() : TEXT("NULL"),
		   (int32)GetWorld()->GetNetMode(),
		   GetWorld()->GetTimeSeconds(),
		   GFrameCounter);
	CurrentGrappleTarget = NewTarget;
}
