// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "SKSessionOpRules.h"

//# 유휴 상태에서는 어떤 작업이든 시작할 수 있다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSKSessionOpIdleAllowsAnyTest,
	"SKOnline.Session.OpRules.IdleAllowsAny",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSKSessionOpIdleAllowsAnyTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Idle allows hosting"), USKSessionOpRules::CanStartOp(ESKSessionOp::None, ESKSessionOp::Hosting));
	TestTrue(TEXT("Idle allows finding"), USKSessionOpRules::CanStartOp(ESKSessionOp::None, ESKSessionOp::Finding));
	TestTrue(TEXT("Idle allows joining"), USKSessionOpRules::CanStartOp(ESKSessionOp::None, ESKSessionOp::Joining));
	TestTrue(TEXT("Idle allows destroying"), USKSessionOpRules::CanStartOp(ESKSessionOp::None, ESKSessionOp::Destroying));

	return true;
}

//# 작업이 진행 중이면 새 작업을 막는다 — 더블클릭·연타 가드
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSKSessionOpBusyBlocksTest,
	"SKOnline.Session.OpRules.BusyBlocks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSKSessionOpBusyBlocksTest::RunTest(const FString& Parameters)
{
	//# 같은 작업 연타
	TestFalse(TEXT("Finding blocks finding"), USKSessionOpRules::CanStartOp(ESKSessionOp::Finding, ESKSessionOp::Finding));
	TestFalse(TEXT("Joining blocks joining"), USKSessionOpRules::CanStartOp(ESKSessionOp::Joining, ESKSessionOp::Joining));

	//# 다른 작업 끼어들기
	TestFalse(TEXT("Finding blocks hosting"), USKSessionOpRules::CanStartOp(ESKSessionOp::Finding, ESKSessionOp::Hosting));
	TestFalse(TEXT("Hosting blocks joining"), USKSessionOpRules::CanStartOp(ESKSessionOp::Hosting, ESKSessionOp::Joining));
	TestFalse(TEXT("Joining blocks finding"), USKSessionOpRules::CanStartOp(ESKSessionOp::Joining, ESKSessionOp::Finding));
	TestFalse(TEXT("Destroying blocks hosting"), USKSessionOpRules::CanStartOp(ESKSessionOp::Destroying, ESKSessionOp::Hosting));

	return true;
}

//# None 을 요청하는 것은 의미가 없다 — 항상 거부해 상태를 우회로 비우지 못하게 한다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSKSessionOpRequestNoneRejectedTest,
	"SKOnline.Session.OpRules.RequestNoneRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSKSessionOpRequestNoneRejectedTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("Requesting None from idle is rejected"), USKSessionOpRules::CanStartOp(ESKSessionOp::None, ESKSessionOp::None));
	TestFalse(TEXT("Requesting None while busy is rejected"), USKSessionOpRules::CanStartOp(ESKSessionOp::Finding, ESKSessionOp::None));

	return true;
}

//# 방 만들기는 진행 중인 검색만 선점한다 — 사용자가 누른 쪽이 이긴다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSKSessionOpPreemptFindForHostTest,
	"SKOnline.Session.OpRules.PreemptFindForHost",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSKSessionOpPreemptFindForHostTest::RunTest(const FString& Parameters)
{
	//# 검색만 취소 대상 — 취소해도 남는 세션 상태가 없다
	TestTrue(TEXT("Finding is preempted"), USKSessionOpRules::ShouldPreemptFindForHost(ESKSessionOp::Finding));

	//# 나머지는 기존 Busy 거부를 유지한다(중간에 끊으면 세션 상태가 꼬인다)
	TestFalse(TEXT("Joining is not preempted"), USKSessionOpRules::ShouldPreemptFindForHost(ESKSessionOp::Joining));
	TestFalse(TEXT("Destroying is not preempted"), USKSessionOpRules::ShouldPreemptFindForHost(ESKSessionOp::Destroying));
	TestFalse(TEXT("Hosting is not preempted"), USKSessionOpRules::ShouldPreemptFindForHost(ESKSessionOp::Hosting));

	//# 유휴 상태는 선점할 대상이 없다 — 평범한 시작 경로로 가야 한다
	TestFalse(TEXT("Idle needs no preemption"), USKSessionOpRules::ShouldPreemptFindForHost(ESKSessionOp::None));

	return true;
}

#endif
