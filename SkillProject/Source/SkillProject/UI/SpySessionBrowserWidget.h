// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SKUserWidget.h"
#include "SKOnlineTypes.h"

#include "SpySessionBrowserWidget.generated.h"

class UPanelWidget;
class UButton;
class UTextBlock;
class USKOnlineSessionSubsystem;
class USpySessionRowWidget;

//# 방 목록 화면 — 검색/생성/조인을 트리거하고 결과를 행으로 그린다.
//# 세션 작업은 SKOnline 이, 트래블은 SpyLoadingSubsystem 이 맡는다.
UCLASS()
class SKILLPROJECT_API USpySessionBrowserWidget : public USKUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
	//# 세션 에러 → 사용자 문구. 빈 문자열이면 표시하지 않는다(무음 처리)
	static FString MakeStatusMessage(ESKSessionOp Op, ESKSessionError Error);

	//# 진행 문구 — 진행 중인 op 가 있으면 그 문구, 없으면 이번에 요청하는 op 의 문구 (기획서 §5-2-1)
	static FString MakeProgressMessage(ESKSessionOp CurrentOp, ESKSessionOp RequestedOp);

protected:
	//# SKOnline 델리게이트 핸들러
	void HandleSessionsFound(const TArray<FSKSessionInfo>& Infos);
	void HandleHostReady();
	void HandleJoinReady(const FString& ConnectString);
	void HandleSessionError(ESKSessionOp Op, ESKSessionError Error, const FString& Detail);
	void HandleFindCancelled();

	//# 브라우저를 닫고 리슨 서버 전환을 시작한다. 세션 생성은 도착 후에 이뤄진다.
	void BeginHostTravel();

	//# 버튼
	UFUNCTION()
	void OnRefreshClicked();

	UFUNCTION()
	void OnHostClicked();

	//# 행 클릭 → 조인
	void HandleRowClicked(int32 SearchResultIndex);

	//# 전환 개시 — 브라우저를 닫고 게임 입력 모드로 되돌린다
	void CloseForTravel();

	//# 상태 문구 표시(빈 문자열이면 숨김)
	void SetStatus(const FString& Message);

	//# 세션 서브시스템 조회 — 없으면 nullptr
	USKOnlineSessionSubsystem* GetSessionSubsystem() const;

protected:
	//# 구체 패널 타입을 못박지 않는다 — VerticalBox/ScrollBox 어느 쪽이든 받는다(목업 승인 결과에 독립)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UPanelWidget> SessionListBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UButton> RefreshButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UButton> HostButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = true))
	TObjectPtr<UTextBlock> StatusText;

	//# 목록 안에 반복 생성되는 조각이라 UI 매니저가 아니라 이 프로퍼티로 참조한다
	UPROPERTY(EditDefaultsOnly, Category = "SessionBrowser")
	TSubclassOf<USpySessionRowWidget> RowWidgetClass;

	FDelegateHandle SessionsFoundHandle;
	FDelegateHandle HostReadyHandle;
	FDelegateHandle JoinReadyHandle;
	FDelegateHandle SessionErrorHandle;
	FDelegateHandle FindCancelledHandle;
};
