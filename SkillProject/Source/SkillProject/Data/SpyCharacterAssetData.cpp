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

	return Result;
}
#endif // WITH_EDITOR

FName USpyCharacterAssetData::GetSkillAssetNameByType(ESpyCharacterType InCharacterType, ESpySkillType InSkillType) const
{
	for (auto& CharacterAsset : CharacterAssets.AssetEntries)
	{
		if (CharacterAsset.CharacterType == InCharacterType)
		{
			for (auto& Skill : CharacterAsset.Skills)
			{
				if (Skill.SkillType == InSkillType)
				{
					return Skill.Name;
				}
			}
		}
	}

	return FName();
}

FSoftObjectPath USpyCharacterAssetData::GetSkillAssetPathByType(ESpyCharacterType InCharacterType, ESpySkillType InSkillType) const
{
	for (auto& CharacterAsset : CharacterAssets.AssetEntries)
	{
		if (CharacterAsset.CharacterType == InCharacterType)
		{
			for (auto& Skill : CharacterAsset.Skills)
			{
				if (Skill.SkillType == InSkillType)
				{
					return Skill.AssetPath;
				}
			}
		}
	}

	return FSoftObjectPath();
}