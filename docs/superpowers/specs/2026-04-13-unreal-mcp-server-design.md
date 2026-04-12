# Unreal MCP Server 설계 문서

**작성일:** 2026-04-13  
**목적:** Claude가 Unreal Editor에 직접 접근하여 에셋·액터·Python을 조작할 수 있는 범용 MCP 서버 구현

---

## 1. 개요

Python 기반 MCP 서버를 로컬에서 실행하고, Unreal Editor 내장 **Remote Control Plugin**의 HTTP API를 통해 에디터를 제어한다. Claude Code는 MCP(stdio) 프로토콜로 이 서버에 연결하여 툴을 호출한다.

---

## 2. 아키텍처

```
Claude Code
    │  MCP (stdio)
    ▼
unreal-mcp/server.py          ← fastmcp 기반 Python 프로세스
    ├── tools/asset_tools.py        ← 에셋 조회·수정·저장
    ├── tools/actor_tools.py        ← 액터 스폰·삭제·속성 변경
    ├── tools/python_exec_tools.py  ← 임의 Python 실행
    └── tools/spy_asset_tools.py    ← SpyAssetData 전용 툴
    │
    │  HTTP (localhost:30010)
    ▼
Unreal Editor
    └── Remote Control Plugin (내장)
          ├── GET|PUT /remote/object/property  ← 속성 읽기/쓰기
          ├── POST    /remote/object/call       ← 함수 호출
          └── PUT     /remote/exec              ← Python 실행
```

### 디렉터리 구조

```
UnrealSkillProject/
└── tools/
    └── unreal-mcp/
        ├── server.py              (MCP 서버 진입점)
        ├── unreal_client.py       (Remote Control HTTP 클라이언트)
        ├── tools/
        │   ├── __init__.py
        │   ├── asset_tools.py
        │   ├── actor_tools.py
        │   ├── python_exec_tools.py
        │   └── spy_asset_tools.py
        └── requirements.txt
```

---

## 3. Claude Code 등록

`~/.claude/settings.json`에 추가:

```json
{
  "mcpServers": {
    "unreal": {
      "command": "python",
      "args": ["C:/Users/Tae/Desktop/MyProject/SpyProject/UnrealSkillProject/tools/unreal-mcp/server.py"]
    }
  }
}
```

---

## 4. Remote Control API 연동

### 기본 설정
- **Base URL**: `http://localhost:30010`
- **Unreal 플러그인**: `Remote Control API` (엔진 내장, 별도 빌드 불필요)
- **활성화 방법**: Edit → Plugins → "Remote Control API" 검색 → Enable

### 핵심 엔드포인트

| 엔드포인트 | 메서드 | 용도 |
|-----------|--------|------|
| `/remote/object/property` | GET | 오브젝트 속성 읽기 |
| `/remote/object/property` | PUT | 오브젝트 속성 쓰기 |
| `/remote/object/call` | POST | 오브젝트 함수 호출 |
| `/remote/exec` | PUT | Python 스크립트 실행 |

### Python 실행 예시

```json
PUT /remote/exec
{
  "objectPath": "/Script/PythonScriptPlugin.Default__PythonScriptLibrary",
  "functionName": "ExecutePythonScript",
  "parameters": {
    "pythonScript": "import unreal; print(unreal.EditorAssetLibrary.list_assets('/Game/'))"
  }
}
```

---

## 5. MCP 툴 목록

### 에셋 툴 (`asset_tools.py`)

| 툴 이름 | 파라미터 | 설명 |
|---------|----------|------|
| `get_asset_properties` | `asset_path: str` | 에셋 모든 속성 조회 |
| `set_asset_property` | `asset_path, property_name, value` | 특정 속성 변경 |
| `save_asset` | `asset_path: str` | 에셋 저장 |
| `list_assets` | `path: str, class_name: str = ""` | 경로 내 에셋 목록 |
| `find_assets_by_class` | `class_name: str` | 클래스로 에셋 검색 |

### 액터 툴 (`actor_tools.py`)

| 툴 이름 | 파라미터 | 설명 |
|---------|----------|------|
| `get_actors_in_level` | `name_filter: str = ""` | 레벨 내 액터 목록 |
| `get_actor_properties` | `actor_name: str` | 액터 속성 조회 |
| `set_actor_property` | `actor_name, property_name, value` | 액터 속성 변경 |
| `spawn_actor` | `class_path, location, rotation` | 액터 스폰 |
| `delete_actor` | `actor_name: str` | 액터 삭제 |

### Python 실행 툴 (`python_exec_tools.py`)

| 툴 이름 | 파라미터 | 설명 |
|---------|----------|------|
| `execute_python` | `script: str` | 에디터 내부 Python 실행 |

### SpyAssetData 전용 툴 (`spy_asset_tools.py`)

| 툴 이름 | 파라미터 | 설명 |
|---------|----------|------|
| `get_spy_asset_data` | `asset_path: str` | 전체 그룹/엔트리 조회 |
| `add_asset_group` | `asset_path, group_name` | 새 그룹 추가 |
| `add_asset_entry` | `asset_path, group_name, entry_name, entry_path` | 엔트리 추가 |
| `set_asset_entry` | `asset_path, group_name, entry_name, entry_path` | 엔트리 경로 수정 |
| `remove_asset_entry` | `asset_path, group_name, entry_name` | 엔트리 삭제 |
| `save_spy_asset_data` | `asset_path: str` | 변경사항 저장 |

---

## 6. 데이터 흐름

### SpyAssetData 엔트리 추가 예시

```
Claude: "Character 그룹에 Player 엔트리 추가해줘"
    │
    ▼
spy_asset_tools.add_asset_entry(
    asset_path="/Game/Data/SpyAssetData",
    group_name="Character",
    entry_name="Player",
    entry_path="/Game/Characters/Player/BP_Player"
)
    │
    ▼ Python 스크립트 생성 후 execute_python()으로 실행
    │
    ▼ Unreal Remote Control → ExecutePythonScript
    │
    ▼ {"result": "success", "group": "Character", "entry": "Player"}
```

### SpyAssetData Python 조작 패턴

`AssetGroupNameToSet`은 UPROPERTY이지만 구조체 중첩으로 Remote Control 직접 조작이 어려울 수 있어, Python 실행으로 통일:

```python
import unreal

asset = unreal.load_asset('/Game/Data/SpyAssetData')
# 속성 접근 및 수정
unreal.EditorAssetLibrary.save_asset('/Game/Data/SpyAssetData')
```

---

## 7. 에러 처리

| 상황 | 반환 메시지 |
|------|------------|
| 에디터 미실행 | `"Unreal Editor가 실행 중이지 않습니다. 에디터를 먼저 실행해주세요."` |
| Remote Control 미활성화 | `"Remote Control Plugin이 비활성화 상태입니다. Edit → Plugins → Remote Control API를 활성화하세요."` |
| 잘못된 에셋 경로 | Unreal 에러 메시지 그대로 반환 |
| Python 스크립트 오류 | 스크립트 출력 + 오류 메시지 반환 |

---

## 8. 스코프 아웃 (이번 구현에서 제외)

- 인증/보안 (로컬 전용이므로 불필요)
- 멀티 에디터 인스턴스 지원
- Unreal 플러그인 커스텀 빌드
- 실시간 이벤트 구독 (OnActorSpawned 등)
