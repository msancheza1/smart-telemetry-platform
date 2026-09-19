# Smart Telemetry Platform

Servidor central en C con recepción UDP de telemetría y consultas TCP, clientes en Python y panel web básico. El servidor está pensado para Linux o WSL con `gcc`, `make` y pthreads; los clientes y el panel requieren Python 3.

## Ejecutar

En tres terminales, desde la raíz del proyecto:

```sh
make -C server
./server/smart_server
```

```sh
python3 telemetry_nodes/node.py NODE01
```

```sh
python3 operator_client/client.py
```

Para abrir el panel web en una cuarta terminal:

```sh
python3 web/dashboard.py
```

Abra `http://localhost:8080`. El panel consulta el servidor C mediante el protocolo TCP; muestra el estado, nodos, lecturas y alertas.

Los clientes usan `localhost` por defecto. Si el servidor está en otro equipo, indique su nombre DNS o IP como segundo argumento del nodo o primer argumento del cliente:

```sh
python3 telemetry_nodes/node.py NODE01 servidor.ejemplo.edu
python3 operator_client/client.py servidor.ejemplo.edu
```

Permita UDP 5000, TCP 5001 y, si se desea acceso al panel, TCP 8080 en el firewall del servidor. También se puede compilar desde `server/src` con `make`; ese Makefile delega al directorio `server`.

## Docker

```sh
docker build -t smart-telemetry .
docker run --rm -p 5000:5000/udp -p 5001:5001/tcp -p 8080:8080/tcp smart-telemetry
```

Para probar cinco nodos simultáneos desde otro equipo con Python 3:

```sh
python3 telemetry_nodes/launch_nodes.py servidor.ejemplo.edu
```

Configure el nombre DNS para que apunte a la IP pública de la instancia y abra los puertos en las reglas de la nube y el firewall del sistema. Pruebe desde una red externa. El repositorio no incluye credenciales ni presume un despliegue real.

## Protocolo

Cada datagrama UDP lleva un mensaje ASCII completo como:

```text
TELEMETRY|NODE01|TEMP|25.5|HUM|60|ENERGY|300|VIB|2.1
```

El ID admite entre 1 y 31 caracteres alfanuméricos, `_` o `-`. Las cuatro mediciones deben ser números finitos no negativos y la humedad debe estar entre 0 y 100. Los campos deben aparecer en el orden mostrado.

El cliente TCP envía un comando terminado en `\n`, recibe la respuesta y el servidor cierra la conexión. Comandos:

| Comando | Respuesta |
| --- | --- |
| `GET_NODES` | `OK|NODES|cantidad|id...` de nodos activos |
| `GET_STATUS|NODE01` | `OK|NODE|NODE01|TEMP|...|HUM|...|ENERGY|...|VIB|...|ACTIVE|0/1` |
| `GET_LATEST` | La última medición recibida, con prefijo `OK|LATEST` |
| `GET_ALERTS` | `OK|ALERTS|cantidad|nodo,tipo,valor...` (hasta 100 alertas recientes) |
| `GET_SYSTEM_STATUS` | `OK|SYSTEM|NODES|cantidad|ALERTS|cantidad` |
| `GET_STATS` | `OK|STATS|RECEIVED|cantidad|INVALID|cantidad` |

Las alertas se generan al superar temperatura 40, humedad 90, energía 800 o vibración 8. Se conserva la última lectura por nodo y las 100 alertas más recientes en memoria; los datos se pierden al reiniciar el servidor. Un nodo aparece activo si reportó en los últimos 15 segundos. `GET_SYSTEM_STATUS` cuenta todos los nodos registrados desde el último arranque.

## Comprobación

Con Python 3 en Linux o WSL:

```sh
python3 tests/smoke_test.py
```

Esta prueba compila el servidor, envía telemetría válida e inválida por UDP y comprueba las consultas TCP.

Para registrar una prueba sencilla de tráfico UDP con cantidades transmitidas, recibidas y pérdidas estimadas, ejecute el servidor sin otros nodos y luego:

```sh
python3 tests/traffic_test.py servidor.ejemplo.edu
```

Las capturas de Wireshark y las evidencias de nube/DNS deben obtenerse durante la ejecución real; el código no genera esas evidencias automáticamente.
