// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "USpyAnimNotifyState_AttackTrace.generated.h"

UCLASS()
class SKILLPROJECT_API UUSpyAnimNotifyState_AttackTrace : public UAnimNotifyState
{
	GENERATED_BODY()
	
public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

private:
	void SendGameplayEventToOwner(AActor* InOwner);

protected:
	UPROPERTY(EditAnywhere)
	FName StartWeaponSocketName;

	UPROPERTY(EditAnywhere)
	FName EndWeaponSocketName;

	UPROPERTY(EditAnywhere)
	bool bShowCollision;

private:
	float Radius = 10.0f;
	float CumulativeTime = 0.0f;
	const float CheckInterval = 0.1f;
};
