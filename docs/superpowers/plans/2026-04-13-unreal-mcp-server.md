# Unreal MCP Server Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Claude가 Unreal Editor에 직접 접근하여 에셋·액터·Python을 조작할 수 있는 Python 기반 MCP 서버 구현

**Architecture:** fastmcp으로 MCP 서버를 구성하고, Unreal Editor 내장 Remote Control Plugin의 HTTP API(localhost:30010)를 통해 에디터를 제어한다. 쿼리 결과는 임시 파일(JSON)을 통해 Unreal Python → MCP 서버로 전달한다.

**Tech Stack:** Python 3.14, fastmcp 1.0, httpx, Unreal Remote Control Plugin (UE 5.7 내장)

---

## File Map

### 신규 생성
| 파일 | 역할 |
|------|------|
| `tools/unreal-mcp/requirements.txt` | 의존성 목록 |
| `tools/unreal-mcp/unreal_client.py` | Remote Control HTTP 클라이언트 |
| `tools/unreal-mcp/tools/__init__.py` | 패키지 마커 |
| `tools/unreal-mcp/tools/asset_tools.py` | 에셋 조회·수정·저장 로직 |
| `tools/unreal-mcp/tools/actor_tools.py` | 액터 조회·스폰·삭제 로직 |
| `tools/unreal-mcp/tools/python_exec_tools.py` | 임의 Python 실행 로직 |
| `tools/unreal-mcp/tools/spy_asset_tools.py` | SpyAssetData 전용 CRUD 로직 |
| `tools/unreal-mcp/server.py` | MCP 서버 진입점, 모든 툴 등록 |
| `tools/unreal-mcp/tests/__init__.py` | 테스트 패키지 마커 |
| `tools/unreal-mcp/tests/test_unreal_client.py` | UnrealClient 단위 테스트 |
| `tools/unreal-mcp/tests/test_asset_tools.py` | asset_tools 단위 테스트 |
| `tools/unreal-mcp/tests/test_actor_tools.py` | actor_tools 단위 테스트 |
| `tools/unreal-mcp/tests/test_python_exec_tools.py` | python_exec_tools 단위 테스트 |
| `tools/unreal-mcp/tests/test_spy_asset_tools.py` | spy_asset_tools 단위 테스트 |

### 수정
| 파일 | 변경 내용 |
|------|----------|
| `~/.claude/settings.json` | mcpServers에 unreal 서버 등록 |

---

## Task 1: 프로젝트 구조 및 의존성 설정

**Files:**
- Create: `tools/unreal-mcp/requirements.txt`
- Create: `tools/unreal-mcp/tools/__init__.py`
- Create: `tools/unreal-mcp/tests/__init__.py`

- [ ] **Step 1: 디렉터리 생성**

```bash
mkdir -p "SkillProject/../tools/unreal-mcp/tools"
mkdir -p "SkillProject/../tools/unreal-mcp/tests"
```

작업 디렉터리: `C:/Users/Tae/Desktop/MyProject/SpyProject/UnrealSkillProject`

- [ ] **Step 2: requirements.txt 생성**

`tools/unreal-mcp/requirements.txt`:
```
httpx>=0.27.0
fastmcp>=1.0
pytest>=8.0.0
```

- [ ] **Step 3: 패키지 마커 생성**

`tools/unreal-mcp/tools/__init__.py`: (빈 파일)

`tools/unreal-mcp/tests/__init__.py`: (빈 파일)

- [ ] **Step 4: 의존성 확인**

```bash
pip show httpx fastmcp pytest
```

Expected: 세 패키지 모두 설치됨 확인. 없으면 `pip install -r tools/unreal-mcp/requirements.txt` 실행.

- [ ] **Step 5: 커밋**

```bash
git add tools/unreal-mcp/
git commit -m "feat: unreal-mcp 프로젝트 구조 초기화"
```

---

## Task 2: UnrealClient 구현

**Files:**
- Create: `tools/unreal-mcp/unreal_client.py`
- Create: `tools/unreal-mcp/tests/test_unreal_client.py`

Remote Control HTTP API와 통신하는 클라이언트. 연결 실패 시 명확한 에러 메시지를 반환한다.

- [ ] **Step 1: 테스트 파일 작성**

