// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UI/SpySessionBrowserWidget.h"
#include "UI/SpySessionRowWidget.h"

//# 행 표시 문자열
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpySessionRowTextTest,
	"SkillProject.UI.SessionBrowser.RowText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpySessionRowTextTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Players text"), USpySessionRowWidget::MakePlayersText(2, 4), FString(TEXT("2 / 4")));
	TestEqual(TEXT("Players text clamps negatives"), USpySessionRowWidget::MakePlayersText(-1, -4), FString(TEXT("0 / 0")));

	TestEqual(TEXT("Ping text"), USpySessionRowWidget::MakePingText(30), FString(TEXT("30 ms")));
	TestEqual(TEXT("Unmeasured ping"), USpySessionRowWidget::MakePingText(0), FString(TEXT("-- ms")));
	TestEqual(TEXT("Negative ping"), USpySessionRowWidget::MakePingText(-5), FString(TEXT("-- ms")));

	return true;
}

//# 에러 사유 → 사용자 문구. OSS 원문을 그대로 노출하지 않는다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpySessionStatusMessageTest,
	"SkillProject.UI.SessionBrowser.StatusMessage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpySessionStatusMessageTest::RunTest(const FString& Parameters)
{
	//# 온라인 기능 자체가 없는 경우
	TestEqual(TEXT("No online subsystem"),
		USpySessionBrowserWidget::MakeStatusMessage(ESKSessionOp::Finding, ESKSessionError::NoOnlineSubsystem),
		FString(TEXT("온라인 기능을 사용할 수 없습니다")));

	//# 작업별 문구가 구분된다
	TestEqual(TEXT("Create failed"),
		USpySessionBrowserWidget::MakeStatusMessage(ESKSessionOp::Hosting, ESKSessionError::CreateFailed),
		FString(TEXT("방을 만들지 못했습니다")));

	TestEqual(TEXT("Find failed"),
		USpySessionBrowserWidget::MakeStatusMessage(ESKSessionOp::Finding, ESKSessionError::FindFailed),
		FString(TEXT("방 목록을 불러오지 못했습니다")));

	TestEqual(TEXT("Join failed"),
		USpySessionBrowserWidget::MakeStatusMessage(ESKSessionOp::Joining, ESKSessionError::JoinFailed),
		FString(TEXT("방에 들어가지 못했습니다")));

	//# 중복 입력은 사용자에게 에러로 보이면 안 된다 — 빈 문자열이면 표시하지 않는다
	TestEqual(TEXT("Busy is silent"),
		USpySessionBrowserWidget::MakeStatusMessage(ESKSessionOp::Finding, ESKSessionError::Busy),
		FString(TEXT("")));

	return true;
}

//# 진행 문구는 "지금 진행 중인 op" 를 말한다 (기획서 §5-2-1 결정표)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpySessionProgressMessageTest,
	"SkillProject.UI.SessionBrowser.ProgressMessage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpySessionProgressMessageTest::RunTest(const FString& Parameters)
{
	//# 유휴 상태 — 이번에 요청하는 op 의 문구
	TestEqual(TEXT("Idle uses requested op"),
		USpySessionBrowserWidget::MakeProgressMessage(ESKSessionOp::None, ESKSessionOp::Finding),
		FString(TEXT("방 목록을 불러오는 중입니다")));

	TestEqual(TEXT("Idle hosting"),
		USpySessionBrowserWidget::MakeProgressMessage(ESKSessionOp::None, ESKSessionOp::Hosting),
		FString(TEXT("방을 만드는 중입니다")));

	TestEqual(TEXT("Idle joining"),
		USpySessionBrowserWidget::MakeProgressMessage(ESKSessionOp::None, ESKSessionOp::Joining),
		FString(TEXT("방에 들어가는 중입니다")));

	//# 교차 op — 진행 중인 작업의 문구가 이긴다(요청은 Busy 로 거부된다)
	TestEqual(TEXT("Busy hosting wins over requested find"),
		USpySessionBrowserWidget::MakeProgressMessage(ESKSessionOp::Hosting, ESKSessionOp::Finding),
		FString(TEXT("방을 만드는 중입니다")));

	//# 선점 — 검색 중 방 만들기는 검색을 취소하고 진행되므로 요청한 쪽이 사실이다
	TestEqual(TEXT("Hosting preempts running find"),
		USpySessionBrowserWidget::MakeProgressMessage(ESKSessionOp::Finding, ESKSessionOp::Hosting),
		FString(TEXT("방을 만드는 중입니다")));

	//# 조인은 선점하지 않는다 — 검색이 끝날 때까지 Busy 로 거부되므로 검색 문구를 유지한다
	TestEqual(TEXT("Joining does not preempt find"),
		USpySessionBrowserWidget::MakeProgressMessage(ESKSessionOp::Finding, ESKSessionOp::Joining),
		FString(TEXT("방 목록을 불러오는 중입니다")));

	//# 접속 실패 복구 직후 — 파괴가 진행 중이면 그 사실을 말한다
	TestEqual(TEXT("Destroying wins over requested find"),
		USpySessionBrowserWidget::MakeProgressMessage(ESKSessionOp::Destroying, ESKSessionOp::Finding),
		FString(TEXT("이전 방을 정리하는 중입니다")));

	return true;
}

#endif
