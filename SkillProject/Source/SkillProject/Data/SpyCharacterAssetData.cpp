#include "SpyCharacterAssetData.h"
#include "Manager/SpyAssetManager.h"
#include "UObject/ObjectSaveContext.h"
#include "Data/SpyAbilityData.h"
#include "Data/SpyComboAssetData.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyCharacterAssetData)

#if WITH_EDITOR
void USpyCharacterAssetData::PreSave(FObjectPreSaveContext ObjectSaveContext)
{
	Super::PreSave(ObjectSaveContext);

}

EDataValidationResult USpyCharacterAssetData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	return Result;
}
#endif

TArray<TSubclassOf<UActorComponent>> USpyCharacterAssetData::GetAllComponentClasses(FGameplayTag InClassType) const
{
	TArray<TSubclassOf<UActorComponent>> ComponentClasses;

	//# 공통 캐릭터 컴포넌트
	for (auto& ComponentClass : CharacterAssets.CommonComponentClasses)
	{
		ComponentClasses.Add(ComponentClass);
	}

	//# 전용 캐릭터 컴포넌트
	for (auto& CharacterAsset : CharacterAssets.AssetEntries)
	{
		if (CharacterAsset.ClassType == InClassType)
		{
			for (auto& ComponentClass : CharacterAsset.CharacterComponentClasses)
			{
				ComponentClasses.Add(ComponentClass);
			}
		}
	}

	return ComponentClasses;
}
FGameplayTag USpyCharacterAssetData::GetComboTag(FGameplayTag InClassType, FGameplayTag InStartSkillTag)
{
	for (auto& CharacterAsset : CharacterAssets.AssetEntries)
	{
		if (CharacterAsset.ClassType == InClassType)
		{
			if (CharacterAsset.ClassCombos != nullptr)
				return CharacterAsset.ClassCombos->GetComboTag(InStartSkillTag);
		}
	}

	return FGameplayTag();
}
