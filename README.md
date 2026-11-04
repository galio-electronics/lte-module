# LTE modules para CCS C

Driver inicial SIMCom A7670/A76XX por AT, probado a nivel de compilacion con
CCS PCH 5.081. Validado en Protolink Station con SIM y broker MQTT el 3 de noviembre de 2026.

- `common/lte_at.h`, `common/lte_at.c`: parser incremental, prompt, respuestas
  OK/ERROR, evento asincrono esperado, timeout y deteccion de desbordamiento.
- `simcom/a7670.c`: registro de red, APN y publicacion MQTT mediante CMQTT.
- `examples/lte_config.h`: configuracion de ejemplo, sin credenciales reales.

Para agregar otra marca, crear su carpeta y reutilizar el transporte AT.
El codigo usa tipos y formato numerico CCS; no es C ISO portable sin adaptacion.
Una instancia y un cliente MQTT (indice 0); no es reentrante.

## Integracion

Incluir configuracion y definir estos hooks antes de incluir `simcom/a7670.c`
una sola vez (estilo CCS):

```c
#define LTE_PUTC(c) board_uart_write(c)
#define LTE_AVAILABLE() board_uart_available()
#define LTE_GETC() board_uart_read()
#define LTE_MILLIS() board_millis()
#define LTE_RX_FAULT() board_rx_overflow()
#include <lte-module/simcom/a7670.c>
```

Usar recepcion UART por interrupcion y un reloj monotono uint32 en ms.
Los hooks no deben bloquear salvo la transmision de un byte. El port debe
garantizar los niveles electricos y manejar PWRKEY/DTR; la libreria no fija pines.
Habilitar `#device PASS_STRINGS=IN_RAM` antes de usar literales con CCS.

Llamar `a7670_init()` despues de encender/despertar el modem y `a7670_task()`
frecuentemente desde el loop. `a7670_ready()` permite aceptar un mensaje con
`a7670_publish(char *json)`. Retornar TRUE significa aceptado en memoria;
`a7670_published` aumenta solo al recibir OK y `+CMQTTPUB: 0,0`.

La maquina es cooperativa: los tiempos de red no bloquean el loop. El envio
UART de cada comando/payload si es sincrono (hasta unos 23 ms a 115200).
Longitudes maximas: APN/host 63, client ID/user/password 48, topic 100,
payload 239 bytes. Strings de configuracion no admiten comillas ni controles.
Configurar puerto 1..65535 y un client ID unico por dispositivo.

## Comportamiento y limites

Inicio/reintento despues de LTE_RETRY_MS: AT, echo off, limpieza DISC/REL/STOP,
CFUN, SIM lista, registro LTE local o roaming, APN, START, ACCQ, CONNECT.
Publicacion: TOPIC/prompt/datos, PAYLOAD/prompt/datos, PUB QoS 1 sin retain.
El parser acepta el evento antes o despues de OK. No confunde OK con confirmacion
de conexion/publicacion. ERROR y resultados MQTT no exitosos reintentan con espera.
Los timeouts de AT/SIM/registro reintentan; otros timeouts, overflow o perdida
de enlace durante transferencia de datos dejan estado 255 para evitar enviar
comandos dentro de un payload incompleto. Recuperar reiniciando modem y MCU.
No se pulsa PWRKEY repetidamente como mecanismo de recuperacion.

Estados: 0 espera; 1..14 conexion; 20 listo; 21..25 publicando; 255 fallo detenido.
`a7670_last_error`: 3 ERROR/resultado MQTT fallido, 4 timeout, 5 overflow,
10 configuracion invalida, 11 perdida de enlace durante datos. Se borra al conectar.
No hay cola persistente, retransmision de aplicacion ni historial offline.
QoS 1 puede entregar duplicados. Sin suscripciones, TLS, PIN de SIM ni autenticacion
PAP/CHAP del APN; se requiere SIM desbloqueada y APN sin esas credenciales.
MQTT user/password son opcionales y viajan sin cifrar en esta version TCP.

## Demo y validacion

La demo vive en `protolink/firmware/apps/station-qdy30a-lte` en el repositorio
hermano. UART5 RE2/RE3, KEY RD2 activo bajo, SLEEP/DTR RD1 bajo despierto;
sensor QDY30A en UART4 RS485. Ver README de la demo para compilar y conectar.

Verificacion de banco pendiente: SIM ausente/PIN, sin cobertura, roaming,
APN incorrecto, broker rechazando credenciales, publicacion recibida, perdida
de red, reinicio solo del PIC y timeout del prompt. En cada caso confirmar
que el contador published solo crece con confirmacion y no se altera el sensor.

Referencia primaria: [SIMCom A76XX AT Manual V1.09, seccion MQTT](https://files.waveshare.com/wiki/A7670E-Cat-1-GNSS-HAT/A76XX_Series_AT_Command_Manual_V1.09.pdf).
Secuencia KEY/DTR: [SIMCom A7670 Hardware Design V1.03](https://cdn.tdlogy.com/public/Datasheet/4G-5G/A7670%20Series_Hardware%20Design_V1.03.pdf).

## Prueba en placa 2026-11-03

A7670 UART5 con QDY30A UART4: SIM/registro/servicio MQTT y conexion exitosos.
14 publicaciones QoS 1 consecutivas confirmadas por +CMQTTPUB: 0,0, cada 5 s.
Se omite password en CMQTTCONNECT cuando esta vacio, conservando el usuario;
el argumento vacio explicito producia ERROR en el modem probado.
Compilacion: 0 errores, 0 advertencias, ROM 14%, RAM 46-56%.
La validacion llega a la confirmacion MQTT del modem, no al dashboard remoto.
Los escenarios de falla enumerados arriba siguen pendientes.

El hook opcional LTE_AT_LINE_HOOK(line) recibe cada linea completa para
instrumentacion; no debe bloquear ni emitir comandos. Filtrar antes de registrar
para no exponer comandos eco, credenciales o mensajes de aplicaciones.
