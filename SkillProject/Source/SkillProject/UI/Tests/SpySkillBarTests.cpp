#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Data/SpySkillBarConfig.h"
#include "UI/SpySkillBarWidget.h"
#include "Util/SpyGameplayTags.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpySkillBarSlotOrderTest,
	"SkillProject.HUD.SkillBar.SlotOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpySkillBarSlotOrderTest::RunTest(const FString& Parameters)
{
	const TArray<FGameplayTag> Tags = USpySkillBarWidget::BuildSlotInputTags();

	TestEqual(TEXT("6 slots"), Tags.Num(), 6);
	TestTrue(TEXT("first is Skill_1"), Tags[0] == SpyGameplayTags::Input_Ability_Skill_1);
	TestTrue(TEXT("last is Skill_6"), Tags[5] == SpyGameplayTags::Input_Ability_Skill_6);

	return true;
}

//# ---- 이하 test-engineer 확장: 전체 순서 회귀 + 중복/유효성 망라 ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpySkillBarExactOrderTest,
	"SkillProject.HUD.SkillBar.ExactOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpySkillBarExactOrderTest::RunTest(const FString& Parameters)
{
	const TArray<FGameplayTag> Tags = USpySkillBarWidget::BuildSlotInputTags();

	//# 6슬롯 전체 순서를 원소별로 회귀 고정 — Skill_1..6 (Parry 는 슬롯 대상 아님)
	if (TestEqual(TEXT("6 slots"), Tags.Num(), 6) == false)
	{
		//# 개수가 다르면 아래 인덱스 접근이 크래시하므로 조기 종료
		return false;
	}

	TestTrue(TEXT("[0] Skill_1"), Tags[0] == SpyGameplayTags::Input_Ability_Skill_1);
	TestTrue(TEXT("[1] Skill_2"), Tags[1] == SpyGameplayTags::Input_Ability_Skill_2);
	TestTrue(TEXT("[2] Skill_3"), Tags[2] == SpyGameplayTags::Input_Ability_Skill_3);
	TestTrue(TEXT("[3] Skill_4"), Tags[3] == SpyGameplayTags::Input_Ability_Skill_4);
	TestTrue(TEXT("[4] Skill_5"), Tags[4] == SpyGameplayTags::Input_Ability_Skill_5);
	TestTrue(TEXT("[5] Skill_6"), Tags[5] == SpyGameplayTags::Input_Ability_Skill_6);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpySkillBarUniquenessTest,
	"SkillProject.HUD.SkillBar.Uniqueness",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpySkillBarUniquenessTest::RunTest(const FString& Parameters)
{
	const TArray<FGameplayTag> Tags = USpySkillBarWidget::BuildSlotInputTags();

	//# 각 슬롯 태그가 유효한 게임플레이 태그인지 — 미등록/빈 태그가 섞이면 안 된다
	for (int32 Index = 0; Index < Tags.Num(); ++Index)
	{
		const FString Label = FString::Printf(TEXT("slot %d is a valid tag"), Index);
		TestTrue(*Label, Tags[Index].IsValid());
	}

	//# 중복 없음 — 같은 입력 태그가 두 슬롯에 배정되면 안 된다
	for (int32 I = 0; I < Tags.Num(); ++I)
	{
		for (int32 J = I + 1; J < Tags.Num(); ++J)
		{
			const FString Label = FString::Printf(TEXT("slot %d != slot %d"), I, J);
			TestTrue(*Label, Tags[I] != Tags[J]);
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpySkillBarConfigExtractTest,
	"SkillProject.HUD.SkillBar.ConfigExtract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpySkillBarConfigExtractTest::RunTest(const FString& Parameters)
{
	//# (a) null → 기본 6태그 폴백
	TestEqual(TEXT("null -> default 6"), USpySkillBarWidget::ExtractSlotInputTags(nullptr).Num(), 6);

	//# (b) 빈 config → 기본 6태그 폴백
	USpySkillBarConfig* Empty = NewObject<USpySkillBarConfig>();
	TestEqual(TEXT("empty -> default 6"), USpySkillBarWidget::ExtractSlotInputTags(Empty).Num(), 6);

	//# (c) 3슬롯 config → 그 3태그 순서대로
	USpySkillBarConfig* Cfg = NewObject<USpySkillBarConfig>();
	FSpySkillBarSlot S1;
	S1.InputTag = SpyGameplayTags::Input_Ability_Skill_3;
	FSpySkillBarSlot S2;
	S2.InputTag = SpyGameplayTags::Input_Ability_Skill_1;
	FSpySkillBarSlot S3;
	S3.InputTag = SpyGameplayTags::Input_Ability_Skill_2;
	Cfg->Slots = {S1, S2, S3};

	const TArray<FGameplayTag> Tags = USpySkillBarWidget::ExtractSlotInputTags(Cfg);
	TestEqual(TEXT("3 slots"), Tags.Num(), 3);
	TestTrue(TEXT("order[0]"), Tags[0] == SpyGameplayTags::Input_Ability_Skill_3);
	TestTrue(TEXT("order[1]"), Tags[1] == SpyGameplayTags::Input_Ability_Skill_1);
	TestTrue(TEXT("order[2]"), Tags[2] == SpyGameplayTags::Input_Ability_Skill_2);

	return true;
}

#endif
