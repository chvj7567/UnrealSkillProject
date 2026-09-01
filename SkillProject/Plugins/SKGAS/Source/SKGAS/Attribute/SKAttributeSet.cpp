// Fill out your copyright notice in the Description page of Project Settings.


#include "Attribute/SKAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "SKAbilitySystemComponent.h"
#include "SKGameplayEffectContext.h"
#include "GameplayEffectExtension.h"
#include "SKGameplayTags.h"
#include "Perception/AISense_Damage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKAttributeSet)

UWorld* USKAttributeSet::GetWorld() const
{
	const UObject* Outer = GetOuter();
	check(Outer);

	return Outer->GetWorld();
}

USKAbilitySystemComponent* USKAttributeSet::GetSKAbilitySystemComponent() const
{
	return Cast<USKAbilitySystemComponent>(GetOwningAbilitySystemComponent());
}


void USKAttributeSet::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	DOREPLIFETIME(USKAttributeSet, Health);
	DOREPLIFETIME(USKAttributeSet, Mana);
	DOREPLIFETIME(USKAttributeSet, MaxHealth);
	DOREPLIFETIME(USKAttributeSet, MaxMana);
}

void USKAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    //# Health가 변경되었는지 확인
    if (Data.EvaluatedData.Attribute == GetHealthAttribute())
    {
        FGameplayEffectContextHandle ContextHandle = Data.EffectSpec.GetContext();
        FSKGameplayEffectContext* CustomContext = FSKGameplayEffectContext::ExtractEffectContext(ContextHandle);
        if (CustomContext)
        {
            const float DeltaValue = Data.EvaluatedData.Magnitude;

            AActor* TargetActor = Data.Target.GetAvatarActor();
            AActor* InstigatorActor = Data.EffectSpec.GetContext().GetInstigator();
            AActor* EffectCauser = Data.EffectSpec.GetContext().GetEffectCauser();

            if (TargetActor)
            {
                UAISense_Damage::ReportDamageEvent(
                    GetWorld(),
                    TargetActor,
                    InstigatorActor,
                    DeltaValue,
                    EffectCauser->GetActorLocation(),
                    TargetActor->GetActorLocation()
                );
            }

            //# 크리티컬 여부 확인
            if (CustomContext->IsCritical())
            {
                //# TODO
            }

			//# Health 를 [0, MaxHealth] 로 클램프 — 클램프 없이 음수로 내려가는 버그 수정.
			//# 사망 판정(<= 0.0f)은 클램프 전 값 기준으로 이미 참이므로 순서를 바꿔도 무관.
			SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));

			//# 사망 확인
            if (GetHealth() <= 0.0f)
            {
                FGameplayEventData Payload;
                Payload.EventTag = SKGameplayTags::Character_State_Death;
                Payload.Instigator = CustomContext->GetInstigator();
                Payload.EventMagnitude = CustomContext->IsCritical() ? 1.0f : 0.0f;

                Data.Target.HandleGameplayEvent(Payload.EventTag, &Payload);

                UE_LOG(LogTemp, Warning, TEXT("# [SKAttributeSet] Death %s"), *TargetActor->GetName());
            }
            else
            {
                //# 피격 방향 태그
                FGameplayTag HitTag = CustomContext->GetHitDirectionTag();
                if (HitTag.IsValid())
                {
                    FGameplayEventData Payload;
                    Payload.EventTag = HitTag;
                    Payload.Instigator = CustomContext->GetInstigator();
                    Payload.EventMagnitude = CustomContext->IsCritical() ? 1.0f : 0.0f;

                    Data.Target.HandleGameplayEvent(Payload.EventTag, &Payload);
                }
            }

            //# 서버에서 OnHealthChanged 브로드캐스트 (클라이언트는 OnRep_Health에서 처리)
            float NewHealth = GetHealth();
            OnHealthChanged.Broadcast(InstigatorActor, EffectCauser, &Data.EffectSpec, DeltaValue, NewHealth - DeltaValue, NewHealth);
        }
    }
	//# Mana 를 [0, MaxMana] 로 클램프 — 코스트 감산·재생 GE 가 범위를 벗어나지 않게 한다.
	//# (Health 분기는 클램프 없이 보고·이벤트만 하고, Mana 만 이 분기에서 [0,MaxMana] 로 클램프한다)
	else if (Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(), 0.f, GetMaxMana()));

		//# 서버에서 OnManaChanged 브로드캐스트 (클라이언트는 OnRep_Mana에서 처리) — 로컬 폰 HUD 갱신용.
		//# 코스트·재생 GE 는 커스텀 컨텍스트를 싣지 않으므로 Health 분기처럼 컨텍스트를 요구하지 않는다
		const float DeltaValue = Data.EvaluatedData.Magnitude;
		AActor* InstigatorActor = Data.EffectSpec.GetContext().GetInstigator();
		AActor* EffectCauser = Data.EffectSpec.GetContext().GetEffectCauser();
		const float NewMana = GetMana();
		OnManaChanged.Broadcast(InstigatorActor, EffectCauser, &Data.EffectSpec, DeltaValue, NewMana - DeltaValue, NewMana);
	}
}

void USKAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USKAttributeSet, Health, OldValue);

	OnHealthChanged.Broadcast(nullptr, nullptr, nullptr, GetHealth() - OldValue.GetCurrentValue(), OldValue.GetCurrentValue(), GetHealth());
}

void USKAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USKAttributeSet, MaxHealth, OldValue);

	OnMaxHealthChanged.Broadcast(nullptr, nullptr, nullptr, GetMaxHealth() - OldValue.GetCurrentValue(), OldValue.GetCurrentValue(), GetMaxHealth());
}

void USKAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USKAttributeSet, Mana, OldValue);

	OnManaChanged.Broadcast(nullptr, nullptr, nullptr, GetMana() - OldValue.GetCurrentValue(), OldValue.GetCurrentValue(), GetMana());
}

void USKAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USKAttributeSet, MaxMana, OldValue);

	OnMaxManaChanged.Broadcast(nullptr, nullptr, nullptr, GetMaxMana() - OldValue.GetCurrentValue(), OldValue.GetCurrentValue(), GetMaxMana());
}
