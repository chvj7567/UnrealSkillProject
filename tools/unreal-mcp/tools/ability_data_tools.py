import json
import os
import tempfile
from unreal_client import UnrealClient

RESULT_FILE = os.path.join(tempfile.gettempdir(), "unreal_mcp_output.json")


def _read_result() -> dict:
    with open(RESULT_FILE, "r", encoding="utf-8") as f:
        return json.load(f)


def get_ability_data(client: UnrealClient, asset_path: str) -> dict:
    """USpyAbilityData의 어빌리티/이펙트/어트리뷰트 배열을 조회합니다."""
    result_file = RESULT_FILE.replace("\\", "/")
    script = f"""
import unreal, json
asset = unreal.load_asset({asset_path!r})
if asset is None:
    result = {{"error": "에셋을 찾을 수 없습니다: {asset_path}"}}
else:
    abilities = []
    for entry in asset.get_editor_property('granted_gameplay_abilities'):
        abilities.append({{
            'ability': str(entry.get_editor_property('ability')),
            'level': int(entry.get_editor_property('ability_level')),
            'input_tag': str(entry.get_editor_property('input_tag'))
        }})
    effects = []
    for entry in asset.get_editor_property('granted_gameplay_effects'):
        effects.append({{
            'gameplay_effect': str(entry.get_editor_property('gameplay_effect')),
            'level': float(entry.get_editor_property('effect_level'))
        }})
    attrs = []
    for entry in asset.get_editor_property('granted_attributes'):
        attrs.append({{'attribute_set': str(entry.get_editor_property('attribute_set'))}})
    result = {{
        'granted_gameplay_abilities': abilities,
        'granted_gameplay_effects': effects,
        'granted_attributes': attrs
    }}
with open({result_file!r}, 'w', encoding='utf-8') as f:
    json.dump(result, f)
"""
    client.execute_python(script.strip())
    return _read_result()
