// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SKAbilitySystemComponent.h"

#include "SpyAbilitySystemComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FAbilityChangedDelegate, FGameplayAbilitySpecHandle, bool/*bGiven*/);

UCLASS()
class SKILLPROJECT_API USpyAbilitySystemComponent : public USKAbilitySystemComponent
{
	GENERATED_BODY()

public:
	USpyAbilitySystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);

	void ProcessAbilityInput(float DeltaTime, bool bGamePaused);
	void ClearAbilityInput();
	void ConsumeInputForHandle(const FGameplayAbilitySpecHandle& Handle);

public:
	bool HasAbilityByTag(const FGameplayTag& AbilityTag) const;
	bool IsActiveAbilityByTag(const FGameplayTag& AbilityTag) const;

public:
	//# 콤보 실행(윈도우) 하나당 미션 진행을 1회만 세기 위한 서버 전용 플래그.
	//# 아직 이번 실행에서 카운트하지 않았으면 true 를 반환하며 카운트됨으로 표시한다.
	bool TryMarkComboMissionCounted();

	//# 새 콤보 실행이 시작될 때(체인 없이 새로 활성화될 때) 카운트 플래그를 리셋한다.
	void ResetComboMissionCounted();

protected:
	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;

private:
	void ForEachAbilitySpec(const TArray<FGameplayAbilitySpecHandle>& Handles, TFunctionRef<void(FGameplayAbilitySpec&)> Func);

	//# 레플리케이트 불필요 — 서버 권한(HasAuthority) 블록 안에서만 읽고 쓴다.
	//# 미션 진행 자체는 FSpyMissionState 로 별도 복제된다.
	bool bComboMissionCounted = false;
};
