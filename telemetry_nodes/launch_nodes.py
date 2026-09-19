"""Launch five independent simulated telemetry nodes."""

import pathlib
import subprocess
import sys


def main():
    if len(sys.argv) > 2:
        print("Uso: python launch_nodes.py [server_host]")
        return
    host = sys.argv[1] if len(sys.argv) == 2 else "localhost"
    node_script = pathlib.Path(__file__).with_name("node.py")
    children = [
        subprocess.Popen([sys.executable, str(node_script), f"NODE{i:02d}", host])
        for i in range(1, 6)
    ]
    try:
        for child in children:
            child.wait()
    except KeyboardInterrupt:
        pass
    finally:
        for child in children:
            if child.poll() is None:
                child.terminate()
        for child in children:
            child.wait()


if __name__ == "__main__":
    main()
