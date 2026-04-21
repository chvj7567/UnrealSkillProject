# SpyDataEditorTool 설계 문서

**날짜:** 2026-04-14  
**브랜치:** tools/SpyDataAsset  
**대상 에셋:** `Content/Spy/Data/` 6개 + `Content/Spy/Data/Config/` 4개 uasset

---

## 개요

`Content/Spy/Data/`와 `Content/Spy/Data/Config/` 안의 총 10개 DataAsset을 한 곳에서 편집할 수 있는 Unreal Editor 전용 플러그인 탭을 구축한다.  
두 경로는 탭으로 구분해 세팅하며, Scan → 검토/편집 → Apply 흐름으로 데이터를 일괄 관리한다.

---

## 대상 에셋 목록

### Spy/Data/ (2개 탭으로 구성)

| 에셋 파일 | 클래스 | 역할 | 탭 |
|---|---|---|---|
| SpyAssetData.uasset | USpyAssetData (USKAssetData) | 에셋 이름 → 경로 룩업 테이블 | Assets |
| SpyAnimAssetData.uasset | USpyAnimAssetData | AnimLayer 맵 (FName → TSoftClassPtr) | Assets |
| SpyCharacterAssetData.uasset | USpyCharacterAssetData | 캐릭터 클래스별 컴포넌트/스킬/콤보/인풋 | Ability |
| SpyCommonCharacterAbility.uasset | USpyAbilityData | 공통 GAS 어빌리티 세트 | Ability |
| SpyNormalAbility.uasset | USpyAbilityData | 일반 어빌리티 GAS 세트 | Ability |
| SpyNormalComboAssetData.uasset | USpyComboAssetData | 콤보 체인 (StartTag → ComboTag) | Ability |

### Spy/Data/Config/ (1개 탭으로 구성)

| 에셋 파일 | 클래스 | 역할 | 탭 |
|---|---|---|---|
| SpyAIConfig.uasset | USpyAIConfig | AI 인식 범위 및 행동 설정 | Config |
| SpyCharacterConfig.uasset | USpyCharacterConfig | 캡슐/메시/카메라 등 캐릭터 물리 설정 | Config |
| SpyInputConfig.uasset | USpyInputConfig | Config 전용 입력 설정 | Config |
| SpyMovementConfig.uasset | USpyMovementConfig | 이동/클라이밍 관련 수치 | Config |

---

## 아키텍처

### 모듈 구조

```
SkillProject/Source/
└── SpyDataEditorTool/               (Editor 전용 모듈, Type=Editor)
    ├── SpyDataEditorTool.h/.cpp         메인 모듈 — 탭 등록, 메뉴 항목
    ├── Tabs/
    │   ├── SSpyAssetsTab.h/.cpp         [Assets] 탭 Slate UI  (Spy/Data/)
    │   ├── SSpyAbilityTab.h/.cpp        [Ability] 탭 Slate UI (Spy/Data/)
    │   └── SSpyConfigTab.h/.cpp         [Config] 탭 Slate UI  (Spy/Data/Config/)
    ├── Customizations/
    │   ├── SpyArrayCopyCustomization.h/.cpp             IDetailCustomization (Copy 버튼 — Ability 탭 전 에셋)
    │   ├── SpyAssetPathCustomization.h/.cpp            IPropertyTypeCustomization (경로 필터)
    │   └── SpyGameplayTagCustomization.h/.cpp          IPropertyTypeCustomization (태그 필터)
    └── Utils/
        ├── SpyDataScanner.h/.cpp        Content/Spy/ 스캔 + 프로퍼티 기록 유틸
        └── SpyEditorUtils.h             에셋 저장 + Diff 다이얼로그 공용 함수
```

### 탭 구성

| 탭 | 경로 | 포함 에셋 | Scan 자동화 |
|---|---|---|---|
| Assets | Spy/Data/ | SpyAssetData, SpyAnimAssetData | O — AssetRegistry로 Content/Spy/ 스캔 |
| Ability | Spy/Data/ | SpyCharacterAssetData, SpyCommonCharacterAbility, SpyNormalAbility, SpyNormalComboAssetData | 부분 — 에셋 목록 스캔, 태그/클래스는 수동 |
| Config | Spy/Data/Config/ | SpyAIConfig, SpyCharacterConfig, SpyInputConfig, SpyMovementConfig | X — 수동 설정 |

---

## UI 흐름

### 공통 패턴

```
[Scan]  ← Assets / Ability 탭에만 표시
─────────────────────────────────────
  IDetailsView (에셋 프로퍼티 편집)
  * Scan으로 바뀐 필드는 노란색 하이라이트
─────────────────────────────────────
[Apply]
```

탭 전환 버튼은 창 상단에 배치하며 `SWidgetSwitcher`로 콘텐츠를 교체한다.

### [Assets] 탭 (Spy/Data/)

