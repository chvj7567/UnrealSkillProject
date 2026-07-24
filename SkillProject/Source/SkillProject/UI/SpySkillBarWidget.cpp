// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SpySkillBarWidget.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "AbilitySystem/Skill/SpyGameplayAbility_SkillAction.h"
#include "Data/SpyCharacterAssetData.h"
#include "Data/SpySkillBarConfig.h"
#include "Input/SpyInputConfig.h"
#include "System/SpyPlayerState.h"
#include "UI/SpySkillSlotWidget.h"
#include "Util/SpyGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpySkillBarWidget)

//# 슬롯 입력 태그별 키힌트(기획 §6-1). Skill_N 은 숫자 N
static FText SlotKeyHint(const FGameplayTag& InputTag)
{
	using namespace SpyGameplayTags;

	if (InputTag == Input_Ability_Skill_1)
		return FText::FromString(TEXT("1"));
	if (InputTag == Input_Ability_Skill_2)
		return FText::FromString(TEXT("2"));
	if (InputTag == Input_Ability_Skill_3)
		return FText::FromString(TEXT("3"));
	if (InputTag == Input_Ability_Skill_4)
		return FText::FromString(TEXT("4"));
	if (InputTag == Input_Ability_Skill_5)
		return FText::FromString(TEXT("5"));
	if (InputTag == Input_Ability_Skill_6)
		return FText::FromString(TEXT("6"));

	return FText::GetEmpty();
}

TArray<FGameplayTag> USpySkillBarWidget::BuildSlotInputTags()
{
	//# 화면 좌→우 슬롯 순서. Skill_1~6 (Parry 는 슬롯 대상 아님)
	return TArray<FGameplayTag>{
		SpyGameplayTags::Input_Ability_Skill_1,
		SpyGameplayTags::Input_Ability_Skill_2,
		SpyGameplayTags::Input_Ability_Skill_3,
		SpyGameplayTags::Input_Ability_Skill_4,
		SpyGameplayTags::Input_Ability_Skill_5,
		SpyGameplayTags::Input_Ability_Skill_6};
}

TArray<FGameplayTag> USpySkillBarWidget::ExtractSlotInputTags(const USpySkillBarConfig* Config)
{
	//# config 가 슬롯을 정의하면 그 순서를 그대로 쓴다
	if (Config != nullptr && Config->Slots.Num() > 0)
	{
		TArray<FGameplayTag> Tags;
		Tags.Reserve(Config->Slots.Num());
		for (const FSpySkillBarSlot& Slot : Config->Slots)
		{
			Tags.Add(Slot.InputTag);
		}
		return Tags;
	}

	//# config 미지정/빈 배열이면 하위호환 — 하드코딩 Skill_1~6 폴백
	return BuildSlotInputTags();
}

void USpySkillBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	bSlotsBuilt = false;
	BuildRetryCount = 0;

	//# ASC·어빌리티 부여가 아직 안 됐을 수 있으니 준비될 때까지 짧은 주기로 재시도한다(SpyMainHUD 패턴)
	if (TryBuildSlots() == false)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(BuildRetryTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]() {
												  if (TryBuildSlots())
												  {
													  if (UWorld* InnerWorld = GetWorld())
													  {
														  InnerWorld->GetTimerManager().ClearTimer(BuildRetryTimerHandle);
													  }

													  return;
												  }

												  //# 상한 도달 — 어빌리티 미부여여도 슬롯 골격은 한 번 만들어 둔다(전부 비활성)
												  BuildRetryCount += 1;
												  if (BuildRetryCount < BuildRetryMaxCount)
													  return;

												  APlayerController* OwningController = GetOwningPlayer();
												  APawn* OwningPawn = (OwningController != nullptr) ? OwningController->GetPawn() : nullptr;
												  if (UAbilitySystemComponent* ASC = (OwningPawn != nullptr) ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwningPawn) : nullptr)
												  {
													  BuildSlots(ASC);
												  }

												  if (UWorld* InnerWorld = GetWorld())
												  {
													  InnerWorld->GetTimerManager().ClearTimer(BuildRetryTimerHandle);
												  }
											  }),
											  0.25f, true);
		}
	}
}

void USpySkillBarWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BuildRetryTimerHandle);
	}

	Super::NativeDestruct();
}

bool USpySkillBarWidget::TryBuildSlots()
{
	if (bSlotsBuilt)
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

	//# 슬롯 태그 중 하나라도 어빌리티가 부여됐을 때 빌드한다(빈 스킬바 방지).
	//# 게이트도 config 슬롯 기준으로 통일한다(BuildSlots 와 동일한 목록)
	const TArray<FGameplayTag> InputTags = ExtractSlotInputTags(ResolveConfig());
	bool bAnyGranted = false;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability == nullptr)
			continue;

		for (const FGameplayTag& InputTag : InputTags)
		{
			if (Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
			{
				bAnyGranted = true;
				break;
			}
		}

		if (bAnyGranted)
			break;
	}

	if (bAnyGranted == false)
		return false;

	BuildSlots(ASC);
	return true;
}

