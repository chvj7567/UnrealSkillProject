#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UI/SpyDialogueWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyDialogueContinueHintMappedKeyTest,
	"SkillProject.HUD.Dialogue.ContinueHint.MappedKey",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyDialogueContinueHintMappedKeyTest::RunTest(const FString& Parameters)
{
	//# 정상 케이스 — 매핑된 키가 있으면 그 키의 표시 이름으로 힌트를 구성한다. 전체 문자열을 완전 비교한다
	const TArray<FKey> MappedKeys = {EKeys::E};
	const FText Hint = USpyDialogueWidget::BuildContinueHintText(MappedKeys);

	TestTrue(TEXT("Hint uses mapped key display name"), Hint.ToString().Equals(TEXT("E 계속")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyDialogueContinueHintFallbackTest,
	"SkillProject.HUD.Dialogue.ContinueHint.FallbackOnNoMapping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyDialogueContinueHintFallbackTest::RunTest(const FString& Parameters)
{
	//# 엣지 케이스 — 매핑을 찾지 못하면(빈 배열) 키 이름 없는 중립 폴백 텍스트로 떨어진다
	//# (실제 바인딩과 무관하게 특정 키를 단정하지 않는다)
	const FText Hint = USpyDialogueWidget::BuildContinueHintText(TArray<FKey>());

	TestTrue(TEXT("Falls back to neutral hint text"), Hint.ToString().Equals(TEXT("계속")));

	return true;
}

#endif
