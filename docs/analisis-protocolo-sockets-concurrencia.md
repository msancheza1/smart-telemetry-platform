# Protocolo, sockets y concurrencia

Secciones 4, 5 y 6 del informe técnico. Las referencias señalan archivo y línea en el código del repositorio.

---

# 4. Diseño y especificación del protocolo

## 4.1. Criterios de diseño

El sistema emplea un protocolo propio de capa de aplicación, basado en texto ASCII, con dos familias de mensajes que viajan sobre transportes distintos. La decisión de no adoptar HTTP, MQTT ni ningún otro protocolo existente responde al requisito explícito del enunciado, pero además permite mantener el formato lo bastante simple como para que el servidor en C pueda analizarlo sin bibliotecas externas.

El formato se apoya en tres convenciones. Los campos se separan con la barra vertical `|`, carácter que no aparece en ningún valor legítimo. Los números decimales usan punto como separador, de modo que `strtof` en C y `float` en Python los interpreten igual. Y los identificadores de operación van siempre en el primer campo, lo que permite decidir el tipo de mensaje leyendo su inicio.

Una consecuencia deliberada de usar texto plano es que los mensajes resultan legibles en un analizador de red sin necesidad de decodificarlos, lo que facilitó el análisis documentado en la sección 8.

## 4.2. Mensajes de telemetría sobre UDP

Los nodos envían sus mediciones al puerto UDP 5000. Cada datagrama contiene un mensaje completo, con la estructura siguiente:

```text
TELEMETRY|<id>|TEMP|<valor>|HUM|<valor>|ENERGY|<valor>|VIB|<valor>
```

Un ejemplo real, tomado de la captura de tráfico:

```text
TELEMETRY|NODE04|TEMP|33.72|HUM|71.86|ENERGY|177.42|VIB|4.58
```

El mensaje consta de exactamente diez campos y el servidor rechaza cualquier variación. El analizador implementado en `server/src/protocol.c:18` aplica las siguientes reglas de validación antes de aceptar una medición:

| Regla | Comprobación | Referencia |
|---|---|---|
| Longitud del mensaje | Entre 1 y 1023 bytes | `protocol.c:28` |
| Separadores | No se admite `|` inicial ni final, ni `||` consecutivos | `protocol.c:28` |
| Número de campos | Exactamente 10, ni más ni menos | `protocol.c:32` |
| Etiquetas | `TELEMETRY`, `TEMP`, `HUM`, `ENERGY`, `VIB` en posiciones fijas | `protocol.c:37` |
| Identificador | De 1 a 31 caracteres alfanuméricos, `_` o `-` | `protocol.c:45` |
| Valores | Finitos y no negativos | `protocol.c:14` |
| Humedad | No superior a 100 | `protocol.c:54` |

El orden de los campos es fijo y no se admiten permutaciones. Un datagrama que incumpla cualquiera de estas reglas se descarta silenciosamente e incrementa un contador de mensajes inválidos, consultable mediante el comando `GET_STATS`. El servidor no emite respuesta ni confirmación alguna ante un mensaje de telemetría, sea válido o no, coherente con el carácter no orientado a conexión del transporte elegido.

## 4.3. Comandos de consulta sobre TCP

Los operadores consultan el sistema a través del puerto TCP 5001. Cada conexión transporta un único comando terminado en salto de línea; el servidor envía la respuesta y cierra la conexión. El límite por comando es de 127 bytes.

| Comando | Respuesta correcta | Error posible |
|---|---|---|
| `GET_NODES` | `OK\|NODES\|<n>\|<id>...` | — |
| `GET_STATUS\|<id>` | `OK\|NODE\|<id>\|TEMP\|<n>\|HUM\|<n>\|ENERGY\|<n>\|VIB\|<n>\|ACTIVE\|<0/1>` | `ERROR\|NODE_NOT_FOUND` |
| `GET_LATEST` | Igual al anterior, con prefijo `OK\|LATEST` | `ERROR\|NO_TELEMETRY` |
| `GET_ALERTS` | `OK\|ALERTS\|<n>\|<id>,<tipo>,<valor>...` | — |
| `GET_SYSTEM_STATUS` | `OK\|SYSTEM\|NODES\|<n>\|ALERTS\|<n>` | — |
| `GET_STATS` | `OK\|STATS\|RECEIVED\|<n>\|INVALID\|<n>` | — |

