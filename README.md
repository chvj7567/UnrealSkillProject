# UnrealSkillProject : Spy Project

이 프로젝트는 언리얼 엔진 5.4를 기반으로 제작된 게임 프로젝트입니다. 
**데디케이티드 서버(Dedicated Server) 환경**을 고려하여 완벽한 멀티플레이 동기화를 목표로 설계되었으며, **라이라(Lyra) 스타일의 모듈형 아키텍처**, **커스텀 GAS 프레임워크**, 그리고 **데이터 지향적 설계(Data-Oriented Design)**를 핵심 기술 스택으로 채택하여 극대화된 확장성과 유지보수성을 제공합니다.

---

## 🚀 코어 아키텍처 하이라이트 (Core Architecture Highlights)

### 1. 🌐 데디케이티드 서버 기반 멀티플레이어 (Dedicated Server Ready)
모든 게임플레이 로직과 파쿠르(Vault, Climb, Hang) 메커니즘은 서버 권한(Server Authority)을 바탕으로 리플리케이션(Replication)되도록 설계되었습니다. `Gameplay Ability System (GAS)`을 활용한 스킬 발동, 모션 워핑(Motion Warping) 데이터의 동기화 등 클라이언트 예측(Client Prediction)과 서버 검증을 통해 안정적인 멀티플레이 경험을 제공합니다.

### 2. 🧩 모듈화 및 컴포넌트 중심 설계 (Modular Framework)
에픽게임즈의 최신 패러다임(Lyra Starter Game)을 차용하여, 하드코딩된 액터 로직을 배제하고 플러그인과 컴포넌트 단위로 기능을 분리했습니다.
- **Modular Gameplay Actors 플러그인 통합**: `AModularCharacter`, `AModularPlayerController` 등을 활용하여 플러그인 형태의 기능 주입(Injection)을 지원합니다.
- **GameFeature 독립적인 `InitState` 동적 주입 및 초기화 동기화**: 무거운 GameFeature 플러그인에 의존하지 않고, `IGameFrameworkInitStateInterface`를 활용해 효율적인 상태 관리 및 서버-클라 동기화를 구현했습니다.
  - 서버에서 주입된 `CharacterAssetData`가 클라이언트에 리플리케이트되고 컨트롤러가 연동될 때까지 대기한 후, `InitState_DataInitialized` 단계에서 안전하게 `InitAbilityActorInfo`를 체결하여 크래시를 원천 차단합니다.
  - 아울러 이 `InitState` 흐름을 타면서 데이터 에셋에 정의된 액터 컴포넌트 목록을 읽어 런타임에 동적으로 컴포넌트를 주입(NewObject & Register)하는 유연한 아키텍처를 가집니다.

### 3. ⚡ 체계적으로 구조화된 GAS (Custom GAS Framework)
언리얼 기본 GAS를 프로젝트에 최적화되게 래핑(Wrapping)한 `SKGAS` 별도 모듈을 구축했습니다.
- **SKGAS 모듈 아키텍처**: 어빌리티의 공통 로직(스킬 액션, 이동기 등)을 `SKGameplayAbility` 베이스 클래스로 캡슐화했습니다.
- **모든 게임플레이 로직의 GA화 (Everything is GA)**: 단순 전투 스킬뿐만 아니라 캐릭터의 **스탯 초기화(Stat Init), 기본 점프(Jump), 파쿠르 액션(Vault, Wall Climb, Hang Up) 및 죽음(Death) 처리까지** 게임 내 일어나는 모든 상태 변화와 액션을 Gameplay Ability(GA)로 구현했습니다. 하드코딩된 로직을 배제하고 모든 행위를 GAS 파이프라인 위로 통합하여 안정적인 멀티플레이 동기화와 유지보수성을 확보했습니다.
- **입력 버퍼링(Input Buffering) & 태그 매핑**: `SKAbilitySystemComponent`에서 어빌리티 입력(Pressed/Released)을 단순 열거형(Enum)이 아닌 **캐싱된 핸들 배열**과 **Gameplay Tag**로 처리하여 유연한 입력-스킬 바인딩을 구현했습니다.
- **Data-Driven `GiveAbility` 파이프라인**: 하드코딩된 어빌리티 부여를 지양하고, `USpyAbilityData` (DataAsset)를 통해 어빌리티를 부여합니다. `GiveToAbilitySystem()` 호출 시 내부적으로 배열을 순회하며 1) 없는 `AttributeSet` 동적 생성 및 추가, 2) 초기 `GameplayEffect` 자동 적용, 3) `GameplayAbility` 투입 시 `DynamicAbilityTags`에 인풋 태그를 꽂아넣어 `GiveAbility`를 수행하며, 발급된 모든 핸들을 `FSpyAbilitySet_GrantedHandles` 단위로 트래킹하여 제거 시점(장착 해제, 사망 등) 메모리 누수를 방지합니다.
- **최적화된 큐 매니저 (`SKCueManager` 비동기 프리로딩)**: 런타임에 빈번히 발생하는 이펙트, 사운드용 큐(Cue) 액터로 인한 렉(Hitch)을 방지하기 위해 정교한 **비동기 프리로딩 메커니즘**을 구현했습니다.

