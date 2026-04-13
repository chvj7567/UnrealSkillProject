import json
import os
import tempfile
from unreal_client import UnrealClient

RESULT_FILE = os.path.join(tempfile.gettempdir(), "unreal_mcp_output.json")


def _read_result() -> dict:
    with open(RESULT_FILE, "r", encoding="utf-8") as f:
        return json.load(f)


def get_combo_asset_data(client: UnrealClient, asset_path: str) -> dict:
    """USpyComboAssetData의 ComboSets(시작태그 → 콤보태그 쌍 배열)을 조회합니다."""
    result_file = RESULT_FILE.replace("\\", "/")
    script = f"""
import unreal, json
asset = unreal.load_asset({asset_path!r})
if asset is None:
    result = {{"error": "에셋을 찾을 수 없습니다: {asset_path}"}}
else:
    combo_sets = asset.get_editor_property('combo_sets')
    result = [
        {{
            'start_skill_tag': str(combo.get_editor_property('start_skill_tag')),
            'combo_tag': str(combo.get_editor_property('combo_tag'))
        }}
        for combo in combo_sets
    ]
with open({result_file!r}, 'w', encoding='utf-8') as f:
    json.dump(result, f)
"""
    client.execute_python(script.strip())
    return _read_result()


def add_combo_set(client: UnrealClient, asset_path: str, start_skill_tag: str, combo_tag: str) -> dict:
    """ComboSets에 새 콤보(시작태그, 콤보태그 쌍)를 추가합니다."""
    script = f"""
import unreal
asset = unreal.load_asset({asset_path!r})
combo_sets = list(asset.get_editor_property('combo_sets'))
new_combo = unreal.SpyComboSet()
new_combo.set_editor_property('start_skill_tag', unreal.GameplayTag.request_gameplay_tag({start_skill_tag!r}))
new_combo.set_editor_property('combo_tag', unreal.GameplayTag.request_gameplay_tag({combo_tag!r}))
combo_sets.append(new_combo)
asset.set_editor_property('combo_sets', combo_sets)
"""
    client.execute_python(script.strip())
    return {"success": True, "start_skill_tag": start_skill_tag, "combo_tag": combo_tag}


def remove_combo_set(client: UnrealClient, asset_path: str, start_skill_tag: str) -> dict:
    """start_skill_tag가 일치하는 콤보를 ComboSets에서 삭제합니다."""
    script = f"""
import unreal
asset = unreal.load_asset({asset_path!r})
combo_sets = list(asset.get_editor_property('combo_sets'))
combo_sets = [
    c for c in combo_sets
    if str(c.get_editor_property('start_skill_tag')) != {start_skill_tag!r}
]
asset.set_editor_property('combo_sets', combo_sets)
"""
    client.execute_python(script.strip())
    return {"success": True, "start_skill_tag": start_skill_tag}
