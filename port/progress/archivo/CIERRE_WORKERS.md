# Cierre de workers y bloques de trabajo


## Chat de integración y jefes — cierre 18/10/2026

Estado: bloque cerrado y entregado para consolidación. Este chat no creó subagentes ni workers; no hay agentes propios que esperar o cerrar. No se abren nuevos frentes.

### Integrado en main

- Guard Armor: `aa2227330`; escenario aceptado por el usuario, extensión usando tiles originales y paleta activa. Pruebas: **13/13**, tres capturas por caso, EU/US/JP, Sora y Riku EU, 16:9 y 32:9; rectángulo GBA sin cambios, controles Original/Hook/Monstro idénticos.
- Escenas 057/061/074: seis commits originales importados hasta `2892f3e8c`, conectados al PPU por `d606d69ca`. Pruebas históricas de integración **23/23**; no equivalen a aprobación de todas las cámaras y zooms.
- Repartija y publicación verificadas: `994540ac2`.
- Correcciones amplias del otro chat: **ya presentes** en `0cb04a8dd`, coordinación `c201169a9`. Este cierre no las rehace ni atribuye sus pruebas a Darkside.
- Nightly 2 publicada: https://github.com/MatyMeatBoy/ReKHCoM/releases/tag/nightly-2026.10.18.2. Windows x86, prerelease; estable permanece v1.9.2. Darkside NO está incluido. BAT local: `Probar ReKHCoM v1.9.3 Nightly 2.bat` en la carpeta padre del repositorio.

### Darkside: WIP recuperable, excluido de main y releases

Commit **`6e7400189cfd9e8fdfd1d3a3bf26941143d6e616`**, rama local y remota **`wip/darkside-backdrop-20261018`**. Contiene únicamente:

- `port/src/port_darkside.c`: continuidad de cielo, horizonte y arena con tiles del cartucho y paleta activa; conserva la isla original, sin repetir palmeras o rocas.
- `port/include/port/port_battle_backdrop.h`: declaraciones del perfil.
- `port/src/port_ppu.c`: detección del mapa de Darkside, extensión de BG0 y fin de BG1 transparente para evitar repetir extremidades cada 512 píxeles.

Recuperar en una rama de trabajo con `git cherry-pick 6e7400189cfd9e8fdfd1d3a3bf26941143d6e616`. Resolver cualquier conflicto del PPU revisando los cambios posteriores; no aplicar a main como mejora terminada. La copia de trabajo de main queda sin estos cambios.

Comprobado: build Windows EU correcto y dos ejecuciones de captura, 16:9/32:9. Evidencia privada en `build/codex-evidence/darkside-preview/`, binario en `build/codex-evidence/darkside-eu/`. Decodificación privada del mapa y cuadrícula en `build/codex-evidence/darkside-design/`. Estos archivos no se distribuyen.

**Defectos que impiden integrar:** el cuerpo de Darkside conserva un recorte rectangular en la zona superior/derecha; el efecto inicial de energía parece repetirse en ultrawide y necesita aislarse. Las extremidades repetidas de BG1 se reducen, pero eso no demuestra que todas las fases estén correctas. Revisar mapas animados `gBosDsdFrameMaps`, coordenadas y composición BG1/sprites en `src/bos/bos_dsd_main.c`. La sospecha de culling no está demostrada: `IsRectOutsideScreen` ya usa VIEW_* en PC y `IsSpriteOutsideScreen` está documentada como no llamada.

Pendiente antes de integrar: solucionar recorte/efecto, comparar región GBA antes/después, probar Original y escenarios de control, compilar EU/US/JP y capturar fases de Sora/Riku. No existen pruebas automatizadas de regresión específicas de Darkside. **No contarlo como terminado.**

### Pendientes para la lista única de Claude

