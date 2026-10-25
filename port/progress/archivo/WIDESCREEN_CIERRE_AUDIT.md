# Auditoría del cierre panorámico — 14/10/2026

Árbol auditado: `53d654365`. Revisión de solo lectura del código/evidencia;
no se ejecutó el juego ni se modificaron porcentajes. Los cierres actuales son
`2ced556b7` (battles) y `ef3a82f3b` (field maps). El informe anterior
`WIDE_WORLD_VERIFICATION.md` describe una fase previa y sus pendientes no deben
interpretarse como el estado actual sin contrastar estos cierres.

## Resultado

La evidencia nueva sí amplía la verificación: existen las 19 arenas regionales,
los reintentos de las dos salas que entran en combate y el recorrido de
100 Acre Wood. No encontré un defecto de extensión de mapas/arenas en las
capturas revisadas. Hay un error concreto de contabilidad y una descripción
demasiado amplia de la cobertura de salas; debe corregirse la documentación.
El 100 % panorámico no demuestra campañas completas ni todas las salas de
ambos personajes.

## Arenas

En **khcom-claude**, no en el `build/` de este checkout:

- US: `build/codex-wide-battles/20261014_044015_773287/report.json`.
- JP: `build/codex-wide-battles/20261014_044345_123046/report.json`.

Ambos manifiestos contienen 19/19 fixtures correctos, con tres capturas por
arena (180/380/600), 1280×720. El cierre informa revisión de frame600 mediante
contact sheet; la sesión anterior ya revisó individualmente ambos extremos de
las 19 arenas EU. En esta auditoría se inspeccionaron individualmente Hook380
US/JP, Wonderland600 JP y Garden600 US: sin padding lateral ni repetición del
mástil, árbol o portón centrales. Las 19 fuentes completas coinciden en RGB
entre EU/US/JP según el inventario de assets.

Los perfiles actuales usan bandas de 64–96 píxeles de los assets originales;
Hook toma la banda derecha de la barandilla izquierda. La extensión del PPU
solo actúa fuera del canvas GBA y en el BG de arena (charbase0/screenbase12);
no altera OBJ ni colisiones. No hay comparación regional automatizada del
canvas GBA en estos manifiestos: `fixture_passed` significa captura/exit sin
crash, no igualdad de píxeles.

Las salidas de combate se apoyan en `build/btlexit` y `build/btlexit2` del
worktree Claude. El log de la primera registra regreso a `mode_chkbtl` en
frame756. Son finales forzados por `battlewin`, ejecutados por la ruta normal
de salida del juego, y no victorias obtenidas jugando. La rotación/zoom citada
en el cierre corresponde al corazón de entrada; no es una batería exhaustiva
de rotación/zoom de la cámara de arena.

Observación antigua aún visible: Hook380 US/JP tiene un trazo rosa sobre el
retrato de Sora. Las bandas de BG no lo provocan. Sigue sin contraste con el
cartucho para distinguir oclusión original y defecto del HUD; no lo clasifico
como bug confirmado ni como regresión panorámica.

## Salas: conteo reproducido

Se leyeron todos los `report.json` de
`khcom-claude/build/codex-gameplay-rooms`, conservando los recorridos por mundo
y deduplicando `(actor, version, world, floor, room)`. El primer intento EU
de 120 casos tiene 0 aprobados y se conserva como intento fallido anterior;
las pasadas posteriores independientes son las relevantes para el cierre.

| Región | Casos únicos Sora | Captura en Mode_MapFld | Encuentro antes de captura |
|---|---:|---:|---:|
| EU | 120 | 118 | 2 |
| US | 120 | 118 | 2 |
| JP | 120 | 118 | 2 |

Los casos que llegan a `mode_battle` son los mismos en las tres regiones:
`world8/floor11/room1` y `world10/floor10/room6`. Sus hojas
`build/roomfail/sheet_10811.png` y `sheet_11010.png`, revisadas aquí, muestran
la sala correctamente seguida de encuentro y combate. No son crashes.

**Corrección necesaria:** `evidence/fieldmaps_wide.txt` afirma 131 salas y
129/131 por región. Los manifiestos dan **120 únicas, 118/120**, más dos
encuentros respaldados por reintentos. No debe contarse un rerun como otra sala.

La selección real del runner es una planta por mundo: floor1 para worlds0–7
y9, floor11 para world8, floor10 para world10 y floor12 para world11. Por
ello “todas las salas alcanzables de las plantas elegidas” es exacto; “todas
las salas de las plantas de la historia” implica una cobertura mayor a la
demostrada. El runner ofrece `--matrix` y `--actor riku`, pero esos recorridos
no aparecen entre los manifiestos auditados.

El BFS de `config/assets/map_room_tables.yaml` contiene 165 nodos alcanzables
en los grafos Sora de las plantas activas0–12, y 122 en Riku0–11. Estos son
nodos de grafos, no la cantidad de combinaciones mundo/planta ni el número de
casos actualmente capturados. Las plantas adicionales de relleno del YAML
no se cuentan. Esta diferencia pertenece también a Gameplay verification.

## Casos especiales y límites

- TwilightTown ya usa la planta10, con capturas válidas; FORMAT dejó de
  figurar como mundo final. Se revisó `20261014_044859_181194/out_02960.bmp`:
  geometría de sala y color exterior íntegros. Las capturas de esta pasada
  tienen 640×360; no se deben atribuir 1280×720 a todo el cierre de field maps.
- Los halls tienen informe separado `evidence/rooms_eu.txt`, 52/52: incluye
  26 entradas/salidas fijas de Sora y 26 entradas de MAP adicionales. El
  alcance anotado allí es EU; no demuestra por sí solo halls US/JP o Riku.
- 100 Acre Wood existe en `build/roomfail/pooh.png` y los BMP individuales:
  el log entra en `mode_pooh` en frame201. La hoja de 18 capturas revisada
  muestra desplazamiento del bosque y fondo continuo. Es EU; no hay en ese
  directorio una matriz regional de minijuegos o mapas del bosque.
- La comparación del callback con los streams originales aporta 417216
  checks de tiles y bordes. Es una regresión del muestreo, no un recorrido
  de todas las geometrías de ambas campañas.

No se propone cambiar código ni porcentajes a partir de esta auditoría.
Primero corresponde corregir la contabilidad/alcance de la evidencia y
mantener las pruebas de campañas dentro de Gameplay verification. Para
afirmar cobertura exhaustiva adicional harían falta manifiestos de las
plantas restantes y Riku, halls regionales y pruebas específicas de los
minijuegos; su ausencia no demuestra un defecto de renderizado.
