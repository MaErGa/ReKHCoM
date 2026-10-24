# Cola vigente hacia la v2.0 (limpiada el 24/10/2026 ~05:00)

Única cola de trabajo. Lo terminado sale de aquí (queda en git y en CHANGELOG.md); los traspasos y estados viejos
están en **PENDIENTES_HISTORIAL_2026-10.md** (evidencia, comandos, hallazgos). Método de depuración: DEBUG_WORKFLOW.md.

Reglas del usuario (completas en REGLAS_DEL_USUARIO.md):
- Claude y Codex pushean a main; si uno lleva más de 10 min sin commits ni archivos tocados, el otro toma sus puntos
  anotando hora y evidencia aquí. Cada pieza verificada se pushea enseguida y se anota aquí en el momento.
- Lo visual que necesite ojo humano va a la app de revisión (`Revisar preliminares.bat`, `import_image.py ... --parent
  <id>`); solo se integra lo aprobado ahí (con honores para el arte). Lógica aprobada: LOGICA_APROBADOS.md.
- La validación en hardware real no es pendiente: llega como reporte. Publicar (releases, comentarios) solo con OK.
- Nunca borrar carpetas ReKHCoM-vX ni ningún release (Quiver nos aceptó el 24/10).

## Aprobaciones nuevas 24/10 13:12–13:17 (Codex)

- **053 con honores integrada para todos**: referencia `553e59d173b14e00bc61c1d513188719`, receta de
  `26396b610`, lista única `kPreliminarApproved`. `KHCOM_SCENE053_WIDE=0` la apaga; no pide bandera preliminar.
  `test_preliminar_v1.py --approved --build build/pc --out build/approved-053
  --only escena-053,evento-177-escena-053 --subject 53`: **24/24 EU**, cuatro anchos × tres cuadros × ambos
  eventos, GBA idéntico, márgenes distintos. `check_sources.py`: ok. El modo `--approved` comprueba el camino
  por defecto del jugador para no confundir una receta de prueba con una integración real.
- **054 con honores, implementación lista para PR** (`18f40e19e5814241a966b3348a542977`): raíz
  toma este carril a las 13:14 Chile. Lista única `kPreliminarApproved`; `KHCOM_SCENE054_WIDE=0` la apaga.
  `test_preliminar_v1.py --approved --build build/pc --out build/approved-054
  --only escena-054,evento-178-escena-054 --subject 54`: **24/24 EU**, GBA idéntico y márgenes distintos
  en ambos eventos, cuatro anchos y tres cuadros. A partir del pedido de las 13:16 todo cambio nuevo va por PR.
- **Carriles tomados 24/10 13:16–13:17:** root integración 054; queue_tools herramienta de toma y exclusión
  online; queue_docs inventario y protocolo para Claude; art_110 escena 110. Worktrees separados. No tomar
  esas piezas mientras sigan activas. El protocolo nuevo sustituye el push directo a main.
- Aprobaciones sin honores recibidas: 044, 045, 098, 104 y Darkside 03 r6. Recetas ya subidas a main;
  se conservan las valoraciones por versión. Criterios y referencias guardados en `LOGICA_APROBADOS.md`.
- Nuevos comentarios a las hijas 087, 090, 095, 099 y 102 y al primer arranque: están en la cola de la app;
  no contarlas como cerradas por haber enviado una revisión anterior. Después de las integraciones, leer esos
  envíos junto a la cola restante (110 y siguientes).

## TRASPASO 24/10 13:14 (Chile, Codex — retoma tras cierre de Claude)

- Base recuperada: main `bfaa5e76c`; 098, 099 y 102 ya respondidas en la app (faltaban en el resumen).
  Trabajo aislado en `khcom-resume`, rama `codex/resume-20261024`. Los dos archivos WIP de Claude (104,
  última escritura 12:45) se copiaron para completarlos; su árbol y el WIP viejo de `khcom-codex` quedan intactos.
