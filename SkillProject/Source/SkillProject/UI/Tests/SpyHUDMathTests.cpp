#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UI/SpyHUDMath.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyHUDMathHeadingTest,
	"SkillProject.HUD.Math.HeadingToCardinal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyHUDMathHeadingTest::RunTest(const FString& Parameters)
{
	//# enum 비교는 int32 캐스팅으로 TestEqual 의 정수 오버로드를 사용한다
	TestEqual(TEXT("0 -> N"), static_cast<int32>(SpyHUDMath::HeadingToCardinal(0.f)), static_cast<int32>(ESpyCardinal::N));
	TestEqual(TEXT("360 wraps to N"), static_cast<int32>(SpyHUDMath::HeadingToCardinal(360.f)), static_cast<int32>(ESpyCardinal::N));
	TestEqual(TEXT("-90 wraps to W"), static_cast<int32>(SpyHUDMath::HeadingToCardinal(-90.f)), static_cast<int32>(ESpyCardinal::W));
	TestEqual(TEXT("90 -> E"), static_cast<int32>(SpyHUDMath::HeadingToCardinal(90.f)), static_cast<int32>(ESpyCardinal::E));
	TestEqual(TEXT("180 -> S"), static_cast<int32>(SpyHUDMath::HeadingToCardinal(180.f)), static_cast<int32>(ESpyCardinal::S));
	TestEqual(TEXT("45 -> NE"), static_cast<int32>(SpyHUDMath::HeadingToCardinal(45.f)), static_cast<int32>(ESpyCardinal::NE));
	TestEqual(TEXT("22.4 still N"), static_cast<int32>(SpyHUDMath::HeadingToCardinal(22.4f)), static_cast<int32>(ESpyCardinal::N));
	TestEqual(TEXT("22.6 tips to NE"), static_cast<int32>(SpyHUDMath::HeadingToCardinal(22.6f)), static_cast<int32>(ESpyCardinal::NE));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyHUDMathCooldownTest,
	"SkillProject.HUD.Math.CooldownNormalized",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyHUDMathCooldownTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("zero duration -> 0"), SpyHUDMath::CooldownNormalized(5.f, 0.f), 0.f);
	TestEqual(TEXT("negative duration -> 0"), SpyHUDMath::CooldownNormalized(5.f, -3.f), 0.f);
	TestEqual(TEXT("full remaining -> 1"), SpyHUDMath::CooldownNormalized(4.f, 4.f), 1.f);
	TestEqual(TEXT("half -> 0.5"), SpyHUDMath::CooldownNormalized(2.f, 4.f), 0.5f);
	TestEqual(TEXT("remaining>duration clamps 1"), SpyHUDMath::CooldownNormalized(9.f, 4.f), 1.f);
	TestEqual(TEXT("negative remaining clamps 0"), SpyHUDMath::CooldownNormalized(-1.f, 4.f), 0.f);

	return true;
}

