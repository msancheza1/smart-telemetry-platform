# Protocolo de aplicación de Smart Telemetry

## Transportes

Los nodos envían mediciones por UDP al puerto 5000. Cada datagrama contiene una medición completa. UDP permite envíos periódicos con poco costo; si se pierde una medición, la siguiente la reemplaza. El servidor no envía confirmación UDP.

Los operadores consultan por TCP al puerto 5001. Cada conexión lleva un comando ASCII terminado en salto de línea (`\n`). El servidor envía una respuesta ASCII y cierra la conexión. TCP conserva el orden y permite detectar errores de conexión en las consultas.

Los campos principales se separan con `|`. Los decimales usan punto. El servidor acepta hasta 1023 bytes por datagrama y un comando de hasta 127 bytes.

## Telemetría UDP

```text
TELEMETRY|<id>|TEMP|<numero>|HUM|<numero>|ENERGY|<numero>|VIB|<numero>
```

Ejemplo:

```text
TELEMETRY|NODE03|TEMP|24.8|HUM|61.5|ENERGY|310|VIB|2.1
```

`id` tiene de 1 a 31 caracteres ASCII alfanuméricos, `_` o `-`. Todos los valores son finitos y no negativos; `HUM` no supera 100. Un datagrama inválido se descarta y aumenta el contador `INVALID` de `GET_STATS`.

## Consultas TCP

| Comando | Respuesta correcta | Error posible |
| --- | --- | --- |
| `GET_NODES` | `OK|NODES|<n>|<id>...` | — |
| `GET_STATUS|<id>` | `OK|NODE|<id>|TEMP|<n>|HUM|<n>|ENERGY|<n>|VIB|<n>|ACTIVE|<0/1>` | `ERROR|NODE_NOT_FOUND` |
| `GET_LATEST` | Igual al estado de nodo con prefijo `OK|LATEST` | `ERROR|NO_TELEMETRY` |
| `GET_ALERTS` | `OK|ALERTS|<n>|<id>,<tipo>,<valor>...` | — |
| `GET_SYSTEM_STATUS` | `OK|SYSTEM|NODES|<n>|ALERTS|<n>` | — |
| `GET_STATS` | `OK|STATS|RECEIVED|<n>|INVALID|<n>` | — |

Los comandos desconocidos devuelven `ERROR|UNKNOWN_COMMAND`; los vacíos, `ERROR|EMPTY_COMMAND`; los demasiado largos, `ERROR|COMMAND_TOO_LONG`; y los que no terminan en salto de línea, `ERROR|INCOMPLETE_COMMAND`. `GET_NODES` incluye solo nodos con reporte en los últimos 15 segundos. `GET_SYSTEM_STATUS` cuenta todos los nodos registrados desde el arranque. `GET_ALERTS` conserva las 100 alertas más recientes en orden cronológico.

Intercambio de ejemplo:

```text
Operador -> Servidor: GET_STATUS|NODE03\n
Servidor -> Operador: OK|NODE|NODE03|TEMP|24.80|HUM|61.50|ENERGY|310.00|VIB|2.10|ACTIVE|1
```

El servicio web en el puerto 8080 usa HTTP para el navegador y consulta el servidor central mediante este protocolo TCP. La comunicación principal de telemetría y operaciones sigue usando el protocolo de texto anterior.