- **104 respondida en la app**: hija `52df9ff43f4e48b4a722240c723bc2f8` de
  `b462df0229ba4e74bd8e9854356fbfc9`, responde las cinco nubes del envío `20261024-055457-55b960`.
  Brillos/arcos completos desde columnas 40..43, algas cerradas con sus puntas; suelo y borde delantero
  prolongados en diagonal, sin tomar la fila vacía 31 ni el rectángulo inferior que generaba cortes negros.
  Activadas las dos filas 104 de la tabla **solo para Preliminar V1**: estaban apagadas y las últimas capturas
  de Claude eran iguales con/sin receta. 104 aprobada sin honores a las 13:12: no se añade a la lista de honores.
  Prueba EU: `python port/tools/test_preliminar_v1.py --build build/pc --out build/resume-104-verified
  --only escena-104,evento-106-escena-104 --subject 104`: 2 eventos × 4 anchos × 3 cuadros = **24/24**, GBA idéntico,
  márgenes distintos, 0 CRASH. Revisión visual a x3 de la captura 3440x1440. `check_sources.py`: ok.
  Evidencia local en `khcom-resume/build/resume-104-verified`; captura entregada en `build/prelim/104-after.png`.
- **Siguiente pieza: 110**, después 112, 115, 116, 117, 118, 120; también 081 (ventana), 097 (sprites),
  Darkside arena y jefes V2 (Oogie, Trickmaster, Maléfica 01). Leer la cola viva de la app antes de cada pieza.
- Las baterías sobrevivieron a Claude: **dos coordinadores** (PID 31852/31856) comparten la salida de skip JP;
  no se lanzó una tercera ni se detuvo ninguna. Al retomar: monkey EU/US/JP y skip EU/US terminados,
  skip JP en curso; siguen sin cerrar rooms EU, battles EU y Android. Revisar `run_batteries.py --status`
  en `khcom-claude-int`, que es donde viven sus resultados (los de este checkout aislado empiezan vacíos).
- No se publicó ningún release ni se cambiaron copias de `jugar/`. La v2.0 estable sigue esperando OK.

## Estado (24/10 ~06:30, Claude — sesión nueva, /goal: terminar todo lo de antes de la v2.0)

- **App de revisión 0.5.0 (b32647332, en main; la app abierta se actualiza sola):** pestañas Pre / Rev / Aprobado
  (subpestañas Aprobados y ★ Honores; Rev = ya enviaste nubes y se espera la versión hija), lápiz con 8 colores, 3
  grosores, borrador, Ctrl+Z y Limpiar (los trazos van al envío: `<id>-garabatos.png` + color y caja de cada trazo),
  zoom de visor (rueda al cursor por escalones, + − 0 1, casilla de %, doble clic ajustar/1:1, herramienta Mover).
  Arreglado: un envío con solo comentario general o trazos se daba por respondido (lista de nubes vacía).
- **Escena 055 con honores (48bef52c7):** lista única de escenas Preliminar V1 con honores (55, 121, 122) en
  port_ppu.c. El juego actual ya mostraba la imagen aprobada (banda morada continua): 0 píxeles cambian.
- **Motor de Switch hecho (72212d3a5, 5c214462d; detalle en SWITCH_MOTOR.md):** las 160 filas de la imagen GBA en
  varios núcleos (copias por línea de registros/afín/paleta/OAM tras los eventos de línea), hilos reales en la imagen
  (entrada del puente `thread`, pilas propias, TLS propio). PC: `build/inner_verify.sh` 60/60 sin diferencias
  (EU/US/JP); VM ARM64 (host Linux, 3 núcleos): capturas idénticas a 1 núcleo, 0 diferencias en verificación, PPU
  19 → 13 ms aun emulado. Arreglados: getenv/malloc del invitado con cerrojo, `SDL_expf/sqrtf` en el shim (Switch y
  Android 64 no compilaban por los botones nuevos). Encendido solo en Switch; en PC `KHCOM_PPU_INNER=1`.
  `.nro` compilándose al escribir (`build/switch/nro-build.log`). Switch sigue fuera de la v2.0 (decisión del
  usuario): vuelve a un release cuando haya medida en consola real.
