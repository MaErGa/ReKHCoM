# Pendientes vigentes hacia la v2.0 (lista limpia, 25/10/2026 noche)

Única cola de trabajo. Lo anterior completo (traspasos, borradores, detalle): **PENDIENTES_ARCHIVO_20261025.md**
y PENDIENTES_ARCHIVO_20261024.md. Método de depuración: DEBUG_WORKFLOW.md. Protocolo de la cola:
COLA_COLABORATIVA.md. Estado vivo de piezas: `python port/tools/art_queue.py --quiet list`.

## Reglas del usuario (completas en REGLAS_DEL_USUARIO.md)

- Rama y PR por entrega; lo visual pasa por la app de revisión (Atlas) y solo se integra lo aprobado.
  Lógica aprobada: LOGICA_APROBADOS.md (cada PR de arte la mejora).
- La validación en hardware real no es pendiente: llega como reporte. Publicar (releases, comentarios) solo con OK.
- Nunca borrar carpetas ReKHCoM-vX ni ningún release. Trabajar solo en KHCoMPC (C:) y D:/01Claude/ReKHCoM-build.
- Toda falla o timeout de batería es un bug: reproducir. Notar cada avance aquí al momento.

## Estado (25/10 noche)

- Traspaso activo de Codex (arte aprobado, ZIP local): TRASPASO_CARGAS_ZIP_20261025.md.
- Sin PR abiertos. Última copia interna: **v2.0-pre15** (zip limpio en KHCoMPC + `jugar/ReKHCoM-v2.0-pre15`).
- Hoy en main: Opciones por secciones (solo A para entrar, Salir del juego, mods en gris con cartel), logros Re:CoM
  con borde e insignias, #83 (carta de Jafar: BIOS_PTR), mazo sin parpadeo en invocaciones, videos HD en Linux,
  #42 (3 pistas HD de eventos), arenas aprobadas (Maléfica, Úrsula, Marluxia, Trickmaster, Oogie, Darkside, 112, 120).

## 1. Código (Claude)

- **#83 sin reproducir en PC:** 24 peleas largas con Jafar (build anterior a #115, con y sin HD, recogidas forzadas):
  0 fallos y nunca una recogida con la selección vacía. El código lo explica: todo lo que vacía `sSoraSelectedCard`
  marca una recarga, y la recogida espera a que no haya recarga. #115 queda como protección, no como causa probada.
  Siguiente: `test_android_scenarios.py --groups jafar83` en el emulador (build nueva y anterior) o el `kh_com.log`
  del jugador. El issue sigue abierto.
- **Android (con el emulador de las baterías):** repetir la batalla 123 US (timeout; en PC no se reproduce),
  rendimiento en gama baja (línea `timing:`), tablets con el encuadre nuevo, botones táctiles (también web),
  elección de modo al primer arranque.
- **Baterías:** `run_batteries.py --status` en `khcom-claude-int` (relanzadas 25/10 noche): combates EU y luego
  Android; después la pasada `--hd`.

## 2. Arte (Atlas: GPT/Codex; Claude ayuda)

- Responder en la app cada envío en turno «agente» con una versión hija (`COLA_PARA_AGENTES.md`).
- Piezas de la cola: 057, 061, 074, 097, 081, diario (entradas y subentradas), Maléfica 01-03.
- Jafar: la columna negra de la subida de nivel y el recuadro de la escena previa son del cartucho (también con
  `KHCOM_JAFAR_WIDE=0`): rellenarlos es arte dentro de la imagen GBA, solo si el usuario lo pide.
- HUD HD a lo Re:CoM: lo que no tiene equivalente 1:1 (contador CARDS, título) es decisión de diseño.

## 3. Esperando reportes (no se trabaja hasta que lleguen)

- #83 confirmación del jugador; Proton estable (`kh_com.log`); cara HD de Sora con el pelo cortado al recibir daño;
  cuadro negro en Agrabah con HUD HD (falta captura); Vita: undub por cabecera y velocidad.
- Si un issue cerrado recibe comentarios: responder y volver a cerrarlo. Sin plataforma/versión: preguntar.

## 4. v2.0 estable (solo con OK del usuario; no saltear nada)

- Sin Switch (sacar el `.nro`). `test_update.py` contra `respaldos/releases-estables` y `--olds respaldos/releases-prueba`.
- No se borra ningún release ni archivo; README con la versión; copia de la estable en `respaldos/releases-estables`.
- `release_check.py v2.0` antes y `--post` después; `check_sources.py`.
- Builds desde el commit final: Windows, Linux, Android, PS4, Vita, PS5 (VM), web.
- `build/smoke_pc.sh` con cada ejecutable de PC; `test_hd_audio.py`, `test_hd_audio_flow.py`, `test_frameskip.py`,
  `test_hd_scenes.py`, `match_hd_scenes.py --check`, `test_launcher.py`.
- Copia de prueba + `.bat`; notas cortas que anuncien el mod HD; nunca decir en público que algo no se probó en
  hardware. Al publicar, responder #83 si quedó confirmado.

## 5. Después de la v2.0 (mejoras, no pendientes de ahora)

- Switch (SWITCH_MOTOR.md); Vita/PS4 a la GPU (RENDIMIENTO.md); PPU de GBA en varios núcleos.
- Online (~70 %, ONLINE_ESTUDIO.md); menús Re:CoM en HD (HUD_HD_RECOM.md); rutas de Sora y Reverse/Rebirth (#39).
- Mejoras de lo «aprobado con mejoras».