`tools/unreal-mcp/tests/test_unreal_client.py`:
```python
import pytest
from unittest.mock import MagicMock, patch
import httpx
from unreal_client import UnrealClient, UnrealConnectionError

BASE_URL = "http://localhost:30010"


def make_client():
    return UnrealClient(base_url=BASE_URL)


def mock_response(json_data: dict, status_code: int = 200) -> MagicMock:
    resp = MagicMock()
    resp.json.return_value = json_data
    resp.status_code = status_code
    resp.raise_for_status = MagicMock()
    return resp


class TestConnectionCheck:
    def test_raises_on_connect_error(self):
        client = make_client()
        with patch.object(client._http, "get", side_effect=httpx.ConnectError("refused")):
            with pytest.raises(UnrealConnectionError) as exc_info:
                client._check_connection()
            assert "실행 중이지 않습니다" in str(exc_info.value)

    def test_passes_when_connected(self):
        client = make_client()
        with patch.object(client._http, "get", return_value=mock_response({})):
            client._check_connection()  # should not raise


class TestGetProperty:
    def test_calls_correct_endpoint(self):
        client = make_client()
        expected = {"propertyValue": {"AssetGroupNameToSet": {}}}
        with patch.object(client._http, "get", return_value=mock_response(expected)) as mock_get, \
             patch.object(client, "_check_connection"):
            result = client.get_property("/Game/Data/DA_SpyAssetData", "AssetGroupNameToSet")
        mock_get.assert_called_once_with(
            f"{BASE_URL}/remote/object/property",
            params={"objectPath": "/Game/Data/DA_SpyAssetData", "propertyName": "AssetGroupNameToSet"}
        )
        assert result == expected

    def test_raises_on_http_error(self):
        client = make_client()
        err_resp = mock_response({}, status_code=404)
        err_resp.raise_for_status.side_effect = httpx.HTTPStatusError("not found", request=MagicMock(), response=err_resp)
        with patch.object(client._http, "get", return_value=err_resp), \
             patch.object(client, "_check_connection"):
            with pytest.raises(httpx.HTTPStatusError):
                client.get_property("/bad/path", "prop")


class TestExecutePython:
    def test_calls_object_call_endpoint(self):
        client = make_client()
        expected = {"returnValues": {}}
        with patch.object(client._http, "post", return_value=mock_response(expected)) as mock_post, \
             patch.object(client, "_check_connection"):
            result = client.execute_python("import unreal")
        mock_post.assert_called_once_with(
            f"{BASE_URL}/remote/object/call",
            json={
                "objectPath": "/Script/PythonScriptPlugin.Default__PythonScriptLibrary",
                "functionName": "ExecutePythonScript",
                "parameters": {"pythonScript": "import unreal"}
            }
        )
        assert result == expected
```

- [ ] **Step 2: 테스트 실행 — FAIL 확인**

```bash
cd tools/unreal-mcp
python -m pytest tests/test_unreal_client.py -v
```

Expected: `ModuleNotFoundError: No module named 'unreal_client'`

- [ ] **Step 3: unreal_client.py 구현**

`tools/unreal-mcp/unreal_client.py`:
```python
import httpx
from typing import Any


REMOTE_CONTROL_URL = "http://localhost:30010"


class UnrealConnectionError(Exception):
    """Unreal Editor에 연결할 수 없을 때 발생"""
    pass


class UnrealClient:
    def __init__(self, base_url: str = REMOTE_CONTROL_URL):
        self.base_url = base_url.rstrip("/")
        self._http = httpx.Client(timeout=10.0)

    def _check_connection(self) -> None:
        try:
            self._http.get(f"{self.base_url}/remote/info", timeout=2.0)
        except httpx.ConnectError:
            raise UnrealConnectionError(
                "Unreal Editor가 실행 중이지 않습니다. 에디터를 먼저 실행하고 "
                "Edit → Plugins → Remote Control API를 활성화하세요."
            )

    def get_property(self, object_path: str, property_name: str) -> dict:
        self._check_connection()
        response = self._http.get(
            f"{self.base_url}/remote/object/property",
            params={"objectPath": object_path, "propertyName": property_name}
        )
        response.raise_for_status()
        return response.json()

    def set_property(self, object_path: str, property_name: str, value: Any) -> dict:
        self._check_connection()
        response = self._http.put(
            f"{self.base_url}/remote/object/property",
            json={
                "objectPath": object_path,
                "propertyName": property_name,
                "propertyValue": {property_name: value}
            }
        )
        response.raise_for_status()
        return response.json()

    def call_function(self, object_path: str, function_name: str, parameters: dict | None = None) -> dict:
        self._check_connection()
        response = self._http.post(
            f"{self.base_url}/remote/object/call",
            json={
                "objectPath": object_path,
                "functionName": function_name,
                "parameters": parameters or {}
            }
        )
        response.raise_for_status()
        return response.json()

    def execute_python(self, script: str) -> dict:
        self._check_connection()
        response = self._http.post(
            f"{self.base_url}/remote/object/call",
            json={
                "objectPath": "/Script/PythonScriptPlugin.Default__PythonScriptLibrary",
                "functionName": "ExecutePythonScript",
                "parameters": {"pythonScript": script}
            }
        )
        response.raise_for_status()
        return response.json()
```

- [ ] **Step 4: 테스트 실행 — PASS 확인**

```bash
python -m pytest tests/test_unreal_client.py -v
```

Expected: `7 passed`

- [ ] **Step 5: 커밋**

```bash
git add tools/unreal-mcp/unreal_client.py tools/unreal-mcp/tests/test_unreal_client.py
git commit -m "feat: UnrealClient Remote Control HTTP 클라이언트 구현"
```

---

## Task 3: asset_tools 구현

**Files:**
- Create: `tools/unreal-mcp/tools/asset_tools.py`
- Create: `tools/unreal-mcp/tests/test_asset_tools.py`

Python 실행으로 에셋 목록·속성 조회 및 저장. 쿼리 결과는 임시 파일로 전달.

