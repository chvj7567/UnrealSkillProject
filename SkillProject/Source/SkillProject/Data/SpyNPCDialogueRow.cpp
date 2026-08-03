// Fill out your copyright notice in the Description page of Project Settings.

#include "Data/SpyNPCDialogueRow.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyNPCDialogueRow)

bool TryGetDialogueLineAtIndex(const UDataTable* InDialogueTable, int32 InDialogueId, int32 InPageIndex, FText& OutLine)
{
	OutLine = FText::GetEmpty();

	if (InDialogueTable == nullptr || InPageIndex < 0)
		return false;

	TArray<FSpyDialogueRow*> AllRows;
	InDialogueTable->GetAllRows<FSpyDialogueRow>(TEXT("TryGetDialogueLineAtIndex"), AllRows);

	TArray<const FSpyDialogueRow*> Matches;
	for (const FSpyDialogueRow* Row : AllRows)
	{
		if (Row != nullptr && Row->DialogueId == InDialogueId)
		{
			Matches.Add(Row);
		}
	}

	if (Matches.Num() == 0 || InPageIndex >= Matches.Num())
		return false;

	Matches.Sort([](const FSpyDialogueRow& A, const FSpyDialogueRow& B) { return A.DialogueIndex < B.DialogueIndex; });

	OutLine = Matches[InPageIndex]->Text;
	return true;
}

ESpyNPCDialogueState ResolveNPCDialogueState(int32 CurrentMissionId, bool bAccepted, int32 OfferMissionId, int32 ReportMissionId)
{
	if (CurrentMissionId == OfferMissionId)
		return (bAccepted ? ESpyNPCDialogueState::InProgress : ESpyNPCDialogueState::Offer);

	if (CurrentMissionId == ReportMissionId)
		return ESpyNPCDialogueState::Report;

	return ESpyNPCDialogueState::Default;
}
