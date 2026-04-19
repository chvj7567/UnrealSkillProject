import json
import os
import tempfile
from unittest.mock import MagicMock, patch, mock_open
import pytest
from tools.asset_tools import (
    RESULT_FILE, get_asset_properties, set_asset_property,
    save_asset, list_assets, find_assets_by_class
)


def make_client():
    client = MagicMock()
    client.execute_python = MagicMock(return_value={})
    return client


class TestGetAssetProperties:
    def test_executes_python_and_reads_result(self, tmp_path):
        client = make_client()
        expected = {"SomeProperty": "value"}
        result_path = str(tmp_path / "result.json")
        with patch("tools.asset_tools.RESULT_FILE", result_path):
            with open(result_path, "w") as f:
                json.dump(expected, f)
            with patch.object(client, "execute_python", return_value={}):
                result = get_asset_properties(client, "/Game/Data/DA_Test")
        assert result == expected

    def test_python_script_contains_asset_path(self, tmp_path):
        client = make_client()
        result_path = str(tmp_path / "result.json")
        with open(result_path, "w") as f:
            json.dump({}, f)
        with patch("tools.asset_tools.RESULT_FILE", result_path):
            get_asset_properties(client, "/Game/Data/DA_Test")
        script = client.execute_python.call_args[0][0]
        assert "/Game/Data/DA_Test" in script


class TestSaveAsset:
    def test_executes_python_with_asset_path(self):
        client = make_client()
        save_asset(client, "/Game/Data/DA_Test")
        script = client.execute_python.call_args[0][0]
        assert "/Game/Data/DA_Test" in script
        assert "save_asset" in script


class TestListAssets:
    def test_passes_path_and_class_filter(self, tmp_path):
        client = make_client()
        result_path = str(tmp_path / "result.json")
        with open(result_path, "w") as f:
            json.dump({"assets": []}, f)
        with patch("tools.asset_tools.RESULT_FILE", result_path):
            list_assets(client, "/Game/Data", class_name="DataAsset")
        script = client.execute_python.call_args[0][0]
        assert "/Game/Data" in script
        assert "DataAsset" in script
