#include "SpyGameplayAbility_SkillAction.h"
#include "AbilitySystem/Effect/SpyGE_ManaCost.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "Attribute/SKAttributeSet.h"
#include "Util/SpyGameplayTags.h"
#include "System/SpyPlayerState.h"
#include "System/SpyMissionComponent.h"
#include "GameFramework/Character.h"
#include "Data/SpyCharacterAssetData.h"
#include "ManagerComponent/SpyTargetingManagerComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Character/SpyCharacter.h"
#include "Item/SpyWeapon.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGameplayAbility_SkillAction)

void USpyGameplayAbility_SkillAction::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    //# 미션 진행 — 콤보로 연결되어 활성화된 경우만 센다.
    //# 최초 입력 활성화는 TriggerEventData가 없으므로 세지 않는다 (3연타 = 연결 2회).
    //# InputPressed는 bReplicateInputDirectly가 False라 데디케이티드 서버에서 원격 폰에 대해 실행되지 않으므로
    //# 서버까지 확정 전달되는 이 경로(ServerTryActivateAbilityWithEventData)에서 잡는다
    if (HasAuthority(&ActivationInfo) && TriggerEventData != nullptr)
    {
        if (SpyGameplayTags::GetComboTags().HasTagExact(TriggerEventData->EventTag))
        {
            if (ASpyCharacter* ComboOwner = Cast<ASpyCharacter>(GetAvatarActorFromActorInfo()))
            {
                if (USpyMissionComponent* MissionComp = USpyMissionComponent::FindMissionComponent(ComboOwner->GetPlayerState()))
                {
                    MissionComp->AddProgress(SpyGameplayTags::Event_Mission_Combo, 1);
                }
            }
        }
    }

    ASpyCharacter* SpyChar = Cast<ASpyCharacter>(GetAvatarActorFromActorInfo());
    if (IsValid(SpyChar) && IsValid(SpyChar->GetSpyWeapon()))
    {
        if (HasAuthority(&ActivationInfo))
        {
            SpyChar->GetSpyWeapon()->Multicast_ActivateTrail();
        }
        else
        {
            SpyChar->GetSpyWeapon()->ActivateTrail();
        }
    }
}

void USpyGameplayAbility_SkillAction::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    ASpyCharacter* SpyChar = Cast<ASpyCharacter>(GetAvatarActorFromActorInfo());
    if (IsValid(SpyChar) && IsValid(SpyChar->GetSpyWeapon()))
    {
        if (HasAuthority(&ActivationInfo))
        {
            SpyChar->GetSpyWeapon()->Multicast_DeactivateTrail();
        }
        else
        {
            SpyChar->GetSpyWeapon()->DeactivateTrail();
        }
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USpyGameplayAbility_SkillAction::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (IsValid(OwnerCharacter) == false)
        return;

    ASpyPlayerState* OwnerPS = OwnerCharacter->GetPlayerState<ASpyPlayerState>();
    if (IsValid(OwnerPS) == false)
        return;

    if (CurrentActorInfo == nullptr)
        return;

    USpyAbilitySystemComponent* OwnerASC = Cast<USpyAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get());
    if (IsValid(OwnerASC) == false)
        return;

    if (OwnerASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Combo))
    {
        FGameplayTag MyTag = AbilityTags.GetByIndex(0);
        if (MyTag.IsValid() == false)
            return;

        if (USpyCharacterAssetData* CharacterAssetData = OwnerPS->GetCharacterAssetData())
        {
            FGameplayTag Tag = CharacterAssetData->GetComboTag(SpyGameplayTags::Character_Class_Normal, MyTag);
            if (Tag.IsValid())
            {
                FGameplayEventData Payload;
                Payload.EventTag = Tag;

                OwnerASC->HandleGameplayEvent(Payload.EventTag, &Payload);
                return;
            }
        }
    }
}

bool USpyGameplayAbility_SkillAction::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	//# 코스트가 없는 어빌리티는 기존(CostGameplayEffectClass) 동작으로 폴백한다
	if (ManaCost <= 0.f)
	{
		return Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags);
	}

	const UAbilitySystemComponent* ASC = (ActorInfo != nullptr) ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (ASC == nullptr)
	{
		return false;
	}

	//# 현재 마나가 코스트 이상이어야 발동 가능 (클라·서버 모두 복제된 Mana 로 판정)
	const float CurMana = ASC->GetNumericAttribute(USKAttributeSet::GetManaAttribute());
	return CurMana >= ManaCost;
}

void USpyGameplayAbility_SkillAction::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	//# 코스트가 없는 어빌리티는 기존(CostGameplayEffectClass) 동작으로 폴백한다
	if (ManaCost <= 0.f)
	{
		Super::ApplyCost(Handle, ActorInfo, ActivationInfo);
		return;
	}

	//# 마나 감산은 게임플레이 상태 변경이므로 서버 권한에서만 적용한다 (클라는 복제로 반영)
	if (HasAuthority(&ActivationInfo) == false)
	{
		return;
	}

	UAbilitySystemComponent* ASC = (ActorInfo != nullptr) ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (ASC == nullptr)
	{
		return;
	}

	FGameplayEffectContextHandle Context = MakeEffectContext(Handle, ActorInfo);
	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(USpyGE_ManaCost::StaticClass(), GetAbilityLevel(), Context);
	if (SpecHandle.IsValid())
	{
		//# Additive 모디파이어에 음수 코스트를 실어 마나를 감산한다
		SpecHandle.Data->SetSetByCallerMagnitude(SpyGameplayTags::Data_Cost_Mana, -ManaCost);
		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}
