#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Components/WidgetComponent.h"

#include "SKUIManager.generated.h"

class UWidgetComponent;
class USKUserWidget;

UCLASS()
class SKUICORE_API USKUIManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//# 파생 서브클래스(프로젝트 UIManager)가 있으면 base 는 생성하지 않음 → leaf 인스턴스 1개만
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	//# base·leaf 어디서든 동일한 leaf 인스턴스를 반환
	static USKUIManager* Get(const UObject* WorldContextObject);

public:
	//# ZOrder 는 뷰포트 레이어 순서. 기본 0 — persistent UI(기본 100) 위에 띄우려면 그보다 큰 값을 넘긴다.
	UFUNCTION(BlueprintCallable)
	void OpenUI(FName InUIName, int32 ZOrder = 0);

	UFUNCTION(BlueprintCallable)
	void CloseUI(FName InUIName);

	UFUNCTION(BlueprintCallable)
	void CloseLastUI();

	UFUNCTION(BlueprintCallable)
	void OpenSubUI(FName InUIName, UWidgetComponent* WidgetComponent, EWidgetSpace Space);

	UFUNCTION(BlueprintCallable)
	void AddCashingUI(USKUserWidget* UserWidget);

public:
	//# 트래블(맵 전환)을 넘어 살아남는 UI 를 연다.
	//# GameInstance 를 아우터로 생성하고 뷰포트 Slate 레이어에 직접 얹으므로 월드가 파괴돼도 유지된다.
	//# 로딩 화면처럼 맵 전환 중에도 계속 보여야 하는 UI 전용. 일반 UI 는 OpenUI 를 쓴다.
	//# 위젯 클래스를 동기 로드하므로 호출 즉시 화면에 뜬다.
	UFUNCTION(BlueprintCallable)
	USKUserWidget* OpenPersistentUI(FName InUIName, int32 ZOrder = 100);

	UFUNCTION(BlueprintCallable)
	void ClosePersistentUI(FName InUIName);

	UFUNCTION(BlueprintCallable)
	bool IsPersistentUIOpen(FName InUIName) const;

protected:
	//# 월드 교체 직전 정리 — 월드 소속 UI(OpenUIList·CashingUIList)만 비운다. persistent 는 대상이 아니다.
	void HandlePreLoadMap(const FString& MapName);

protected:
	//# persistent UI 는 OpenUIList 와 분리해 관리한다 (CloseLastUI 등 스택 동작에 섞이면 안 됨)
	UPROPERTY()
	TArray<TObjectPtr<USKUserWidget>> PersistentUIList;

	//# PreLoadMap 구독 핸들
	FDelegateHandle PreLoadMapHandle;

protected:
	const int MaxCashingUICount = 5;

protected:
	UPROPERTY()
	TArray<TObjectPtr<USKUserWidget>> OpenUIList;

	UPROPERTY()
	TArray<TObjectPtr<USKUserWidget>> CashingUIList;
};