- **Editor de posición de botones táctiles (2e613ab45):** Opciones > "Mover botones táctiles": juego en pausa,
  se arrastra cada grupo, ✓ guarda (`touch_<grupo>_x/_y` en kh_com.ini), ↺ restablece. test_android_touch lo
  recorre. Captura en la app de revisión (Pre). Falta verlo en el emulador Android y en la web (punto 5).
- **TRASPASO de Codex (24/10 ~14:30, Claude):** Codex sin commits ni cambios desde 02:15 (último commit 395beb69d hace
  8 h; su árbol khcom-codex con WIP sin commitear de escena 124 en port_ppu.c y preliminar_v1_*). Por la regla de los
  10 min, Claude toma su cola de la app de revisión (escenas V1 044-120 con nubes, jefes V2 Oogie/Trickmaster/Maléfica,
  Darkside arena). Su WIP queda como está (no se toca su árbol).
- **Cola de arte tomada de Codex, en curso (Claude):** WIP de Codex integrado tal cual (ab1389aa5, escena 124; para
  jugadores nada cambia: eventos 119-125 idénticos). Método por escena: mapa con rejilla desde
  `build/preliminar-v1/inventory/eu/gEventBgNNN.png` (inventory_preliminar_v1.py), corregir la receta en
  port_preliminar_v1_tiles.h, `sh build/prelim_shot.sh NNN` (3440x1440, con y sin receta), mirar, y
  `answer_scene.py NNN --change "..."` (versión hija en la app que responde el envío). Hechas (respondidas en la app): **090** (3149de052), **044**, **045** (bc010848a), **053** (26396b610,
  gen_scene053_plateau.py), **054** (ab3753b8a), **087** (0872e65d1), **095** (df97e8cf8).
  **081** revertida: el patrón no repite bien con la ventana actual (necesita extender la ventana; queda para después).
  **097** (gato de Cheshire visible fuera de la zona GBA): es lógica de sprites, no de receta; anotada.
  Faltan: 110 112 115 116 117 118 120, 081, 097, jefes V2 (Oogie, Trickmaster, Maléfica 01),
  Darkside arena.
- **Darkside r6 en la app (3f5f7c235):** 03 el ala (mechones que siguen su pendiente y se difuminan), 06 el brazo nace
  en el hombro (23 px más arriba), costura redibujada. Responde los envíos 052330 y 054756.
- **Calidad HD (punto 7) cerrado:** calidad constante por GPU vs "Buena" en 5 videos (`build/hdprobe-2410.txt`):
  Buena 266,9 MB VMAF 97,4; Q75 243,9 MB (−9 %) 96,6; Q80 282,7 MB (+6 %) 97,2; Q85 344,9 MB 97,8. Ninguna gana:
  se queda "Buena". Ahorro medido (751 MB en el 39 % del mod) ≈ 2 GB: coincide con `kScenesSize`.
- **Web (build EU de prueba):** botones táctiles nuevos dibujados sobre el juego en tamaño móvil; las opciones
  nuevas del launcher web se ven sin errores.
- **Launcher de Android = el de PC (e7223a862):** decompui compilado para Android (LAUNCHER_ANDROID, liblauncher.so en
  LauncherActivity, que ahora es SDLActivity): mismas pestañas, horizontal, desplazamiento con el dedo; selectores del
  sistema por JNI (ROM con SHA-1, partidas, carpeta hd del PC), juego en su proceso. Probado en el emulador: páginas,
  JUGAR y volver, ROM US instalada por el selector. **Web:** opciones del PC que faltaban (a4e9a982f).
- **Android en el emulador (24/10 tarde):** instalación limpia → el título muestra solo NEW GAME (sin Riku) ✓;
  Jafar EU ✓ (test_android_scenarios); la elección de modo responde al táctil (el emulador es lento: tarda). Arreglado:
  el título decía "TOUCH Options" en Android 64/Switch/Vita (parche de PS5 heredado). "Logros pegado": espera log.
