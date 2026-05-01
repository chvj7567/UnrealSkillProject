#pragma once
#include "CoreMinimal.h"

struct FSpyTagEntry
{
	FString VarName;    // "Skill_Action_A"
	FString TagString;  // "Skill.Action.A"
};

struct FSpyTagGroup
{
	FString Comment;            // "액션 스킬"
	TArray<FSpyTagEntry> Tags;
};

class FSpyTagFileEditor
{
public:
	static FString GetHeaderPath();
	static FString GetCppPath();

	// h에서 그룹+VarName, cpp에서 TagString을 파싱해 합친 결과 반환
	static TArray<FSpyTagGroup> ParseFiles();

	// "Skill.Action.G" -> "Skill_Action_G"
	static FString TagStringToVarName(const FString& TagString);

	// 파싱된 Groups에 VarName이 존재하면 true
	static bool DoesVarNameExist(const TArray<FSpyTagGroup>& Groups, const FString& VarName);

	// h/cpp 파일에 NewEntries를 추가. bIsNewGroup=true면 새 그룹 생성
	static bool AppendTags(
		const TArray<FSpyTagGroup>& ParsedGroups,
		const FSpyTagGroup& TargetGroup,
		const TArray<FSpyTagEntry>& NewEntries,
		bool bIsNewGroup);

private:
	static TArray<FSpyTagGroup> ParseHeaderGroups();
	static TMap<FString, FString> ParseCppTagMap();
};
