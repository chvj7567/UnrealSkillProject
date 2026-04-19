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
import tools.blueprint_tools as blueprint_tools
import tools.ability_data_tools as ability_data_tools
import tools.anim_asset_tools as anim_asset_tools
import tools.character_asset_tools as character_asset_tools
import tools.combo_asset_tools as combo_asset_tools

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


# ── Blueprint CDO 툴 ─────────────────────────────────────

@mcp.tool()
def get_blueprint_cdo_properties(asset_path: str) -> str:
    """Blueprint CDO의 모든 프로퍼티를 조회합니다. GA/GE/AttributeSet 등 모든 BP 클래스에 사용 가능."""
    return _safe(blueprint_tools.get_blueprint_cdo_properties, asset_path)


@mcp.tool()
def set_blueprint_cdo_property(asset_path: str, property_name: str, value: str) -> str:
    """Blueprint CDO의 특정 프로퍼티를 변경하고 저장합니다."""
    return _safe(blueprint_tools.set_blueprint_cdo_property, asset_path, property_name, value)


# ── SpyAbilityData 툴 ─────────────────────────────────────

@mcp.tool()
def get_ability_data(asset_path: str) -> str:
    """USpyAbilityData의 어빌리티/이펙트/어트리뷰트 배열을 조회합니다."""
    return _safe(ability_data_tools.get_ability_data, asset_path)


# ── SpyAnimAssetData 툴 ───────────────────────────────────

@mcp.tool()
def get_anim_asset_data(asset_path: str) -> str:
    """USpyAnimAssetData의 AnimLayerMap(이름 → AnimInstance 클래스)을 조회합니다."""
    return _safe(anim_asset_tools.get_anim_asset_data, asset_path)


@mcp.tool()
def set_anim_layer(asset_path: str, layer_name: str, class_path: str) -> str:
    """AnimLayerMap에 항목을 추가하거나 덮어씁니다. class_path: /Game/... 형식."""
    return _safe(anim_asset_tools.set_anim_layer, asset_path, layer_name, class_path)


@mcp.tool()
def remove_anim_layer(asset_path: str, layer_name: str) -> str:
    """AnimLayerMap에서 항목을 삭제합니다."""
    return _safe(anim_asset_tools.remove_anim_layer, asset_path, layer_name)


# ── SpyCharacterAssetData 툴 ──────────────────────────────

@mcp.tool()
def get_character_asset_data(asset_path: str) -> str:
    """USpyCharacterAssetData의 CharacterAssets 전체 구조를 조회합니다."""
    return _safe(character_asset_tools.get_character_asset_data, asset_path)


# ── SpyComboAssetData 툴 ──────────────────────────────────

@mcp.tool()
def get_combo_asset_data(asset_path: str) -> str:
    """USpyComboAssetData의 ComboSets(시작태그 → 콤보태그 쌍 배열)을 조회합니다."""
    return _safe(combo_asset_tools.get_combo_asset_data, asset_path)


@mcp.tool()
def add_combo_set(asset_path: str, start_skill_tag: str, combo_tag: str) -> str:
    """ComboSets에 새 콤보(시작태그, 콤보태그 쌍)를 추가합니다."""
    return _safe(combo_asset_tools.add_combo_set, asset_path, start_skill_tag, combo_tag)


@mcp.tool()
def remove_combo_set(asset_path: str, start_skill_tag: str) -> str:
    """start_skill_tag가 일치하는 콤보를 ComboSets에서 삭제합니다."""
    return _safe(combo_asset_tools.remove_combo_set, asset_path, start_skill_tag)


if __name__ == "__main__":
    mcp.run()
