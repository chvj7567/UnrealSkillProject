// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "Character/SpyCharacterMovementComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ModularCharacter.h"

#include "SpyCharacter.generated.h"

class USpyCharacterConfig;
class USpringArmComponent;
class UCameraComponent;
class UBoxComponent;
class ASpyWeapon;
class USpyPawnExtensionComponent;
class USpyHealthComponent;
class USpyLevelComponent;
class USpyAbilitySystemComponent;

struct FOnAttributeChangeData;

UCLASS(config = Game)
class ASpyCharacter : public AModularCharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ASpyCharacter(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

	//# 부착 액터는 소유자가 파괴돼도 함께 파괴되지 않는다 — 스폰한 무기를 여기서 정리한다
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	virtual void OnRep_Controller() override;
	virtual void OnRep_PlayerState() override;

	virtual void
	SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	//# # IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//# # ~IAbilitySystemInterface
public:
	FORCEINLINE USpringArmComponent* GetCameraBoom() const
	{
		return CameraBoom;
	}
	FORCEINLINE UCameraComponent* GetFollowCamera() const
	{
		return FollowCamera;
	}
	FORCEINLINE USpyHealthComponent* GetSpyHealthComponent() const
	{
		return SpyHealthComponent;
	}
	FORCEINLINE USpyCharacterMovementComponent*
	GetSpyCharacterMovementComponent() const
	{
		return GetCharacterMovement<USpyCharacterMovementComponent>();
	}
	FORCEINLINE ASpyWeapon* GetSpyWeapon() const
	{
		return SpyWeapon;
	}
	FORCEINLINE USpyCharacterConfig* GetCharacterConfig() const
	{
		return CharacterConfig;
	}

public:
	//# 파쿠르처럼 캐릭터가 벽에 밀착하는 액션 동안 SpringArm 콜리전 테스트를 억제한다.
	//# 밀착 상태에서는 프로브 시작점이 벽 지오메트리 안으로 들어가 팔 길이가 0 근처로 붕괴한다.
	//# 액션이 체이닝될 수 있으므로 bool 이 아니라 참조 카운트로 관리한다 —
	//# 먼저 끝난 액션이 아직 진행 중인 다른 액션의 억제를 풀어 버리면 안 된다.
	//# 레플리케이트되지 않는 순수 로컬 카메라 연출이라 서버/클라 어디서 불러도 무방하다.
	void PushCameraCollisionSuppress();
	void PopCameraCollisionSuppress();

	FORCEINLINE int32 GetCameraCollisionSuppressCount() const
	{
		return CameraCollisionSuppressCount;
	}

public:
	UFUNCTION(BlueprintCallable)
	USpyAbilitySystemComponent* GetSpyAbilitySystemComponent() const;

	UFUNCTION()
	virtual void OnHealthChanged(USpyHealthComponent* InHealthComponent,
								 float InOldValue, float InNewValue,
								 AActor* InInstigator);

	UFUNCTION()
	virtual void OnDeath(AActor* InOwningActor, AActor* InCauserActor);

protected:
	virtual void OnAbilitySystemInitialized();
	virtual void OnAbilitySystemUninitialized();

	void InitializeGameplayTags();

	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode,
									   uint8 PreviousCustomMode) override;
	void SetMovementModeTag(EMovementMode MovementMode, uint8 CustomMovementMode,
							bool bTagEnabled);

	void SpawnAndAttachWeapon();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PawnExtension")
	TObjectPtr<USpyPawnExtensionComponent> SpyPawnExtensionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	TObjectPtr<USpyHealthComponent> SpyHealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level")
	TObjectPtr<USpyLevelComponent> SpyLevelComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", Replicated)
	TObjectPtr<ASpyWeapon> SpyWeapon;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TObjectPtr<USpyCharacterConfig> CharacterConfig;

private:
	FTimerHandle WeaponSpawnTimerHandle;

	//# 카메라 콜리전 억제를 요청한 액션 수. 0 -> 1 에서 끄고 1 -> 0 에서 되돌린다.
	int32 CameraCollisionSuppressCount = 0;

	//# 억제 진입 시점의 bDoCollisionTest 원래 값. 하드코딩 true 로 되돌리면
	//# BP 에서 false 로 세팅한 캐릭터의 설정을 덮어쓰게 된다.
	bool bCachedDoCollisionTest = true;
};
