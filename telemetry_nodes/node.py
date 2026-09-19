import socket
import random
import time
import sys


# Configuración por defecto
from config import SERVER_HOST, SERVER_PORT, SEND_INTERVAL


def generate_telemetry():
    """
    Genera valores simulados para las variables del nodo.
    """
    temperature = round(random.uniform(20.0, 45.0), 2)
    humidity = round(random.uniform(30.0, 95.0), 2)
    energy = round(random.uniform(100.0, 900.0), 2)
    vibration = round(random.uniform(0.0, 10.0), 2)

    return temperature, humidity, energy, vibration


def build_message(node_id, temperature, humidity, energy, vibration):
    """
    Construye el mensaje según el protocolo de aplicación del proyecto.
    """
    message = (
        f"TELEMETRY|{node_id}"
        f"|TEMP|{temperature}"
        f"|HUM|{humidity}"
        f"|ENERGY|{energy}"
        f"|VIB|{vibration}"
    )

    return message


def resolve_server(host):
    """
    Resuelve el nombre del servidor utilizando DNS.
    """
    try:
        server_ip = socket.gethostbyname(host)

        print(f"[DNS] Servidor '{host}' resuelto como {server_ip}")

        return server_ip

    except socket.gaierror as error:
        print(f"[ERROR DNS] No fue posible resolver '{host}': {error}")
        return None


def send_telemetry(node_id, server_host, server_port):
    """
    Envía mediciones periódicamente mediante UDP.
    """

    server_ip = resolve_server(server_host)

    if server_ip is None:
        return

    udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    print("-----------------------------------------")
    print(f"Nodo iniciado: {node_id}")
    print(f"Servidor: {server_host}")
    print(f"IP servidor: {server_ip}")
    print(f"Puerto UDP: {server_port}")
    print(f"Intervalo: {SEND_INTERVAL} segundos")
    print("-----------------------------------------")

    try:

        while True:

            temperature, humidity, energy, vibration = generate_telemetry()

            message = build_message(
                node_id,
                temperature,
                humidity,
                energy,
                vibration
            )

            try:

                udp_socket.sendto(
                    message.encode("utf-8"),
                    (server_ip, server_port)
                )

                print(f"[{node_id}] Enviado:")
                print(message)

            except OSError as error:

                print(
                    f"[ERROR] No fue posible enviar telemetría: {error}"
                )

            time.sleep(SEND_INTERVAL)

    except KeyboardInterrupt:

        print(f"\n[{node_id}] Nodo detenido manualmente.")

    finally:

        udp_socket.close()
        print(f"[{node_id}] Socket UDP cerrado.")


def main():

    if len(sys.argv) < 2 or len(sys.argv) > 3:

        print("Uso:")
        print("python node.py NODE01 [server_host]")

        return

    node_id = sys.argv[1].upper()

    if not node_id or not node_id.isascii() or len(node_id) >= 32 or not all(c.isalnum() or c in "_-" for c in node_id):
        print("[ERROR] El ID debe tener 1 a 31 caracteres alfanumericos, _ o -.")
        return

    send_telemetry(
        node_id,
        sys.argv[2] if len(sys.argv) == 3 else SERVER_HOST,
        SERVER_PORT
    )


if __name__ == "__main__":
    main()