### 4. 💾 데이터 지향 구조 (Data-Oriented Architecture)
하드코딩을 배제하고 모든 기획 요소(애니메이션, 스킬, 스탯, 콤보 등)를 `DataAsset` 기반으로 분리했습니다.
- **`SpyAssetData`, `SpyAbilityData`, `SpyAnimAssetData` 등**: 다수의 DataAsset 프레임워크를 통해 기획자가 프로그래머의 도움 없이 데이터만 수정하여 밸런싱 및 기능 확장이 가능하도록 구축했습니다.
- 상태 기계(State Machine)나 애니메이션 노티파이(`SpyAnimNotify_State_Combo` 등) 처리 역시 이 데이터 지향 구조와 유기적으로 연결되어 동작합니다.

### 5. 📦 커스텀 에셋 매니저 (Sync/Async Asset Manager 및 Config 기반 자동 스캔)
게임 규모 확장과 데이터 무결성을 위해, `SpyAssetManager`를 에셋 접근의 **진실의 원천(Single Source of Truth)**이자 전역 중앙 허브로 자체 구현했습니다.
- **글로벌 필수 데이터 한정 초기 동기 로딩**: 무거운 초기 로딩 병목 현상을 타파하기 위해, 프로젝트 세팅(`PrimaryAssetTypesToScan`)에서 자동 스캔시키는 PrimaryDataAsset의 범위를 무분별한 에셋이 아닌 **'게임에 꼭 필요한 글로벌 코어 데이터'**로만 엄격히 제한했습니다. 시작 시 이 데이터들만 동기 로드(`LoadAllPrimaryAssetsSync`)하여, 시스템 전반의 런타임 안정성을 확보했습니다.
- **작업 병목을 줄인 `SpyAssetData` 중앙 사전 구성**: `SpyAssetData`라는 단일 PrimaryDataAsset 하나를 통해 전반적인 게임 데이터를 이름 테이블(Dictionary)로 관리해 주는 로딩 인터페이스 허브 구조를 가집니다. 이를 통해 수많은 코드에서의 접근 편의성을 극대화하였으며, 치명적인 바이너리 파일 머지 충돌(Merge Conflict) 문제를 해결하기 위해 팀원들이 각자 개별 PrimaryDataAsset을 생성 및 작업하되, **최종 통합(Merge) 시점에만 `SpyAssetData`에 추가로 등록하는 파이프라인 협업 규칙**을 적용했습니다.
- **코드 수정 없는 Data-Driven 로딩**: 위 세팅을 바탕으로 C++ 코드 수정 하나 없이 프로젝트에 필요한 부가 에셋 추가가 가능하며, 필요한 시점에 언제든 `LoadAssetAsync`, `LoadAssetSync` 메커니즘을 유동적으로 제어할 수 있습니다.

### 6. 🎮 Enhanced Input 구조화 (Structured Input System)
언리얼 최신 입력 체계(Enhanced Input)를 Gameplay Tag 시스템과 완벽히 융합했습니다.
- **`SpyInputConfig` DataAsset**: `UInputAction`과 `GameplayTag`를 1:1 방식이 아닌 N:M 매핑 구조로 유연하게 설정할 수 있는 데이터 구조.
- **`SpyEnhancedInputComponent`**: 폰(Pawn)의 입력 바인딩을 하드코딩하지 않고, `SpyInputConfig`에서 읽어온 태그 기반으로 `Input Pressed`, `Input Released` 이벤트를 즉각적으로 `AbilitySystemComponent`에 전달하도록 파이프라인을 구축했습니다.