- [ ] **Step 1: 테스트 파일 작성**

`tools/unreal-mcp/tests/test_asset_tools.py`:
```python
import json
import os
import tempfile
from unittest.mock import MagicMock, patch, mock_open
import pytest
from tools.asset_tools import (
    RESULT_FILE, get_asset_properties, set_asset_property,
    save_asset, list_assets, find_assets_by_class
)


def make_client():
    client = MagicMock()
    client.execute_python = MagicMock(return_value={})
    return client


class TestGetAssetProperties:
    def test_executes_python_and_reads_result(self, tmp_path):
        client = make_client()
        expected = {"SomeProperty": "value"}
        result_path = str(tmp_path / "result.json")
        with patch("tools.asset_tools.RESULT_FILE", result_path):
            with open(result_path, "w") as f:
                json.dump(expected, f)
            with patch.object(client, "execute_python", return_value={}):
                result = get_asset_properties(client, "/Game/Data/DA_Test")
        assert result == expected

    def test_python_script_contains_asset_path(self, tmp_path):
        client = make_client()
        result_path = str(tmp_path / "result.json")
        with open(result_path, "w") as f:
            json.dump({}, f)
        with patch("tools.asset_tools.RESULT_FILE", result_path):
            get_asset_properties(client, "/Game/Data/DA_Test")
        script = client.execute_python.call_args[0][0]
        assert "/Game/Data/DA_Test" in script


class TestSaveAsset:
    def test_executes_python_with_asset_path(self):
        client = make_client()
        save_asset(client, "/Game/Data/DA_Test")
        script = client.execute_python.call_args[0][0]
        assert "/Game/Data/DA_Test" in script
        assert "save_asset" in script


class TestListAssets:
    def test_passes_path_and_class_filter(self, tmp_path):
        client = make_client()
        result_path = str(tmp_path / "result.json")
        with open(result_path, "w") as f:
            json.dump({"assets": []}, f)
        with patch("tools.asset_tools.RESULT_FILE", result_path):
            list_assets(client, "/Game/Data", class_name="DataAsset")
        script = client.execute_python.call_args[0][0]
        assert "/Game/Data" in script
        assert "DataAsset" in script
```

- [ ] **Step 2: 테스트 실행 — FAIL 확인**

```bash
python -m pytest tests/test_asset_tools.py -v
```

Expected: `ModuleNotFoundError: No module named 'tools.asset_tools'`

- [ ] **Step 3: asset_tools.py 구현**

`tools/unreal-mcp/tools/asset_tools.py`:
```python
import json
import os
import tempfile
from unreal_client import UnrealClient

RESULT_FILE = os.path.join(tempfile.gettempdir(), "unreal_mcp_output.json")


def _read_result() -> dict:
    with open(RESULT_FILE, "r", encoding="utf-8") as f:
        return json.load(f)


def get_asset_properties(client: UnrealClient, asset_path: str) -> dict:
    result_file = RESULT_FILE.replace("\\", "/")
    script = f"""
import unreal, json
asset = unreal.load_asset({asset_path!r})
result = {{}}
if asset is None:
    result = {{"error": "에셋을 찾을 수 없습니다: {asset_path}"}}
else:
    for prop in asset.get_class().iterate_properties():
        try:
            result[prop.name] = str(asset.get_editor_property(prop.name))
        except Exception:
            pass
with open({result_file!r}, 'w', encoding='utf-8') as f:
    json.dump(result, f)
"""
    client.execute_python(script.strip())
    return _read_result()


def set_asset_property(client: UnrealClient, asset_path: str, property_name: str, value: str) -> dict:
    script = f"""
import unreal
asset = unreal.load_asset({asset_path!r})
if asset is not None:
    asset.set_editor_property({property_name!r}, {value})
    unreal.EditorAssetLibrary.save_asset({asset_path!r})
"""
    client.execute_python(script.strip())
    return {"success": True, "asset": asset_path, "property": property_name}


def save_asset(client: UnrealClient, asset_path: str) -> dict:
    script = f"""
import unreal
unreal.EditorAssetLibrary.save_asset({asset_path!r})
"""
    client.execute_python(script.strip())
    return {"success": True, "asset": asset_path}


def list_assets(client: UnrealClient, path: str, class_name: str = "") -> dict:
    result_file = RESULT_FILE.replace("\\", "/")
    class_filter = f", class_names=[{class_name!r}]" if class_name else ""
    script = f"""
import unreal, json
assets = unreal.EditorAssetLibrary.list_assets({path!r}, recursive=True{class_filter})
with open({result_file!r}, 'w', encoding='utf-8') as f:
    json.dump({{"assets": list(assets)}}, f)
"""
    client.execute_python(script.strip())
    return _read_result()


def find_assets_by_class(client: UnrealClient, class_name: str) -> dict:
    result_file = RESULT_FILE.replace("\\", "/")
    script = f"""
import unreal, json
registry = unreal.AssetRegistryHelpers.get_asset_registry()
assets = registry.get_assets_by_class({class_name!r})
paths = [str(a.object_path) for a in assets]
with open({result_file!r}, 'w', encoding='utf-8') as f:
    json.dump({{"assets": paths}}, f)
"""
    client.execute_python(script.strip())
    return _read_result()
```