- **debug_kit report:** 1 sitio, viejo (event_message.c:5073, ya arreglado en 188dfc4c3; no se reproduce).
- **Elección de modo con vista previa (5c214462d):** detrás de la ventana se ve cómo quedará la pantalla (amplia /
  240x160 con borde). En la app (Pre); si no gusta, se quita el bloque `preview` en port_ui.c.

## Estado anterior (24/10 ~05:00, Claude)

- main al día (aac6e9af0 + esta limpieza). Issues abiertos en MatyMeatBoy/ReKHCoM: **0** (`gh` apunta por defecto al
  upstream Pheenoh/khcom: usar `-R MatyMeatBoy/ReKHCoM`).
- **Builds internas para el usuario (no publicadas):** v2.0-pre6 (main ac97b2c33) y **v2.0-pre7** (main aac6e9af0):
  zips en la raíz de KHCoMPC, copias `jugar/ReKHCoM-v2.0-pre6|7` + `.bat` (partidas de la pre anterior, HD enlazado a
  D:/01Claude/ReKHCoM-build/hd-completo). Humo oculto `sh build/smoke_pc.sh <copia>`: 9/9 las dos. Receta: ninja
  `build/{pc,pc-us,pc-jp}` (PATH `../toolchain/bin`), `package_release.py --version vX --sdl ../toolchain/SDL3-3.4.18`.
- **Hecho hoy (24/10 madrugada), importador del mod HD (launcher):** varios videos a la vez según el PC (hilos,
  codificador H.264 de la GPU, memoria libre; `KHCOM_IMPORT_JOBS` lo fuerza): **226 s → 120 s (~1,9x)** en la prueba
  de videos base + música + HUD a calidad Buena; el % avanza por peso (tiempo) y dentro de cada mosaico; una carga
  cortada (cierre, crash, corte de luz) se **retoma** (diario `hd/import.journal`, archivos `.part` hasta completarse,
  chequeo rápido de MP4) o se **descarta** (borra solo lo que creó esa carga); cerrar a mitad pregunta (seguir /
  cerrar y retomar / cerrar y borrar); el 1.5+2.5 se busca en Steam (registro, bibliotecas), Epic (manifiestos,
  LauncherInstalled.dat), Heroic/Legendary, programas instalados de Windows y carpetas habituales de cada disco.
  Probado: corte a mitad + `--resume` (ROP conservado, RRE/RSE completos, ffprobe ok), test_launcher 11/11.
- **v2.0-pre8 interna (PC, pedida por el usuario 24/10 ~07:20):** main 2ebb71501 (pre7 + Coliseo 121/122 con
  honores + app 0.4.2 + botones táctiles nuevos, que en PC no se ven), launcher SIN la vía GPU (está en
  `claude/hd-gpu`, sin probar). Zip `KHCoMPC/ReKHCoM-v2.0-pre8-windows-x86.zip` (sha 7bacc516...), copia
  `jugar/ReKHCoM-v2.0-pre8` + `Probar ReKHCoM v2.0 pre8.bat` (partidas de la pre7, HD enlazado). Humo oculto 9/9
  sin fallos y test_launcher 11/11. No publicada.
- **TRASPASO (en curso al escribir, ~05:30):** (a) medición de la carga HD viejo (pre6) vs nuevo en los 3 modos:
  `sh build/hdbench.sh old=<exe> new=<exe> [gpu=<exe>]` sobre `D:/01Claude/ReKHCoM-build/hdbench/src` (33 videos
  enlazados con enlaces duros = 2,63 GB, el 39 % del mod; solo videos) → `build/hdbench.txt`; tabla para el usuario
  con lo medido y extrapolado al mod entero. (b) **Vía GPU del importador** (`TranscodeBest`/`Gpu` en hd_import.inc:
  D3D11 + DXGI device manager para decodificar y codificar en la tarjeta, como HandBrake/OBS/ejemplos de MF; vuelve
  sola a la CPU si falla, `KHCOM_ENC_GPU=0` la apaga): compilada en build/pc, **sin commitear ni medir**; medirla
  con hdbench (`gpu=`), revisar el MP4 (ffprobe, una mirada a un cuadro) y subir los workers si la CPU queda libre.
  Ideas siguientes: audio AAC copiado sin recodificar; leer el video del juego sin la copia previa (IStream que
  des-XORea la cabecera) para no escribir 6,7 GB de más.
