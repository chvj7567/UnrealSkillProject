// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "SpyGameplayCueNotify_Death.generated.h"

class UMaterialInstanceDynamic;

//# 봇 사망 디졸브 연출 — 캐릭터 메시에 Dynamic Material Instance 를 만들어 Dissolve 파라미터를 보간한다.
//# AGameplayCueNotify_Actor 는 AActor 파생이므로 A 접두사를 쓴다 (U 가 아님 — cpp-style §1).
UCLASS()
class SKILLPROJECT_API ASpyGameplayCueNotify_Death : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	ASpyGameplayCueNotify_Death();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

private:
	static constexpr float DefaultDissolveDurationSeconds = 2.0f;
	static const FName DissolveParameterName;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterials;

	float DissolveDurationSeconds = DefaultDissolveDurationSeconds;
	float ElapsedSeconds = 0.f;
	bool bIsDissolving = false;
};
