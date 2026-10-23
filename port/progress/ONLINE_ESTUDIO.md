# Online: estudio preliminar (23/10/2026)

Pedido del usuario (23/10): llevar el modo cable link (batallas VS de 2 jugadores) a online moderno manteniendo la
lógica del juego; ver si aplica rollback; estudiar cómo lo hicieron otros (decomps con online, Steam / AppID 480
"Spacewar"); reconstruir en alpha la lógica del multijugador.

## Cómo es el cable en el juego (src/sio)

- `sio.c` es el driver de modo multijugador de GBA (SIOCNT en modo multi, 115200 bps). El padre dispara las
  transferencias con el Timer 3 después de cada VBlank; cada transferencia intercambia una palabra de 16 bits de cada
  consola (SIOMULTI0..3).
- Encima del registro hay una **cola de paquetes**: un paquete son 4 palabras + 1 palabra de checksum (5
  transferencias). `SioQueueSendFrame` encola (32 lugares), `SioSendWord` / `SioRecvWord` los mueven por el cable,
  `SioReadRecvFrame` entrega al juego un paquete con las palabras **de los dos jugadores** de la misma ranura (la
  propia incluida, como en el hardware). Paquetes todo cero no cuentan; sin paquete el juego ve `recvEmpty`.
- Control de flujo propio: si el que recibe acumula más de 3 paquetes pide pausa (bit `SIO_PAUSE_REQUEST`) y el otro
  deja de encolar 5 VBlanks. Errores: checksum, cola llena, timeout (hijo sin transferencias 6 VBlanks).
- Quien lo usa: `chara.c` (`SioRunStateMachine` / `SioTransferFrames` una vez por cuadro, con `gSioSendFrame` y
  `gSioRecvFrame`), y encima `mode_sio.c`, `btl_vs.c`, `mode_vsbattle.c` (la batalla VS, el intercambio de naipes).

Conclusión: el juego ya tolera que un paquete tarde (cola, estado vacío, pausa). La red puede entrar **a nivel de
paquete**, reemplazando solo el transporte (registros, timer, interrupción) y dejando intacta toda la lógica de
`chara.c` / `mode_sio.c` / `btl_vs.c`. Es lo que el usuario pidió ("manteniendo su lógica").

## Opciones

1. **Cable virtual a nivel de registro** (como VBA-M / mGBA con link por red): cada transferencia de 16 bits viaja por
   la red. Fiel pero inviable: ~5 transferencias por cuadro, cada una espera la ida y vuelta (decenas de ms).
2. **Cable virtual a nivel de paquete** (elegida para la alpha): una ranura por cuadro de juego. Cada lado manda su
   paquete de la ranura n (aunque sea vacío) por UDP; cuando tiene el del otro arma el par (jugador 0, jugador 1) de
   la ranura n y lo deja en la cola de recepción del juego, igual para los dos. Con latencia el juego ve más
   cuadros "vacíos" y su propio control de flujo hace el resto. Sin predicción ni resimulación.
3. **Rollback (GGPO / GGRS)**: necesita que lo que se intercambia sean **entradas** y que la simulación sea
   determinista y se pueda guardar/restaurar cada cuadro. Los menús del enlace intercambian comandos (conectar,
   naipes, opciones): ahí no aplica. Pero **la batalla VS sí va por entradas** (`chara.c` `SioKeySyncSend/Recv`,
   comando 0x0ACD): cada consola manda sus teclas, el padre reenvía las de los dos y ambas simulan con las mismas, y
   `AgbMain` no avanza un cuadro sin el paquete (lockstep con demora). Rollback en la batalla es viable: predecir las
   teclas del otro (repetir las últimas), simular, y al llegar las reales restaurar con savestate (`port_state.c`,
   ~400 KB por estado) y resimular. Proyecto aparte, para después de la alpha del cable.

## Transporte y emparejamiento

- **UDP directo / LAN** (alpha): IP:puerto, sin servidor. Winsock en Windows, sockets BSD en Linux/Android/Switch.
  Paquete de red: número de ranura + 5 palabras + reenvío de las últimas ranuras no confirmadas (UDP pierde).
- **Steam** (después): Steamworks `ISteamNetworkingMessages` / `ISteamMatchmaking` (lobbies, invitar amigos, relay de
  Valve que atraviesa NAT). El AppID 480 ("Spacewar") es el de pruebas de Valve: sirve para desarrollar, **no para
  publicar** (todos los juegos de prueba comparten ese AppID y Steam no lo aprueba para distribución). Para publicar
  hace falta AppID propio (Steam Direct, 100 USD) o no usar Steamworks.
- **Sin Steam ni servidor propio**: GameNetworkingSockets (el código abierto de Valve, con ICE/WebRTC para NAT) o
  ENet + un servidor STUN público; para el navegador, WebRTC data channels. Un servidor de emparejamiento gratuito
  (Cloudflare Worker / Durable Object) puede repartir códigos de sala.
- Otros juegos/decomps: los ports de decomps con online (p. ej. los de Mario Kart 64 / Super Mario 64 coop, "sm64ex-coop")
  sincronizan **estado** propio por red, no el cable original; los emuladores (mGBA, VBA-M, NO$GBA) hacen link a nivel
  de registro, con los problemas de latencia de la opción 1. Nuestro caso (protocolo con cola) permite algo mejor que
  un emulador sin reescribir la lógica.

## Plan de la alpha (orden)

1. `port/src/port_netlink.c`: transporte UDP y ranuras; `KHCOM_NETLINK=host:<puerto>` o `join:<ip>:<puerto>` (luego
   en Opciones/launcher). Con la red activa, `SioRunStateMachine` / `SioTransferFrames` (bajo `PLATFORM_PC`) toman
   el handshake y la cola desde la red en vez del hardware; sin red todo queda como hoy.
2. Prueba con dos procesos locales ocultos (127.0.0.1) que entran al menú de enlace y a una batalla VS con
   `KHCOM_KEYS`; latencia y pérdida simuladas (`KHCOM_NETLINK_LAG`, `KHCOM_NETLINK_LOSS`).
3. Interfaz: opción "Enlace en red" con alojar/unirse e IP; luego códigos de sala.
4. Steam (lobby + relay) como transporte alternativo, si se consigue AppID.
5. Rollback solo si la batalla VS se reconstruye sobre entradas (opción 3).

## Estado de la alpha (23/10, `port/src/port_netlink.c` + `src/sio/sio.c`, solo PC y solo con `KHCOM_NETLINK`)

- Dos procesos ocultos en 127.0.0.1 (`mode_sio_battle` por harness, teclas por `KHCOM_KEYS`): handshake, conexión,
  intercambio de datos/barajas, menú de opciones VS y entrada a la batalla en los dos. Los dos ven exactamente la
  misma secuencia de pares (traza `KHCOM_NETLINK_TRACE`).
- Con 6 cuadros de demora por envío (~100 ms) y 10% de pérdida también llegan juntos a la batalla. Hizo falta subir
  el límite de reintentos de la aceptación (10 cuadros en el cable, 120 en red: `chara.c` SIO_CONNECT_RETRY_LIMIT).
- En la batalla las teclas viajan (DERECHA del jugador 1 llega reenviada a los dos), pero en las capturas la
  batalla se ve quieta (también sin demora): siguiente paso, ver por qué no avanza la batalla (¿espera algo del
  cartucho que el harness no da?) antes de la interfaz (alojar/unirse en Opciones).