- **TRASPASO ~05:45 (Claude, pocos tokens) — SEGUIR AQUÍ, en orden:**
  1. **App de revisión 0.4.1 (hecho, en main):** el aviso "hay nueva versión" ya no lleva a otra pieza (era el aviso
     de la pieza anterior: ahora se borra al cambiar y se comprueba `forId`); la pestaña no salta a Preliminares al
     moverse entre revisiones de una aceptada; los aprobados pasan al instante a "Ya aceptados" (vuelven a
     Preliminares solo con una propuesta más nueva); sin selector de densidad (queda la compacta). Verificar en la
     app abierta con el usuario. Pendiente pedido: que lo aprobado quede "en la cola listo para cargar al enviar
     comentarios" (hoy va en `aprobaciones_build.json` de cada envío: confirmar que el agente lo integra desde ahí).
  2. **Aprobaciones del usuario 24/10 05:03-05:05 (llegaron):** Darkside 01 CON HONORES (b7b683, integrar al
     juego), Darkside 02 aprobado, Darkside 07 r3 aprobado (24a9a9). Coliseo121 con honores (9917b6) sigue sin integrar.
  3. **Comentario del usuario (envío 20261024-050724-3c4f44):** Darkside 05 r3 (0e9183), nube 1 zona [742,138,858,422):
     "aún dibuja un brazo a la derecha; debe ser solo su codo; entender el dibujo y la perspectiva". Hacer r4 hija
     con `import_image.py --parent 0e918387...` (darkside_revise.py; arte = carril de Codex si está activo).
  4. **Carga HD:** terminar la medición (`build/hdbench.txt`; viejo Original 2,63 GB = 4 s) y medir la vía GPU
     (`gpu=build/pc/ReKHCoM.exe` en hdbench.sh, en la rama `claude/hd-gpu` a73f81340, sin probar; build/pc/ReKHCoM.exe ya la trae); tabla para el
     usuario: modo × antes/ahora/GPU, extrapolada al mod (subset = 39 %). Si la GPU rinde y el MP4 está bien
     (ffprobe + un cuadro), commitear, subir workers, rehacer zip (pre8) y humo 9/9.
  6. **Regla del usuario (24/10): siempre que haya cola en la app (`COLA_PARA_AGENTES.md`, turno "agente"), procesarla**
     respondiendo cada envío con una versión hija. Cola al ~06:00: Darkside 03 r3 ("no cambió nada entre versiones":
     solo subió 6 px; el brazo añadido es una mancha, hacer pixel art de verdad con el flujo de WORKFLOW_ARTE_AMPLIO),
     Darkside 04 ("no espejar: completar el lado derecho imaginándolo, a pixel art"), Darkside 05 r3 (solo el codo a
     la derecha, perspectiva), Darkside 06 r3 ("mejorando pero no tan bien"). Las reglas automáticas de
     `gen_darkside_pose.py`/`darkside_revise.py` no alcanzan: dibujar a mano por píxel (paleta e índices del juego).
  6b. **Hecho ~06:20: r4 de Darkside 03/04/05/06 en la app** (`darkside_revise.py --round r4`, ids 3628d7, db369c,
     60f8f0, 429c00), respondiendo los envíos. **Regla del usuario (24/10):** se GENERA lo que falta y el tramado a
     sombra se aplica SOLO a lo añadido; el pixel art del cartucho queda 100 % igual (la técnica aprobada en 07 r3).
     Aplicarlo solo a 03, 04, 05 y un poco a 06 (06: el brazo debe verse bien dibujado). **01 y 02 quedan completos
     como están** (usuario: "queda bien"). 03: no un tercer brazo adelante: un poco de torso que se difumina; 05: solo
     el codo. Falta (pedido): **mejorar el dibujo de lo generado debajo del tramado** con los comentarios previos (en
     03/04 se ve como franja vertical: lo generado es un bloque); hacerlo a mano por píxel (paleta 4bpp del juego) y
     entregar r5 hijas.
  6c. **Hecho ~06:40: r5 de Darkside 04/05/06** (`--round r5`; 32d663, cb75a0, f28d6b): 04 difuminado más corto y
     también abajo; 05 el codo + un pedacito de ala cortado en el borde que se difumina (apenas se ve: agrandar si la
     nube lo pide); 06 brazo según el boceto del usuario (más pegado al cuerpo, 2 px arriba; boceto: el brazo baja
     pegado al costado desde el hombro, con el antebrazo junto al corte), entero, solo la punta tramada.
     **Carga HD medida (33 videos, 2,63 GB = 39 %):** Original 4 s → 6 s; Buena 413 → 237 s (1,74x); Ahorro 452 →
     247 s (1,83x); mod completo Buena ~17,6 → ~10 min, Ahorro ~19,3 → ~10,5 min. Vía GPU: medición corriendo.
  6d. **HECHO ~07:15: Coliseo 121 y 122 con honores integrados y en main** (verificado EU/US/JP: 0 píxeles del original cambiados, márgenes nuevos; `build/scene121_check.sh <escena> <evento>`). Detalle:
     Coliseo 121 (1ee40e4e6) y **Coliseo 122 (c62c7d8ee, honores 08:27 UTC)** encendidos para todos, aislados de la
     Preliminar V1: bloques `PortPreliminarPixelSource` 121/122 en port_preliminar_v1_tiles.h, filas 121/122 en
     _data.inc (v[21]=1), port_ppu.c (`sPreliminarApprovedOnly`, `sScene121On/122On`, `KHCOM_SCENE121_WIDE=0` /
     `KHCOM_SCENE122_WIDE=0` los apagan; DrawTextBg incluye 121/122). Verificar: `sh build/scene121_check.sh 121 0:120`
     y `sh build/scene121_check.sh 122 0:121` (serie; GBA idéntico, márgenes distintos EU/US/JP); si sale bien,
     commitear y pushear; si un caso se corta por tiempo, es bug (reproducir solo). Agrabah 107/108 recibió ★ de nuevo
     08:27 (ya integrado). Darkside 01: receta = main; espera "Darkside · arena y contexto".
     **Carga HD vía GPU medida:** Buena 237 → 203 s con GPU (total vs pre6: 413 → 203 s = 2,03x). Está en la rama
     `claude/hd-gpu` (a73f81340): revisar el MP4 (ffprobe + un cuadro) y mergear a main; luego subir workers si la CPU
     queda libre; siguientes ideas: audio AAC sin recodificar, leer el video sin la copia previa (IStream des-XOR).
     El usuario pidió seguir mejorando el "descompresor"/importador.
  7. App 0.4.2: cada pieza en UNA pestaña (con una propuesta nueva pendiente queda solo en Preliminares, conservando
     la aprobación anterior a la vista); las nubes se ven aunque el envío no se haya procesado (pedido del usuario).
  8. Coliseo 121 integrado aislado en el árbol (sin commitear al escribir): receta 121 de 1ee40e4e6 en
     port_preliminar_v1_tiles.h/_data.inc + port_ppu.c (`sPreliminarApprovedOnly`, `KHCOM_SCENE121_WIDE=0` lo
     apaga). Comprobar con `sh build/scene121_check.sh` (evento 0:120, teclas A del selector, cuadro 1000): GBA
     idéntico y márgenes distintos en EU/US/JP antes de commitear. Darkside 01: la receta aprobada ya es la de main
     (SHA bf8bd681); llega a jugadores cuando se apruebe "Darkside · arena y contexto".
  5. Pedidos del usuario anotados en la cola de abajo: editor de posición de botones; ver botones en emulador/web.