void USpySkillBarWidget::BuildSlots(UAbilitySystemComponent* ASC)
{
	if (bSlotsBuilt)
		return;

	//# SlotWidgetClass 는 EditDefaultsOnly 라 임베드된 UserWidget 인스턴스에서 런타임에 null 로 올 수 있다.
	//# 인스턴스 값이 null 이면 클래스 기본값(CDO)에서 폴백 해석한다(CDO 는 WBP_SkillSlot_C 로 신뢰 가능).
	TSubclassOf<USpySkillSlotWidget> ResolvedSlotClass = SlotWidgetClass;
	if (ResolvedSlotClass == nullptr)
	{
		if (const USpySkillBarWidget* DefaultBar = GetClass()->GetDefaultObject<USpySkillBarWidget>())
		{
			ResolvedSlotClass = DefaultBar->SlotWidgetClass;
		}
	}

	if (ASC == nullptr || Panel_Slots == nullptr || ResolvedSlotClass == nullptr)
		return;

	//# 슬롯 구성 config 도 인스턴스→CDO 폴백 (SlotWidgetClass 폴백과 동일 패턴)
	const USpySkillBarConfig* ResolvedConfig = ResolveConfig();

	//# 슬롯 태그 목록: config 있으면 그것, 없으면 하드코딩 Skill_1~6 폴백
	const TArray<FGameplayTag> InputTags = ExtractSlotInputTags(ResolvedConfig);
	for (const FGameplayTag& InputTag : InputTags)
	{
		USpySkillSlotWidget* SlotWidget = CreateWidget<USpySkillSlotWidget>(this, ResolvedSlotClass);
		if (SlotWidget == nullptr)
			continue;

		FGameplayTagContainer CooldownTags;
		float SlotManaCost = 0.f;
		bool bHasAbility = false;

		//# 입력 태그로 부여된 어빌리티 스펙을 찾아 쿨다운 태그·마나 코스트를 해석한다
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			if (Spec.Ability != nullptr && Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
			{
				bHasAbility = true;

				if (const FGameplayTagContainer* AbilityCooldownTags = Spec.Ability->GetCooldownTags())
				{
					CooldownTags = *AbilityCooldownTags;
				}

				if (const USpyGameplayAbility_SkillAction* SkillAction = Cast<USpyGameplayAbility_SkillAction>(Spec.Ability))
				{
					SlotManaCost = SkillAction->GetManaCost();
				}

				break;
			}
		}

		//# 이 InputTag 에 대응하는 config 아이콘을 찾아 로드한다(없으면 nullptr → 더미색 유지)
		UTexture2D* SlotIcon = nullptr;
		if (ResolvedConfig != nullptr)
		{
			for (const FSpySkillBarSlot& CfgSlot : ResolvedConfig->Slots)
			{
				if (CfgSlot.InputTag == InputTag)
				{
					SlotIcon = CfgSlot.Icon.LoadSynchronous();
					break;
				}
			}
		}

		SlotWidget->Setup(ASC, InputTag, CooldownTags, ResolveSlotKeyHint(InputTag), SlotManaCost, SlotIcon);

		//# 어빌리티 미부여 슬롯은 비활성(회색) 표시로 남긴다(기획 §6-1)
		SlotWidget->SetIsEnabled(bHasAbility);

		//# 슬롯 루트가 CanvasPanel(desired 0)이라 HorizontalBox Auto 로는 0폭 → Fill 로 균등 분배
		UPanelSlot* AddedSlot = Panel_Slots->AddChild(SlotWidget);
		if (UHorizontalBoxSlot* HBoxSlot = Cast<UHorizontalBoxSlot>(AddedSlot))
		{
			FSlateChildSize FillSize;
			FillSize.SizeRule = ESlateSizeRule::Fill;
			FillSize.Value = 1.0f;
			HBoxSlot->SetSize(FillSize);
			HBoxSlot->SetPadding(FMargin(3.f));
		}
	}

	bSlotsBuilt = true;
}

const USpySkillBarConfig* USpySkillBarWidget::ResolveConfig() const
{
	//# 인스턴스 값이 유효하면 그것, 아니면 클래스 기본값(CDO)에서 폴백
	if (SkillBarConfig != nullptr)
		return SkillBarConfig;

	if (const USpySkillBarWidget* DefaultBar = GetClass()->GetDefaultObject<USpySkillBarWidget>())
		return DefaultBar->SkillBarConfig;

	return nullptr;
}

FText USpySkillBarWidget::ResolveSlotKeyHint(FGameplayTag InputTag) const
{
	APlayerController* OwningController = GetOwningPlayer();
	if (OwningController == nullptr)
		return SlotKeyHint(InputTag);

	//# CharacterAssetData → (엔트리별) InputConfig 에서 이 InputTag 의 InputAction 을 찾는다
	const UInputAction* InputAction = nullptr;
	if (const ASpyPlayerState* OwningState = OwningController->GetPlayerState<ASpyPlayerState>())
	{
		if (const USpyCharacterAssetData* CharacterAssetData = OwningState->GetCharacterAssetData())
		{
			for (const FCharacterAssetEntry& Entry : CharacterAssetData->CharacterAssets.AssetEntries)
			{
				if (const USpyInputConfig* InputConfig = Entry.InputConfig)
				{
					if (const UInputAction* Found = InputConfig->FindAbilityInputActionForTag(InputTag))
					{
						InputAction = Found;
						break;
					}
				}
			}
		}
	}

	if (InputAction == nullptr)
		return SlotKeyHint(InputTag);

	//# InputAction 에 실제 매핑된 첫 유효 키의 표시문자를 쓴다 — config/리바인드와 무관하게 항상 정확
	const ULocalPlayer* LocalPlayer = OwningController->GetLocalPlayer();
	if (LocalPlayer == nullptr)
		return SlotKeyHint(InputTag);

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (InputSubsystem == nullptr)
		return SlotKeyHint(InputTag);

	const TArray<FKey> MappedKeys = InputSubsystem->QueryKeysMappedToAction(InputAction);
	for (const FKey& Key : MappedKeys)
	{
		if (Key.IsValid())
		{
			return Key.GetDisplayName();
		}
	}

	//# 매핑된 키가 없으면 하드코딩 폴백
	return SlotKeyHint(InputTag);
}
