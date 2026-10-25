# Relevo de Claude — 18/10/2026, 15:35

El usuario continúa con otro chat (Sonnet). Este documento deja todo listo
para retomar. Cola única: PENDIENTES_VIGENTES.md. Reglas: cada uno con sus
puntos; si el otro lleva más de 10 min sin commits ni archivos tocados, se
toman sus pendientes anotando hora y evidencia en la cola, recogiendo su WIP
sin rehacerlo. Ambos commitean y pushean a main.

## Dónde está todo

- Checkout de Claude: `khcom-claude-int`, rama `claude/next`, igual a main
  (`0f7a3bced` + este documento). ROMs en `roms/` (ignoradas).
- Codex trabaja en `khcom-codex` (main). No tocar su árbol de trabajo.
- Toolchain: `../toolchain/llvm-mingw-*/bin` en PATH; `python port/configure_pc.py
  --version eu|us|jp --out build/pc-<v>` y `ninja -f build/pc-<v>/build.ninja`.
- Harness: SIEMPRE `SDL_VIDEO_DRIVER=dummy SDL_AUDIO_DRIVER=dummy` (si no, el
  usuario oye el audio). Capturas de ventana con `KHCOM_OUTPUT_SHOT`.

## Publicado

Nightly 4 `v1.9.3-nightly.20261018.4` (prerelease; stable sigue v1.9.2):
Windows, Linux, Steam Deck, Anbernic, Android, Web (Pages en vivo). PS5 no se
recompiló (requiere host Linux). Receta completa y hashes en
NIGHTLY4_PUBLICACION.md. Desde entonces solo hay commits de documentación y
pruebas (Codex), así que no hace falta otra nightly hasta que haya cambios.

## Estado de la cola (48 puntos)

- Cerrados: 18 (1, 3, 5, 6, 7, 9, 15, 28, 38, 41, 42, 45 y los publicados en
  Nightly 4: 8, 30, 31, 46, 47, 48).
- En curso: 8 (2, 10, 11, 12, 13, 14, 25, 26).
- Abiertos: 22 (4, 16-24, 27, 29, 32-37, 39, 40, 43, 44).

### De Claude

- **10 / #17 (siguiente):** muro de una sala de Hollow Bastion (Riku, B1,
  sala 3) cortado recto arriba en vista amplia. El usuario pidió cerrarlo
  con un borde curvo como el de abajo pero hacia arriba, con tiles propios.
  - Reproducción exacta: `KHCOM_MODE="300:roomriku:10700,800:roomshape:18,
    801:roomseed:138815524,900:roomgoto:3" KHCOM_KEYS="1400-1520:LEFT"`,
    capturas en 1450/1560 a 1920x1080.
  - El código está en `src/map/map_cell.c`, `MapWorldTile`: por encima de la
    fila 0 (y < 0) las celdas son NULL. Ya existe una continuación vertical del
    muro, apagada (KHCOM_TOPCLOSE=1), que el usuario rechazó porque repite
    ornamentos.
  - Intento guardado en el stash `claude-17-border-strip-attempt`: buscaba un
    tramo horizontal del borde inferior para voltearlo (bit vflip 0x800). En
    esta sala no hay ninguno: el borde inferior es un zigzag diagonal (pendiente
    2 tiles por 1). Siguiente paso: componer una banda horizontal alternando
    los pasos de bajada y subida de ese zigzag (periodo 4 tiles), voltearla y
    ponerla en las filas y = -1/-2, con el muro (BG2 de la fila 0) detrás.
  - Stash `claude-17-roomdump-temp`: volcado temporal de BG1/BG2 de la sala
    (`KHCOM_ROOMDUMP=1` → roomdump.txt) para estudiar los tiles del borde.
  - Mostrar antes/después al usuario y esperar aprobación antes de activarlo.
- **11 / #18:** el savestate 3 (Mapamundi Hollow Bastion) se ve completo al
  cargarlo; falta que el usuario indique la zona exacta.
- **32-35:** hardware real (Linux/Steam Deck, Anbernic, Android): requieren
  dispositivos. 34 (fuente CJK en Linux) se puede hacer sin hardware.
- **39:** rutas completas de partida (harness KHCOM_BOT); largo.
- **43:** mantener la cola y los documentos. **44:** limpieza de worktrees y
  builds, preguntando antes de borrar; nunca carpetas ReKHCoM-vX.

### De Codex (no tocar salvo que esté inactivo más de 10 min)

2 y 4 (bug de sincorazones; Codex añadió test_field_enemy_culling.py),
12-14 (revisión visual de escenas), 16-24 (arte de jefes y escenas, al final),
25 (mando físico), 26 (#2), 27 (RetroAchievements), 29, 36-37 (web en
navegador real), 40 (audio PSG).

## Evidencia útil (privada, en build/)

build/evidence/: capturas de trucos, editor, revancha, aviso moguri, diagrama
de mando, #16 y el intento de #17. build/userstates/nightly2: savestates del
usuario con su ejecutable (solo cargan con ese ejecutable).
