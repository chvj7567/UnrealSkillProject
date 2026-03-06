// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/SpyComboAssetData.h"

FGameplayTag USpyComboAssetData::GetComboTag(int ComboStep)
{
	for (FSpyComboSet ComboSet : ComboSets)
	{
		if (ComboSet.ComboStep == ComboStep)
			return ComboSet.ComboTag;
	}

	return FGameplayTag();
}
