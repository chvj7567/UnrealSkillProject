#include "SpyGameplayAbility_SkillAction.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
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
