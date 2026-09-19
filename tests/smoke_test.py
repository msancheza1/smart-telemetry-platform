"""End-to-end check for the C server. Run with Python 3 on Linux/WSL."""

import pathlib
import socket
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor
from urllib.request import urlopen


ROOT = pathlib.Path(__file__).resolve().parents[1]


def command(text):
    with socket.create_connection(("127.0.0.1", 5001), timeout=2) as connection:
        connection.sendall((text + "\n").encode("ascii"))
        data = bytearray()
        while chunk := connection.recv(4096):
            data.extend(chunk)
    return data.decode("ascii")


def wait_until(predicate, timeout=3):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            if predicate():
                return
        except OSError:
            pass
        time.sleep(0.05)
    raise AssertionError("Server did not reach the expected state")


def main():
    subprocess.run(["make", "-C", str(ROOT / "server")], check=True)
    server = subprocess.Popen(
        [str(ROOT / "server" / "smart_server")],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
    )
    web = None
    try:
        wait_until(lambda: command("GET_NODES") == "OK|NODES|0")
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as udp:
            udp.sendto(b"TELEMETRY|BAD|TEMP|41|HUM|50|ENERGY|100", ("127.0.0.1", 5000))
            message = b"TELEMETRY|NODE01|TEMP|41|HUM|91|ENERGY|801|VIB|9"
            def received():
                udp.sendto(message, ("127.0.0.1", 5000))
                return command("GET_NODES") == "OK|NODES|1|NODE01"
            wait_until(received)
        wait_until(lambda: int(command("GET_ALERTS").split("|")[2]) >= 4)
        assert command("GET_STATUS|NODE01").startswith("OK|NODE|NODE01|TEMP|41.00")
        assert command("GET_LATEST").startswith("OK|LATEST|NODE01|TEMP|41.00")
        assert "TEMP_HIGH" in command("GET_ALERTS")
        assert command("GET_SYSTEM_STATUS").startswith("OK|SYSTEM|NODES|1|ALERTS|")
        assert command("GET_STATUS|MISSING") == "ERROR|NODE_NOT_FOUND"
        assert command("BOGUS") == "ERROR|UNKNOWN_COMMAND"
        stats = command("GET_STATS").split("|")
        assert stats[:3] == ["OK", "STATS", "RECEIVED"]
        assert int(stats[3]) >= 1 and int(stats[5]) >= 1
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as udp:
            for i in range(2, 6):
                udp.sendto(
                    f"TELEMETRY|NODE{i:02d}|TEMP|25|HUM|50|ENERGY|100|VIB|1".encode(),
                    ("127.0.0.1", 5000),
                )
        wait_until(lambda: command("GET_NODES").startswith("OK|NODES|5|"))
        with ThreadPoolExecutor(max_workers=2) as pool:
            results = list(pool.map(command, ["GET_STATUS|NODE01", "GET_STATUS|NODE05"]))
        assert all(result.startswith("OK|NODE|") for result in results)
        web = subprocess.Popen(
            [sys.executable, str(ROOT / "web" / "dashboard.py")],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        def web_ready():
            with urlopen("http://127.0.0.1:8080/", timeout=2) as response:
                return response.status == 200 and b"NODE01" in response.read()
        wait_until(web_ready)
        print("Smoke test passed")
    finally:
        if web is not None:
            web.terminate()
            web.wait(timeout=3)
        server.terminate()
        try:
            server.wait(timeout=3)
        except subprocess.TimeoutExpired:
            server.kill()
            server.wait()
        server.stderr.close()


if __name__ == "__main__":
    main()