- [ ] **Step 4: 테스트 실행 — PASS 확인**

```bash
python -m pytest tests/test_asset_tools.py -v
```

Expected: `7 passed`

- [ ] **Step 5: 커밋**

```bash
git add tools/unreal-mcp/tools/asset_tools.py tools/unreal-mcp/tests/test_asset_tools.py
git commit -m "feat: asset_tools 에셋 조회·수정·저장 구현"
```

---

## Task 4: actor_tools 구현

**Files:**
- Create: `tools/unreal-mcp/tools/actor_tools.py`
- Create: `tools/unreal-mcp/tests/test_actor_tools.py`

- [ ] **Step 1: 테스트 파일 작성**

`tools/unreal-mcp/tests/test_actor_tools.py`:
```python
import json
from unittest.mock import MagicMock, patch
import pytest
from tools.actor_tools import (
    get_actors_in_level, get_actor_properties,
    set_actor_property, spawn_actor, delete_actor
)


def make_client():
    client = MagicMock()
    client.execute_python = MagicMock(return_value={})
    return client


class TestGetActorsInLevel:
    def test_script_contains_name_filter(self, tmp_path):
        client = make_client()
        result_path = str(tmp_path / "result.json")
        with open(result_path, "w") as f:
            json.dump({"actors": []}, f)
        with patch("tools.actor_tools.RESULT_FILE", result_path):
            get_actors_in_level(client, name_filter="TargetPoint")
        script = client.execute_python.call_args[0][0]
        assert "TargetPoint" in script

    def test_no_filter_returns_all(self, tmp_path):
        client = make_client()
        result_path = str(tmp_path / "result.json")
        actors = [{"name": "Actor1", "class": "AActor", "location": {"x": 0, "y": 0, "z": 0}}]
        with open(result_path, "w") as f:
            json.dump({"actors": actors}, f)
        with patch("tools.actor_tools.RESULT_FILE", result_path):
            result = get_actors_in_level(client)
        assert result == {"actors": actors}


class TestSpawnActor:
    def test_script_contains_class_and_location(self):
        client = make_client()
        spawn_actor(client, "/Game/BP_Enemy", (100.0, 200.0, 0.0))
        script = client.execute_python.call_args[0][0]
        assert "/Game/BP_Enemy" in script
        assert "100.0" in script
        assert "200.0" in script

    def test_returns_success(self):
        client = make_client()
        result = spawn_actor(client, "/Game/BP_Enemy", (0, 0, 0))
        assert result["success"] is True


class TestDeleteActor:
    def test_script_contains_actor_name(self):
        client = make_client()
        delete_actor(client, "BP_Enemy_0")
        script = client.execute_python.call_args[0][0]
        assert "BP_Enemy_0" in script

    def test_returns_success(self):
        client = make_client()
        result = delete_actor(client, "BP_Enemy_0")
        assert result["success"] is True
```

- [ ] **Step 2: 테스트 실행 — FAIL 확인**

```bash
python -m pytest tests/test_actor_tools.py -v
```

Expected: `ModuleNotFoundError: No module named 'tools.actor_tools'`

- [ ] **Step 3: actor_tools.py 구현**

`tools/unreal-mcp/tools/actor_tools.py`:
```python
import json
import os
import tempfile
from unreal_client import UnrealClient

RESULT_FILE = os.path.join(tempfile.gettempdir(), "unreal_mcp_output.json")


def _read_result() -> dict:
    with open(RESULT_FILE, "r", encoding="utf-8") as f:
        return json.load(f)


def get_actors_in_level(client: UnrealClient, name_filter: str = "") -> dict:
    result_file = RESULT_FILE.replace("\\", "/")
    script = f"""
import unreal, json
name_filter = {name_filter!r}
world = unreal.EditorLevelLibrary.get_editor_world()
all_actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)
result = []
for actor in all_actors:
    name = actor.get_name()
    if not name_filter or name_filter.lower() in name.lower():
        loc = actor.get_actor_location()
        result.append({{
            'name': name,
            'class': actor.get_class().get_name(),
            'location': {{'x': loc.x, 'y': loc.y, 'z': loc.z}}
        }})
with open({result_file!r}, 'w', encoding='utf-8') as f:
    json.dump({{'actors': result}}, f)
"""
    client.execute_python(script.strip())
    return _read_result()


def get_actor_properties(client: UnrealClient, actor_name: str) -> dict:
    result_file = RESULT_FILE.replace("\\", "/")
    script = f"""
import unreal, json
world = unreal.EditorLevelLibrary.get_editor_world()
all_actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)
result = {{"error": "액터를 찾을 수 없습니다: {actor_name}"}}
for actor in all_actors:
    if actor.get_name() == {actor_name!r}:
        props = {{}}
        for prop in actor.get_class().iterate_properties():
            try:
                props[prop.name] = str(actor.get_editor_property(prop.name))
            except Exception:
                pass
        result = {{"name": actor.get_name(), "properties": props}}
        break
with open({result_file!r}, 'w', encoding='utf-8') as f:
    json.dump(result, f)
"""
    client.execute_python(script.strip())
    return _read_result()


def set_actor_property(client: UnrealClient, actor_name: str, property_name: str, value: str) -> dict:
    script = f"""
import unreal
world = unreal.EditorLevelLibrary.get_editor_world()
all_actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)
for actor in all_actors:
    if actor.get_name() == {actor_name!r}:
        actor.set_editor_property({property_name!r}, {value})
        break
"""
    client.execute_python(script.strip())
    return {"success": True, "actor": actor_name, "property": property_name}


def spawn_actor(client: UnrealClient, class_path: str, location: tuple, rotation: tuple = (0.0, 0.0, 0.0)) -> dict:
    x, y, z = location
    rx, ry, rz = rotation
    script = f"""
import unreal
actor_class = unreal.load_class(None, {class_path!r})
loc = unreal.Vector({x}, {y}, {z})
rot = unreal.Rotator({rx}, {ry}, {rz})
unreal.EditorLevelLibrary.spawn_actor_from_class(actor_class, loc, rot)
"""
    client.execute_python(script.strip())
    return {"success": True, "class": class_path, "location": {"x": x, "y": y, "z": z}}


def delete_actor(client: UnrealClient, actor_name: str) -> dict:
    script = f"""
import unreal
world = unreal.EditorLevelLibrary.get_editor_world()
all_actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)
for actor in all_actors:
    if actor.get_name() == {actor_name!r}:
        unreal.EditorLevelLibrary.destroy_actor(actor)
        break
"""
    client.execute_python(script.strip())
    return {"success": True, "actor": actor_name}
```

