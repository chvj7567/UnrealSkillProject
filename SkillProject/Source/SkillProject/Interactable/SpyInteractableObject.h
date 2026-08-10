// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Interactable/CommonInterface.Interactable.h"
#include "System/CommonInterface.System.h"

#include "SpyInteractableObject.generated.h"

class USphereComponent;
class UBoxComponent;

//# 레벨 배치형 상호작용 오브젝트. F키로 1회 상호작용하면 지정된 미션 태그로
//# 진행도를 올리고 스스로 소진된다.
UCLASS()
class SKILLPROJECT_API ASpyInteractableObject : public AActor, public ISpyInteractableRoot, public ISpyMissionTargetHideVolume
{
	GENERATED_BODY()

public:
	ASpyInteractableObject();

	//# ISpyInteractableRoot
	virtual void RequestInteract(APlayerController* Requester) override;
	virtual bool IsPawnInRange(const AActor* RequesterPawn) const override;
	virtual FText GetInteractVerb() const override
	{
		return InteractVerb;
	}

	//# ISpyMissionTargetHideVolume
	virtual UPrimitiveComponent* GetHideTriggerComponent() const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
										 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
									   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void OnRep_Consumed();

	//# 소진 직후 이 오브젝트와 여전히 오버랩 중인 로컬 폰들에게 범위 이탈을 알린다 —
	//# 콜리전 비활성화가 EndOverlap을 유발하지만, 명시적으로도 호출해 두어 안전망을 이중화한다
	void NotifyLocalOverlapEnd();

	//# 연출 훅 — VFX/사운드는 이번 범위 밖
	UFUNCTION(BlueprintImplementableEvent, Category = "Interactable")
	void OnConsumed();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Interactable")
	TObjectPtr<USphereComponent> InteractionSphere;

	//# 레벨 배치 시 오브젝트마다 다르게 조정할 수 있어야 해 인스턴스 편집 가능하게 노출한다 (cpp-style §15)
	UPROPERTY(EditAnywhere, Category = "Interactable")
	float InteractionRadius = 300.f;

	//# 이 오브젝트가 상호작용 시 발신할 미션 진행 태그 (Event.Mission.Interact 계열)
	UPROPERTY(EditAnywhere, Category = "Interactable")
	FGameplayTag MissionEventTag;

	//# 근접 프롬프트에 표시할 동사
	UPROPERTY(EditAnywhere, Category = "Interactable")
	FText InteractVerb;

	//# 사용자 요청(2026-08-10) — 기본 true: 트리거를 기본 활성화한다. 인스턴스에서 false로
	//# 끄면 거리 히스테리시스로 폴백한다(design §5의 opt-in 방향을 opt-out으로 반전).
	UPROPERTY(EditAnywhere, Category = "Navigation")
	bool bEnableHideTrigger = true;

	//# InteractionSphere(상호작용 판정)와 완전히 분리된 네비 숨김 전용 볼륨(design 2026-08-10 §5)
	UPROPERTY(VisibleAnywhere, Category = "Navigation")
	TObjectPtr<UBoxComponent> HideTriggerVolume;

	UPROPERTY(ReplicatedUsing = OnRep_Consumed)
	bool bConsumed = false;
};
