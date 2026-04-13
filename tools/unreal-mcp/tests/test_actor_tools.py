import json
from unittest.mock import MagicMock, patch
import pytest
from tools.actor_tools import (
    get_actors_in_level, get_actor_properties,
    set_actor_property, spawn_actor, delete_actor
)


def make_client():
    client = MagicMock()
    client.execute_python = MagicMock(return_value={})
    return client


class TestGetActorsInLevel:
    def test_script_contains_name_filter(self, tmp_path):
        client = make_client()
        result_path = str(tmp_path / "result.json")
        with open(result_path, "w") as f:
            json.dump({"actors": []}, f)
        with patch("tools.actor_tools.RESULT_FILE", result_path):
            get_actors_in_level(client, name_filter="TargetPoint")
        script = client.execute_python.call_args[0][0]
        assert "TargetPoint" in script

    def test_no_filter_returns_all(self, tmp_path):
        client = make_client()
        result_path = str(tmp_path / "result.json")
        actors = [{"name": "Actor1", "class": "AActor", "location": {"x": 0, "y": 0, "z": 0}}]
        with open(result_path, "w") as f:
            json.dump({"actors": actors}, f)
        with patch("tools.actor_tools.RESULT_FILE", result_path):
            result = get_actors_in_level(client)
        assert result == {"actors": actors}


class TestSpawnActor:
    def test_script_contains_class_and_location(self):
        client = make_client()
        spawn_actor(client, "/Game/BP_Enemy", (100.0, 200.0, 0.0))
        script = client.execute_python.call_args[0][0]
        assert "/Game/BP_Enemy" in script
        assert "100.0" in script
        assert "200.0" in script

    def test_returns_success(self):
        client = make_client()
        result = spawn_actor(client, "/Game/BP_Enemy", (0, 0, 0))
        assert result["success"] is True


class TestDeleteActor:
    def test_script_contains_actor_name(self):
        client = make_client()
        delete_actor(client, "BP_Enemy_0")
        script = client.execute_python.call_args[0][0]
        assert "BP_Enemy_0" in script

    def test_returns_success(self):
        client = make_client()
        result = delete_actor(client, "BP_Enemy_0")
        assert result["success"] is True
