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

## Estado (24/10 ~05:00, Claude)

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
   `input_controls_conformance` (test_web_controls) no compila desde antes: port_input.c ahora incluye cabeceras
   del juego (mode.h, text_types.h); darle sus includes o separar lo que prueba.
2. **Integrar lo aprobado con honores en la app**: Coliseo121 (`1ee40e4e6`), integración aislada; consultar
   `build_reference` / `aprobaciones_build.json`. Darkside 01 y 03v2 aprobados normal (no entran aún).
3. **Elección de modo al primer arranque**: ejemplos visuales en cada opción (port_ui.c WIZ_MODE, a la app de
   revisión) y probarla en Android (sin mod HD ya va directo a GBA mejorado / clásico).
4. **Launchers de Android y web iguales al de PC** (diseño y funciones; plan en el historial, "TRASPASO 24/10 tarde"):
   Android = el launcher ImGui de PC compilado para Android (SHA-1 portable, diálogos SDL, Intent al juego, URLs
   SDL_OpenURL, táctil); web = completar lo que falte frente al de PC.
5. **Android antes de la v2.0**: título con "Nueva partida Riku" sin truco ni partida terminada (comprobar en el
   emulador con instalación limpia); "Logros pegado" (pedir `kh_com.log` si vuelve); rendimiento en gama baja (línea
   `timing:`, toques tomados una sola vez con el juego lento); tablets: artes/fondos con el encuadre nuevo
   (LOGICA_APROBADOS.md); Jafar en el emulador (`test_android_scenarios.py --groups bosses`).
6. `test_text_options.py`: rehacer sus expectativas viejas (filas y claves de config).
7. Mod HD: calidad constante por hardware (`hd_quality_probe.py`, probar Q 75/80/85 y elegir la que pese menos que
   Buena con su VMAF; la VMAF de ROP está desalineada); peso real de Ahorro en `kScenesSize` (hd_import.inc).
   Opcional: decodificar por GPU en el importador (hoy decodifica la CPU, ~5 hilos por video).
8. Repo autosuficiente: revisar que las herramientas de EMPEZAR_AQUI.md corran sin rutas de este PC (23 ya usan
   `REKHCOM_WORK`); `git count-objects -vH`.
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
