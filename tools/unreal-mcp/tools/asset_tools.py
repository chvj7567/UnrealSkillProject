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
    result['class'] = asset.get_class().get_name()
    result['path'] = asset.get_path_name()
    for prop_name in dir(asset):
        if prop_name.startswith('_'):
            continue
        try:
            val = asset.get_editor_property(prop_name)
            result[prop_name] = str(val)
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
