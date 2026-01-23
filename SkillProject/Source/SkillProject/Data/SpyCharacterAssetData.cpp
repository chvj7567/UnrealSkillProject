#include "SpyCharacterAssetData.h"
#include "Manager/SpyAssetManager.h"
#include "UObject/ObjectSaveContext.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyCharacterAssetData)

const USpyCharacterAssetData& USpyCharacterAssetData::Get()
{
	return *USpyAssetManager::Get().GetCharacterAssetData();
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
				Context.AddError(FText::FromString(FString::Printf(TEXT("Character Skill Name is None : [CharacterType : %s]"), *AssetEntry.CharacterType.ToString())));
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

FName USpyCharacterAssetData::GetCharacterSkillAssetName(FGameplayTag InCharacterType, FGameplayTag InSkillTag) const
{
	for (auto& CharacterAsset : CharacterAssets.AssetEntries)
	{
		if (CharacterAsset.CharacterType == InCharacterType)
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