Toda respuesta comienza por `OK` o por `ERROR`, lo que permite al cliente decidir cómo interpretarla leyendo el primer campo. Los códigos de error definidos son seis:

`NODE_NOT_FOUND` cuando se consulta un identificador no registrado. `NO_TELEMETRY` cuando se solicita la última medición y el servidor aún no ha recibido ninguna. `UNKNOWN_COMMAND` ante un comando no reconocido. `EMPTY_COMMAND` ante una línea vacía. `COMMAND_TOO_LONG` cuando se superan los 127 bytes. E `INCOMPLETE_COMMAND` cuando la conexión se cierra o expira antes de que llegue el salto de línea terminador.

Los cuatro últimos se generan en `server/src/tcp_server.c:102` antes siquiera de intentar interpretar el comando, mientras que los dos primeros dependen del estado del servidor.

## 4.4. Intercambio de ejemplo

El siguiente diálogo, reconstruido a partir de la captura de tráfico real documentada en la sección 8, ilustra una consulta completa:

```text
Operador → Servidor:  GET_NODES\n
Servidor → Operador:  OK|NODES|5|NODE01|NODE02|NODE03|NODE04|NODE05
```

Y un caso de error, observado durante las pruebas cuando se introdujo un identificador incompleto:

```text
Operador → Servidor:  GET_STATUS|3\n
Servidor → Operador:  ERROR|NODE_NOT_FOUND
```

## 4.5. Semántica del estado

Dos reglas del protocolo dependen del estado que mantiene el servidor y conviene documentarlas porque afectan a la interpretación de las respuestas.

Un nodo se considera **activo** si reportó en los últimos 15 segundos, umbral definido en `server/src/models.h:7`. El comando `GET_NODES` lista únicamente los activos, mientras que `GET_SYSTEM_STATUS` cuenta todos los registrados desde el arranque del servidor. Un nodo detenido, por tanto, desaparece del primer listado pero sigue contando en el segundo.

Las alertas se conservan en un búfer circular de 100 posiciones. Cuando se llena, cada alerta nueva sobrescribe la más antigua, de modo que `GET_ALERTS` devuelve siempre las cien más recientes en orden cronológico. Durante las pruebas con cinco nodos activos ese búfer se renovaba por completo en menos de dos minutos.

Todo el estado reside en memoria y se pierde al reiniciar el servidor o el contenedor.

---

# 5. Implementación de sockets TCP y UDP

## 5.1. Socket UDP del servidor

El servidor crea el socket de telemetría en `server/src/udp_server.c:57` mediante `socket(AF_INET, SOCK_DGRAM, 0)`, donde `SOCK_DGRAM` selecciona el servicio de datagramas. A continuación asocia la dirección en `udp_server.c:111` con `bind`, empleando `INADDR_ANY` para aceptar tráfico dirigido a cualquier interfaz del equipo, condición necesaria para que el contenedor reciba paquetes procedentes de Internet.

No existen llamadas a `listen` ni a `accept`, porque UDP no establece conexiones: el socket queda directamente en disposición de recibir. La recepción se realiza en un bucle infinito mediante `recvfrom` en `udp_server.c:152`, función que además entrega la dirección del emisor, lo que permite registrar en consola la IP de origen de cada medición.

El tratamiento de errores distingue dos situaciones. Los fallos en la creación del socket o en el `bind` son irrecuperables y terminan el proceso con un mensaje de diagnóstico. En cambio, un error puntual en `recvfrom` no detiene el servidor: el código lo reporta y continúa el bucle, de modo que un paquete problemático no interrumpe el servicio para los demás nodos.

