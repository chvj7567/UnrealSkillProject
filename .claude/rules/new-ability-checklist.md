# 새 Gameplay Ability 추가 체크리스트

새 GA를 추가할 때 이 순서대로 작업한다.

## 1. 태그 등록

- `SpyGameplayTags.h` — `UE_DECLARE_GAMEPLAY_TAG_EXTERN` 선언
- `SpyGameplayTags.cpp` — `UE_DEFINE_GAMEPLAY_TAG` 정의
- 문자열 리터럴로 태그를 직접 참조하지 말 것

## 2. GA 클래스 작성

- `SKGameplayAbility` (또는 하위 베이스)를 상속
- `ActivateAbility` 첫 줄에 `HasAuthority()` 체크
- 클라이언트 측 연출(카메라, 사운드)은 `Authority` 블록 밖에서

```cpp
void USpyGA_Example::ActivateAbility(...)
{
    Super::ActivateAbility(...);

    if (HasAuthority(&ActivationInfo))
    {
        // 서버 전용 게임플레이 로직
    }

    // 클라이언트 포함 연출
}
```

## 3. DataAsset 등록

- `USpyAbilityData` 에셋에 GA 클래스 추가
- `SpyDataEditorTool`의 Ability 탭에서 확인

## 4. 입력 바인딩 (입력이 필요한 경우)

- `SpyInputConfig` DataAsset에 InputAction → AbilityTag 매핑 추가
- `IMC_Default` InputMappingContext에 키 바인딩 추가

## 5. 부여 핸들 관리

- `GiveToAbilitySystem()` 호출 결과를 `FSpyAbilitySet_GrantedHandles`에 저장
- 해제 시 반드시 `TakeFromAbilitySystem()` 호출
