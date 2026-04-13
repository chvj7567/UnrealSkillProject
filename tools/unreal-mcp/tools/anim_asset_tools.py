import json
import os
import tempfile
from unreal_client import UnrealClient

RESULT_FILE = os.path.join(tempfile.gettempdir(), "unreal_mcp_output.json")


def _read_result() -> dict:
    with open(RESULT_FILE, "r", encoding="utf-8") as f:
        return json.load(f)


def get_anim_asset_data(client: UnrealClient, asset_path: str) -> dict:
    """USpyAnimAssetData의 AnimLayerMap(이름 → AnimInstance 클래스)을 조회합니다."""
    result_file = RESULT_FILE.replace("\\", "/")
    script = f"""
import unreal, json
asset = unreal.load_asset({asset_path!r})
if asset is None:
    result = {{"error": "에셋을 찾을 수 없습니다: {asset_path}"}}
else:
    anim_map = asset.get_editor_property('anim_layer_map')
    result = {{str(k): str(v) for k, v in anim_map.items()}}
with open({result_file!r}, 'w', encoding='utf-8') as f:
    json.dump(result, f)
"""
    client.execute_python(script.strip())
    return _read_result()


def set_anim_layer(client: UnrealClient, asset_path: str, layer_name: str, class_path: str) -> dict:
    """AnimLayerMap에 항목을 추가하거나 덮어씁니다. class_path: /Game/... 형식."""
    script = f"""
import unreal
asset = unreal.load_asset({asset_path!r})
anim_map = asset.get_editor_property('anim_layer_map')
anim_map[{layer_name!r}] = unreal.SoftClassPath({class_path!r})
asset.set_editor_property('anim_layer_map', anim_map)
"""
    client.execute_python(script.strip())
    return {"success": True, "layer": layer_name, "class_path": class_path}


def remove_anim_layer(client: UnrealClient, asset_path: str, layer_name: str) -> dict:
    """AnimLayerMap에서 항목을 삭제합니다."""
    script = f"""
import unreal
asset = unreal.load_asset({asset_path!r})
anim_map = asset.get_editor_property('anim_layer_map')
if {layer_name!r} in anim_map:
    del anim_map[{layer_name!r}]
    asset.set_editor_property('anim_layer_map', anim_map)
"""
    client.execute_python(script.strip())
    return {"success": True, "layer": layer_name}
