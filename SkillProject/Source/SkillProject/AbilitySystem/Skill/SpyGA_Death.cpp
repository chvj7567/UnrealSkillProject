// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyGA_Death.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Character/CommonInterface.Character.h"
#include "System/SpyAIController.h"
#include "Util/SpyGameplayTags.h"

void USpyGA_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
    {
        if (UCapsuleComponent* CapsuleComp = OwnerCharacter->GetCapsuleComponent())
        {
            CapsuleComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
        }

		ISpyCharacterRoot* RootPtr = Cast<ISpyCharacterRoot>(OwnerCharacter);
		TScriptInterface<ISpyTargetProvider> TargetProviderHandle = RootPtr ? RootPtr->GetTargetProvider() : TScriptInterface<ISpyTargetProvider>();
		if (ISpyTargetProvider* TargetingComp = IsValid(TargetProviderHandle.GetObject()) ? TargetProviderHandle.GetInterface() : nullptr)
		{
			TargetingComp->FindTarget(0.f);
		}

		if (HasAuthority(&ActivationInfo))
		{
			//# 디졸브 지속시간을 Cue 파라미터에 실어 보낸다 — 클라이언트에는 AIController 가 없을 수 있어
			//# Cue 가 직접 컨트롤러를 재조회하지 않고 이 값을 그대로 쓴다. 못 구하면 0 을 보내
			//# Cue 쪽(USpyGameplayCueNotify_Death)이 자체 기본값으로 대체하게 한다.
			float DissolveDurationSeconds = 0.f;
			if (ASpyAIController* SpyAIController = Cast<ASpyAIController>(OwnerCharacter->GetController()))
			{
				DissolveDurationSeconds = SpyAIController->GetDissolveDurationSeconds();
			}

			//# Add(지속) 로 걸어야 GA_Death 종료 시(OnRespawn 의 CancelAbilities) 자동으로 Remove 이벤트가 발화한다.
			//# K2_ExecuteGameplayCue 는 1회성 Execute 이벤트만 쏘아 OnActive/OnRemove 가 호출되지 않는다.
			FGameplayCueParameters CueParameters;
			CueParameters.RawMagnitude = DissolveDurationSeconds;
			K2_AddGameplayCueWithParams(SpyGameplayTags::GameplayCue_Actor_Death, CueParameters);
		}
	}
}