- Scan: `AssetRegistry`로 `Content/Spy/` 하위 에셋 경로 수집
- SpyAssetData의 `AssetGroupNameToSet` 자동 채움
- SpyAnimAssetData의 `AnimLayerMap` 자동 채움
- Apply: 두 에셋 일괄 저장

### [Ability] 탭 (Spy/Data/)

- Scan: `Content/Spy/Data/` 내 USpyAbilityData, USpyComboAssetData 에셋 목록 수집 후 필드 제안
- SpyCharacterAssetData, SpyAbilityData, SpyComboAssetData의 **모든 배열 프로퍼티에 Copy 버튼** 포함
- SpyCommonCharacterAbility, SpyNormalAbility, SpyNormalComboAssetData 인라인 프리뷰
- Apply: 연관된 4개 에셋 일괄 저장

### [Config] 탭 (Spy/Data/Config/)

- Scan: `Content/Spy/Data/Config/` 하위 에셋 목록을 재수집해 DetailsView를 갱신 (에셋 교체·추가 시 반영)
- 4개 에셋(SpyAIConfig, SpyCharacterConfig, SpyInputConfig, SpyMovementConfig)을 세로로 나열
- Apply: 4개 에셋 일괄 저장

---

## 커스터마이징 목록

### 1. 프로퍼티 필터링
`IDetailsView::SetIsPropertyVisibleDelegate`로 내부 캐시/불필요 프로퍼티 숨김.

### 2. 카테고리 자동 펼치기
탭 진입 시 `ForceRefreshDetails()` + 주요 카테고리 자동 Expand.

### 3. 에셋 피커 경로 필터 (`SpyAssetPathCustomization`)
`FSoftObjectPath` / `TSoftClassPtr` 편집 시 파일 탐색 경로를 `Content/Spy/`로 제한. `IPropertyTypeCustomization`으로 구현.

### 4. Scan 변경 필드 하이라이트 (`SpyAssetPathCustomization` 내 처리)
Scan 결과와 현재 값을 비교해 변경된 필드 배경을 노란색으로 표시.  
Apply 후 초기화.

### 5. FGameplayTag 필터링 (`SpyGameplayTagCustomization`)
`ClassType` 등 태그 필드에 `Spy.Class.*` 네임스페이스만 표시하도록 커스텀 피커 적용.  
코드에서 `meta=(Categories="Spy.Class")`도 함께 추가.

### 6. Apply 전 Diff 요약 다이얼로그
Apply 버튼 클릭 시 변경 예정인 에셋 목록을 `FMessageDialog`로 표시.  
사용자가 "확인" 눌러야 실제 저장 진행.

### 7. 배열 엔트리 Copy 버튼 (`SpyArrayCopyCustomization`)
`IDetailCustomization`으로 Ability 탭의 모든 에셋(USpyCharacterAssetData, USpyAbilityData, USpyComboAssetData) 내 **모든 TArray 프로퍼티**의 각 엘리먼트 옆에 "Copy" 버튼 추가.  
클릭 시 해당 엔트리의 모든 프로퍼티 값을 복제해 배열 끝에 새 엔트리로 추가.  
동일 커스터마이징 클래스를 세 에셋 클래스에 각각 등록해 재사용한다.

---

## 데이터 흐름

```
[Scan 버튼]  (Assets / Ability 탭)
    ↓
SpyDataScanner::ScanContent()
    ↓ AssetRegistry 조회
TMap<FName, FSoftObjectPath> 수집
    ↓
IPropertyHandle::SetValue() 로 Details 업데이트
    ↓ (하이라이트 표시)
[사용자 검토/수정]
    ↓
[Apply 버튼]  (모든 탭)
    ↓
Diff 요약 다이얼로그
    ↓ 확인
UPackage::SavePackage() × N
```

---

## 에러 처리 및 검증

- Scan 시 에셋을 찾지 못한 경우: 해당 필드를 빈 상태로 두고 상단에 경고 배너 표시
- Apply 시 에셋 로드 실패: 실패한 에셋만 스킵하고 결과 메시지 표시 (성공 N개 / 실패 N개)
- `IsDataValid()` 실패 시: Apply 차단하고 오류 내용을 다이얼로그로 안내
- 에디터 외부(PIE 중 등) Apply 시도: 비활성화 처리
- Config 탭: Spy/Data/Config/ 경로에 에셋이 없으면 "에셋을 찾을 수 없습니다" 텍스트 표시

---

## 빌드 설정

`SkillProject.uproject` 에 Editor 모듈 추가:
```json
{
  "Name": "SpyDataEditorTool",
  "Type": "Editor",
  "LoadingPhase": "PostEngineInit"
}
```

`SpyDataEditorTool.Build.cs` 의존 모듈:
```
"UnrealEd", "PropertyEditor", "EditorStyle", "AssetRegistry",
"EditorSubsystem", "GameplayTags", "SkillProject"
```
