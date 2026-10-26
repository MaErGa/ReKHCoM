# Pendientes vigentes hacia la v2.0 (lista limpia, 26/10/2026)

Solo lo abierto. Lo terminado y los traspasos anteriores: **archivo/PENDIENTES_ARCHIVO_20261026.md** (y los
archivos anteriores). Método de depuración: DEBUG_WORKFLOW.md. Cola de arte: `python port/tools/art_queue.py --quiet
list` (Atlas). Reglas del usuario: REGLAS_DEL_USUARIO.md. Última prerelease: **v2.0-pre19** (26/10, Windows, Linux,
Steam Deck; zip limpio en KHCoMPC).

## TRASPASO — Claude, 26/10 tarde: cierre de la 2.0 (Codex dormido, Claude con todo)

- **Integrado (#232, cierra #231 de Codex):** título HD completo (make_title_words.py, grupo 7 embebido), barra Pulsa
  Start lisa, fundidos del título como el juego (PortPaletteFade reconoce los fundidos restando/sumando), Opciones HD
  (ilustración HD bajo el panel; textos con avances KHMenu), avisos del port en HD; proporción Re:CoM con `pad=`
  (fix_hd_aspect.py: avisos de combate, ¡Nivel!, carteles); números de cartas grandes como Re:CoM y HD para
  mejoradas/premium (card_digit_recipes.py, op `duo`); números de tienda Moguri/mapa como texto HD.
- **Ojo al probar:** un hd/sprites importado antes de `pad=` se ignora (log «import the HD mod again»): reimportar con
  `ReKHCoM.exe --hd-import --sprites-only`. Carpeta de prueba D:/01Claude/ReKHCoM-build/v20test (estado
  st/tt50.khst = Sora nv50 en Ciudad de Paso; KHCOM_BATTLE=10:4:0 KHCOM_BATTLE_DECK=2 KHCOM_BOT=1 para combate).
- **Sigue (en orden):** números HD en el resto de menús; galería desde saves viejos; Steam (logo sin «Re:», banner);
  #202 CRT; PR #210 (Atlas 090, sin valorar: decide el usuario); builds y pruebas de la 2.0; preguntar al usuario
  antes de publicar (HUD HD sigue apagado para jugadores: el título/Opciones HD solo se ven con él).

## Reglas que no se olvidan

- Rama y PR por entrega; lo visual pasa por Atlas y solo entra lo aprobado (LOGICA_APROBADOS.md).
- Publicar (releases, comentarios) con OK del usuario; la estable solo con su OK.
- Nunca borrar releases ni carpetas del usuario; trabajar en KHCoMPC (C:) y D:/01Claude/ReKHCoM-build.
- Un timeout o fallo de batería es un bug: reproducir (antes, mirar kh_com_host.log: ¿le falta la imagen?).
- Responder al usuario en español y breve.

## 1. Código (Claude)

- [ ] **Baterías** (khcom-claude-int, `run_batteries.py --status`): relanzadas el 26/10 con marcas frescas; su parte
  Android trae Agrabah 123/124 (app smoke completa). Revisar resultados y arreglar lo que salga.
- [ ] **Linux 64: fila de pantalla completa** (rama `wip/linux64-fullscreen-row`, ver TRASPASO) y PR.
- [ ] **Antes de la 2.0** (sección 3): correr la lista de pruebas sobre el commit final.

## 2. Arte (Atlas — Codex)

- Codex manda por PR lo aprobado en Atlas; Claude lo integra al llegar. Los PR abiertos #206, #210–#216, #222 y
  #226 son propuestas sin aprobación en la cola: no se integran hasta que lo estén.
- Cola: arena de Agrabah (frente), tienda moguri, cambiar naipes, añadir naipes (esquina), botón Opciones del menú,
  banner de Steam, y lo que Codex tiene en curso (057, 061, 097, 095 castillo, diario, Maléfica, remates, título HD).
- Al entregar el botón Opciones: el código lo agrega al menú y quita Start+Select para Opciones (solo menú y título).
- Título HD hasta cargar la partida y Opciones en HD con el HUD HD: diseño en Atlas, luego código.

## 3. v2.0 estable (solo con OK del usuario)

- `release_check.py v2.0` antes y `--post` después; `check_sources.py`.
- Builds desde el commit final: Windows, Linux, Steam Deck, Android, PS4, Vita, PS5 (VM), web (sin Switch).
- Pruebas: `test_update.py` (baja las estables de GitHub), `test_hd_audio.py`, `test_hd_audio_flow.py`,
  `test_frameskip.py`, `test_hd_scenes.py`, `match_hd_scenes.py --check`, `test_launcher.py`, smoke de cada exe.
- Al publicar la 2.0: avisar en #140 a quien nos ayudó con Steam Deck que llega el modo 64 bits y que debería ir
  mejor; si ve una regresión o un bug, que nos avise y lo arreglamos (y, como siempre, que Windows va bien por Proton).
- Al publicar la 2.0: preguntar en #44 (tiene Vita) si corre a velocidad completa y cómo va; dar las gracias.
- Notas cortas que anuncien el mod HD; `announce_release.py v2.0` para X, Reddit y Patreon (publicar con OK).

## 4. Esperando a otros (no se trabaja hasta que llegue)

- #163 duartepedro: revisión de la galería de escenas y del test3 (videos HD desde el segundo justo).
- #202 itsNateDev: prueba del modo CRT (240p, superresolución, 480p) enviada por test-builds.
- Material del usuario para los modos PS2 con REA (ISO/ELF de PS2 y carpeta 1.5 HD).
- Reportes: cara HD de Sora con el pelo cortado al recibir daño; cuadro negro en Agrabah con HUD HD (falta captura);
  Vita: undub por cabecera y velocidad.

## 5. Después de la v2.0

- Modos de Re:CoM basados en PS2 con REA: mejorar o arreglar los modos que ya hay (no modos nuevos).
- Consolas a más potencia solo con ajustes documentados.
- iOS (IPA por fuera de la App Store; hace falta una Mac para compilar).
- Switch (SWITCH_MOTOR.md); Vita/PS4 a la GPU; online (~70 %); rutas de Sora y Reverse/Rebirth (#39).
