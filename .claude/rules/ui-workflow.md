# ui-workflow — UI 작업 워크플로우 규칙

> UI 편집에 **착수한 뒤**의 방법·함정을 정의한다 — unreal-mcp 로 위젯을 어떻게 만지고 무엇을 피해야 하는가.
> 착수 **전** 게이트(목업 우선 승인)는 메인 오케스트레이터 행동 규칙이라 [.claude/project.md](../project.md) 가 SoT다.
> 위젯/매니저 구현 규칙은 [plugin-skuicore.md](plugin-skuicore.md), MCP 상세 레시피는 메모리 `project-mcp-umg-editing` 를 따른다.

---

## 1. 목업 우선 승인 게이트 → `.claude/project.md` 로 이관

이 게이트는 "메인이 언제 멈추는가"를 정하는 오케스트레이터 행동 규칙이라, 같은 성격의 다른 게이트들(스킬 후보 제시·에셋 한정 사이클)과 함께 [.claude/project.md](../project.md) 의 **"메인 오케스트레이터 행동 규칙 › UI 작업 — 목업 승인 게이트"** 에 있다.

요약: **화면 배치를 만들거나 바꾸는 작업은 목업 승인 전까지 위젯 트리·WBP 에셋을 만지지 않는다.** 아래 §2 는 승인이 떨어진 뒤에 읽는다.

---

## 2. unreal-mcp 편집 — 방법과 함정 (실측 확정)

`execute_python` 은 언리얼 Remote Control(`ExecutePythonScript`)로 실행된다. **상세 레시피·데드락 함정은 메모리 `project-mcp-umg-editing` 가 SoT다** — 승인 후 MCP 편집에 착수하기 전 그 메모리를 반드시 읽는다. 아래는 요약이다.

### 2-1. 직접 되는 것
- 에셋 복제(`EditorAssetLibrary.duplicate_asset`), reparent(`BlueprintEditorLibrary.reparent_blueprint` — timeout 을 반환해도 실제 성공), save(`save_asset(only_if_is_dirty=False)`).
- **CDO 직속** 스칼라·클래스·enum 프로퍼티 `set_editor_property` — 예: GA `mana_cost`(float), GE `duration_magnitude` 스칼라 값, SkillBar `slot_widget_class`(클래스 참조).

### 2-2. 워크어라운드로 되는 것 (단순 `set_editor_property` 는 막히지만 방법이 있음)
- **위젯 트리 편집(명명 자식 위젯 생성·배치)** — 서브오브젝트 경로(`…:WidgetTree`)로 트리를 잡고, 기존 위젯의 `get_parent()` 로 루트 패널을 얻어, `unreal.new_object(unreal.TextBlock, wt, "이름")` 로 생성 후 `canvas.add_child_to_canvas(w)` 로 붙인다. (`unreal.new_object` 는 존재한다.)
- **`EditDefaultsOnly` 중첩 struct 배열** — GE `Modifiers[]`·어빌리티 `GrantedGameplayEffects[]` 등은 `set_editor_property`(사본/신규 struct)가 `"cannot be edited on instances"` 로 막힌다. **`export_text()` 로 포맷을 뽑아 문자열을 편집하고 `import_text()` 로 통째 구성해 대입**하면 통과한다(태그·FText·클래스 참조까지 정상).
- 리드백: `execute_python` 은 stdout/`print` 를 캡처하지 않는다 → 결과를 파일로 덤프하고 Read 로 읽는다. 예외는 `traceback.format_exc()` 로 파일에 남긴다.

### 2-3. ⚠ 반드시 지킬 함정
- **WidgetBlueprint 에 `compile_blueprint()` 를 호출하지 않는다 — 에디터가 데드락된다.** 생성/편집 후 `save_asset` 만 하고, **컴파일은 사용자가 디자이너에서 1회** 수행한다(위젯 GUID 도 그때 해소).
- 빈 WBP 는 루트를 파이썬으로 못 만든다 → 기존 WBP **복제 + reparent** 로 우회(복제 원본 자식 잔재는 이후 제거). `reparent_blueprint` 는 **위젯 배치를 끝낸 뒤 마지막에** 호출한다(BindWidget 충족 후라야 컴파일 에러 없음).
- `save_asset` 은 `only_if_is_dirty=False` 필수. 신뢰 못 하는 도구(`add_asset_entry` 등)는 되읽어(`.uasset` 타임스탬프·재조회) 검증한다.

체크리스트:
- [ ] 착수 전 메모리 `project-mcp-umg-editing` 의 레시피·함정을 확인했는가? (특히 WidgetBlueprint 컴파일 금지)
- [ ] 위젯 생성은 `new_object`+`add_child_to_canvas`, 중첩 struct 배열은 `import_text` 로 처리했는가?
- [ ] 리드백을 파일 덤프→Read 로 검증했는가?
