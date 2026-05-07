#include "BTService_CheckCooldown.h"
#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(BTService_CheckCooldown)

UBTService_CheckCooldown::UBTService_CheckCooldown()
{
	NodeName = TEXT("Check Cooldown");
	Interval = 0.1f;
	RandomDeviation = 0.0f;

	IsOnCooldownKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_CheckCooldown, IsOnCooldownKey));
}

void UBTService_CheckCooldown::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (IsValid(BB) == false) return;

	bool bOnCooldown = false;

	if (CooldownTags.IsEmpty() == false)
	{
		AAIController* AIController = OwnerComp.GetAIOwner();
		APawn* Pawn = IsValid(AIController) ? AIController->GetPawn() : nullptr;

		UAbilitySystemComponent* ASC = nullptr;
		if (IsValid(Pawn))
		{
			APlayerState* PS = Pawn->GetPlayerState();
			if (IsValid(PS))
			{
				ASC = PS->FindComponentByClass<UAbilitySystemComponent>();
			}
			if (IsValid(ASC) == false)
			{
				ASC = Pawn->FindComponentByClass<UAbilitySystemComponent>();
			}
		}

		if (IsValid(ASC))
		{
			bOnCooldown = ASC->HasAnyMatchingGameplayTags(CooldownTags);

			if (bOnCooldown)
			{
				FGameplayTagContainer ActiveTags;
				ASC->GetOwnedGameplayTags(ActiveTags);
				UE_LOG(LogTemp, Warning, TEXT("[CheckCooldown] bOnCooldown=1, 활성태그: %s"), *ActiveTags.ToStringSimple());
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[CheckCooldown] bOnCooldown=0, 찾는태그: %s"), *CooldownTags.ToStringSimple());
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[CheckCooldown] ASC 못 찾음 — Pawn=%s"),
				IsValid(Pawn) ? *Pawn->GetName() : TEXT("null"));
		}
	}

	BB->SetValueAsBool(IsOnCooldownKey.SelectedKeyName, bOnCooldown);
}
