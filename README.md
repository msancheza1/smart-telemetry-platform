# Smart Telemetry Platform

Plataforma distribuida de telemetría compuesta por un servidor central en C, nodos y clientes en Python y un panel web básico. Los nodos envían mediciones por UDP, los operadores realizan consultas confiables por TCP y el navegador accede al estado del sistema mediante HTTP.

El servidor está pensado para Linux o WSL con `gcc`, `make` y pthreads; los clientes y el panel requieren Python 3.

## Estado del proyecto

El sistema fue desplegado y probado en una instancia EC2 de AWS Academy Learner Lab el 20 de septiembre de 2026. La ejecución documentada utilizó Amazon Linux 2023, una instancia `t3.small` en `us-east-1` y la imagen Docker `smart-telemetry:v1`.

Se comprobaron:

- construcción y ejecución del contenedor en EC2;
- acceso externo por DNS a TCP 5001 y HTTP 8080;
- recepción de telemetría mediante UDP 5000;
- funcionamiento simultáneo de cinco nodos;
- consultas desde dos clientes operador;
- detección de anomalías y generación de alertas;
- recuperación de nodos después de inactividad y reinicio;
- una prueba con 100 mensajes UDP transmitidos, 100 recibidos y 0 pérdidas estimadas en esa ejecución.

Las capturas y el análisis de Wireshark continúan pendientes para completar las evidencias de red de la entrega.

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

Configure el nombre DNS para que apunte a la IP pública de la instancia y abra los puertos en las reglas de la nube y el firewall del sistema. Pruebe desde una red externa. El repositorio no incluye credenciales, llaves privadas ni tokens de AWS.

## Despliegue comprobado en AWS

El despliegue documentado ejecuta el servidor y el panel en un mismo contenedor EC2 con estos puertos publicados:

| Servicio | Transporte | Puerto |
| --- | --- | ---: |
| Telemetría de los nodos | UDP | 5000 |
| Protocolo de consultas | TCP | 5001 |
| Panel web | HTTP sobre TCP | 8080 |

En EC2 se utilizó:

```sh
sudo docker build -t smart-telemetry:v1 .
sudo docker run -d \
  --name smart-telemetry \
  --restart unless-stopped \
  -e PYTHONUNBUFFERED=1 \
  -p 5000:5000/udp \
  -p 5001:5001/tcp \
  -p 8080:8080/tcp \
  smart-telemetry:v1
```

El Security Group utilizado durante las pruebas autorizó TCP 22, TCP 5001, TCP 8080 y UDP 5000 desde las direcciones públicas de los integrantes mediante rangos `/32`.

La dirección pública de una instancia de laboratorio puede cambiar al detenerla e iniciarla. Antes de ejecutar los clientes, consulte el **Public IPv4 DNS** vigente en EC2 y páselo como argumento:

```sh
python3 telemetry_nodes/launch_nodes.py DNS_PUBLICO_ACTUAL
python3 operator_client/client.py DNS_PUBLICO_ACTUAL
```

El panel queda disponible en:

```text
http://DNS_PUBLICO_ACTUAL:8080
```

El procedimiento aplicado, los datos, los resultados y las capturas de la ejecución están documentados en [docs/smart_telemetry_resultados_despliegue.md](docs/smart_telemetry_resultados_despliegue.md).

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

Durante el despliegue en AWS, esta prueba reportó:

```text
Transmitidos: 100
Recibidos: 100
Pérdidas UDP estimadas: 0
```

Este resultado corresponde únicamente a esa ejecución y no implica que UDP garantice la entrega. La prueba debe ejecutarse sin otros nodos activos para aislar correctamente el conteo.

También se verificó que los nodos dejan de aparecer como activos después de 15 segundos sin reportar y vuelven a registrarse al reanudar sus envíos. Al reiniciar el contenedor se pierden las lecturas y alertas anteriores porque el almacenamiento actual está en memoria.

## Documentación

- [Especificación del protocolo](docs/protocol.md)
- [Informe de despliegue y pruebas](docs/smart_telemetry_resultados_despliegue.md)

Las capturas de Wireshark deben obtenerse durante una ejecución real; el código no genera esas evidencias automáticamente.