- **Baterías:** todos los cuelgues conocidos cerrados (ver historial). Riku JP corre (2 copias duplicadas: el usuario
  no deja cortarlas), 0 fallos hasta ~100. Faltan (`run_batteries.py --status`): rooms eu, battles eu, monkey
  eu/us/jp, skip eu/us/jp y los pasos android; luego `run_batteries.py --hd`. Toda falla o timeout = bug: reproducir.

## Cola hacia la v2.0 (en orden)

1. **Botones táctiles: editor de posición** de botones y HUD (arrastrar, fluido, guardado en kh_com.ini).
   Hecho 24/10 (11be88cb9 y siguiente): los 3 temas aprobados dibujados en `touch_android.c` (Android, web, Switch),
   fila "Tema de botones" en Opciones, letras con la fuente del texto HD (`gen_touch_letters.py` →
   `touch_letters.inc`, sin pixelado), A/B/rodar/acumular en rombo como un mando (pedido del usuario); capturas
   `pruebas/Botones tactiles - implementados en el juego (Clear Dark Color).png` (build `build/pc-touch` =
   build/pc con `-DPORT_TOUCH` y touch_android.c; `KHCOM_SHOT_TOUCH=1` los pone en las capturas del arnés).
   test_android_touch y test_web_controls (parte táctil) ok. Falta verlos en el emulador Android y en la web.
   `input_controls_conformance` vuelve a compilar y pasa (83d174409).