- [ ] **Step 4: 테스트 실행 — PASS 확인**

```bash
python -m pytest tests/test_actor_tools.py -v
```

Expected: `8 passed`

- [ ] **Step 5: 커밋**

```bash
git add tools/unreal-mcp/tools/actor_tools.py tools/unreal-mcp/tests/test_actor_tools.py
git commit -m "feat: actor_tools 액터 조회·스폰·삭제 구현"
```

---

## Task 5: python_exec_tools 구현

**Files:**
- Create: `tools/unreal-mcp/tools/python_exec_tools.py`
- Create: `tools/unreal-mcp/tests/test_python_exec_tools.py`

- [ ] **Step 1: 테스트 파일 작성**

`tools/unreal-mcp/tests/test_python_exec_tools.py`:
```python
from unittest.mock import MagicMock
from tools.python_exec_tools import execute_python


def make_client():
    client = MagicMock()
    client.execute_python = MagicMock(return_value={"returnValues": {}})
    return client


class TestExecutePython:
    def test_passes_script_to_client(self):
        client = make_client()
        script = "import unreal; print(unreal.__version__)"
        execute_python(client, script)
        client.execute_python.assert_called_once_with(script)

    def test_returns_client_result(self):
        client = make_client()
        client.execute_python.return_value = {"returnValues": {"output": "3.14"}}
        result = execute_python(client, "print(3.14)")
        assert result == {"returnValues": {"output": "3.14"}}
```

- [ ] **Step 2: 테스트 실행 — FAIL 확인**

```bash
python -m pytest tests/test_python_exec_tools.py -v
```

Expected: `ModuleNotFoundError: No module named 'tools.python_exec_tools'`

- [ ] **Step 3: python_exec_tools.py 구현**

`tools/unreal-mcp/tools/python_exec_tools.py`:
```python
from unreal_client import UnrealClient


def execute_python(client: UnrealClient, script: str) -> dict:
    return client.execute_python(script)
```

- [ ] **Step 4: 테스트 실행 — PASS 확인**

```bash
python -m pytest tests/test_python_exec_tools.py -v
```

Expected: `2 passed`

- [ ] **Step 5: 커밋**

```bash
git add tools/unreal-mcp/tools/python_exec_tools.py tools/unreal-mcp/tests/test_python_exec_tools.py
git commit -m "feat: python_exec_tools 임의 Python 실행 구현"
```

---

## Task 6: spy_asset_tools 구현

**Files:**
- Create: `tools/unreal-mcp/tools/spy_asset_tools.py`
- Create: `tools/unreal-mcp/tests/test_spy_asset_tools.py`

SpyAssetData 전용 CRUD. `AssetGroupNameToSet`은 `UPROPERTY(EditDefaultsOnly)`이므로 Python의 `get_editor_property('asset_group_name_to_set')`으로 접근한다.

- [ ] **Step 1: 테스트 파일 작성**

