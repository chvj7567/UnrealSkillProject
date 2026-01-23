#include "Data/SKAssetData.h"
#include "Manager/SpyAssetManager.h"
#include "UObject/ObjectSaveContext.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKAssetData)

const USKAssetData& USKAssetData::Get()
{
	return USpyAssetManager::Get().GetAssetData();
}

#if WITH_EDITOR
void USKAssetData::PreSave(FObjectPreSaveContext ObjectSaveContext)
{
	Super::PreSave(ObjectSaveContext);

	AssetNameToPath.Empty();

	AssetGroupNameToSet.KeySort([](const FName& A, const FName& B)
	{
		return (A.Compare(B) < 0);
	});
	
	for (const auto& Pair : AssetGroupNameToSet)
	{
		const FAssetSet& AssetSet = Pair.Value;
		for (FAssetEntry AssetEntry : AssetSet.AssetEntries)
		{
			AssetNameToPath.Emplace(AssetEntry.AssetName, AssetEntry.AssetPath);
		}
	}
}

EDataValidationResult USKAssetData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	for (const auto& Pair : AssetGroupNameToSet)
	{
		const FAssetSet& AssetSet = Pair.Value;
		for (int32 i = 0; i < AssetSet.AssetEntries.Num(); i++)
		{
			const FAssetEntry& AssetEntry = AssetSet.AssetEntries[i];
			if (AssetEntry.AssetName.IsNone())
			{
				Context.AddError(FText::FromString(FString::Printf(TEXT("Asset Name is None : [Group Name : %s] - [Entry Index : %d]"), *Pair.Key.ToString(), i)));
				Result = EDataValidationResult::Invalid;
			}

			if (AssetEntry.AssetPath.IsValid() == false)
			{
				Context.AddError(FText::FromString(FString::Printf(TEXT("Asset Path is Invalid : [Group Name : %s] - [Entry Index : %d]"), *Pair.Key.ToString(), i)));
				Result = EDataValidationResult::Invalid;
			}
		}
	}

	return Result;
}
#endif

FSoftObjectPath USKAssetData::GetAssetPathByName(const FName& AssetName) const
{
	const FSoftObjectPath* AssetPath = AssetNameToPath.Find(AssetName);
	ensureAlwaysMsgf(AssetPath, TEXT("Can't find Asset Path from Asset Name [%s]."), *AssetName.ToString());
	return *AssetPath;
}
