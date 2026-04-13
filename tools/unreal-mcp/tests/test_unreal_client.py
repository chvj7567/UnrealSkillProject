import pytest
from unittest.mock import MagicMock, patch
import httpx
from unreal_client import UnrealClient, UnrealConnectionError

BASE_URL = "http://localhost:30010"


def make_client():
    return UnrealClient(base_url=BASE_URL)


def mock_response(json_data: dict, status_code: int = 200) -> MagicMock:
    resp = MagicMock()
    resp.json.return_value = json_data
    resp.status_code = status_code
    resp.raise_for_status = MagicMock()
    return resp


class TestConnectionCheck:
    def test_raises_on_connect_error(self):
        client = make_client()
        with patch.object(client._http, "get", side_effect=httpx.ConnectError("refused")):
            with pytest.raises(UnrealConnectionError) as exc_info:
                client._check_connection()
            assert "실행 중이지 않습니다" in str(exc_info.value)

    def test_passes_when_connected(self):
        client = make_client()
        with patch.object(client._http, "get", return_value=mock_response({})):
            client._check_connection()  # should not raise


class TestGetProperty:
    def test_calls_correct_endpoint(self):
        client = make_client()
        expected = {"propertyValue": {"AssetGroupNameToSet": {}}}
        with patch.object(client._http, "get", return_value=mock_response(expected)) as mock_get, \
             patch.object(client, "_check_connection"):
            result = client.get_property("/Game/Data/DA_SpyAssetData", "AssetGroupNameToSet")
        mock_get.assert_called_once_with(
            f"{BASE_URL}/remote/object/property",
            params={"objectPath": "/Game/Data/DA_SpyAssetData", "propertyName": "AssetGroupNameToSet"}
        )
        assert result == expected

    def test_raises_on_http_error(self):
        client = make_client()
        err_resp = mock_response({}, status_code=404)
        err_resp.raise_for_status.side_effect = httpx.HTTPStatusError("not found", request=MagicMock(), response=err_resp)
        with patch.object(client._http, "get", return_value=err_resp), \
             patch.object(client, "_check_connection"):
            with pytest.raises(httpx.HTTPStatusError):
                client.get_property("/bad/path", "prop")


class TestExecutePython:
    def test_calls_object_call_endpoint(self):
        client = make_client()
        expected = {"returnValues": {}}
        with patch.object(client._http, "post", return_value=mock_response(expected)) as mock_post, \
             patch.object(client, "_check_connection"):
            result = client.execute_python("import unreal")
        mock_post.assert_called_once_with(
            f"{BASE_URL}/remote/object/call",
            json={
                "objectPath": "/Script/PythonScriptPlugin.Default__PythonScriptLibrary",
                "functionName": "ExecutePythonScript",
                "parameters": {"pythonScript": "import unreal"}
            }
        )
        assert result == expected