`tools/unreal-mcp/tests/test_spy_asset_tools.py`:
```python
import json
from unittest.mock import MagicMock, patch
import pytest
from tools.spy_asset_tools import (
    get_spy_asset_data, add_asset_group, add_asset_entry,
    set_asset_entry, remove_asset_entry, save_spy_asset_data
)

ASSET_PATH = "/Game/Data/DA_SpyAssetData"


def make_client():
    client = MagicMock()
    client.execute_python = MagicMock(return_value={})
    return client


class TestGetSpyAssetData:
    def test_reads_result_from_temp_file(self, tmp_path):
        client = make_client()
        result_path = str(tmp_path / "result.json")
        expected = {"Character": [{"name": "Player", "path": "/Game/BP_Player"}]}
        with open(result_path, "w") as f:
            json.dump(expected, f)
        with patch("tools.spy_asset_tools.RESULT_FILE", result_path):
            result = get_spy_asset_data(client, ASSET_PATH)
        assert result == expected

    def test_script_contains_asset_path(self, tmp_path):
        client = make_client()
        result_path = str(tmp_path / "result.json")
        with open(result_path, "w") as f:
            json.dump({}, f)
        with patch("tools.spy_asset_tools.RESULT_FILE", result_path):
            get_spy_asset_data(client, ASSET_PATH)
        script = client.execute_python.call_args[0][0]
        assert ASSET_PATH in script
        assert "asset_group_name_to_set" in script


class TestAddAssetGroup:
    def test_script_contains_group_name(self):
        client = make_client()
        add_asset_group(client, ASSET_PATH, "NewGroup")
        script = client.execute_python.call_args[0][0]
        assert "NewGroup" in script
        assert ASSET_PATH in script

    def test_returns_success(self):
        client = make_client()
        result = add_asset_group(client, ASSET_PATH, "NewGroup")
        assert result["success"] is True
        assert result["group"] == "NewGroup"


class TestAddAssetEntry:
    def test_script_contains_all_params(self):
        client = make_client()
        add_asset_entry(client, ASSET_PATH, "Character", "Player", "/Game/BP_Player")
        script = client.execute_python.call_args[0][0]
        assert "Character" in script
        assert "Player" in script
        assert "/Game/BP_Player" in script

    def test_returns_success(self):
        client = make_client()
        result = add_asset_entry(client, ASSET_PATH, "Character", "Player", "/Game/BP_Player")
        assert result["success"] is True
        assert result["entry"] == "Player"


class TestRemoveAssetEntry:
    def test_script_contains_group_and_entry(self):
        client = make_client()
        remove_asset_entry(client, ASSET_PATH, "Character", "Player")
        script = client.execute_python.call_args[0][0]
        assert "Character" in script
        assert "Player" in script

    def test_returns_success(self):
        client = make_client()
        result = remove_asset_entry(client, ASSET_PATH, "Character", "Player")
        assert result["success"] is True


class TestSaveSpyAssetData:
    def test_script_contains_asset_path(self):
        client = make_client()
        save_spy_asset_data(client, ASSET_PATH)
        script = client.execute_python.call_args[0][0]
        assert ASSET_PATH in script
        assert "save_asset" in script
```

- [ ] **Step 2: 테스트 실행 — FAIL 확인**

```bash
python -m pytest tests/test_spy_asset_tools.py -v
```

Expected: `ModuleNotFoundError: No module named 'tools.spy_asset_tools'`

- [ ] **Step 3: spy_asset_tools.py 구현**

`tools/unreal-mcp/tools/spy_asset_tools.py`:
```python
import json
import os
import tempfile
from unreal_client import UnrealClient

RESULT_FILE = os.path.join(tempfile.gettempdir(), "unreal_mcp_output.json")


def _read_result() -> dict:
    with open(RESULT_FILE, "r", encoding="utf-8") as f:
        return json.load(f)


def get_spy_asset_data(client: UnrealClient, asset_path: str) -> dict:
    result_file = RESULT_FILE.replace("\\", "/")
    script = f"""
import unreal, json
asset = unreal.load_asset({asset_path!r})
groups = asset.get_editor_property('asset_group_name_to_set')
result = {{}}
for group_name, asset_set in groups.items():
    entries = []
    for entry in asset_set.get_editor_property('asset_entries'):
        entries.append({{
            'name': str(entry.get_editor_property('asset_name')),
            'path': str(entry.get_editor_property('asset_path'))
        }})
    result[str(group_name)] = entries
with open({result_file!r}, 'w', encoding='utf-8') as f:
    json.dump(result, f)
"""
    client.execute_python(script.strip())
    return _read_result()


def add_asset_group(client: UnrealClient, asset_path: str, group_name: str) -> dict:
    script = f"""
import unreal
asset = unreal.load_asset({asset_path!r})
groups = asset.get_editor_property('asset_group_name_to_set')
group_keys = [str(k) for k in groups.keys()]
if {group_name!r} not in group_keys:
    new_set = unreal.AssetSet()
    new_set.set_editor_property('asset_entries', [])
    groups[{group_name!r}] = new_set
    asset.set_editor_property('asset_group_name_to_set', groups)
"""
    client.execute_python(script.strip())
    return {"success": True, "group": group_name}


def add_asset_entry(client: UnrealClient, asset_path: str, group_name: str,
                    entry_name: str, entry_path: str) -> dict:
    script = f"""
import unreal
asset = unreal.load_asset({asset_path!r})
groups = asset.get_editor_property('asset_group_name_to_set')
asset_set = groups[{group_name!r}]
entries = list(asset_set.get_editor_property('asset_entries'))
new_entry = unreal.AssetEntry()
new_entry.set_editor_property('asset_name', {entry_name!r})
new_entry.set_editor_property('asset_path', unreal.SoftObjectPath({entry_path!r}))
entries.append(new_entry)
asset_set.set_editor_property('asset_entries', entries)
"""
    client.execute_python(script.strip())
    return {"success": True, "group": group_name, "entry": entry_name}


def set_asset_entry(client: UnrealClient, asset_path: str, group_name: str,
                    entry_name: str, entry_path: str) -> dict:
    script = f"""
import unreal
asset = unreal.load_asset({asset_path!r})
groups = asset.get_editor_property('asset_group_name_to_set')
asset_set = groups[{group_name!r}]
entries = list(asset_set.get_editor_property('asset_entries'))
for entry in entries:
    if str(entry.get_editor_property('asset_name')) == {entry_name!r}:
        entry.set_editor_property('asset_path', unreal.SoftObjectPath({entry_path!r}))
        break
asset_set.set_editor_property('asset_entries', entries)
"""
    client.execute_python(script.strip())
    return {"success": True, "group": group_name, "entry": entry_name, "path": entry_path}


def remove_asset_entry(client: UnrealClient, asset_path: str, group_name: str, entry_name: str) -> dict:
    script = f"""
import unreal
asset = unreal.load_asset({asset_path!r})
groups = asset.get_editor_property('asset_group_name_to_set')
asset_set = groups[{group_name!r}]
entries = list(asset_set.get_editor_property('asset_entries'))
entries = [e for e in entries if str(e.get_editor_property('asset_name')) != {entry_name!r}]
asset_set.set_editor_property('asset_entries', entries)
"""
    client.execute_python(script.strip())
    return {"success": True, "group": group_name, "entry": entry_name}


def save_spy_asset_data(client: UnrealClient, asset_path: str) -> dict:
    script = f"""
import unreal
unreal.EditorAssetLibrary.save_asset({asset_path!r})
"""
    client.execute_python(script.strip())
    return {"success": True, "asset": asset_path}
```

