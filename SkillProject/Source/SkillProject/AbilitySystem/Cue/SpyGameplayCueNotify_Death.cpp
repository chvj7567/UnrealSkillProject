// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Cue/SpyGameplayCueNotify_Death.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGameplayCueNotify_Death)

const FName ASpyGameplayCueNotify_Death::DissolveParameterName(TEXT("Dissolve"));

ASpyGameplayCueNotify_Death::ASpyGameplayCueNotify_Death()
{
	//# bCanEverTick 을 여기서 false 로 되돌리면 AActor::RegisterActorTickFunctions 가 스폰 시점에
	//# 틱 함수를 아예 등록하지 않는다 — 이후 OnActive 에서 true 로 되돌려도 이미 늦다(재등록되지 않음).
	//# 베이스(AGameplayCueNotify_Actor)가 이미 bCanEverTick=true 로 등록해 두므로, 초기 비활성은
	//# SetActorTickEnabled 로만 표현한다.
	SetActorTickEnabled(false);
	bAutoDestroyOnRemove = false;
}

bool ASpyGameplayCueNotify_Death::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	//# 이 Cue 는 Execute(1회성)가 아니라 Add(지속) 경로로만 트리거된다(SpyGA_Death, Task7) — 여기서는 아무것도 하지 않는다.
	return false;
}

bool ASpyGameplayCueNotify_Death::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	//# 데디케이티드 서버는 시각 연출을 재생하지 않는다 — ASKCueNotify_Actor 와 동일한 방어 패턴.
	if (GetNetMode() == NM_DedicatedServer)
		return true;

	ACharacter* TargetCharacter = Cast<ACharacter>(MyTarget);
	if (TargetCharacter == nullptr)
		return false;

	USkeletalMeshComponent* Mesh = TargetCharacter->GetMesh();
	if (Mesh == nullptr)
		return false;

	DissolveDurationSeconds = (Parameters.RawMagnitude > 0.f) ? Parameters.RawMagnitude : DefaultDissolveDurationSeconds;
	ElapsedSeconds = 0.f;
	bIsDissolving = true;

	DynamicMaterials.Reset();
	const int32 NumMaterials = Mesh->GetNumMaterials();
	for (int32 SlotIndex = 0; SlotIndex < NumMaterials; ++SlotIndex)
	{
		UMaterialInstanceDynamic* MID = Mesh->CreateDynamicMaterialInstance(SlotIndex);
		if (MID != nullptr)
		{
			MID->SetScalarParameterValue(DissolveParameterName, 0.f);
			DynamicMaterials.Add(MID);
		}
	}

	PrimaryActorTick.bCanEverTick = true;
	SetActorTickEnabled(true);

	return false;
}

bool ASpyGameplayCueNotify_Death::WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	return false;
}

void ASpyGameplayCueNotify_Death::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsDissolving == false)
		return;

	ElapsedSeconds += DeltaSeconds;
	const float Alpha = (DissolveDurationSeconds > 0.f) ? FMath::Clamp(ElapsedSeconds / DissolveDurationSeconds, 0.f, 1.f) : 1.f;

	for (const TObjectPtr<UMaterialInstanceDynamic>& MID : DynamicMaterials)
	{
		if (IsValid(MID))
		{
			MID->SetScalarParameterValue(DissolveParameterName, Alpha);
		}
	}

	if (Alpha >= 1.f)
	{
		bIsDissolving = false;
		SetActorTickEnabled(false);
	}
}

bool ASpyGameplayCueNotify_Death::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	//# 캐릭터 인스턴스가 Destroy 없이 재활용되므로(ASpyCharacter::OnRespawn) 다음 사망을 위해
	//# 파라미터를 0으로 되돌려 둔다 — 재등장 시 반투명 상태로 시작하는 상태 누수를 막는다.
	for (const TObjectPtr<UMaterialInstanceDynamic>& MID : DynamicMaterials)
	{
		if (IsValid(MID))
		{
			MID->SetScalarParameterValue(DissolveParameterName, 0.f);
		}
	}

	DynamicMaterials.Reset();
	bIsDissolving = false;
	SetActorTickEnabled(false);

	return false;
}
