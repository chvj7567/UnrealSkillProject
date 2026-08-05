// Fill out your copyright notice in the Description page of Project Settings.

#include "Interactable/SpyInteractableObject.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Character/CommonInterface.Character.h"
#include "ManagerComponent/CommonInterface.Manager.h"
#include "System/SpyMissionComponent.h"
#include "System/SpyMissionTargetRegistrySubsystem.h"

ASpyInteractableObject::ASpyInteractableObject()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	SetRootComponent(InteractionSphere);
	InteractionSphere->SetSphereRadius(InteractionRadius);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	InteractVerb = NSLOCTEXT("SpyInteractable", "DefaultInteractVerb", "조사하기");
}

void ASpyInteractableObject::BeginPlay()
{
	Super::BeginPlay();

	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ASpyInteractableObject::OnInteractionSphereBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &ASpyInteractableObject::OnInteractionSphereEndOverlap);

	//# 미션 목표 좌표 레지스트리 자기등록(design §5-3·§5-4) — 소진(consume) 시 해제 여부는
	//# 이번 범위 밖(design §7-6, 현재 Interact 데이터 없음)
	if (MissionEventTag.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			if (USpyMissionTargetRegistrySubsystem* Registry = World->GetSubsystem<USpyMissionTargetRegistrySubsystem>())
				Registry->RegisterMissionTargetLocation(MissionEventTag, this);
		}
	}
}

void ASpyInteractableObject::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MissionEventTag.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			if (USpyMissionTargetRegistrySubsystem* Registry = World->GetSubsystem<USpyMissionTargetRegistrySubsystem>())
				Registry->UnregisterMissionTargetLocation(MissionEventTag, this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ASpyInteractableObject::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	//# 레벨 배치 시 인스턴스별로 조정한 InteractionRadius 를 실제 컴포넌트에 반영한다 —
	//# 생성자에서 1회 세팅한 값은 이후 에디터 프로퍼티 편집을 반영하지 못한다
	if (InteractionSphere != nullptr)
		InteractionSphere->SetSphereRadius(InteractionRadius);
}

void ASpyInteractableObject::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASpyInteractableObject, bConsumed);
}

void ASpyInteractableObject::OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
															 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const APawn* OtherPawn = Cast<APawn>(OtherActor);
	if (OtherPawn == nullptr || OtherPawn->IsLocallyControlled() == false)
		return;

	//# TScriptInterface(RawPtr) 생성자는 인터페이스 미구현이어도 ObjectPointer 를 그대로 저장한다 —
	//# GetObject() 널체크로는 구현 여부를 걸러낼 수 없다. Cast<Interface> 로 먼저 판정한다.
	ISpyCharacterRoot* CharRoot = Cast<ISpyCharacterRoot>(OtherActor);
	if (CharRoot == nullptr)
		return;

	TScriptInterface<ISpyInteractionHost> Host = CharRoot->GetInteractionHost();
	if (Host.GetObject() == nullptr)
		return;

	Host->NotifyInteractableRangeChanged(this, true);
}

void ASpyInteractableObject::OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
														   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	const APawn* OtherPawn = Cast<APawn>(OtherActor);
	if (OtherPawn == nullptr || OtherPawn->IsLocallyControlled() == false)
		return;

	ISpyCharacterRoot* CharRoot = Cast<ISpyCharacterRoot>(OtherActor);
	if (CharRoot == nullptr)
		return;

	TScriptInterface<ISpyInteractionHost> Host = CharRoot->GetInteractionHost();
	if (Host.GetObject() == nullptr)
		return;

	Host->NotifyInteractableRangeChanged(this, false);
}

bool ASpyInteractableObject::IsPawnInRange(const AActor* RequesterPawn) const
{
	if (RequesterPawn == nullptr || InteractionSphere == nullptr)
		return false;

	return InteractionSphere->IsOverlappingActor(RequesterPawn);
}

void ASpyInteractableObject::RequestInteract(APlayerController* Requester)
{
	//# 게임플레이 상태(미션 진행)를 바꾸는 함수라 호출부 권한 체크에만 기대지 않는다 — 자체 방어
	if (HasAuthority() == false)
		return;

	if (bConsumed)
		return;

	if (Requester == nullptr)
		return;

	APawn* RequesterPawn = Requester->GetPawn();
	if (RequesterPawn == nullptr)
		return;

	APlayerState* RequesterPS = RequesterPawn->GetPlayerState();
	if (RequesterPS == nullptr)
		return;

	USpyMissionComponent* MissionComp = USpyMissionComponent::FindMissionComponent(RequesterPS);
	if (MissionComp == nullptr)
		return;

	MissionComp->AddProgress(MissionEventTag, 1);

	bConsumed = true;

	//# 서버는 자신의 OnRep 콜백이 발화하지 않는다 — 직접 호출해 소진 처리를 공유한다
	OnRep_Consumed();
}

void ASpyInteractableObject::OnRep_Consumed()
{
	//# SetCollisionEnabled(NoCollision) 은 즉시 UpdateOverlaps 를 유발해 오버랩 목록을 비운다 —
	//# 그 뒤에 조회하면 NotifyLocalOverlapEnd 가 대상을 못 찾아 no-op 이 된다. 순서를 반드시 지킨다.
	NotifyLocalOverlapEnd();

	if (InteractionSphere != nullptr)
		InteractionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	OnConsumed();
}

void ASpyInteractableObject::NotifyLocalOverlapEnd()
{
	if (InteractionSphere == nullptr)
		return;

	TArray<AActor*> OverlappingPawns;
	InteractionSphere->GetOverlappingActors(OverlappingPawns, APawn::StaticClass());

	for (AActor* OverlappingActor : OverlappingPawns)
	{
		const APawn* OverlappingPawn = Cast<APawn>(OverlappingActor);
		if (OverlappingPawn == nullptr || OverlappingPawn->IsLocallyControlled() == false)
			continue;

		ISpyCharacterRoot* CharRoot = Cast<ISpyCharacterRoot>(OverlappingActor);
		if (CharRoot == nullptr)
			continue;

		TScriptInterface<ISpyInteractionHost> Host = CharRoot->GetInteractionHost();
		if (Host.GetObject() == nullptr)
			continue;

		Host->NotifyInteractableRangeChanged(this, false);
	}
}
