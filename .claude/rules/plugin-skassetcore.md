# plugin-skassetcore — SKAssetCore 규칙

> 프로젝트 무관 에셋 매니저 + 이름→경로 룩업 DataAsset. 하드코딩 에셋 경로를 제거하는 것이 목적.
> 관련: [unreal-infra.md](unreal-infra.md) · 상위 모듈 규약

---

## 1. 제공 클래스

| 클래스 | 베이스 | 역할 |
|--------|--------|------|
| `USKAssetManager` | `UAssetManager` | 동기/비동기 에셋 로드, 이름/경로 룩업 진입점 |
| `USKAssetData` | `UPrimaryDataAsset` | 이름→`FSoftObjectPath` 룩업 테이블 (`GetAssetPathByName`) |

---

## 2. 에셋 접근은 반드시 SKAssetManager 경유

모든 에셋 로드는 `USKAssetManager` 를 통한다. 하드코딩된 `/Game/...` 경로 직접 참조 금지.

- 동기(경로): `USKAssetManager::LoadAssetSync(Path)`
- 동기(이름): `USKAssetManager::GetAssetByName<T>(Name)` / `GetSubclassByName<T>(Name)`
- 비동기: `USKAssetManager::LoadAssetAsync(Path, Delegate)`
- 언로드: `USKAssetManager::UnloadAsset(Path)`
- 이름→경로 룩업은 `USKAssetData::GetAssetPathByName` 을 통한다. 문자열 리터럴 경로 금지.

```cpp
//# (X) 하드코딩 경로
ConstructorHelpers::FObjectFinder<UTexture2D> Tex(TEXT("/Game/UI/Icon"));

//# (O) AssetManager 경유 (이름 룩업)
UTexture2D* Tex = USKAssetManager::GetAssetByName<UTexture2D>(TEXT("Icon"));
```

- `GetSubclassByName<T>` 은 cook 후 BP 오브젝트가 stripped 되는 것을 고려해 generated class(`_C`) 경로를 자동 처리한다. BP 클래스 로드는 이 API 를 쓴다.

체크리스트:
- [ ] 하드코딩된 `/Game/...` 경로 리터럴이 없는가?
- [ ] 에셋 접근이 `USKAssetManager` API 를 통하는가?
- [ ] 이름→경로 매핑을 `USKAssetData` 에 등록했는가?

---

## 3. 게임 소비 패턴 — 서브클래싱

`USKAssetManager` 는 base 다. 게임은 자신의 concrete AssetData 타입을 위해 서브클래싱한다.

- 프로젝트 매니저 = `UMyAssetManager : USKAssetManager`.
- `GetAssetData()` 를 오버라이드해 프로젝트의 concrete `USKAssetData` 서브클래스를 반환한다.
  - PrimaryAssetType·캐시 키가 concrete 클래스명에 묶이므로 base 에서 직접 구현하지 않는다.
- 로딩 진행률 훅이 필요하면 `OnLoadProgress(Loaded, Total)` 오버라이드 (로딩스크린 % 등).
- `.uproject` / `DefaultEngine.ini` 의 `AssetManagerClassName` 을 프로젝트 매니저로 지정해야 실제로 사용된다.

```cpp
//# 프로젝트 매니저에서
virtual const USKAssetData& GetAssetData() override
{
    return GetOrLoadTypedGameData<UMyAssetData>(MyAssetDataPath);
}
```

---

## 4. DataAsset 계층

```
USKAssetData (이름→경로 룩업 베이스)
└── U<Project>AssetData     # 전체 에셋 중앙 허브 (매니저가 시작 시 동기 로드)
    └── 캐릭터/어빌리티/애님/Config 등 하위 DataAsset 참조
```

- 하드코딩 수치·문자열은 Config DataAsset 으로 이전한다.
- 런타임 참조는 하드 참조 대신 `TSoftObjectPtr`/`TSoftClassPtr` + AssetManager 로드.
