// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SpyMainHUD.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/SpyCharacterAttributeSet.h"
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
	const bool bManaBound = TryBindManaAttribute();

	if (bLevelBound == false || bMissionBound == false || bManaBound == false)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(BindRetryTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]() {
												  const bool bLevelOk = TryBindLevelComponent();
												  const bool bMissionOk = TryBindMissionComponent();
												  const bool bManaOk = TryBindManaAttribute();

												  //# 세 바인딩 중 하나라도 남아 있으면 계속 재시도하므로 전부 성공했을 때만 멈춘다
												  if (bLevelOk && bMissionOk && bManaOk)
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
														 TEXT("# [SpyMainHUD] 위젯 바인딩에 %.1f초간 실패해 재시도를 중단합니다. Level=%d Mission=%d Mana=%d — ")
															 TEXT("캐릭터 BP의 LevelComponent에 LevelConfig(DA_SpyLevelConfig)가, ")
																 TEXT("PlayerState BP의 MissionComponent에 MissionConfig가, ")
																	 TEXT("GE_InitStat에 MaxMana 모디파이어가 지정됐는지 확인하세요."),
														 BindRetryMaxCount * 0.25f, bLevelOk, bMissionOk, bManaOk);

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
	UnbindManaAttribute();

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

bool USpyMainHUD::TryBindManaAttribute()
{
	if (BoundManaSet)
		return true;

	APlayerController* OwningController = GetOwningPlayer();
	if (OwningController == nullptr)
		return false;

	APawn* OwningPawn = OwningController->GetPawn();
	if (OwningPawn == nullptr)
		return false;

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwningPawn);
	if (ASC == nullptr)
		return false;

	const USpyCharacterAttributeSet* AttributeSet = ASC->GetSet<USpyCharacterAttributeSet>();
	if (AttributeSet == nullptr)
		return false;

	//# MaxMana 가 아직 초기화(GE_InitStat)·복제되지 않았으면 어트리뷰트 준비 전이다 — 다음 주기에 다시 시도
	if (AttributeSet->GetMaxMana() <= 0.f)
		return false;

	BoundManaSet = AttributeSet;

	//# FSKAttributeEvent 는 비동적 멀티캐스트라 AddUObject 로 연결한다(mutable 이라 const 포인터에서 호출 가능)
	BoundManaSet->OnManaChanged.AddUObject(this, &USpyMainHUD::HandleManaChanged);
	BoundManaSet->OnMaxManaChanged.AddUObject(this, &USpyMainHUD::HandleMaxManaChanged);

	//# 같은 AttributeSet 이므로 Health 도 이 경로에서 함께 구독한다
	BoundManaSet->OnHealthChanged.AddUObject(this, &USpyMainHUD::HandleHealthChanged);
	BoundManaSet->OnMaxHealthChanged.AddUObject(this, &USpyMainHUD::HandleMaxHealthChanged);

	//# 구독 전에 이미 설정된 값을 놓치지 않도록 즉시 1회 갱신
	RefreshMana();
	RefreshHealth();

	return true;
}

void USpyMainHUD::UnbindManaAttribute()
{
	if (BoundManaSet)
	{
		BoundManaSet->OnManaChanged.RemoveAll(this);
		BoundManaSet->OnMaxManaChanged.RemoveAll(this);
		BoundManaSet->OnHealthChanged.RemoveAll(this);
		BoundManaSet->OnMaxHealthChanged.RemoveAll(this);
	}

	BoundManaSet = nullptr;
}

void USpyMainHUD::HandleManaChanged(AActor* Instigator, AActor* Causer, const FGameplayEffectSpec* Spec, float Magnitude, float OldValue, float NewValue)
{
	RefreshMana();
}

void USpyMainHUD::HandleMaxManaChanged(AActor* Instigator, AActor* Causer, const FGameplayEffectSpec* Spec, float Magnitude, float OldValue, float NewValue)
{
	RefreshMana();
}

void USpyMainHUD::RefreshMana()
{
	if (BoundManaSet == nullptr)
		return;

	if (PB_Mana)
	{
		const float MaxMana = BoundManaSet->GetMaxMana();

		//# 0 나눗셈 방어 — USpyHPBar::UpdateHP 와 동일한 처리
		PB_Mana->SetPercent((MaxMana > 0.f) ? (BoundManaSet->GetMana() / MaxMana) : 0.f);
	}
}

void USpyMainHUD::HandleHealthChanged(AActor* Instigator, AActor* Causer, const FGameplayEffectSpec* Spec, float Magnitude, float OldValue, float NewValue)
{
	RefreshHealth();
}

void USpyMainHUD::HandleMaxHealthChanged(AActor* Instigator, AActor* Causer, const FGameplayEffectSpec* Spec, float Magnitude, float OldValue, float NewValue)
{
	RefreshHealth();
}

void USpyMainHUD::RefreshHealth()
{
	if (BoundManaSet == nullptr)
		return;

	if (PB_HP)
	{
		const float MaxHealth = BoundManaSet->GetMaxHealth();

		//# 0 나눗셈 방어 — USpyHPBar::UpdateHP 와 동일한 처리
		PB_HP->SetPercent((MaxHealth > 0.f) ? (BoundManaSet->GetHealth() / MaxHealth) : 0.f);
	}
}
