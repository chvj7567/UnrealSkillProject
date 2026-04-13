import json
import os
import tempfile
from unreal_client import UnrealClient

RESULT_FILE = os.path.join(tempfile.gettempdir(), "unreal_mcp_output.json")


def _read_result() -> dict:
    with open(RESULT_FILE, "r", encoding="utf-8") as f:
        return json.load(f)


def get_blueprint_cdo_properties(client: UnrealClient, asset_path: str) -> dict:
    """Blueprint CDO(클래스 기본 오브젝트)의 모든 프로퍼티를 조회합니다.
    UGameplayAbility, UGameplayEffect, UAttributeSet 등 모든 BP 클래스에 사용 가능합니다.
    asset_path: /Game/... 형식
    """
    result_file = RESULT_FILE.replace("\\", "/")
    script = f"""
import unreal, json
bp = unreal.load_asset({asset_path!r})
if bp is None:
    result = {{"error": "에셋을 찾을 수 없습니다: {asset_path}"}}
else:
    try:
        cdo = bp.generated_class().get_default_object()
        result = {{}}
        for prop in cdo.get_class().iterate_properties():
            try:
                result[prop.name] = str(cdo.get_editor_property(prop.name))
            except Exception:
                pass
    except Exception as e:
        result = {{"error": str(e)}}
with open({result_file!r}, 'w', encoding='utf-8') as f:
    json.dump(result, f)
"""
    client.execute_python(script.strip())
    return _read_result()


def set_blueprint_cdo_property(client: UnrealClient, asset_path: str, property_name: str, value: str) -> dict:
    """Blueprint CDO의 특정 프로퍼티를 변경하고 저장합니다."""
    script = f"""
import unreal
bp = unreal.load_asset({asset_path!r})
cdo = bp.generated_class().get_default_object()
cdo.set_editor_property({property_name!r}, {value})
unreal.EditorAssetLibrary.save_asset({asset_path!r})
"""
    client.execute_python(script.strip())
    return {"success": True, "asset": asset_path, "property": property_name}