### 7. 🏃 파쿠르 시스템 (Smooth Parkour Physics)
서버 기반 아키텍처 위에서 `SpyParkourManagerComponent`를 통해 처리되며, 모션 워핑(Motion Warping)과 완벽히 동기화됩니다.
- **다중 레이캐스트(Raycast) 기반 지형 분석 (Vault / Wall Climb / Hang Up)**: 
  단순한 충돌 판정이 아닌, 전방 및 하단 다중 라인트레이스(LineTrace)를 통해 장애물의 형태를 정밀하게 분석하여 액션을 결정합니다.
  1. **전방 검출 (Forward Raycast)**: 캐릭터 전방으로 레이를 쏴 벽면의 법선 벡터(Normal)와 거리를 추출합니다.
  2. **높이 및 상단 표면 검출 (Top-Down Raycast Iteration)**: 닿은 벽의 법선을 역산하여 일정 간격(`RayInterval`)마다 위에서 아래로 레이를 쏘면서 장애물의 정확한 높이(Height)와 손 짚을 타겟 위치(HitVector)를 계산합니다.
  3. **깊이 식별 및 착지점 도출 (Depth Check)**: 상단 레이가 닿지 않고 빗나가는 시점(장애물이 끝나는 지점)을 캐치하여 역방향으로 레이를 쏴 장애물의 두께(Depth)를 산출하고 넘어갈 수 있는 최종 착지점(LandVector)을 찾아냅니다.
- **GA(Gameplay Ability) 기반 액션 발동 플로우**: 이 모든 파쿠르 액션은 단순한 몽타주 재생이 아닌, **GAS의 개별 어빌리티(`GA_Vault`, `GA_WallClimb`, `GA_HangUp` 등) 단위로 캡슐화되어 실행**됩니다.
  - **Vault(장애물 넘기)**와 **Wall Climb(벽 타기)**는 특정 입력 키셋에 매핑되어 지형 데이터가 조건에 부합할 때 해당 GA를 발동(TryActivate)합니다.
  - 반면 **Hang Up(매달리기)**은 별도 키 조작 없이, Wall Climb GA를 수행하며 벽의 끝부분(상단 엣지)에 도달했을 때 상태를 자동 감지하여 시스템적으로 시전되는 유기적인 GA 연계 구조를 가집니다.
- 서버에서 이렇게 계산된 정밀한 위치 데이터를 `FMotionWarpingData`로 변환하고 클라이언트에 리플리케이션(`OnRep_VaultMotionWarpingData` 등)하여, 파쿠르 애니메이션 시 모션 워핑 앵커포인트를 매칭시켜 딜레이나 끊김 없는 부드러운 밀착 액션을 보장합니다.

### 8. ⚔️ 데이터 지향 콤보 시스템 (Data-Driven Combo System)
애니메이션 몽타주 노티파이, GAS(Gameplay Ability System), 그리고 자체 DataAsset을 유기적으로 결합하여 하드코딩 없는 콤보 연계 시스템을 구현했습니다.
- **`SpyAnimNotify_State_Combo` (콤보 윈도우 개방)**: 공격 애니메이션의 허용 구간(AnimNotifyState) 동안 캐릭터의 ASC(Ability System Component)에 콤보 대기 상태 태그(`Character_State_Combo`)를 가상 태그(Loose Tag)로 동적 부여 및 해제합니다.
- **`SpyComboAssetData` (콤보 체인 데이터화)**: 'A 스킬 구간에서 콤보 입력 시 B 스킬 발동'이라는 연계 공식을 하드코딩 로직이 아닌, `StartSkillTag` &rarr; `ComboTag` 1:1 매핑 딕셔너리를 갖춘 데이터 에셋(`PrimaryDataAsset`)으로 분리했습니다.
- **작동 플로우**: 플레이어 입력 시 현재 ASC에 콤보 태그가 켜져 있는지 검증 &rarr; 켜져 있다면 가장 최근 시전된 스킬 태그를 `SpyComboAssetData`에서 색인하여 연결된 다음 타격(ComboTag) 어빌리티를 즉각 실행(Activate)하는 완벽한 데이터 지향(Data-Driven) 콤보 사이클을 완성했습니다.

---