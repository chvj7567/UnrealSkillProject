# 스킬바 슬롯 config (DataAsset) — 설계

- 날짜: 2026-07-24
- 관련: 데이터 구동 스킬바 `USpySkillBarWidget`(2026-07-24 HUD ⑤), `SpySkillSlotWidget`
- 상태: 브레인스토밍 승인 완료 → writing-plans 진입 대기

---

## 1. 목표

스킬바의 **각 슬롯에 어떤 스킬(입력태그)을 넣을지 + 어떤 아이콘을 적용할지**를 코드 수정 없이 에디터에서 설정한다. 현재는 `BuildSlotInputTags()` 가 `Skill_1~6` 을 **하드코딩**하고 아이콘은 플레이스홀더(더미색)다. 이를 DataAsset 기반으로 데이터 구동화한다.

**"툴" 형태 결정(사용자)**: 커스텀 Slate 툴이 아니라 **DataAsset + 에디터 기본 Details 패널**. 슬레이트 코드 0.

## 2. 데이터 모델

```cpp
USTRUCT(BlueprintType)
struct FSpySkillBarSlot
{
    //# 이 슬롯이 표시·활성·쿨다운 조회에 쓸 입력태그 (Details 드롭다운, meta=(Categories="Input.Ability"))
    UPROPERTY(EditAnywhere, meta=(Categories="Input.Ability"))
    FGameplayTag InputTag;

    //# 슬롯 아이콘. 소프트 참조(프로젝트 소프트로드 관례) — 슬롯 빌드 시 로드
    UPROPERTY(EditAnywhere)
    TSoftObjectPtr<UTexture2D> Icon;
};

UCLASS()  // : public UDataAsset
class USpySkillBarConfig : public UDataAsset
{
    //# 배열 순서 = 화면 좌→우 슬롯 순서
    UPROPERTY(EditDefaultsOnly, Category="SkillBar")
    TArray<FSpySkillBarSlot> Slots;
};
```

- 편집: 에디터에서 `DA_SpySkillBarConfig` 를 열어 Details 패널에서 행 추가 → InputTag 드롭다운 → Icon 이미지 지정. 배열 순서로 슬롯 순서 조정.

## 3. 스킬바 연동 (`USpySkillBarWidget`)

- 신규 멤버 `TObjectPtr<USpySkillBarConfig> SkillBarConfig` (EditDefaultsOnly, Category="SkillBar").
  - **참조 해석은 `SlotWidgetClass` 와 동일한 CDO 폴백 패턴 재사용**: 임베드 인스턴스에서 EditDefaultsOnly 가 런타임 null 로 오는 문제(2026-07-24 실측)를 위해 `ResolvedConfig = SkillBarConfig` → null 이면 `GetClass()->GetDefaultObject<USpySkillBarWidget>()->SkillBarConfig`.
- **슬롯 목록 해석** — 순수 정적 함수:
  ```cpp
  static TArray<FGameplayTag> USpySkillBarWidget::ExtractSlotInputTags(const USpySkillBarConfig* Config);
  ```
  - `Config` 가 유효하고 `Slots` 가 비어있지 않으면 → `Slots` 의 `InputTag` 를 순서대로 반환.
  - 아니면 → 기존 `BuildSlotInputTags()`(하드코딩 Skill_1~6) 폴백. **하위호환**: config 미지정 시 현행 동작 유지.
- **슬롯 빌드** (`BuildSlots`):
  - 해석된 슬롯 목록으로 슬롯 생성(기존 `bAnyGranted` 게이트·`ResolvedSlotClass`·`UHorizontalBoxSlot` Fill 유지).
  - 각 슬롯의 아이콘: config 항목의 `Icon`(소프트) 을 로드해 `USpySkillSlotWidget::Setup` 에 전달. config 없는 폴백 경로는 아이콘 없음(현행 더미색 유지).

## 4. 슬롯 위젯 아이콘 적용 (`USpySkillSlotWidget`)

- `Setup(...)` 시그니처에 아이콘 인자 추가:
  ```cpp
  void Setup(UAbilitySystemComponent* InASC, FGameplayTag InInputTag,
             FGameplayTagContainer InCooldownTags, FText InKeyHint,
             float InManaCost, UTexture2D* InIcon);
  ```
- `InIcon != nullptr` 이면 `Img_Icon->SetBrushFromTexture(InIcon)` 로 아이콘 표시. null 이면 기존 브러시(더미색) 유지.
- 마나부족 적색 틴트(`SetColorAndOpacity`)는 브러시 위에 곱해지므로 그대로 동작.

## 5. 순수함수 + 테스트

Unreal Automation `SkillProject.HUD.SkillBar.*`:
- `ExtractSlotInputTags` — (a) config null → 기본 태그(6개), (b) config 빈 배열 → 기본 태그, (c) config 3슬롯 → 그 3태그 순서대로, (d) config 순서 보존.
- 기존 `BuildSlotInputTags`/`ExactOrder`/`Uniqueness`(6슬롯) 는 폴백 기본값으로 유지.

## 6. 에셋

- `DA_SpySkillBarConfig` 생성 — 슬롯별 InputTag + 아이콘 텍스처 입력(초기값은 현행 Skill_1~6 + 사용 가능한 아이콘/플레이스홀더).
- `WBP_SkillBar` 기본값(Class Defaults)에 `SkillBarConfig = DA_SpySkillBarConfig` 지정. (임베드 인스턴스는 CDO 폴백이 처리.)
- 아이콘 텍스처 에셋: 이번 범위는 config **파이프라인**까지. 실제 아이콘 아트는 사용자가 지정(없으면 플레이스홀더 텍스처).

## 7. 범위 밖 (YAGNI)

- 커스텀 Slate 탭/에디터, 드래그 재정렬 UI.
- 키힌트 문자 config화(현행 `SlotKeyHint` 코드 유지).
- 슬롯별 색·크기·툴팁(Details/디자이너로 충분).
- 아이콘 아트 제작.

## 8. 룰 준수

- 하드코딩 경로 금지: 아이콘은 `TSoftObjectPtr` + config 참조(에디터 지정). config 자체도 WBP 기본값 지정(하드 문자열 경로 없음).
- EditDefaultsOnly 배열 편집은 에디터 Details(사람) — MCP 자동편집은 export_text/import_text 필요(ui-workflow 룰).
- 위젯은 `USpyUserWidget` 계열 유지. DataAsset 은 `UDataAsset`(또는 프로젝트 `USKAssetData` 계열 검토 — §참고).

## 9. 참고 — DataAsset 베이스 선택

`UDataAsset` 직접 상속으로 충분(단순 config). 이름 룩업이 필요해지면 `USKAssetData` 계열로 승격 검토하되, 이번엔 WBP 기본값 직접 참조라 불필요(YAGNI).
