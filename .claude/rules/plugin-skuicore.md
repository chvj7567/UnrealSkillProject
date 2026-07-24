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

### 2-1. persistent UI — 트래블(맵 전환)을 넘어 살아남는 UI

- 열기/닫기: `OpenPersistentUI(FName, ZOrder = 100)` / `ClosePersistentUI(FName)` / `IsPersistentUIOpen(FName)`
- **로딩 화면처럼 맵 전환 중에도 계속 보여야 하는 UI 전용.** 일반 UI 는 `OpenUI` 를 쓴다.
- GameInstance 를 아우터로 생성하고 `UGameViewportClient::AddViewportWidgetContent` 로 얹으므로 월드가 파괴돼도 유지된다. (`OpenUI` 는 `CreateWidget(GetWorld(), ...)` + `AddToViewport` 라 월드와 함께 소멸한다.)
- 위젯 클래스를 **동기 로드**한다 — 즉시 표시가 목적이기 때문. `OpenUI` 의 비동기 로드는 표시가 지연된다(실측 1.23초). 큰 위젯에는 쓰지 않는다.
- 뷰포트가 없는 환경(데디케이티드 서버)에서는 아무것도 하지 않고 `nullptr` 을 반환한다.
- persistent UI 는 `OpenUIList` · 캐시 풀과 **분리 관리**된다 — `CloseLastUI` 로 닫히지 않고 `AddCashingUI` 대상도 아니다(뷰포트 콘텐츠와 `AddToViewport` 가 이중 부착되면 안 됨).
- 게임 모듈은 여전히 매니저 API 만 호출한다. `GEngine->GameViewport` 를 게임 코드에서 직접 만지지 않는다.

체크리스트:
- [ ] 위젯을 직접 생성/뷰포트 추가하지 않고 `USKUIManager` API 로 여는가?
- [ ] UI 위젯이 `USKUserWidget` 을 상속하는가?
- [ ] UI 에셋 참조가 이름 룩업(SKAssetCore) 을 통하는가?
- [ ] 맵 전환을 넘어야 하는 UI 에 `OpenUI` 대신 `OpenPersistentUI` 를 썼는가?

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
