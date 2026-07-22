// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SpyMainHUD.h"
#include "Character/SpyLevelComponent.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Manager/SpyUIManager.h"
#include "System/SpyMissionComponent.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyMainHUD)

void USpyMainHUD::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Menu)
	{
		Btn_Menu->OnClicked.AddDynamic(this, &USpyMainHUD::ShowMenu);
	}

	//# 클라이언트에서는 이 시점에 Pawn/PlayerState/ASC가 아직 없을 수 있다.
	//# 준비될 때까지 짧은 주기로 재시도하고, 둘 다 성공하면 타이머를 끈다.
	BindRetryCount = 0;

	const bool bLevelBound = TryBindLevelComponent();
	const bool bMissionBound = TryBindMissionComponent();

	if (bLevelBound == false || bMissionBound == false)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(BindRetryTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]() {
												  const bool bLevelOk = TryBindLevelComponent();
												  const bool bMissionOk = TryBindMissionComponent();

												  //# 미션 바인딩이 남아 있으면 계속 재시도해야 하므로 둘 다 성공했을 때만 멈춘다
												  if (bLevelOk && bMissionOk)
												  {
													  if (UWorld* InnerWorld = GetWorld())
													  {
														  InnerWorld->GetTimerManager().ClearTimer(BindRetryTimerHandle);
													  }

													  return;
												  }

												  //# 실패 누적 — 상한을 넘으면 세션 내내 도는 것을 막고 원인을 로그로 남긴다.
												  //# MaxExperience가 계속 0이면 캐릭터 BP의 LevelComponent에
												  //# LevelConfig(DA_SpyLevelConfig)가 지정되지 않았을 가능성이 높다.
												  BindRetryCount += 1;
												  if (BindRetryCount < BindRetryMaxCount)
													  return;

												  UE_LOG(LogTemp, Warning,
													  TEXT("# [SpyMainHUD] 위젯 바인딩에 %.1f초간 실패해 재시도를 중단합니다. Level=%d Mission=%d — ")
													  TEXT("캐릭터 BP의 LevelComponent에 LevelConfig(DA_SpyLevelConfig)가, ")
													  TEXT("PlayerState BP의 MissionComponent에 MissionConfig가 지정됐는지 확인하세요."),
													  BindRetryMaxCount * 0.25f, bLevelOk, bMissionOk);

												  if (UWorld* InnerWorld = GetWorld())
												  {
													  InnerWorld->GetTimerManager().ClearTimer(BindRetryTimerHandle);
												  }
											  }),
											  0.25f, true);
		}
	}
}

void USpyMainHUD::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindRetryTimerHandle);
	}

	UnbindLevelComponent();
	UnbindMissionComponent();

	Super::NativeDestruct();

	if (Btn_Menu)
	{
		Btn_Menu->OnClicked.Clear();
	}
}

void USpyMainHUD::ShowMenu()
{
	USpyUIManager::Get(this)->OpenSpyUI(ESpyUIType::Menu);
}

bool USpyMainHUD::TryBindLevelComponent()
{
	if (BoundLevelComponent)
		return true;

	APlayerController* OwningController = GetOwningPlayer();
	if (OwningController == nullptr)
		return false;

	APawn* OwningPawn = OwningController->GetPawn();
	if (OwningPawn == nullptr)
		return false;

	USpyLevelComponent* LevelComponent = USpyLevelComponent::FindLevelComponent(OwningPawn);
	if (LevelComponent == nullptr)
		return false;

	//# MaxExperience가 아직 복제되지 않았으면 어트리뷰트 준비 전이다 — 다음 주기에 다시 시도
	if (LevelComponent->GetMaxExperience() <= 0.f)
		return false;

	BoundLevelComponent = LevelComponent;

	BoundLevelComponent->OnExperienceChanged.AddDynamic(this, &USpyMainHUD::HandleExperienceChanged);
	BoundLevelComponent->OnLevelChanged.AddDynamic(this, &USpyMainHUD::HandleLevelChanged);

	//# 구독 전에 이미 변한 값을 놓치지 않도록 즉시 1회 갱신
	RefreshAll();

	return true;
}

