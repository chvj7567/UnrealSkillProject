# unreal-infra — SpyProject 인프라 규칙

> 원본 Unity 룰 03(ChvjPackage)·04(에셋) 대체. SpyProject 재사용 인프라(SKGAS·SKAssetCore·SpyAssetManager)와 GAS·DataAsset·모듈 규약을 정의한다.

---

## 1. 에셋 접근은 SpyAssetManager 경유

모든 에셋 로드는 `SpyAssetManager` 를 통한다. 하드코딩된 에셋 경로 직접 참조 금지.

- 동기: `USpyAssetManager::LoadAssetSync(Path)` / `GetAssetByName<T>(Name)` / `GetSubclassByName<T>(Name)`
- 비동기: `LoadAssetAsync(Path, Delegate)`
- 이름→경로 룩업은 `USKAssetData`(`GetAssetPathByName`) 를 통한다. 문자열 리터럴 경로 금지.

```cpp
//# (X) 하드코딩 경로
ConstructorHelpers::FObjectFinder<UTexture2D> Tex(TEXT("/Game/Spy/UI/Icon"));

//# (O) AssetManager 경유
const USKAssetData& Data = USpyAssetManager::Get().GetAssetData();
UTexture2D* Tex = USpyAssetManager::GetAssetByName<UTexture2D>(TEXT("Icon"));
```

체크리스트:
- [ ] 하드코딩된 `/Game/...` 경로 리터럴이 없는가?
- [ ] 에셋 접근이 SpyAssetManager API 를 통하는가?

---

## 2. GAS 데이터 파이프라인

- `USpyAbilityData`(DataAsset) → `GiveToAbilitySystem()` 로 AttributeSet 동적 생성·초기 GE 적용·GA 부여를 한 번에 수행.
- 모든 부여 핸들은 `FSpyAbilitySet_GrantedHandles` 로 트래킹 → 장착 해제·사망 시 반드시 `TakeFromAbilitySystem()` 으로 해제 (누수 금지).
- 입력은 `SpyEnhancedInputComponent` 에서 Gameplay Tag → ASC `AbilityLocalInputPressed/Released` 로 연결.
- GA 추가/ASC 조작은 `InitState_DataInitialized` 이후에만 (아래 §4).

체크리스트:
- [ ] 부여 핸들이 `FSpyAbilitySet_GrantedHandles` 에 저장되고 해제 경로가 있는가?
- [ ] 새 태그를 문자열이 아니라 `UE_DECLARE/DEFINE_GAMEPLAY_TAG` 로 등록했는가? (SpyGameplayTags.h/.cpp)

---

## 3. DataAsset 계층

```
USKAssetData (이름→경로 룩업 베이스)
└── USpyAssetData          # 전체 에셋 중앙 허브 (SpyAssetManager 가 시작 시 동기 로드)
USpyCharacterAssetData     # 캐릭터별 컴포넌트·어빌리티 세트·입력·콤보
USpyAbilityData            # GA/AttributeSet/GE 묶음
USpyComboAssetData / USpyAnimAssetData / Config DataAsset 들
```

- 하드코딩 수치·문자열은 Config DataAsset 으로 이전 (`docs/hardcoded-values.md` 참조).
- 새 에셋 타입 추가 시 `SpyDataEditorTool` 의 해당 탭 Slate 코드 + `SpyDataScanner` 동반 수정.

---

## 4. InitState 초기화 흐름

`SpyPawnExtensionComponent` 가 `IGameFrameworkInitStateInterface` 구현. `CharacterAssetData` 레플리케이트 완료 + Controller 연동 후 `InitState_DataInitialized` 단계에서 `InitAbilityActorInfo` 호출. **GA 추가·ASC 조작 코드는 반드시 이 흐름 이후에 실행.**

---

## 5. 모듈 의존 방향

```
SkillProject (게임)  →  SKGAS / SKAssetCore  →  UE 표준 모듈
```

- 역방향 참조 금지 (SKGAS/SKAssetCore 가 SkillProject 를 참조하지 않음).
- 공통 기능은 재사용 모듈(SKGAS/SKAssetCore)에 두고 게임 코드에서 중복 구현 금지.
- 새 의존성은 해당 모듈 `.Build.cs` 에 명시.

체크리스트:
- [ ] 재사용 모듈이 게임 모듈을 include 하지 않는가?
- [ ] `.Build.cs` 의존성이 올바른 방향인가?

---

## 6. 서버 권한 / 레플리케이션

- 게임플레이 상태 변경은 서버에서 실행하고 클라이언트에 레플리케이트.
- GA 내에서 `HasAuthority(&ActivationInfo)` 체크 후 서버 전용 로직. 클라 연출은 Authority 블록 밖.
- 레플리케이트 프로퍼티는 `Replicated` + `GetLifetimeReplicatedProps` 등록.
- 런타임 컴포넌트 추가는 `CharacterAssetData` 컴포넌트 목록을 읽어 `NewObject & RegisterComponent` — `BeginPlay` 하드코딩 금지.

---

## 7. 에셋/Blueprint

- 반복·재사용 구성은 Blueprint 또는 DataAsset 으로 (동일 구조 중복 금지).
- 런타임 참조는 하드 참조 대신 `TSoftObjectPtr`/`TSoftClassPtr` + AssetManager 로드.
- 패키지 빌드에서 BP 오브젝트(`BP_X.BP_X`)는 cook 시 stripped → generated class(`BP_X.BP_X_C`) 경로로 로드 (`GetSubclassByName` 이 `_C` 처리).