2. **Integrar lo aprobado con honores en la app**: hecho 121, 122, 055 (lista única en port_ppu.c). Darkside 01
   espera "Darkside · arena y contexto" (en Rev, turno del agente).
3. **Elección de modo al primer arranque**: ejemplos visuales en cada opción (port_ui.c WIZ_MODE, a la app de
   revisión) y probarla en Android (sin mod HD ya va directo a GBA mejorado / clásico).
4. ~~Launchers de Android y web~~ hecho (Android nativo = PC; web con las opciones que faltaban). **Launchers de Android y web iguales al de PC** (diseño y funciones; plan en el historial, "TRASPASO 24/10 tarde"):
   Android = el launcher ImGui de PC compilado para Android (SHA-1 portable, diálogos SDL, Intent al juego, URLs
   SDL_OpenURL, táctil); web = completar lo que falte frente al de PC.
5. **Android antes de la v2.0**: título con "Nueva partida Riku" sin truco ni partida terminada (comprobar en el
   emulador con instalación limpia); "Logros pegado" (pedir `kh_com.log` si vuelve); rendimiento en gama baja (línea
   `timing:`, toques tomados una sola vez con el juego lento); tablets: artes/fondos con el encuadre nuevo
   (LOGICA_APROBADOS.md); Jafar en el emulador (`test_android_scenarios.py --groups bosses`).
6. ~~`test_text_options.py`~~ hecho (9e8316f06): por nombre de fila, corrida base + ruta; 7/7 EU/US/JP.
7. ~~Mod HD: calidad constante por hardware~~ (cerrado 24/10: se queda Buena) (`hd_quality_probe.py`, probar Q 75/80/85 y elegir la que pese menos que
   Buena con su VMAF; la VMAF de ROP está desalineada); peso real de Ahorro en `kScenesSize` (hd_import.inc).
   Vía GPU (decodificar y codificar en la tarjeta) en main (0a42bcedf): Buena 237 → 203 s en la muestra; MP4
   revisado (H.264 1080p, duración completa, decodifica limpio, cuadro correcto); test_launcher 11/11.
8. ~~Repo autosuficiente~~ hecho (9e8316f06): ninguna herramienta con rutas de este PC (build_switch usa
   `REKHCOM_WORK`); `git count-objects` sin paquetes sueltos.
9. Baterías pendientes (arriba) y `debug_kit.py report` sin sitios nuevos.
10. **Lista de verificación antes de publicar** (abajo) y la **v2.0 estable solo con el OK del usuario**.

## Espera al usuario (no ir a jugadores antes)

- Revisión en la app: Darkside r3 (03, 05, 06, 07); Darkside 04 necesita dibujo nuevo sin espejar (Codex o a mano).
- Preliminar V2 de jefes (`KHCOM_ARENAS_V2=1`, `pruebas/Probar Preliminar V2 jefes.bat`): Trickmaster, Oogie,
  Maléfica, Úrsula, Marluxia final (comentarios).