- [ ] **Step 4: 테스트 실행 — PASS 확인**

```bash
python -m pytest tests/test_spy_asset_tools.py -v
```

Expected: `12 passed`

- [ ] **Step 5: 커밋**

```bash
git add tools/unreal-mcp/tools/spy_asset_tools.py tools/unreal-mcp/tests/test_spy_asset_tools.py
git commit -m "feat: spy_asset_tools SpyAssetData CRUD 구현"
```

---

## Task 7: server.py — MCP 서버 진입점

**Files:**
- Create: `tools/unreal-mcp/server.py`

모든 툴을 fastmcp에 등록한다. 각 툴은 `UnrealConnectionError` 를 잡아 사용자 친화적인 메시지로 반환한다.

- [ ] **Step 1: server.py 작성**

`tools/unreal-mcp/server.py`:
```python
import sys
import os

# 패키지 루트를 sys.path에 추가
sys.path.insert(0, os.path.dirname(__file__))

from fastmcp import FastMCP
from unreal_client import UnrealClient, UnrealConnectionError
import tools.asset_tools as asset_tools
import tools.actor_tools as actor_tools
import tools.python_exec_tools as python_exec_tools
import tools.spy_asset_tools as spy_asset_tools

mcp = FastMCP("unreal")
_client = UnrealClient()


def _safe(func, *args, **kwargs) -> str:
    try:
        result = func(_client, *args, **kwargs)
        import json
        return json.dumps(result, ensure_ascii=False, indent=2)
    except UnrealConnectionError as e:
        return str(e)
    except Exception as e:
        return f"오류: {e}"


# ── 에셋 툴 ──────────────────────────────────────────────

@mcp.tool()
def get_asset_properties(asset_path: str) -> str:
    """에셋의 모든 속성을 조회합니다. asset_path: /Game/... 형식"""
    return _safe(asset_tools.get_asset_properties, asset_path)


@mcp.tool()
def set_asset_property(asset_path: str, property_name: str, value: str) -> str:
    """에셋의 특정 속성을 변경하고 저장합니다."""
    return _safe(asset_tools.set_asset_property, asset_path, property_name, value)


@mcp.tool()
def save_asset(asset_path: str) -> str:
    """에셋을 저장합니다."""
    return _safe(asset_tools.save_asset, asset_path)


@mcp.tool()
def list_assets(path: str, class_name: str = "") -> str:
    """지정 경로의 에셋 목록을 반환합니다. class_name으로 필터링 가능."""
    return _safe(asset_tools.list_assets, path, class_name)


@mcp.tool()
def find_assets_by_class(class_name: str) -> str:
    """클래스 이름으로 에셋을 검색합니다."""
    return _safe(asset_tools.find_assets_by_class, class_name)


# ── 액터 툴 ──────────────────────────────────────────────

@mcp.tool()
def get_actors_in_level(name_filter: str = "") -> str:
    """현재 레벨의 액터 목록을 반환합니다. name_filter로 이름 필터링 가능."""
    return _safe(actor_tools.get_actors_in_level, name_filter)


@mcp.tool()
def get_actor_properties(actor_name: str) -> str:
    """액터의 속성을 조회합니다."""
    return _safe(actor_tools.get_actor_properties, actor_name)


@mcp.tool()
def set_actor_property(actor_name: str, property_name: str, value: str) -> str:
    """액터의 속성을 변경합니다."""
    return _safe(actor_tools.set_actor_property, actor_name, property_name, value)


@mcp.tool()
def spawn_actor(class_path: str, x: float, y: float, z: float) -> str:
    """액터를 스폰합니다. class_path: /Game/... 형식, x/y/z: 월드 좌표."""
    return _safe(actor_tools.spawn_actor, class_path, (x, y, z))


@mcp.tool()
def delete_actor(actor_name: str) -> str:
    """액터를 삭제합니다."""
    return _safe(actor_tools.delete_actor, actor_name)


# ── Python 실행 툴 ────────────────────────────────────────

@mcp.tool()
def execute_python(script: str) -> str:
    """Unreal 에디터 내부에서 임의의 Python 스크립트를 실행합니다."""
    return _safe(python_exec_tools.execute_python, script)


# ── SpyAssetData 전용 툴 ──────────────────────────────────

@mcp.tool()
def get_spy_asset_data(asset_path: str) -> str:
    """SpyAssetData의 전체 그룹/엔트리를 조회합니다."""
    return _safe(spy_asset_tools.get_spy_asset_data, asset_path)


@mcp.tool()
def add_asset_group(asset_path: str, group_name: str) -> str:
    """SpyAssetData에 새 그룹을 추가합니다."""
    return _safe(spy_asset_tools.add_asset_group, asset_path, group_name)


@mcp.tool()
def add_asset_entry(asset_path: str, group_name: str, entry_name: str, entry_path: str) -> str:
    """SpyAssetData 그룹에 새 엔트리를 추가합니다."""
    return _safe(spy_asset_tools.add_asset_entry, asset_path, group_name, entry_name, entry_path)


@mcp.tool()
def set_asset_entry(asset_path: str, group_name: str, entry_name: str, entry_path: str) -> str:
    """SpyAssetData 엔트리의 AssetPath를 수정합니다."""
    return _safe(spy_asset_tools.set_asset_entry, asset_path, group_name, entry_name, entry_path)


@mcp.tool()
def remove_asset_entry(asset_path: str, group_name: str, entry_name: str) -> str:
    """SpyAssetData에서 엔트리를 삭제합니다."""
    return _safe(spy_asset_tools.remove_asset_entry, asset_path, group_name, entry_name)


@mcp.tool()
def save_spy_asset_data(asset_path: str) -> str:
    """SpyAssetData 변경사항을 저장합니다."""
    return _safe(spy_asset_tools.save_spy_asset_data, asset_path)


if __name__ == "__main__":
    mcp.run()
```