void USpyMainHUD::UnbindLevelComponent()
{
	if (BoundLevelComponent)
	{
		BoundLevelComponent->OnExperienceChanged.RemoveDynamic(this, &USpyMainHUD::HandleExperienceChanged);
		BoundLevelComponent->OnLevelChanged.RemoveDynamic(this, &USpyMainHUD::HandleLevelChanged);
	}

	BoundLevelComponent = nullptr;
}

void USpyMainHUD::HandleExperienceChanged(USpyLevelComponent* InLevelComponent, float InOldValue, float InNewValue)
{
	RefreshAll();
}

void USpyMainHUD::HandleLevelChanged(USpyLevelComponent* InLevelComponent, int32 InOldLevel, int32 InNewLevel)
{
	RefreshAll();
}

void USpyMainHUD::RefreshAll()
{
	if (BoundLevelComponent == nullptr)
		return;

	if (PB_Exp)
	{
		//# 0 나눗셈 방어는 USpyHPBar::UpdateHP와 동일한 처리
		PB_Exp->SetPercent(BoundLevelComponent->GetExperienceNormalized());
	}

	if (Txt_Level)
	{
		Txt_Level->SetText(FText::Format(NSLOCTEXT("SpyMainHUD", "LevelFormat", "Lv.{0}"), FText::AsNumber(BoundLevelComponent->GetLevel())));
	}
}

bool USpyMainHUD::TryBindMissionComponent()
{
	if (BoundMissionComponent)
		return true;

	APlayerController* OwningController = GetOwningPlayer();
	if (OwningController == nullptr)
		return false;

	APlayerState* OwningState = OwningController->PlayerState;
	if (OwningState == nullptr)
		return false;

	USpyMissionComponent* MissionComponent = USpyMissionComponent::FindMissionComponent(OwningState);
	if (MissionComponent == nullptr)
		return false;

	BoundMissionComponent = MissionComponent;

	BoundMissionComponent->OnMissionProgressChanged.AddDynamic(this, &USpyMainHUD::HandleMissionProgressChanged);
	BoundMissionComponent->OnAllMissionsCompleted.AddDynamic(this, &USpyMainHUD::HandleAllMissionsCompleted);

	//# 구독 전에 이미 진행된 값을 놓치지 않도록 즉시 1회 갱신
	RefreshMission();

	return true;
}

void USpyMainHUD::UnbindMissionComponent()
{
	if (BoundMissionComponent)
	{
		BoundMissionComponent->OnMissionProgressChanged.RemoveDynamic(this, &USpyMainHUD::HandleMissionProgressChanged);
		BoundMissionComponent->OnAllMissionsCompleted.RemoveDynamic(this, &USpyMainHUD::HandleAllMissionsCompleted);
	}

	BoundMissionComponent = nullptr;
}

void USpyMainHUD::HandleMissionProgressChanged(USpyMissionComponent* InMissionComponent, int32 InMissionIndex, int32 InCount, int32 InTargetCount)
{
	RefreshMission();
}

void USpyMainHUD::HandleAllMissionsCompleted(USpyMissionComponent* InMissionComponent)
{
	RefreshMission();
}

void USpyMainHUD::RefreshMission()
{
	if (BoundMissionComponent == nullptr)
		return;

	const bool bAllDone = BoundMissionComponent->IsAllCompleted();

	if (Txt_MissionName)
	{
		Txt_MissionName->SetText(bAllDone
			? NSLOCTEXT("SpyMainHUD", "MissionAllCompleted", "모든 미션 완료")
			: BoundMissionComponent->GetDisplayName());
	}

	if (Txt_MissionProgress)
	{
		if (bAllDone)
		{
			Txt_MissionProgress->SetText(FText::GetEmpty());
		}
		else
		{
			Txt_MissionProgress->SetText(FText::Format(
				NSLOCTEXT("SpyMainHUD", "MissionProgressFormat", "{0} / {1}"),
				FText::AsNumber(BoundMissionComponent->GetCount()),
				FText::AsNumber(BoundMissionComponent->GetTargetCount())));
		}
	}
}