- Diseños que piden decisión: videos HD en Linux nativo (proceso gst-launch/ffmpeg aparte vs nada), HUD HD a lo
  Re:CoM sin equivalente 1:1 (contador CARDS, entradas del título).

## Codex (sus carriles; Claude no toca esos archivos)

- Artes restantes y terminar las Preliminares (V1 escenas, V2 jefes) con el método de WORKFLOW_ARTE_AMPLIO.md,
  LOGICA_APROBADOS.md y la app de revisión (`khcom-codex/build/art-review/COLA_PARA_AGENTES.md`).
- 39: rutas completas de Sora y de Reverse/Rebirth.

## Esperando reportes o datos (no se trabaja hasta que lleguen)

- #42 "pistas HD intercambiadas" (solo un video; pedir nombres si reabre). Proton estable (pedir kh_com.log).
- Cara HD de Sora con el pelo cortado al recibir daño; cuadro negro en el combate de Agrabah con HUD HD: falta
  captura/savestate del momento.
- Vita: undub por cabecera y velocidad; Switch #35: rendimiento (motor nuevo, después de la v2.0).

## Después de la v2.0

- Switch fuera de la v2.0 (queda en la última pre): motor nuevo (SWITCH_MOTOR.md: fase 0 medir, fase 1 PPU en 3
  núcleos del invitado). Vita/PS4 a la GPU (RENDIMIENTO.md). PPU de GBA en varios núcleos para 60 fps a x1.
- Online (~70 %, tabla en el historial y ONLINE_ESTUDIO.md): prueba real entre dos personas, relay en un VPS,
  rollback y lobby en Android, sockets en Switch.
- Menús de Re:CoM en HD uno por uno (HUD_HD_RECOM.md), fondos de menús, números de cartas HD vs Re:CoM, más assets
  (MelonMIX y similares), datos de Re:CoM para el modo Re:CoM, rendimiento de consolas medible y sin riesgo.
- Guard Armor (008) con `bosskill` del arnés no cierra (BgFxIsActive): ver si es del arnés o del juego.

## Lista de verificación antes de publicar la v2.0 (no saltear)

- **Sin Switch en la v2.0** (usuario 24/10): sacar el `.nro` de la publicación.
- `port/tools/test_update.py` con el paquete nuevo contra `respaldos/releases-estables` y `--olds
  respaldos/releases-prueba` (v1.9.15-pre1 verificada en Quiver, v2.0-pre2): todo ok.
- **No se borra NINGÚN release ni archivo**; publicar = solo agregar. README con la versión nueva. Copiar los
  archivos de la estable a `KHCoMPC/respaldos/releases-estables` (mismo SHA).
- `release_check.py v2.0` antes y `--post` después (launcher, rc, CHANGELOG, web, paquetes, Quiver);
  `check_sources.py` (shim de consolas).
- Builds desde el commit final: Windows, Linux, Android, PS4, Vita, PS5 (VM), web; ningún paquete viejo.
- Combate de prueba con cada ejecutable de PC (`KHCOM_BATTLE=10:4`, `build/smoke_pc.sh`), sin `bios:`/CRASH.
- `debug_kit.py report` sin sitios nuevos; Android 64 (imagen movida + jefes EU/US/JP, Riku).
- `test_hd_audio.py`, `test_hd_audio_flow.py` (`uv run --with numpy --with soundfile`), `test_frameskip.py`,
  `test_hd_scenes.py`, `match_hd_scenes.py --check`, videos HD (`KHCOM_HDMOVIE_PAUSETEST`, renderer por software,
  Android), `test_launcher.py` (sin `SDL_VIDEO_DRIVER=offscreen`: el launcher necesita GL; ventana oculta igual).
- Copia de prueba para el usuario + `.bat`; notas cortas que anuncien el mod HD; nunca decir en público que algo no
  se probó en hardware.
