# Contexto recuperado — 14 de octubre de 2026

## Fuente y estado

ReKHCoM es el port nativo Windows x86 de KH: Chain of Memories de GBA,
basado en la decompilación de Pheenoh. Las 269 unidades y 5212 funciones
están implementadas; esto no equivale a verificación completa del juego.
La prerelease local más reciente es v0.7. PROGRESS.md/status.json indican
94,5 % global; faltan partidas completas de Sora y Reverse/Rebirth.

El reparto vigente es archivo/REPARTO.md, reordenado el 14/10, commit 3e5c0b93b.
Tiene prioridad sobre el bloque más antiguo de reparto en PENDIENTES.md.
README/CONTRIBUTING y varios puntos históricos de PENDIENTES contienen
limitaciones ya superadas; contrastarlas con status.json, REPARTO y evidencia.

## Checkouts y trabajo existente

- `khcom`: rama feat/native-movies, HEAD 1c828fd4d al retomar.
- `khcom-claude`: worktree claude/work, mismo HEAD al retomar.
- Cambios preexistentes en khcom: port_main.c (monkey inputs y tiempos),
  test_levelup.py (--timing), test_monkey.py y reparto en PENDIENTES.md.
  Se conservaron; no asumir que son cambios de esta retomada.
- Claude tiene src/main.c modificado con el harness `room` basado en MAP.
  Hay capturas en build/roomtest y carpetas build/room_1_* de su worktree;
  no se encontró aquí un informe completo que cierre la batería de salas.
- Stash `claude-hd2d-wip`: experimental, fuera del bloque prioritario.

## Prioridades

Codex: presentación panorámica de menús/transiciones, mapas y arenas.
Claude: harness/verificación de jugabilidad, texto HD, opciones y cierre PPU.
Las partidas completas, estrés de submenús y tiempos de subida de nivel
siguen pendientes según el reparto. No duplicar tareas ni modificar archivos
compartidos a la vez. Los cambios originales de combate/timing se preservan;
BUGS_GBA.md distingue hipótesis del código y fallos confirmados.

## Revisión de esta retomada

Build EU aislada: `build/codex-resume-eu`, compilada desde el árbol actual
con debug y dos trabajos. Incluye las modificaciones preexistentes del árbol;
no es una release y no se instaló en carpetas de juego.

- Subida de nivel: test_levelup.py pasó en modo original (0) y automático (4).
  Captura individual automática a 640×360 en
  build/levelup-regressions/20261014_033322_944630/out_01600.bmp.
- Revisión adicional a 1280×720, modo automático, pixel art, EU español:
  build/codex-resume-review/levelup. 75 capturas, salida 0, sin CRASH.
  Paneles derechos en reposo llegan al borde y conservan relleno/contorno;
  no hay motivo visual para modificarlos. La hoja reducida anterior no
  permitía juzgarlo bien. Captura individual: out_01600.bmp.
- Hallazgo en la salida forzada: parte del BG de estadísticas aparece por
  la izquierda (frames 2120–2136), mientras las cifras salen por la derecha.
  Revisar ScrollBgMapTo y SplitBgLine; contrastar entrada/salida natural y
  original antes de decidir si corregir. No dar el popeo por verificado.
- Mapamundi: build/codex-resume-review/allmap, 25 capturas a 1280×720,
  salida 0, sin CRASH; trace entra en mode_allmap (1401) y vuelve a
  Mode_MapFld (1586). Las bandas ornamentales se mantienen continuas en
  los fotogramas capturados. Faltan otras plantas, regreso desde START,
  entradas naturales y US/JP para cerrar el pendiente completo.

Los dos recorridos adicionales los reproduce el script local ignorado
`build/codex-resume-eu/review_scenes.py`. Son fixtures forzados, no partidas.
No se subieron porcentajes por esta revisión parcial.

## Protección de partidas

Mantener `build/priority-reference` y estados del usuario intactos.
Ranuras recientes 3/4/7/8/9: build/pcwork/states, específicas de su build.
Probar en directorios aislados con copias de SRAM, pocos procesos y sin
alterar configuraciones ni ejecutables en uso. No commitear ROMs/assets.
