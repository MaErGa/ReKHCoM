# Pendientes vigentes hacia la v2.0 (lista limpia, 26/10/2026)

Solo lo abierto. Lo terminado y los traspasos anteriores: **archivo/PENDIENTES_ARCHIVO_20261026.md** (y los
archivos anteriores). Método de depuración: DEBUG_WORKFLOW.md. Cola de arte: `python port/tools/art_queue.py --quiet
list` (Atlas). Reglas del usuario: REGLAS_DEL_USUARIO.md. Última prerelease: **v2.0-pre19** (26/10, Windows, Linux,
Steam Deck; zip limpio en KHCoMPC).

## Retoma — Codex, 26/10: cierre técnico de la 2.0

El usuario pidió continuar hasta acabar, tomar las decisiones y usar workers; autorizó relevar las reservas
anteriores. Base de revisión: `origin/main` **bf09cf001**. Rama coordinadora
`codex/claude-close-20261026`; tarea remota `claude-close-20261026`.

**Autorización de publicación recibida el 26/10:** «al acabar todos los pendientes haz una depuracion general y
tras todos los ok tuyos lanza la 2.0 stable». Completar los pedidos recuperados de la sesión de Claude en
`RETOMA_CLAUDE_20261026.md`, depurar y verificar el commit final; después publicar sin volver a pedir permiso.

- **Linux64 pantalla completa ya integrado:** PR #227, merge `1fbd9eef4c890be5ce433015249ad94de1a4b9a7`.
  La rama `wip/linux64-fullscreen-row` coincide con main en los archivos afectados. La fila, el mensaje
  `fullscreen=0/1` al host y `SDL_SetWindowFullscreen` están en main; no reimplementar ni abrir otro PR.
  La prueba en VM está documentada en #227; esta retoma aún no repitió esa ejecución.
- **Batería viva:** el runner de `khcom-claude-int` continúa los eventos anchos JP; se conserva sin duplicar.
  El timeout Android US batalla 123 guardado corresponde a `guest-us.macho is missing from the APK`,
  con log del juego vacío. Diagnóstico del arnés corregido en #239 (`065f10ba4`, seis regresiones); repetir con
  APK completo antes de cerrar Android.
- **Empaquetado Linux64:** #240 (`0419243fb`) construye y exige host64, SDL y EU/US/JP incluso en árbol limpio.
- **Controles y launcher:** #241 (`3ab48f7a2`) restablece también input/captura/Nintendo automático; #242
  (`df028860e`) agrega bandera de Undub y usa la portada Steam también como fondo superior.
- **Builds finales:** `build_v20.log` quedó en 482/1313 y no había compilador vivo. El árbol
  `D:/01Claude/ReKHCoM-build/v20-wt` conserva WIP de la ilustración del aviso de logros al activar estados
  (`port_ui.c`, `port_achievement_menu.inc`); no incluirlo sin verificar ni borrarlo. Se retoma en árbol limpio.
- **Atlas:** consultar la API viva, no la foto histórica. Hay entregas integradas que la lista antigua vuelve
  a enumerar. La evidencia del título con textos HD se verifica contra #232/#238 antes de responder las nubes.

## Traspaso anterior — Claude, 26/10 tarde

- **Integrado (#232, cierra #231 de Codex):** título HD completo (make_title_words.py, grupo 7 embebido), barra Pulsa
  Start lisa, fundidos del título como el juego (PortPaletteFade reconoce los fundidos restando/sumando), Opciones HD
  (ilustración HD bajo el panel; textos con avances KHMenu), avisos del port en HD; proporción Re:CoM con `pad=`
  (fix_hd_aspect.py: avisos de combate, ¡Nivel!, carteles); números de cartas grandes como Re:CoM y HD para
  mejoradas/premium (card_digit_recipes.py, op `duo`); números de tienda Moguri/mapa como texto HD.
- **Ojo al probar:** un hd/sprites importado antes de `pad=` se ignora (log «import the HD mod again»): reimportar con
  `ReKHCoM.exe --hd-import --sprites-only`. Carpeta de prueba D:/01Claude/ReKHCoM-build/v20test (estado
  st/tt50.khst = Sora nv50 en Ciudad de Paso; KHCOM_BATTLE=10:4:0 KHCOM_BATTLE_DECK=2 KHCOM_BOT=1 para combate).
- **Hecho después:** números HD de guardado/link (#233), galería con saves viejos (#234), Steam banner+logo (#235),
  v2.0 versión+CHANGELOG (#236). #202 verificado (320x240, 1920x240, 640x480 exactos).
- **Sigue:** builds v2.0 (build_all --platform all en D:/01Claude/ReKHCoM-build/v20-wt, log ../build_v20.log), PS4/
  Vita/PS5, pruebas (release_check, test_update, hd_audio, escenas, launcher, humo); preguntas al usuario antes de
  publicar (HUD HD a jugadores, PR #210, comentarios #140 #44 #163 #202, anuncios).

## Reglas que no se olvidan

- Rama y PR por entrega; lo visual pasa por Atlas y solo entra lo aprobado (LOGICA_APROBADOS.md).
- Publicar (releases, comentarios) con OK del usuario; la estable solo con su OK.
- Nunca borrar releases ni carpetas del usuario; trabajar en KHCoMPC (C:) y D:/01Claude/ReKHCoM-build.
- Un timeout o fallo de batería es un bug: reproducir (antes, mirar kh_com_host.log: ¿le falta la imagen?).
- Responder al usuario en español y breve.

## 1. Código (Claude)

- [ ] **Baterías** (khcom-claude-int, `run_batteries.py --status`): relanzadas el 26/10 con marcas frescas; su parte
  Android trae Agrabah 123/124 (app smoke completa). Revisar resultados y arreglar lo que salga.
- [x] **Linux 64: fila de pantalla completa:** integrado en #227 (`1fbd9eef4`); rama WIP equivalente a main.
- [ ] **Antes de la 2.0** (sección 3): correr la lista de pruebas sobre el commit final.

## 2. Arte (Atlas — Codex)

- Las entregas van por PR. Comprobar estado real en GitHub y valoración exacta en Atlas antes de integrar.
  Al retomar no había PR abiertos: #206, #210–#216, #222 y #226 ya no deben figurar como abiertos.
- Tienda Moguri: #228/#230 integrados; título y Opciones HD: #232/#238; banner y logo de Steam: #235.
  Los comentarios posteriores siguen en Atlas y no deshacen esas integraciones.
- Cola artística viva: arena de Agrabah (frente), cambiar naipes, añadir naipes (esquina), botón Opciones del menú,
  y comentarios de escenas, diario, Maléfica y remates. «Aprobado con mejoras» conserva su cierre; una propuesta
  nueva no hereda aprobación. No usar esta enumeración como reserva ni inventar valoraciones.
- Al entregar el botón Opciones: el código lo agrega al menú y quita Start+Select para Opciones (solo menú y título).
- Título y Opciones HD ya siguen «Título y menús HD» (`hdtitle`), no el ajuste oculto de HUD HD (#238).

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
