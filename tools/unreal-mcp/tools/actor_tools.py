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
