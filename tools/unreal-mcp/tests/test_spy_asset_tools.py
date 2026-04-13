import json
from unittest.mock import MagicMock, patch
import pytest
from tools.spy_asset_tools import (
    get_spy_asset_data, add_asset_group, add_asset_entry,
    set_asset_entry, remove_asset_entry, save_spy_asset_data
)

ASSET_PATH = "/Game/Data/DA_SpyAssetData"


def make_client():
    client = MagicMock()
    client.execute_python = MagicMock(return_value={})
    return client


class TestGetSpyAssetData:
    def test_reads_result_from_temp_file(self, tmp_path):
        client = make_client()
        result_path = str(tmp_path / "result.json")
        expected = {"Character": [{"name": "Player", "path": "/Game/BP_Player"}]}
        with open(result_path, "w") as f:
            json.dump(expected, f)
        with patch("tools.spy_asset_tools.RESULT_FILE", result_path):
            result = get_spy_asset_data(client, ASSET_PATH)
        assert result == expected

    def test_script_contains_asset_path(self, tmp_path):
        client = make_client()
        result_path = str(tmp_path / "result.json")
        with open(result_path, "w") as f:
            json.dump({}, f)
        with patch("tools.spy_asset_tools.RESULT_FILE", result_path):
            get_spy_asset_data(client, ASSET_PATH)
        script = client.execute_python.call_args[0][0]
        assert ASSET_PATH in script
        assert "asset_group_name_to_set" in script


class TestAddAssetGroup:
    def test_script_contains_group_name(self):
        client = make_client()
        add_asset_group(client, ASSET_PATH, "NewGroup")
        script = client.execute_python.call_args[0][0]
        assert "NewGroup" in script
        assert ASSET_PATH in script

    def test_returns_success(self):
        client = make_client()
        result = add_asset_group(client, ASSET_PATH, "NewGroup")
        assert result["success"] is True
        assert result["group"] == "NewGroup"


class TestAddAssetEntry:
    def test_script_contains_all_params(self):
        client = make_client()
        add_asset_entry(client, ASSET_PATH, "Character", "Player", "/Game/BP_Player")
        script = client.execute_python.call_args[0][0]
        assert "Character" in script
        assert "Player" in script
        assert "/Game/BP_Player" in script

    def test_returns_success(self):
        client = make_client()
        result = add_asset_entry(client, ASSET_PATH, "Character", "Player", "/Game/BP_Player")
        assert result["success"] is True
        assert result["entry"] == "Player"


class TestSetAssetEntry:
    def test_script_contains_all_params(self):
        client = make_client()
        set_asset_entry(client, ASSET_PATH, "Character", "Player", "/Game/BP_Player_Updated")
        script = client.execute_python.call_args[0][0]
        assert "Character" in script
        assert "Player" in script
        assert "/Game/BP_Player_Updated" in script

    def test_returns_success(self):
        client = make_client()
        result = set_asset_entry(client, ASSET_PATH, "Character", "Player", "/Game/BP_Player_Updated")
        assert result["success"] is True
        assert result["entry"] == "Player"
        assert result["path"] == "/Game/BP_Player_Updated"


class TestRemoveAssetEntry:
    def test_script_contains_group_and_entry(self):
        client = make_client()
        remove_asset_entry(client, ASSET_PATH, "Character", "Player")
        script = client.execute_python.call_args[0][0]
        assert "Character" in script
        assert "Player" in script

    def test_returns_success(self):
        client = make_client()
        result = remove_asset_entry(client, ASSET_PATH, "Character", "Player")
        assert result["success"] is True


class TestSaveSpyAssetData:
    def test_script_contains_asset_path(self):
        client = make_client()
        save_spy_asset_data(client, ASSET_PATH)
        script = client.execute_python.call_args[0][0]
        assert ASSET_PATH in script
        assert "save_asset" in script
