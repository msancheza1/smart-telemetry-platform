import socket

SERVER_HOST = "localhost"
SERVER_PORT = 5001


def connect_to_server():
    """
    Crea una conexión TCP con el servidor central.
    """
    try:
        client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        client_socket.connect(
            (SERVER_HOST, SERVER_PORT)
        )

        return client_socket

    except ConnectionRefusedError:
        print("\n[ERROR] No se pudo conectar con el servidor.")
        print("Verifica que el servidor TCP esté ejecutándose.\n")
        return None

    except socket.gaierror as error:
        print(f"\n[ERROR DNS] No fue posible resolver el servidor: {error}\n")
        return None

    except OSError as error:
        print(f"\n[ERROR] Problema de comunicación: {error}\n")
        return None


def send_command(command):
    """
    Envía un comando al servidor y espera su respuesta.
    """

    client_socket = connect_to_server()

    if client_socket is None:
        return None

    try:
        client_socket.sendall(
            command.encode("utf-8")
        )

        response = client_socket.recv(4096)

        if not response:
            print("\n[ERROR] El servidor cerró la conexión sin responder.\n")
            return None

        return response.decode("utf-8")

    except OSError as error:
        print(f"\n[ERROR] Falló la comunicación con el servidor: {error}\n")
        return None

    finally:
        client_socket.close()


def show_active_nodes():
    response = send_command("GET_NODES")

    if response:
        print("\n--- NODOS ACTIVOS ---")
        print(response)


def show_node_status():
    node_id = input("\nIngrese el ID del nodo: ").strip().upper()

    if not node_id:
        print("\n[ERROR] Debes ingresar un ID de nodo.")
        return

    command = f"GET_STATUS|{node_id}"

    response = send_command(command)

    if response:
        print("\n--- ESTADO DEL NODO ---")
        print(response)


def show_latest_measurements():
    response = send_command("GET_LATEST")

    if response:
        print("\n--- ÚLTIMAS MEDICIONES ---")
        print(response)


def show_alerts():
    response = send_command("GET_ALERTS")

    if response:
        print("\n--- ALERTAS ---")
        print(response)


def show_system_status():
    response = send_command("GET_SYSTEM_STATUS")

    if response:
        print("\n--- ESTADO GENERAL DEL SISTEMA ---")
        print(response)


def show_menu():
    print("\n===================================")
    print(" SMART TELEMETRY - OPERATOR CLIENT")
    print("===================================")
    print("1. Consultar nodos activos")
    print("2. Consultar nodo específico")
    print("3. Ver últimas mediciones")
    print("4. Ver alertas")
    print("5. Ver estado general")
    print("6. Salir")


def main():

    while True:

        show_menu()

        option = input("\nSeleccione una opción: ").strip()

        if option == "1":
            show_active_nodes()

        elif option == "2":
            show_node_status()

        elif option == "3":
            show_latest_measurements()

        elif option == "4":
            show_alerts()

        elif option == "5":
            show_system_status()

        elif option == "6":
            print("\nCliente operador finalizado.")
            break

        else:
            print("\n[ERROR] Opción inválida.")


if __name__ == "__main__":
    main()