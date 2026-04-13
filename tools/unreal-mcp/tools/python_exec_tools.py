from unreal_client import UnrealClient


def execute_python(client: UnrealClient, script: str) -> dict:
    return client.execute_python(script)
