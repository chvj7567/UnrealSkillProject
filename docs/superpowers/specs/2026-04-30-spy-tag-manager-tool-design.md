# Spy Tag Manager Tool — Design Spec
Date: 2026-04-30

## 개요

`SpyGameplayTags.h` / `.cpp` 파일을 파싱해 기존 태그를 트리로 시각화하고, 새 태그를 UI에서 직접 추가(소스 파일 자동 수정)하는 에디터 전용 모듈.  
Spy Tools 상단 탭 아래 "Spy Tag Manager" 항목으로 등록. 기존 `SpyDataEditorTool`, `SpyGACreatorTool`과 완전히 독립된 별도 모듈.

---

## 1. 모듈 구조

### 신규 모듈: `SpyTagManagerTool` (Type: Editor)

```
SkillProject/Source/SpyTagManagerTool/
├── SpyTagManagerTool.Build.cs
├── Public/
│   ├── SpyTagManagerTool.h          (IModuleInterface 구현)
│   └── SSpyTagManagerDialog.h       (탭 콘텐츠 SCompoundWidget)
└── Private/
    ├── SpyTagManagerTool.cpp
    ├── SSpyTagManagerDialog.cpp
    └── SpyTagFileEditor.h/.cpp      (파싱·쓰기 전담 클래스)
```

### Build.cs 의존 모듈

```
PublicDependencyModuleNames:
  "Core", "CoreUObject", "Engine"

PrivateDependencyModuleNames:
  "Slate", "SlateCore", "InputCore",
  "UnrealEd", "ToolMenus", "EditorFramework"
```

### 등록 파일 수정

| 파일 | 변경 |
|------|------|
| `SkillProject.uproject` | `SpyTagManagerTool` 모듈 추가 (Type: Editor, LoadingPhase: PostEngineInit) |
| `SpyDataEditorTool.cpp`, `SpyGACreatorTool.cpp` | 변경 없음 — Spy Tools 메뉴는 각 모듈이 독립적으로 extend |

대상 소스 파일 (하드코딩 경로):
```
SkillProject/Source/SkillProject/Util/SpyGameplayTags.h
SkillProject/Source/SkillProject/Util/SpyGameplayTags.cpp
```

---

## 2. 데이터 모델 (`SpyTagFileEditor`)

### 구조체

```cpp
struct FSpyTagEntry
{
    FString VarName;    // "Skill_Action_A"
    FString TagString;  // "Skill.Action.A"
};

struct FSpyTagGroup
{
    FString Comment;            // "액션 스킬"  (//# 뒤 텍스트)
    TArray<FSpyTagEntry> Tags;
};
```

### 트리 노드

```cpp
struct FTagTreeNode
{
    FString Segment;                          // "Action"
    TArray<TSharedPtr<FTagTreeNode>> Children;
    bool bIsGroupHeader = false;
    FString GroupComment;                     // 그룹 헤더 노드 전용
};
```

---

## 3. 파일 파싱 (`SpyTagFileEditor::ParseCppFile`)

cpp 파일을 줄 단위로 읽어 아래 패턴 인식:

| 패턴 | 처리 |
|------|------|
| `//# 그룹명` | 새 `FSpyTagGroup` 시작, Comment = "그룹명" |
| `UE_DEFINE_GAMEPLAY_TAG(VarName, "a.b.c")` | 현재 그룹에 `FSpyTagEntry` 추가 |
| 그 외 줄 | 무시 |

파싱 결과: `TArray<FSpyTagGroup> Groups`.  
파싱 실패(파일 없음 등) 시 빈 배열 반환 후 UI에서 오류 표시.

---

## 4. UI 레이아웃 (`SSpyTagManagerDialog`)

좌우 `SSplitter` 분할:

### 4-1. 왼쪽 패널 — 태그 트리

`STreeView<TSharedPtr<FTagTreeNode>>` 구성.

트리 노드 두 종류:
- **그룹 헤더 노드** — `bIsGroupHeader = true`, 굵은 글씨 + 연한 배경, 선택 불가
- **태그 경로 노드** — 선택 가능, 클릭 시 오른쪽 "부모 경로"에 dot-notation 자동 입력

표시 예시:
```
━━ # 락 태그 ━━━━━━━━━━━━
  ▶ Lock
      ▶ Input
          All, Move, Look

━━ # 액션 스킬 ━━━━━━━━━━
  ▶ Skill
      ▶ Action
          A, B, C, D, E, F
```

트리는 모듈 로드 시 1회 파싱. "새로고침" 버튼으로 재파싱 가능.

### 4-2. 오른쪽 패널 — 태그 추가

| 행 | 위젯 | 동작 |
|----|------|------|
| 그룹 선택 | `SComboBox<TSharedPtr<FSpyTagGroup>>` | 기존 그룹 + "새 그룹 추가..." |
| 그룹 주석 | `SEditableTextBox` | 기존 그룹: 읽기 전용 / 새 그룹: 편집 가능 |
| 부모 경로 | `SEditableTextBox` | 트리 클릭 시 자동 입력, 직접 수정 가능 |
| 리프 입력 목록 | `SListView` + `SEditableTextBox` 행 | "+" 버튼으로 행 추가 |
| 미리보기 | `STextBlock` 목록 | `Skill.Action.G` → `Skill_Action_G` 실시간 표시 |
| Add Tags 버튼 | `SButton` | 유효성 검사 → 파일 쓰기 |

---

## 5. 파일 쓰기 (`SpyTagFileEditor::AppendTags`)

입력: `FSpyTagGroup& TargetGroup`, `TArray<FSpyTagEntry> NewEntries`

### h 파일 수정

대상 그룹의 마지막 `UE_DECLARE_GAMEPLAY_TAG_EXTERN` 줄 뒤에 삽입:
```cpp
SKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Action_G);
```

새 그룹인 경우 namespace 닫기 `}` 직전에 삽입:
```cpp

	//# 새그룹명
	SKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(NewTag);
```

### cpp 파일 수정

대상 그룹의 마지막 `UE_DEFINE_GAMEPLAY_TAG` 줄 뒤에 삽입:
```cpp
UE_DEFINE_GAMEPLAY_TAG(Skill_Action_G, "Skill.Action.G");
```

새 그룹인 경우 `MovementModeTagMap` 선언 직전(또는 namespace 닫기 `}` 직전)에 삽입:
```cpp

	//# 새그룹명
	UE_DEFINE_GAMEPLAY_TAG(NewTag, "New.Tag");
```

### 변수명 자동 변환 규칙

`dot.notation.string` → `.`을 `_`로 치환 → `dot_notation_string`

### 오류 처리

| 상황 | 처리 |
|------|------|
| 부모 경로 비어있음 | Add Tags 버튼 비활성화 |
| 리프 이름 비어있음 | 해당 행 무시 (나머지 행은 추가) |
| 동일 변수명 이미 존재 | 오류 다이얼로그 후 전체 취소 |
| 파일 쓰기 실패 | 오류 다이얼로그, 파일 상태 원복 시도 없음 |

---

## 6. 변수명 충돌 검사

`SpyTagFileEditor::DoesVarNameExist(FString VarName)` — 파싱된 모든 그룹의 VarName과 대조.  
Add Tags 클릭 시 새 항목 전체를 일괄 검사 → 하나라도 충돌 시 전체 취소.

---

## 7. 범위 밖 (이번 구현에서 제외)

- 태그 삭제
- 기존 그룹 주석 편집
- SKGameplayTags.h 지원 (SKGAS 모듈 태그)
- 태그 참조 검색 (다른 파일에서 쓰이는지 확인)
- 자동 빌드 트리거
