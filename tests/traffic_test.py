"""Send a known UDP batch and report transmitted, received and possible loss."""

import socket
import sys
import time


def received(host):
    with socket.create_connection((host, 5001), timeout=5) as connection:
        connection.sendall(b"GET_STATS\n")
        data = bytearray()
        while chunk := connection.recv(4096):
            data.extend(chunk)
    parts = data.decode("ascii").split("|")
    if parts[:3] != ["OK", "STATS", "RECEIVED"]:
        raise RuntimeError("Unexpected GET_STATS response")
    return int(parts[3])


def main():
    if len(sys.argv) > 2:
        print("Uso: python traffic_test.py [server_host]")
        return
    host = sys.argv[1] if len(sys.argv) == 2 else "localhost"
    sent = 100
    before = received(host)
    address = (host, 5000)
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as udp:
        for i in range(sent):
            message = f"TELEMETRY|TRAFFIC|TEMP|25|HUM|50|ENERGY|{i}|VIB|1"
            udp.sendto(message.encode("ascii"), address)
            time.sleep(0.01)
    time.sleep(1)
    arrived = received(host) - before
    print(f"Transmitidos: {sent}")
    print(f"Recibidos: {arrived}")
    print(f"Pérdidas UDP estimadas: {max(0, sent - arrived)}")
    print("Ejecute la prueba sin otros nodos activos para aislar el conteo.")


if __name__ == "__main__":
    main()