Existe además una comprobación defensiva en `udp_server.c:176`. Antes de tratar el contenido recibido como una cadena de texto, el servidor descarta los datagramas que ocupan todo el búfer —señal de posible truncamiento— y los que contienen bytes nulos intercalados, que romperían el análisis posterior. Solo tras superar esa comprobación se añade el terminador nulo y se invoca al analizador del protocolo.

## 5.2. Socket TCP del servidor

El socket de consultas se crea en `server/src/tcp_server.c:123` con `socket(AF_INET, SOCK_STREAM, 0)`. Antes de asociarlo se activa la opción `SO_REUSEADDR` en `tcp_server.c:126`, que permite reutilizar el puerto inmediatamente tras un reinicio del servidor sin esperar a que expiren las conexiones en estado `TIME_WAIT`. Esta opción resultó especialmente útil durante las pruebas de reinicio del contenedor.

La secuencia continúa con `bind` y `listen` en `tcp_server.c:131`, este último con una cola de diez conexiones pendientes. El servidor entra entonces en un bucle permanente de `accept` en `tcp_server.c:141`, que bloquea hasta que un operador solicita conexión y devuelve un descriptor nuevo dedicado a ese cliente.

La recepción del comando se realiza byte a byte en `tcp_server.c:94` hasta encontrar el salto de línea, descartando los retornos de carro para tolerar clientes que terminen las líneas al estilo de Windows. Este enfoque evita tener que gestionar la fragmentación de TCP: dado que el protocolo delimita los comandos por salto de línea y no por longitud, leer hasta el delimitador garantiza que se procese un comando completo con independencia de cómo se hayan repartido los bytes entre segmentos.

Para impedir que un cliente que abra una conexión y no envíe nada mantenga un hilo bloqueado indefinidamente, se establece un tiempo máximo de espera de tres segundos mediante `SO_RCVTIMEO` en `tcp_server.c:91`. Si el plazo expira sin haber recibido el terminador, el servidor responde `ERROR|INCOMPLETE_COMMAND` y cierra.

El envío de la respuesta, en `tcp_server.c:112`, se realiza dentro de un bucle que contabiliza los bytes efectivamente transmitidos y reintenta hasta completar el total. Esta precaución es necesaria porque `send` puede transmitir menos bytes de los solicitados cuando el búfer del sistema está lleno, situación verosímil con respuestas extensas como la de `GET_ALERTS`, que llegó a ocupar cerca de tres mil bytes durante las pruebas. Finalmente, el descriptor se cierra en `tcp_server.c:117`.

## 5.3. Sockets en los clientes

Los nodos de telemetría, implementados en Python, comienzan resolviendo el nombre del servidor mediante `socket.gethostbyname` en `telemetry_nodes/node.py:43`. Esta llamada es la que materializa el requisito de localizar el servidor por DNS: el código no contiene ninguna dirección IP, y el nombre se recibe como argumento de línea de órdenes. Un fallo de resolución se captura como `socket.gaierror` y el nodo termina con un mensaje explicativo en lugar de abortar con una traza.

El socket se crea en `node.py:64` con `socket.SOCK_DGRAM` y el envío se realiza con `sendto` en `node.py:90`, indicando destino en cada llamada, puesto que un socket UDP no conserva un interlocutor fijo. El cierre se garantiza mediante un bloque `finally` en `node.py:112`, que se ejecuta incluso cuando el nodo se detiene con una interrupción de teclado.

El cliente operador, en `operator_client/client.py`, crea un socket `SOCK_STREAM` en `client.py:13`, fija un tiempo máximo de espera de cinco segundos en `client.py:15` y establece la conexión con `connect` en `client.py:16`. El envío emplea `sendall` en `client.py:47`, que a diferencia de `send` garantiza la transmisión completa del comando. La recepción se realiza en un bucle en `client.py:51` que acumula fragmentos hasta que `recv` devuelve una cadena vacía, condición que en TCP señala que el otro extremo ha cerrado su lado de la conexión; ese es precisamente el mecanismo por el que el cliente detecta el final de la respuesta, dado que el protocolo no incluye un delimitador explícito de fin de mensaje.

