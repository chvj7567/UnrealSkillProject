from unittest.mock import MagicMock
from tools.python_exec_tools import execute_python


def make_client():
    client = MagicMock()
    client.execute_python = MagicMock(return_value={"returnValues": {}})
    return client


class TestExecutePython:
    def test_passes_script_to_client(self):
        client = make_client()
        script = "import unreal; print(unreal.__version__)"
        execute_python(client, script)
        client.execute_python.assert_called_once_with(script)

    def test_returns_client_result(self):
        client = make_client()
        client.execute_python.return_value = {"returnValues": {"output": "3.14"}}
        result = execute_python(client, "print(3.14)")
        assert result == {"returnValues": {"output": "3.14"}}
