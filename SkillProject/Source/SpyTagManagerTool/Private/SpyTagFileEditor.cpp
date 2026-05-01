#include "SpyTagFileEditor.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"

FString FSpyTagFileEditor::GetHeaderPath()
{
	return FPaths::ConvertRelativePathToFull(
		FPaths::ProjectDir() / TEXT("Source/SkillProject/Util/SpyGameplayTags.h"));
}

FString FSpyTagFileEditor::GetCppPath()
{
	return FPaths::ConvertRelativePathToFull(
		FPaths::ProjectDir() / TEXT("Source/SkillProject/Util/SpyGameplayTags.cpp"));
}

TArray<FSpyTagGroup> FSpyTagFileEditor::ParseHeaderGroups()
{
	TArray<FSpyTagGroup> Groups;
	TArray<FString> Lines;
	if (!FFileHelper::LoadFileToStringArray(Lines, *GetHeaderPath()))
		return Groups;

	FSpyTagGroup* Current = nullptr;
	for (const FString& Line : Lines)
	{
		FString T = Line.TrimStartAndEnd();
		if (T.StartsWith(TEXT("//#")))
		{
			FSpyTagGroup G;
			G.Comment = T.RightChop(3).TrimStartAndEnd();
			Groups.Add(G);
			Current = &Groups.Last();
			continue;
		}
		if (Current && T.Contains(TEXT("UE_DECLARE_GAMEPLAY_TAG_EXTERN(")))
		{
			FString Inner = T;
			Inner.RemoveFromStart(TEXT("SKILLPROJECT_API "));
			Inner.RemoveFromStart(TEXT("UE_DECLARE_GAMEPLAY_TAG_EXTERN("));
			Inner.RemoveFromEnd(TEXT(");"));
			FSpyTagEntry Entry;
			Entry.VarName = Inner.TrimStartAndEnd();
			Current->Tags.Add(Entry);
		}
	}
	return Groups;
}

TMap<FString, FString> FSpyTagFileEditor::ParseCppTagMap()
{
	TMap<FString, FString> Result;
	TArray<FString> Lines;
	if (!FFileHelper::LoadFileToStringArray(Lines, *GetCppPath()))
		return Result;

	for (const FString& Line : Lines)
	{
		FString T = Line.TrimStartAndEnd();
		if (!T.StartsWith(TEXT("UE_DEFINE_GAMEPLAY_TAG("))) continue;

		FString Inner = T;
		Inner.RemoveFromStart(TEXT("UE_DEFINE_GAMEPLAY_TAG("));
		Inner.RemoveFromEnd(TEXT(");"));

		TArray<FString> Parts;
		Inner.ParseIntoArray(Parts, TEXT(","), true);
		if (Parts.Num() < 2) continue;

		FString VarName = Parts[0].TrimStartAndEnd();
		FString TagStr  = Parts[1].TrimStartAndEnd();
		TagStr.RemoveFromStart(TEXT("\""));
		TagStr.RemoveFromEnd(TEXT("\""));
		Result.Add(VarName, TagStr);
	}
	return Result;
}

TArray<FSpyTagGroup> FSpyTagFileEditor::ParseFiles()
{
	TArray<FSpyTagGroup> Groups = ParseHeaderGroups();
	TMap<FString, FString> CppMap = ParseCppTagMap();
	for (FSpyTagGroup& Group : Groups)
		for (FSpyTagEntry& Entry : Group.Tags)
			if (FString* S = CppMap.Find(Entry.VarName))
				Entry.TagString = *S;
	return Groups;
}

FString FSpyTagFileEditor::TagStringToVarName(const FString& TagString)
{
	return TagString.Replace(TEXT("."), TEXT("_"));
}

bool FSpyTagFileEditor::DoesVarNameExist(const TArray<FSpyTagGroup>& Groups, const FString& VarName)
{
	for (const FSpyTagGroup& G : Groups)
		for (const FSpyTagEntry& E : G.Tags)
			if (E.VarName == VarName) return true;
	return false;
}

