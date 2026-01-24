#include "SpyCharacterAssetData.h"
#include "Manager/SpyAssetManager.h"
#include "UObject/ObjectSaveContext.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyCharacterAssetData)

const USpyCharacterAssetData& USpyCharacterAssetData::Get()
{
	return USpyAssetManager::Get().GetCharacterAssetData();
}

#if WITH_EDITOR
void USpyCharacterAssetData::PreSave(FObjectPreSaveContext ObjectSaveContext)
{
	Super::PreSave(ObjectSaveContext);

}

EDataValidationResult USpyCharacterAssetData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	for (FSkillAssetEntry AssetEntry : CharacterAssets.CommonSkills)
	{
		if (AssetEntry.Name.IsNone())
		{
			Context.AddError(FText::FromString(FString::Printf(TEXT("Common Skill Name is None"))));
			Result = EDataValidationResult::Invalid;
		}
	}

	for (FCharacterAssetEntry AssetEntry : CharacterAssets.AssetEntries)
	{
		for (FSkillAssetEntry SkillEntry : AssetEntry.CharacterSkills)
		{
			if (SkillEntry.Name.IsNone())
			{
				Context.AddError(FText::FromString(FString::Printf(TEXT("Character Skill Name is None : [CharacterType : %s]"), *AssetEntry.ClassType.ToString())));
				Result = EDataValidationResult::Invalid;
			}
		}
	}

	return Result;
}
#endif

FName USpyCharacterAssetData::GetCommonSkillAssetName(FGameplayTag InSkillTag) const
{
	for (auto& CommonSkill : CharacterAssets.CommonSkills)
	{
		if (CommonSkill.SkillTag == InSkillTag)
		{
			return CommonSkill.Name;
		}
	}

	return FName();
}

FName USpyCharacterAssetData::GetCharacterSkillAssetName(FGameplayTag InClassType, FGameplayTag InSkillTag) const
{
	for (auto& CharacterAsset : CharacterAssets.AssetEntries)
	{
		if (CharacterAsset.ClassType == InClassType)
		{
			for (auto& Skill : CharacterAsset.CharacterSkills)
			{
				if (Skill.SkillTag == InSkillTag)
				{
					return Skill.Name;
				}
			}
		}
	}

	return FName();
}

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
