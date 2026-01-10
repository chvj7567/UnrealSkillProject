// Fill out your copyright notice in the Description page of Project Settings.

#include "Ability/SKGameplayAbility.h"
#include "SKAbilitySourceInterface.h"
#include "SKGameplayEffectContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKGameplayAbility)

void USKGameplayAbility::OnMontageCompleted()
{
    //UE_LOG(LogTemp, Warning, TEXT("USKGameplayAbility OnMontageCompleted"));
}

void USKGameplayAbility::OnMontageCancelled()
{
    //UE_LOG(LogTemp, Warning, TEXT("USKGameplayAbility OnMontageCancelled"));
}

void USKGameplayAbility::OnWaitGameplayEvent(FGameplayEventData Payload)
{
    //UE_LOG(LogTemp, Warning, TEXT("USKGameplayAbility OnWaitGameplayEvent"));
}

void USKGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    //UE_LOG(LogTemp, Warning, TEXT("USKGameplayAbility ActivateAbility called"));
}

void USKGameplayAbility::CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateCancelAbility)
{
    Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);

    //UE_LOG(LogTemp, Warning, TEXT("USKGameplayAbility CancelAbility called"));
}

bool USKGameplayAbility::CommitAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, OUT FGameplayTagContainer* OptionalRelevantTags)
{
    bool bResult = Super::CommitAbility(Handle, ActorInfo, ActivationInfo, OptionalRelevantTags);

    //UE_LOG(LogTemp, Warning, TEXT("USKGameplayAbility CommitAbility called, returning: %s"), bResult ? TEXT("true") : TEXT("false"));

    return bResult;
}

void USKGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

    //UE_LOG(LogTemp, Warning, TEXT("USKGameplayAbility EndAbility called, WasCancelled: %s"), bWasCancelled ? TEXT("true") : TEXT("false"));
}

bool USKGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
    bool bResult = Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);

    //UE_LOG(LogTemp, Warning, TEXT("USKGameplayAbility CanActivateAbility called, returning: %s"), bResult ? TEXT("true") : TEXT("false"));

    return bResult;
}

bool USKGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
    bool bResult = Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags);

    //UE_LOG(LogTemp, Warning, TEXT("USKGameplayAbility CheckCost called, returning: %s"), bResult ? TEXT("true") : TEXT("false"));

    return bResult;
}

void USKGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
    Super::ApplyCost(Handle, ActorInfo, ActivationInfo);

    //UE_LOG(LogTemp, Warning, TEXT("USKGameplayAbility ApplyCost called"));
}

const FGameplayTagContainer* USKGameplayAbility::GetCooldownTags() const
{
    const FGameplayTagContainer* Result = Super::GetCooldownTags();

    //UE_LOG(LogTemp, Warning, TEXT("USKGameplayAbility GetCooldownTags called"));

    return Result;
}

void USKGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
    Super::OnGiveAbility(ActorInfo, Spec);

    //UE_LOG(LogTemp, Warning, TEXT("USKGameplayAbility OnGiveAbility called"));
}

void USKGameplayAbility::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
    Super::OnRemoveAbility(ActorInfo, Spec);

    //UE_LOG(LogTemp, Warning, TEXT("USKGameplayAbility OnRemoveAbility called"));
}

FGameplayEffectContextHandle USKGameplayAbility::MakeEffectContext(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const
{
    FGameplayEffectContextHandle ContextHandle = Super::MakeEffectContext(Handle, ActorInfo);

    //UE_LOG(LogTemp, Warning, TEXT("USKGameplayAbility MakeEffectContext called"));

    FSKGameplayEffectContext* EffectContext = FSKGameplayEffectContext::ExtractEffectContext(ContextHandle);
    check(EffectContext);
    check(ActorInfo);

    //# Default
    AActor* Instigator = ActorInfo->OwnerActor.Get();
    AActor* EffectCauser = ActorInfo->AvatarActor.Get();
    UObject* SourceObject = GetSourceObject(Handle, ActorInfo);

    EffectContext->AddInstigator(Instigator, EffectCauser);
    EffectContext->AddSourceObject(SourceObject);

    //# Custom
    ISKAbilitySourceInterface* AbilitySource = Cast<ISKAbilitySourceInterface>(SourceObject);

    EffectContext->SetAbilitySource(AbilitySource, 0.0f);

    return ContextHandle;
}