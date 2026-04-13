import httpx
from typing import Any


REMOTE_CONTROL_URL = "http://localhost:30010"


class UnrealConnectionError(Exception):
    """Unreal Editor에 연결할 수 없을 때 발생"""
    pass


class UnrealClient:
    def __init__(self, base_url: str = REMOTE_CONTROL_URL):
        self.base_url = base_url.rstrip("/")
        self._http = httpx.Client(timeout=10.0)

    def _check_connection(self) -> None:
        try:
            self._http.get(f"{self.base_url}/remote/info", timeout=2.0)
        except httpx.ConnectError:
            raise UnrealConnectionError(
                "Unreal Editor가 실행 중이지 않습니다. 에디터를 먼저 실행하고 "
                "Edit → Plugins → Remote Control API를 활성화하세요."
            )

    def get_property(self, object_path: str, property_name: str) -> dict:
        self._check_connection()
        response = self._http.get(
            f"{self.base_url}/remote/object/property",
            params={"objectPath": object_path, "propertyName": property_name}
        )
        response.raise_for_status()
        return response.json()

    def set_property(self, object_path: str, property_name: str, value: Any) -> dict:
        self._check_connection()
        response = self._http.put(
            f"{self.base_url}/remote/object/property",
            json={
                "objectPath": object_path,
                "propertyName": property_name,
                "propertyValue": {property_name: value}
            }
        )
        response.raise_for_status()
        return response.json()

    def call_function(self, object_path: str, function_name: str, parameters: dict | None = None) -> dict:
        self._check_connection()
        response = self._http.post(
            f"{self.base_url}/remote/object/call",
            json={
                "objectPath": object_path,
                "functionName": function_name,
                "parameters": parameters or {}
            }
        )
        response.raise_for_status()
        return response.json()

    def execute_python(self, script: str) -> dict:
        self._check_connection()
        response = self._http.post(
            f"{self.base_url}/remote/object/call",
            json={
                "objectPath": "/Script/PythonScriptPlugin.Default__PythonScriptLibrary",
                "functionName": "ExecutePythonScript",
                "parameters": {"pythonScript": script}
            }
        )
        response.raise_for_status()
        return response.json()
