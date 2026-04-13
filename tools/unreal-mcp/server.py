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
