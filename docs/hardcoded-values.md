# 하드코딩 값 목록

> 분석 기준일: 2026-04-13  
> 대상 경로: `SkillProject/Source/`

---

## 1. 매직 넘버

### SpyCharacter.cpp

| 라인 | 값 | 설명 |
|------|----|------|
| 45 | `42.f, 96.0f` | 캡슐 컴포넌트 반경/높이 |
| 52 | `500.0f` | RotationRate Yaw |
| 54 | `1080.f` | RotationRate Yaw (덮어쓰기) |
| 55 | `700.f` | JumpZVelocity |
| 56 | `0.35f` | AirControl |
| 57 | `500.f` | MaxWalkSpeed |
| 58 | `20.f` | MinAnalogWalkSpeed |
| 59 | `2000.f` | BrakingDecelerationWalking |
| 60 | `1500.0f` | BrakingDecelerationFalling |
| 64 | `400.0f` | CameraBoom TargetArmLength |
| 74 | `200.f` | HP바 위치 오프셋 (Z) |
| 91 | `1.0f` | 무기 스폰 지연 시간(초) |

### SpyCharacterMovementComponent.cpp

| 라인 | 값 | 설명 |
|------|----|------|
| 118 | `50.f` | 클라이밍 레이 시작점 전방 오프셋 |
| 118 | `200.f` | 클라이밍 레이 시작점 상단 오프셋 |
| 119 | `500.f` | 클라이밍 레이 하향 거리 |
| 170 | `1.0f` | 클라이밍 종료 시 GravityScale 복원값 |
| 187 | `100.f` | HangUp 체크 레이 전방 오프셋 |

### SpyParkourManagerComponent.cpp

| 라인 | 값 | 설명 |
|------|----|------|
| 192 | `180.f` | 볼트 방향 보정 각도 |
| 209~250 | `10.f, 12, 1.f` | 디버그 구체 크기/세그먼트/지속시간 |
| 299 | `1000.f` | 파쿠르 높이 오프셋 |

### SpyDamageCalculation.cpp

| 라인 | 값 | 설명 |
|------|----|------|
| 30 | `0.5f` | 크리티컬 확률 (50%) |
| 89 | `100.f` | 기본 데미지 |
| 92 | `10.f` | AI 데미지 |
| 97 | `30.f` | 플레이어 데미지 |

### SpyAIController.cpp

| 라인 | 값 | 설명 |
|------|----|------|
| 33 | `500.f` | SightRadius |
| 34 | `700.f` | LoseSightRadius |
| 35 | `90.f` | PeripheralVisionAngleDegrees |
| 36 | `5.f` | 시각 정보 유효 시간(초) |
| 42 | `200.f` | HearingRange |
| 43 | `5.f` | 청각 정보 유효 시간(초) |
| 49 | `5.f` | 데미지 정보 유효 시간(초) |
| 58 | `1` | AI 팀 ID |

### SpyPlayerState.cpp

| 라인 | 값 | 설명 |
|------|----|------|
| 25 | `100.0f` | NetUpdateFrequency |
| 151 | `100` | 플레이어 팀 ID |
| 151 | `1` | AI 팀 ID |

### SpyPlayerController.cpp

| 라인 | 값 | 설명 |
|------|----|------|
| 64 | `-100.f` | 타겟 조준점 Z 오프셋 |
| 68 | `10.f` | 회전 보간 속도 (RInterpTo) |

### SpyGA_Jump.cpp

| 라인 | 값 | 설명 |
|------|----|------|
| 16 | `0.15f` | 점프 종료 판정 지연 시간(초) |

### SpyCharacterAnimInstance.cpp

| 라인 | 값 | 설명 |
|------|----|------|
| 134 | `3.f` | 이동 판정 최소 속도 |

### SpyInputComponent.cpp

| 라인 | 값 | 설명 |
|------|----|------|
| 79 | `[0]` | InputMappingContexts 접근 인덱스 |
| 94 | `[0]` | InputConfig 접근 인덱스 |

### SpyHealCalculation.cpp

| 라인 | 값 | 설명 |
|------|----|------|
| 25 | `1.0f` | 회복량 |

---

## 2. 하드코딩된 문자열

### 에셋 / 클래스명

| 파일 | 값 | 설명 |
|------|----|------|
| `SpyCharacter.cpp` | `"OneHandSword"` | 무기 에셋명 |
| `SpyCharacterAnimInstance.cpp` | `"SpyAnimAssetData"` | 애니메이션 에셋 데이터명 |
| `SpyGameMode.cpp` | `"SpyCharacterAssetData"` | 캐릭터 에셋 데이터명 |
| `SpyGameMode.cpp` | `"SpyCharacter"` | 기본 폰 클래스명 |
| `SpyGameMode.cpp` | `"SpyPlayerController"` | 플레이어 컨트롤러 클래스명 |
| `SpyGameMode.cpp` | `"SpyGameState"` | 게임 스테이트 클래스명 |

### 소켓 / 본 이름

| 파일 | 값 | 설명 |
|------|----|------|
| `SpyCharacter.cpp` | `"weapon_socket"` | 무기 어태치 소켓 |
| `SpyCharacterMovementComponent.cpp` | `"hand_l"`, `"hand_r"` | IK 손 본 이름 |
| `SpyCharacterMovementComponent.cpp` | `"foot_l"`, `"foot_r"` | IK 발 본 이름 |
| `SpyCharacterMovementComponent.cpp` | `"Hand_L_IK_Weight"`, `"Hand_R_IK_Weight"` | 손 IK 웨이트 파라미터명 |
| `SpyCharacterMovementComponent.cpp` | `"Foot_L_IK_Weight"`, `"Foot_R_IK_Weight"` | 발 IK 웨이트 파라미터명 |

### 태그 / 이벤트명

| 파일 | 값 | 설명 |
|------|----|------|
| `SpySpawnBotManagerComponent.cpp` | `"SpawnEnemy"` | 스폰 포인트 액터 태그 |
| `SpyGA_SkillMove_Vault.cpp` | `"AttackNoise"` | AI 노이즈 태그 |
| `BTTask_MoveToTarget.cpp` | `"MoveFinished"` | AI BT 메시지명 |

### 애니메이션 관련

| 파일 | 값 | 설명 |
|------|----|------|
| `SpyCharacterAnimInstance.cpp` | `"OHS"` | 애니메이션 레이어 키 (한손검) |
| `SpyCharacterAnimInstance.cpp` | `"Custom"` | 클라이밍 이동 모드명 |
