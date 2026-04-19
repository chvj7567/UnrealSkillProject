import json
import os
import tempfile
from unreal_client import UnrealClient

RESULT_FILE = os.path.join(tempfile.gettempdir(), "unreal_mcp_output.json")


def _read_result() -> dict:
    with open(RESULT_FILE, "r", encoding="utf-8") as f:
        return json.load(f)


def get_character_asset_data(client: UnrealClient, asset_path: str) -> dict:
    """USpyCharacterAssetData의 CharacterAssets 전체 구조를 조회합니다."""
    result_file = RESULT_FILE.replace("\\", "/")
    script = f"""
import unreal, json
asset = unreal.load_asset({asset_path!r})
if asset is None:
    result = {{"error": "에셋을 찾을 수 없습니다: {asset_path}"}}
else:
    char_assets = asset.get_editor_property('character_assets')
    common_skills = [str(s) for s in char_assets.get_editor_property('common_skills')]
    common_components = [str(c) for c in char_assets.get_editor_property('common_component_classes')]
    entries = []
    for entry in char_assets.get_editor_property('asset_entries'):
        entries.append({{
            'class_type': str(entry.get_editor_property('class_type')),
            'character_component_classes': [str(c) for c in entry.get_editor_property('character_component_classes')],
            'class_skills': [str(s) for s in entry.get_editor_property('class_skills')],
            'class_combos': str(entry.get_editor_property('class_combos')),
            'input_config': str(entry.get_editor_property('input_config')),
        }})
    result = {{
        'common_skills': common_skills,
        'common_component_classes': common_components,
        'asset_entries': entries
    }}
with open({result_file!r}, 'w', encoding='utf-8') as f:
    json.dump(result, f)
"""
    client.execute_python(script.strip())
    return _read_result()