//# ---- 이하 test-engineer 확장: 8방위 전수 + 경계 회귀 + 랩 망라, 쿨다운 티어/엣지 ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyHUDMathHeadingCentersTest,
	"SkillProject.HUD.Math.HeadingCenters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyHUDMathHeadingCentersTest::RunTest(const FString& Parameters)
{
	//# 8방위 각 중심(0/45/…/315) 전수 — 정상 케이스 망라
	auto CheckDir = [this](float Yaw, ESpyCardinal Expected, const TCHAR* Label)
	{
		TestEqual(Label, static_cast<int32>(SpyHUDMath::HeadingToCardinal(Yaw)), static_cast<int32>(Expected));
	};

	CheckDir(0.f, ESpyCardinal::N, TEXT("center 0 -> N"));
	CheckDir(45.f, ESpyCardinal::NE, TEXT("center 45 -> NE"));
	CheckDir(90.f, ESpyCardinal::E, TEXT("center 90 -> E"));
	CheckDir(135.f, ESpyCardinal::SE, TEXT("center 135 -> SE"));
	CheckDir(180.f, ESpyCardinal::S, TEXT("center 180 -> S"));
	CheckDir(225.f, ESpyCardinal::SW, TEXT("center 225 -> SW"));
	CheckDir(270.f, ESpyCardinal::W, TEXT("center 270 -> W"));
	CheckDir(315.f, ESpyCardinal::NW, TEXT("center 315 -> NW"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyHUDMathHeadingBoundaryTest,
	"SkillProject.HUD.Math.HeadingBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyHUDMathHeadingBoundaryTest::RunTest(const FString& Parameters)
{
	//# 각 섹터 경계(±22.5 지점)의 tie-break 를 회귀로 고정한다.
	//# 구현은 half-open-forward — 정확히 X.5 는 다음(시계방향) 방위로 넘어간다.
	//# 이 tie-break 방향은 기획서 §5 가 명시하지 않은 현 구현 계약이다.
	//# 뒤집으려면 의도적 변경이어야 하며, 이 테스트가 그 사실을 알린다.
	auto CheckDir = [this](float Yaw, ESpyCardinal Expected, const TCHAR* Label)
	{
		TestEqual(Label, static_cast<int32>(SpyHUDMath::HeadingToCardinal(Yaw)), static_cast<int32>(Expected));
	};

	//# N/NE 경계 22.5
	CheckDir(22.4f, ESpyCardinal::N, TEXT("22.4 still N"));
	CheckDir(22.5f, ESpyCardinal::NE, TEXT("22.5 tips to NE (forward)"));
	CheckDir(22.6f, ESpyCardinal::NE, TEXT("22.6 NE"));

	//# NE/E 경계 67.5
	CheckDir(67.4f, ESpyCardinal::NE, TEXT("67.4 still NE"));
	CheckDir(67.5f, ESpyCardinal::E, TEXT("67.5 tips to E"));

	//# E/SE 경계 112.5
	CheckDir(112.4f, ESpyCardinal::E, TEXT("112.4 still E"));
	CheckDir(112.5f, ESpyCardinal::SE, TEXT("112.5 tips to SE"));

	//# 나머지 섹터 정확 경계 — 전부 다음 방위로
	CheckDir(157.5f, ESpyCardinal::S, TEXT("157.5 tips to S"));
	CheckDir(202.5f, ESpyCardinal::SW, TEXT("202.5 tips to SW"));
	CheckDir(247.5f, ESpyCardinal::W, TEXT("247.5 tips to W"));
	CheckDir(292.5f, ESpyCardinal::NW, TEXT("292.5 tips to NW"));

	//# NW/N 경계 337.5 — 유일하게 섹터 인덱스 8 이 %8 로 0(N)으로 랩되는 지점
	CheckDir(337.4f, ESpyCardinal::NW, TEXT("337.4 still NW"));
	CheckDir(337.5f, ESpyCardinal::N, TEXT("337.5 wraps to N"));
	CheckDir(337.6f, ESpyCardinal::N, TEXT("337.6 N"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyHUDMathHeadingWrapTest,
	"SkillProject.HUD.Math.HeadingWrap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyHUDMathHeadingWrapTest::RunTest(const FString& Parameters)
{
	//# [0,360) 밖의 입력(양/음의 랩, 큰 절댓값)이 올바르게 정규화되는지
	auto CheckDir = [this](float Yaw, ESpyCardinal Expected, const TCHAR* Label)
	{
		TestEqual(Label, static_cast<int32>(SpyHUDMath::HeadingToCardinal(Yaw)), static_cast<int32>(Expected));
	};

	//# 양의 랩
	CheckDir(360.f, ESpyCardinal::N, TEXT("360 -> N"));
	CheckDir(405.f, ESpyCardinal::NE, TEXT("405 (=45) -> NE"));
	CheckDir(450.f, ESpyCardinal::E, TEXT("450 (=90) -> E"));
	CheckDir(720.f, ESpyCardinal::N, TEXT("720 -> N"));
	CheckDir(3600.f, ESpyCardinal::N, TEXT("3600 large positive -> N"));

	//# 음의 랩
	CheckDir(-45.f, ESpyCardinal::NW, TEXT("-45 (=315) -> NW"));
	CheckDir(-90.f, ESpyCardinal::W, TEXT("-90 (=270) -> W"));
	CheckDir(-720.f, ESpyCardinal::N, TEXT("-720 -> N"));
	CheckDir(-810.f, ESpyCardinal::W, TEXT("-810 (=270) -> W"));
	CheckDir(-1000.f, ESpyCardinal::E, TEXT("-1000 (=80) -> E"));
	CheckDir(-3600.f, ESpyCardinal::N, TEXT("-3600 large negative -> N"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyHUDMathCooldownEdgeTest,
	"SkillProject.HUD.Math.CooldownEdge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyHUDMathCooldownEdgeTest::RunTest(const FString& Parameters)
{
	//# 경계·이상 입력 — Duration<=0 우선, Remaining 클램프 계약을 망라
	TestEqual(TEXT("both zero -> 0"), SpyHUDMath::CooldownNormalized(0.f, 0.f), 0.f);
	TestEqual(TEXT("remaining 0 with positive duration -> 0"), SpyHUDMath::CooldownNormalized(0.f, 5.f), 0.f);
	TestEqual(TEXT("remaining == duration -> 1 (cast moment)"), SpyHUDMath::CooldownNormalized(2.f, 2.f), 1.f);
	TestEqual(TEXT("remaining >> duration clamps 1"), SpyHUDMath::CooldownNormalized(10.f, 2.f), 1.f);
	TestEqual(TEXT("negative remaining clamps 0"), SpyHUDMath::CooldownNormalized(-5.f, 10.f), 0.f);
	TestEqual(TEXT("negative duration -> 0 (before division)"), SpyHUDMath::CooldownNormalized(5.f, -1.f), 0.f);
	//# 아주 작은 잔여 — 준비 직전
	TestEqual(TEXT("tiny remaining -> ~0.01"), SpyHUDMath::CooldownNormalized(0.1f, 10.f), 0.01f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyHUDMathCooldownTierTest,
	"SkillProject.HUD.Math.CooldownTier",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyHUDMathCooldownTierTest::RunTest(const FString& Parameters)
{
	//# 기획서 §6-2 쿨다운 초안(T1=2/T2=6/T3=10초)에서 스윕 진행값 검증.
	//# 1=방금발동, 0=준비 계약(SpyHUDMath.h) 을 각 티어에서 확인.
	//# 발동 순간
	TestEqual(TEXT("T1 cast -> 1"), SpyHUDMath::CooldownNormalized(2.f, 2.f), 1.f);
	TestEqual(TEXT("T2 cast -> 1"), SpyHUDMath::CooldownNormalized(6.f, 6.f), 1.f);
	TestEqual(TEXT("T3 cast -> 1"), SpyHUDMath::CooldownNormalized(10.f, 10.f), 1.f);
	//# 절반 진행
	TestEqual(TEXT("T1 half -> 0.5"), SpyHUDMath::CooldownNormalized(1.f, 2.f), 0.5f);
	TestEqual(TEXT("T2 half -> 0.5"), SpyHUDMath::CooldownNormalized(3.f, 6.f), 0.5f);
	TestEqual(TEXT("T3 half -> 0.5"), SpyHUDMath::CooldownNormalized(5.f, 10.f), 0.5f);
	//# 4분의 1 진행(스윕 상단 근처)
	TestEqual(TEXT("T1 quarter -> 0.25"), SpyHUDMath::CooldownNormalized(0.5f, 2.f), 0.25f);
	//# 준비 임박
	TestEqual(TEXT("T3 near ready -> 0.02"), SpyHUDMath::CooldownNormalized(0.2f, 10.f), 0.02f);

	return true;
}

#endif