El tratamiento de errores del cliente distingue tres casos con mensajes específicos: `ConnectionRefusedError` cuando el servidor no está escuchando, `socket.gaierror` cuando falla la resolución DNS y `OSError` para el resto de fallos de comunicación. En todos los casos el cliente informa y regresa al menú en lugar de terminar.

## 5.4. Correspondencia con las operaciones exigidas

| Operación | Servidor UDP | Servidor TCP | Clientes |
|---|---|---|---|
| Creación | `udp_server.c:57` | `tcp_server.c:123` | `node.py:64`, `client.py:13` |
| Asociación y escucha | `udp_server.c:111` | `tcp_server.c:131` | — |
| Aceptación de clientes | no aplica | `tcp_server.c:141` | — |
| Conexión | no aplica | no aplica | `client.py:16` |
| Envío | no aplica | `tcp_server.c:112` | `node.py:90`, `client.py:47` |
| Recepción | `udp_server.c:152` | `tcp_server.c:94` | `client.py:51` |
| Cierre | `udp_server.c:255` (*) | `tcp_server.c:117` | `node.py:112`, `client.py:68` |
| Manejo de errores | `udp_server.c:163` | `tcp_server.c:102` | `client.py:22` |

(*) La llamada a `close` del socket UDP está escrita tras el bucle de recepción, que es infinito, por lo que en la práctica no llega a ejecutarse: el descriptor lo libera el sistema operativo al terminar el proceso. Es aceptable en un servicio cuyo ciclo de vida coincide con el del programa, pero se señala por rigor. El cierre efectivo y verificable en cada operación corresponde al socket TCP de cada cliente, que sí se cierra explícitamente al completar la respuesta.

---

# 6. Concurrencia y programación de clientes y servidor

## 6.1. Estrategia adoptada

El servidor atiende simultáneamente dos tipos de tráfico con requisitos distintos: un flujo continuo de datagramas procedente de cinco o más nodos, y consultas esporádicas de varios operadores. La estrategia elegida fue el uso de **hilos POSIX**, en dos niveles.

En el primer nivel, `server/src/main.c:57` y `main.c:73` crean sendos hilos permanentes, uno por cada servicio. Ambos ejecutan bucles infinitos sobre sus respectivos sockets, de modo que la recepción de telemetría y la atención de operadores progresan de forma independiente: una consulta TCP lenta no retrasa el procesamiento de mediciones, y viceversa. El hilo principal queda bloqueado en `pthread_join` en `main.c:90`, manteniendo vivo el proceso.

En el segundo nivel, el hilo TCP crea un hilo adicional por cada operador que se conecta, en `tcp_server.c:144`. Ese hilo se marca como desvinculado mediante `pthread_detach` en `tcp_server.c:149`, lo que permite que el sistema libere sus recursos automáticamente al terminar, sin que nadie tenga que recogerlo. Gracias a este esquema el servidor atiende un número arbitrario de operadores concurrentes, muy por encima de los dos que exige el enunciado.

Se descartó el uso de procesos independientes porque los tres componentes del servidor comparten el mismo estado —el registro de nodos y el historial de alertas—, y con procesos habría sido necesario recurrir a memoria compartida o a algún mecanismo de comunicación entre procesos, aumentando la complejidad sin beneficio. También se descartó la multiplexación con `select` o `poll`, que habría evitado los hilos a costa de una máquina de estados más difícil de leer y de justificar en la sustentación.

## 6.2. Protección del estado compartido

Al compartir estado entre hilos aparece el riesgo de que dos accesos simultáneos dejen las estructuras en un estado inconsistente. El servidor lo evita con dos exclusiones mutuas independientes.

La primera, declarada en `server/src/telemetry.c:11`, protege el arreglo de nodos y la última medición recibida. La adquiere tanto el hilo UDP al registrar una medición como cualquier hilo de operador al consultar. La segunda, en `server/src/alerts.c:10`, protege el búfer circular de alertas.

Haberlas separado no es accidental: una consulta de alertas no bloquea el registro de una medición, porque cada operación toma únicamente el cerrojo que necesita.

