// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Util/DefineEnum.h"
#include "ModularPlayerController.h"
#include "SpyPlayerController.generated.h"

class USpyAbilitySystemComponent;
class UCameraShakeBase;

UCLASS()
class SKILLPROJECT_API ASpyPlayerController : public AModularPlayerController
{
	GENERATED_BODY()

public:
	ASpyPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void AcknowledgePossession(APawn* InPawn) override;
	virtual void OnRep_PlayerState() override;
	virtual void UpdateRotation(float DeltaTime) override;
	virtual void PreProcessInput(const float DeltaTime, const bool bGamePaused) override;
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;
	virtual void SetupInputComponent() override;

protected:
	//# ESC — 종료 확인 팝업 토글. 로컬 UI 액션이라 서버 권한 체크 대상 아님 (unreal-infra §2)
	void HandleEscapePressed();

public:
	//# QuitConfirm 이 닫힌 뒤 커서 모드를 정리한다(ESC 재입력·팝업 내 No 버튼 양쪽에서 호출) —
	//# 미션카드 등 다른 UI 가 여전히 커서를 요구하면 끄지 않는다(최소 확인, 완전한 스택 복원은 아님)
	void HandleQuitConfirmClosed();

public:
	UFUNCTION(BlueprintCallable)
	USpyAbilitySystemComponent* GetSpyAbilitySystemComponent() const;

	void ToggleCursorMode();

	//# 현재 상태와 무관하게 원하는 커서 모드로 확정한다(멱등) — ToggleCursorMode 와
	//# UI 트리거(미션카드 등) 양쪽이 이 한 곳만 거치도록 전환 로직을 여기에 모은다
	void SetCursorMode(bool bEnabled);

	UFUNCTION(Client, Reliable)
	void Client_TriggerShake(bool bCritical, bool bFromReceivedHit);

	//# RPC 우회용 — 로컬 컨트롤러는 ProcessEvent 거치지 않고 직접 호출
	void TriggerShakeLocal(bool bCritical, bool bFromReceivedHit);

protected:
	bool bCursorMode = false;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Shake")
	TSubclassOf<UCameraShakeBase> HitShakeLight;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Shake")
	TSubclassOf<UCameraShakeBase> HitShakeHeavy;
};
