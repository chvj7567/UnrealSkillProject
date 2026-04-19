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