- [ ] **Step 2: 서버 임포트 확인**

```bash
cd tools/unreal-mcp
python -c "import server; print('server.py 임포트 성공')"
```

Expected: `server.py 임포트 성공`

- [ ] **Step 3: 전체 테스트 실행**

```bash
python -m pytest tests/ -v
```

Expected: 전체 테스트 PASS

- [ ] **Step 4: 커밋**

```bash
git add tools/unreal-mcp/server.py
git commit -m "feat: MCP 서버 진입점 구현, 전체 툴 등록"
```

---

## Task 8: Claude Code MCP 등록 및 연결 확인

**Files:**
- Modify: `~/.claude/settings.json`

- [ ] **Step 1: settings.json에 MCP 서버 등록**

`C:/Users/Tae/.claude/settings.json` 의 `mcpServers` 키 추가:

```json
{
  "MAX_THINKING_TOKENS": "10000",
  "CLAUDE_AUTOCOMPACT_PCT_OVERRIDE": "50",
  "CLAUDE_CODE_SUBAGENT_MODEL": "haiku",
  "autoUpdatesChannel": "latest",
  "enabledPlugins": {
    "superpowers@claude-plugins-official": true
  },
  "mcpServers": {
    "unreal": {
      "command": "python",
      "args": [
        "C:\\Users\\Tae\\Desktop\\MyProject\\SpyProject\\UnrealSkillProject\\tools\\unreal-mcp\\server.py"
      ]
    }
  }
}
```

- [ ] **Step 2: Unreal Editor에서 Remote Control Plugin 활성화 확인**

에디터에서:
1. Edit → Plugins 열기
2. 검색창에 "Remote Control" 입력
3. **Remote Control API** 항목 Enabled 체크 확인
4. 에디터 재시작 (처음 활성화 시)

- [ ] **Step 3: Claude Code 재시작 후 MCP 툴 확인**

Claude Code를 재시작하면 `unreal_*` 또는 MCP 툴 목록에 등록된 툴들이 나타남.

에디터가 실행 중인 상태에서 테스트:
```
get_actors_in_level()
→ {"actors": [...]} 반환 확인
```

- [ ] **Step 4: 커밋**

```bash
git add tools/unreal-mcp/
git commit -m "feat: Unreal MCP 서버 완성 - Claude Code 연동 준비"
```