bool FSpyTagFileEditor::AppendTags(
    const TArray<FSpyTagGroup>& ParsedGroups,
    const FSpyTagGroup& TargetGroup,
    const TArray<FSpyTagEntry>& NewEntries,
    bool bIsNewGroup)
{
    if (NewEntries.IsEmpty()) return true;

    // ── h 파일 수정 ──────────────────────────────────────────────
    TArray<FString> HLines;
    if (!FFileHelper::LoadFileToStringArray(HLines, *GetHeaderPath()))
        return false;

    if (bIsNewGroup)
    {
        // namespace 닫기 } 직전에 삽입
        int32 CloseIdx = INDEX_NONE;
        for (int32 i = HLines.Num() - 1; i >= 0; --i)
        {
            if (HLines[i].TrimStartAndEnd() == TEXT("}"))
            { CloseIdx = i; break; }
        }
        if (CloseIdx == INDEX_NONE) return false;

        int32 Pos = CloseIdx;
        HLines.Insert(TEXT(""), Pos++);
        HLines.Insert(FString::Printf(TEXT("\t//# %s"), *TargetGroup.Comment), Pos++);
        for (const FSpyTagEntry& E : NewEntries)
            HLines.Insert(
                FString::Printf(TEXT("\tSKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(%s);"), *E.VarName),
                Pos++);
    }
    else
    {
        // 대상 그룹 헤더 줄 찾기
        int32 HeaderIdx = INDEX_NONE;
        for (int32 i = 0; i < HLines.Num(); ++i)
        {
            FString T = HLines[i].TrimStartAndEnd();
            if (T.StartsWith(TEXT("//#")) && T.RightChop(3).TrimStartAndEnd() == TargetGroup.Comment)
            { HeaderIdx = i; break; }
        }
        if (HeaderIdx == INDEX_NONE) return false;

        // 그룹 범위 내 마지막 UE_DECLARE 줄
        int32 LastTag = HeaderIdx;
        for (int32 i = HeaderIdx + 1; i < HLines.Num(); ++i)
        {
            FString T = HLines[i].TrimStartAndEnd();
            if (T.StartsWith(TEXT("//#")) || T == TEXT("}")) break;
            if (T.Contains(TEXT("UE_DECLARE_GAMEPLAY_TAG_EXTERN("))) LastTag = i;
        }

        int32 Pos = LastTag + 1;
        for (const FSpyTagEntry& E : NewEntries)
            HLines.Insert(
                FString::Printf(TEXT("\tSKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(%s);"), *E.VarName),
                Pos++);
    }

    FString HContent = FString::Join(HLines, TEXT("\n")) + TEXT("\n");
    if (!FFileHelper::SaveStringToFile(HContent, *GetHeaderPath()))
        return false;

    // ── cpp 파일 수정 ─────────────────────────────────────────────
    TArray<FString> CppLines;
    if (!FFileHelper::LoadFileToStringArray(CppLines, *GetCppPath()))
        return false;

    if (bIsNewGroup)
    {
        // 마지막 UE_DEFINE_GAMEPLAY_TAG 줄 뒤에 삽입
        int32 LastDef = INDEX_NONE;
        for (int32 i = 0; i < CppLines.Num(); ++i)
            if (CppLines[i].TrimStartAndEnd().StartsWith(TEXT("UE_DEFINE_GAMEPLAY_TAG(")))
                LastDef = i;
        if (LastDef == INDEX_NONE) return false;

        int32 Pos = LastDef + 1;
        CppLines.Insert(TEXT(""), Pos++);
        CppLines.Insert(FString::Printf(TEXT("\t//# %s"), *TargetGroup.Comment), Pos++);
        for (const FSpyTagEntry& E : NewEntries)
            CppLines.Insert(
                FString::Printf(TEXT("\tUE_DEFINE_GAMEPLAY_TAG(%s, \"%s\");"), *E.VarName, *E.TagString),
                Pos++);
    }
    else
    {
        // 대상 그룹의 마지막 VarName을 cpp에서 찾기
        FString LastVar = TargetGroup.Tags.IsEmpty() ? TEXT("") : TargetGroup.Tags.Last().VarName;
        int32 InsertIdx = INDEX_NONE;
        if (!LastVar.IsEmpty())
            for (int32 i = 0; i < CppLines.Num(); ++i)
                if (CppLines[i].Contains(TEXT("UE_DEFINE_GAMEPLAY_TAG(")) &&
                    CppLines[i].Contains(LastVar))
                    InsertIdx = i;

        // Fallback: 마지막 UE_DEFINE 줄
        if (InsertIdx == INDEX_NONE)
            for (int32 i = 0; i < CppLines.Num(); ++i)
                if (CppLines[i].TrimStartAndEnd().StartsWith(TEXT("UE_DEFINE_GAMEPLAY_TAG(")))
                    InsertIdx = i;

        if (InsertIdx == INDEX_NONE) return false;

        int32 Pos = InsertIdx + 1;
        for (const FSpyTagEntry& E : NewEntries)
            CppLines.Insert(
                FString::Printf(TEXT("\tUE_DEFINE_GAMEPLAY_TAG(%s, \"%s\");"), *E.VarName, *E.TagString),
                Pos++);
    }

    FString CppContent = FString::Join(CppLines, TEXT("\n")) + TEXT("\n");
    return FFileHelper::SaveStringToFile(CppContent, *GetCppPath());
}