El código emplea además un patrón de instantánea que merece mención. Funciones como `get_all_nodes`, en `telemetry.c:51`, adquieren el cerrojo, copian los datos a un arreglo local y lo liberan de inmediato; el formateo de la respuesta se realiza después, ya fuera de la sección crítica. De este modo el tiempo durante el cual el estado permanece bloqueado se reduce al mínimo imprescindible, incluso cuando la respuesta que se va a construir es extensa.

## 6.3. Contadores sin bloqueo

Los contadores de mensajes recibidos e inválidos, declarados en `server/src/udp_server.c:24`, utilizan el tipo `atomic_ulong` de C11 en lugar de una exclusión mutua. Son valores que se incrementan en cada datagrama procesado —del orden de una vez por segundo con cinco nodos activos— y que se leen esporádicamente desde los hilos de operador.

Emplear un cerrojo para proteger un simple incremento habría introducido contención innecesaria en la ruta más frecuente del servidor. Las operaciones atómicas garantizan que el incremento sea indivisible sin necesidad de bloquear a nadie.

Estos contadores son los que sustentan el comando `GET_STATS` y, con él, la prueba de tráfico UDP que reportó 100 mensajes transmitidos y 100 recibidos.

## 6.4. Robustez frente a desconexiones

El enunciado exige que el servidor maneje desconexiones y errores sin finalizar abruptamente. Tres mecanismos lo garantizan.

El primero es la desactivación de la señal `SIGPIPE` en `main.c:37`. Sin esa línea, si el servidor intentara escribir en un socket cuyo extremo remoto ya se ha cerrado —situación normal cuando un operador interrumpe el cliente a mitad de una consulta—, el sistema operativo terminaría el proceso completo. Al ignorar la señal, la escritura fallida se convierte en un simple error que el bucle de envío detecta y trata.

El segundo es el tratamiento local de los errores en los bucles de servicio: tanto un fallo de `recvfrom` como uno de `accept` o de `pthread_create` provocan que el servidor libere lo que corresponda y continúe con la siguiente iteración, en lugar de abortar.

El tercero es la validación estricta del protocolo. Un mensaje malformado nunca alcanza las estructuras de datos, porque el analizador lo rechaza antes; se limita a incrementar el contador de inválidos.

Estos mecanismos se verificaron durante las pruebas. Al detener los cinco nodos, el servidor continuó operativo y las consultas pasaron a reportar cero nodos activos; al reanudarlos, los cinco volvieron a registrarse sin intervención manual.

## 6.5. Concurrencia en los clientes

Los cinco nodos se ejecutan como **procesos independientes**, lanzados desde `telemetry_nodes/launch_nodes.py:15` mediante `subprocess.Popen`. Se optó por procesos y no por hilos porque cada nodo simula un dispositivo físicamente separado, y con procesos distintos se puede detener uno solo para observar el comportamiento del servidor ante la inactividad de un dispositivo, tal como se hizo en las pruebas de recuperación. El lanzador captura la interrupción de teclado y termina ordenadamente todos sus hijos.

El cliente operador, en cambio, es deliberadamente secuencial: presenta un menú, espera una elección y realiza una consulta cada vez. La concurrencia entre operadores se produce porque varios clientes se ejecutan simultáneamente en equipos distintos, no porque un mismo cliente emita consultas en paralelo.

## 6.6. Limitaciones conocidas

El diseño presenta tres límites que conviene declarar.

El registro de nodos tiene capacidad fija para 100 dispositivos, definida en `models.h:5`; superado ese número, los nuevos nodos se descartan silenciosamente. El historial de alertas conserva únicamente las 100 más recientes. Y todo el estado reside en memoria, por lo que un reinicio del servidor o del contenedor lo elimina por completo, comportamiento que se observó durante las pruebas de reinicio.

Ninguna de estas limitaciones afecta a los requisitos del proyecto, que exige cinco nodos y no establece requisitos de persistencia, pero todas serían relevantes en un despliegue real.