1. Darkside: recuperar el WIP y cerrar los defectos y pruebas anteriores.
2. Jefes: Oogie, Jafar, Maléfica dragón, Úrsula, Marluxia final y solo franja inferior de tablero de Trickmaster/Alicia. Contador: **1/8 aceptado e integrado**, Guard Armor; Darkside pendiente. 28/28 fondos inspeccionados inicialmente, sin certificar todas las fases. Mantener Hook e interior de Monstro de combate según aceptación del usuario.
3. RetroAchievements: investigación realizada; integración jugable **pendiente**. RaceWave46 sirve de referencia, pero no se añadió rcheevos ni login ni logros al juego. Las variables nativas no equivalen directamente a memoria GBA; se requiere diseñar el mapeo para los sets. No solicitar contraseña por chat.
4. #12: selector de guardado opcional, Off por defecto bajo Experimental. Nuevos reportes de resumen incorrecto, persistencia de activación y último slot siguen abiertos; las pruebas anteriores de menú/config no resuelven gameplay.
5. #13: remapeo de controles implementado; comprobación física del controlador pendiente.
6. Savestates del borde superior abierto y checkerboard de Hollow Bastion: encargos de la repartija, sin corrección hecha por este chat.
7. Escenas amplias: seguir el cierre del otro chat y `0cb04a8dd`; distinguir integración, pruebas y aprobación visual. No duplicar su trabajo.
8. Publicación siguiente: reunir correcciones realmente terminadas, comprobar regiones, publicar como nightly/prerelease y actualizar BAT. No publicar estable. Nightly 2 no contiene cambios posteriores a `d606d69ca`.
9. Issues: respuestas previas registradas en CODEX_RESPUESTAS.md; no cerrar issues ni afirmar corrección sin evidencia. Evitar repetir respuestas publicadas.

Referencias de detalle: BATTLE_SCENERY_AUDIT.md/.json, BATTLE_SCENERY_TECHNIQUE.md, NIGHTLY2_INTEGRATION.md, REPARTIJA_20261018.md y CODEX_TAREAS.md. Este documento entrega el bloque; no sustituye la lista consolidada del integrador.

## Worker PS5 / revisión PR #15 — 18/10/2026

Estado: PR [#15 Ps5 Port](https://github.com/MatyMeatBoy/ReKHCoM/pull/15) fusionado en `main` por el autor el 18/10/2026 (merge `5a068ce82`). Se tomó su versión probada como base: el PR contiene el mismo código PS5 que ya se había importado localmente desde `9e16f2bf`. Se suman ajustes acotados para compilar contra el `main` actual y adjuntar/descargar el paquete desde releases del proyecto. La integración se publicó en `main` como `13fba3337`. Checks automáticos de GitHub: ninguno reportado.

Commits propios en `feature/ps5`: `aecd9356d` (LF del wrapper), `f29b63486` (eventos SDL requeridos por el menú de controles), `6cedd6200` (empaquetado y versionado en release principal), `34f7e0e7a` (actualizador desde `MatyMeatBoy/ReKHCoM`, asset exacto `PPSA98511.zip`, tags `vX.Y.Z`). Incluye además corrección del origen de zlib y `PORT_LINUX` para evitar que el build x32 de PS5 compile hooks de escritorio incompatibles. El build maestro acepta PS5 en host Linux y exige ROMs EU/US/JP; en Windows informa que debe ejecutarse en la VM Linux preparada.

Pruebas: el build de app en VM Debian x86_64 finalizó con `BUILD 0`, creó `eboot.bin` y `PPSA98511.zip` (22 MB); `unzip -t` pasó. Las tres imágenes guest se compilaron: EU 2,982,692 bytes; US 3,121,668; JP 2,985,860. `build_all.py --platform ps5 --dry-run --package --release-version v1.9.3-nightly.20261018.2` produjo el comando esperado con versión `1.9.3`; `py_compile` pasó. El runner `test/linux_host.c` se ejecutó con las tres regiones y segfaultó en IP 0 al arrancar cada juego; esto no valida gameplay y no se ha probado en consola PS5. Una compilación versionada estaba activa en QEMU, pero empleó un checkout persistente anterior a `34f7e0e7a`, por lo que no confirma los últimos ajustes de updater ni la versión final empaquetada.

Pendiente real: completar una compilación versionada desde el snapshot `main` posterior al PR, probar el flujo de actualización del asset principal y validar gameplay en hardware PS5 o resolver el segfault del runner headless. No se publicó un release en este trabajo. Se preservaron los cambios locales no confirmados de `REPARTO.md`/`AVISO_PR_PS5.md` en el checkout original.

### Confirmación del usuario — cierre PS5, 18/10/2026

El usuario confirma que el worker terminó su bloque y solicita incluir PS5 en
futuras releases. Entrega de implementación cerrada; la lista única marca el
punto 38 terminado. Compilación versionada de cada paquete, publicación del
asset PPSA98511.zip y comprobaciones de updater/hardware corresponden al
ciclo de publicación y conservan sus límites documentados arriba. No se
atribuyen nuevas pruebas físicas a esta confirmación.
