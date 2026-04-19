"""
Unreal MCP Python HTTP Server
에디터 시작 시 자동으로 실행되어 포트 30011에서 Python 실행 요청을 받습니다.
"""
import threading
import json
import traceback
from http.server import BaseHTTPRequestHandler, HTTPServer

MCP_PORT = 30011


class PythonExecHandler(BaseHTTPRequestHandler):
    def do_POST(self):
        if self.path != "/exec":
            self.send_response(404)
            self.end_headers()
            return

        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length)
        try:
            data = json.loads(body)
            script = data.get("script", "")
        except Exception:
            self.send_response(400)
            self.end_headers()
            return

        result = {"output": "", "error": ""}
        try:
            import unreal
            exec_globals = {"unreal": unreal}
            exec(script, exec_globals)
            result["output"] = str(exec_globals.get("__mcp_result__", ""))
        except Exception:
            result["error"] = traceback.format_exc()

        response = json.dumps(result).encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(response)))
        self.end_headers()
        self.wfile.write(response)

    def log_message(self, format, *args):
        pass  # 로그 억제


def start_mcp_server():
    server = HTTPServer(("localhost", MCP_PORT), PythonExecHandler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    import unreal
    unreal.log(f"[MCP] Python HTTP server started on port {MCP_PORT}")


start_mcp_server()
