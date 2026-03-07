// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/SpyComboAssetData.h"

FGameplayTag USpyComboAssetData::GetComboTag(FGameplayTag InStartSkillTag)
{
	for (FSpyComboSet ComboSet : ComboSets)
	{
		if (ComboSet.StartSkillTag != InStartSkillTag)
			continue;

		return ComboSet.ComboTag;
	}

	return FGameplayTag();
}
