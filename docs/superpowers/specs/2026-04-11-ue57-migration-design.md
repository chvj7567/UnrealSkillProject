# UE 5.4 → 5.7 마이그레이션 설계

**날짜:** 2026-04-11  
**프로젝트:** SpyProject (SkillProject)  
**대상 엔진 경로:** `D:\UE_5.7`  
**작업 브랜치:** `migration/5.7`

---

## 목표

엔진 버전 업그레이드(최신 유지)를 위해 UE 5.4 → 5.7로 마이그레이션한다.  
핵심 파일 수정 후 컴파일 에러를 함께 대응하는 방식으로 진행한다.

---

## 프로젝트 현황

| 항목 | 내용 |
|------|------|
| 현재 엔진 | UE 5.4 (`EngineAssociation: "5.4"`) |
| 커스텀 모듈 | `SKGAS`, `SkillProject` |
| 프로젝트 플러그인 | `ModularGameplayActors` (Epic 제작, 프로젝트 내 소스 보유) |
| 엔진 플러그인 의존 | GameplayAbilities, ModularGameplay, MotionWarping, Enhanced Input |
| 핵심 시스템 | GAS (ASC/AttributeSet/GA), AI, 파쿠르, 타겟팅, UMG |

---

## 마이그레이션 단계

### Phase 1 — 브랜치 + 핵심 파일 수정

1. `migration/5.7` 브랜치 생성
2. `SkillProject.uproject`의 `EngineAssociation` 을 `"5.4"` → `"5.7"` 로 변경
3. 프로젝트 레벨 캐시 클린:
   - `SkillProject/Intermediate/` 삭제
   - `SkillProject/Binaries/` 삭제
   - `SkillProject/Plugins/ModularGameplayActors/Intermediate/` 삭제
   - `SkillProject/Plugins/ModularGameplayActors/Binaries/` 삭제
4. 프로젝트 파일 재생성: `D:\UE_5.7\Engine\Build\BatchFiles\GenerateProjectFiles.bat` 실행

### Phase 2 — 알려진 Breaking Changes 선제 수정

5.4 → 5.7 구간에서 이 프로젝트에 영향을 줄 수 있는 항목을 미리 확인 및 수정한다.

| 변경 항목 | 영향 파일 | 비고 |
|-----------|-----------|------|
| `FGameplayEffectSpec` deprecated 생성자 | `SKGAS/`, `SpyAbilityData` | 5.5에서 생성자 시그니처 변경 |
| `IGameFrameworkInitStateInterface` API | `SpyPawnExtensionComponent` | 5.5~5.6 구간 변경 가능성 |
| Enhanced Input 일부 API | `SpyEnhancedInputComponent` | deprecated 바인딩 방식 변경 |
| `ModularGameplayActors` 플러그인 소스 | `ModularAIController` 등 | 5.7 엔진 API와 호환성 확인 |

### Phase 3 — 컴파일 + 에러 대응

- Visual Studio 2022에서 컴파일 시도
- 에러 로그 기반으로 순차적으로 수정
- 컴파일 성공 후 에디터 실행 및 기본 동작 확인

---

## 리스크

| 리스크 | 대응 |
|--------|------|
| `ModularGameplayActors` 소스가 5.7 API와 호환되지 않을 경우 | 플러그인 소스 직접 수정 또는 엔진 내장 버전으로 교체 |
| GAS API 대규모 변경 | 에러 로그 기반 수정, Epic 마이그레이션 노트 참조 |
| Blueprint 호환성 문제 | 에디터 실행 후 Blueprint 재컴파일 |

---

## 완료 기준

- [ ] `migration/5.7` 브랜치에서 컴파일 성공
- [ ] 에디터 정상 실행
- [ ] 기본 플레이(이동/스킬/AI) 동작 확인
