# plugin-skuicore — SKUICore 규칙

> 프로젝트 무관 UI 매니저(open/close/cache/reuse) + 위젯 베이스. `SKAssetCore` 에 의존.
> 관련: [unreal-infra.md](unreal-infra.md) · [plugin-skassetcore.md](plugin-skassetcore.md)

---

## 1. 제공 클래스

| 클래스 | 베이스 | 역할 |
|--------|--------|------|
| `USKUIManager` | `UGameInstanceSubsystem` | 이름으로 UI open/close, 열림 스택·캐시 관리 |
| `USKUserWidget` | `UUserWidget` | UI 위젯 베이스 (UIName, 포인터 입력 소비, 터치/마우스 핸들러) |

---

## 2. UI 열기/닫기는 SKUIManager 경유

위젯을 직접 `CreateWidget` + `AddToViewport` 하지 않고 매니저 API 로 연다.

- 열기: `USKUIManager::OpenUI(FName)` / `OpenSubUI(FName, WidgetComponent, Space)`
- 닫기: `CloseUI(FName)` / `CloseLastUI()`
- 캐시: `AddCashingUI(USKUserWidget*)` — 재사용 대상 위젯을 캐시 풀에 등록 (최대 `MaxCashingUICount`).
- 인스턴스 접근: `USKUIManager::Get(WorldContextObject)` — base·leaf 어디서 호출해도 동일한 leaf 인스턴스 반환.
- UI 에셋(위젯 BP)은 이름으로 참조하며 실제 로드는 `SKAssetCore` 경유 (하드코딩 경로 금지, plugin-skassetcore.md §2).

체크리스트:
- [ ] 위젯을 직접 생성/뷰포트 추가하지 않고 `USKUIManager` API 로 여는가?
- [ ] UI 위젯이 `USKUserWidget` 을 상속하는가?
- [ ] UI 에셋 참조가 이름 룩업(SKAssetCore) 을 통하는가?

---

## 3. 위젯 작성 규칙

- 모든 게임 위젯은 `USKUserWidget` (또는 그 하위)을 상속한다.
- `SetUIName` / `GetUIName` 으로 매니저가 식별하는 이름을 관리한다 — 매니저가 open/close 시 이 이름을 키로 쓴다.
- 위젯이 하위 입력을 가로채야 하면 `SetConsumePointerInput(true)` 사용 (기본 `false`).
- 닫기 로직은 `Close()` 오버라이드로 확장한다 (base 가 매니저 정리까지 처리).
- BP 노출 함수는 `UFUNCTION(BlueprintCallable)` 필수 (cpp-style.md).

---

## 4. 게임 소비 패턴 — 서브클래싱

- 프로젝트 UI 매니저 = `UMyUIManager : USKUIManager` 로 확장 가능.
  - 파생 서브클래스가 있으면 base 는 `ShouldCreateSubsystem` 으로 생성되지 않아 leaf 인스턴스 1개만 남는다.
  - 즉 게임 코드에서도 `USKUIManager::Get()` 이 항상 leaf 를 돌려주므로 캐스팅해 쓴다.
- 공통 위젯 동작은 `USKUserWidget` 하위 중간 베이스에 두고 개별 위젯에서 중복 구현하지 않는다.
