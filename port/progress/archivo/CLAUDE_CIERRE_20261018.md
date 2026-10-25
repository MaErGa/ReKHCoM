# Claude — cierre de su parte de la repartija (18/10/2026)

Rama `claude/saves` (checkout khcom-claude-int), sobre `c201169a9` de main.

## Hecho

- **Guardados, puntos 5–9: `e9f053595`.** Al cambiar de ranura se muestra lo que
  contiene la ranura (vacía si no hay partida), como la ventana original: decisión
  del usuario. Guardar en la otra ranura la convierte en la partida actual: se
  escribe solo su resumen y su cabecera, los autoguardados posteriores van ahí y
  el cristal vuelve a abrirse en ella (la marca va dentro de la partida). El
  launcher ya no borra `experimental_save_point_slot_select` de kh_com.ini,
  conserva las claves que no conoce y muestra la opción en Experimental.
  Prueba: harness `room:20000` + `mapsave` (EU): guardar en la 1, cambiar,
  guardar en la 2, reabrir. La 1 conserva 00:00:11, la 2 muestra 00:00:22 y el
  cristal reabre en la 2. EU/US/JP compilan.
- Issue #12 actualizado con estos arreglos (pendientes de publicar).
- **Codex:** los guardados no son tuyos. Descarta tu copia sin commitear de
  `src/map/map.c` y `port/decompui/decompui.cpp` en khcom-codex.

## Savestates (respaldados en build/userstates/nightly2 con su ejecutable)

- slot1 (04:27): pasillo de Hollow Bastion con Riku; la pared izquierda queda
  con el borde superior recto en vista amplia. Issue #17.
- slot3 (04:34): Mapamundi de Hollow Bastion; damero de la banda inferior.
  Al cargarlo a 16:9 y 32:9 la banda sale completa; falta confirmar con el
  usuario la zona exacta. Issue #18.

## Avance posterior (18/10, tarde) — en main 2fdca4ca7

- **8 [x]** Auditoría de persistencia: las 37 claves del juego están en el
  launcher, que además conserva claves desconocidas; el selector de ranura
  está en Experimental en el juego y en el launcher. Regresión:
  `port/tools/test_settings_keys.py` (PASS). Las opciones del juego se guardan
  al cerrar el menú y al salir; los controles, al asignarlos.
- **30 [x]** Trucos (Opciones → Trucos), commit 2fdca4ca7:
  - Interruptores guardados, apagados por defecto: enemigos caen de un golpe
    (9999 de daño) y vida infinita (0 de daño al jugador). Después de la
    dificultad Re:CoM; solo daño, las curas no cambian.
  - Acciones solo andando por una sala (mundo o vestíbulo del castillo):
    nivel 99 con todos los bonos (HP 560 = tope; Sora CP 2725 y los 11
    sleights de subida; Riku AP 30 y DP 204); cartas de mapa a 9 cada una
    (máximo que el juego admite por carta: 99 no cabe en su formato); colección
    de combate llena (999 huecos, máx. 99 por carta; una de cada carta y luego
    copias, Llaves espada primero; las cartas de mazos no se mueven). Riku no
    tiene colección: esa acción queda deshabilitada con él.
  - Prueba: harness `cheat:1/2/3` en Sora y Riku; combate con ambos
    interruptores y `KHCOM_DAMAGELOG` (enemigo 10→9999, jugador 4→0).
    Captura de la página: build/evidence/cheats_page_es.png. EU/US/JP compilan.
- **10 [~]** #17: reproducida la sala exacta del savestate (roomType 18,
  semilla 138815524) con los modos de harness `roomshape`/`roomseed`.
  Continuación de pared sobre la fila superior en 249aceff5, **apagada por
  defecto** (`KHCOM_TOPCLOSE=1`) hasta revisión visual del usuario: repite
  trozos de ornamento porque las celdas superiores no tienen pared lisa.
  Evidencia: build/evidence/top_close_hb_room3_pending.png.
- **11 [~]** #18: al cargar el savestate 3 a 16:9 y 32:9 la banda aparece
  completa; falta que el usuario señale la zona exacta.
