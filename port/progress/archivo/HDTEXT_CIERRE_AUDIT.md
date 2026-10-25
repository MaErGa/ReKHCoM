# Auditoría del cierre de HD text

Revisión de solo lectura sobre HEAD `53d654365`, 14/10/2026. Se consultaron
el inventario, informes, código actual y capturas recientes de `khcom-claude`.
No se ejecutó el juego ni se modificó el compositor o los porcentajes.

## Resultado

El cierre al 100% de `9965042f0` **no cubre todavía todas las cifras que el
propio inventario clasifica como texto**. EXP y los contadores principales de
las tres barajas están implementados y tienen evidencia regional reciente;
categorías, cantidades por valor, CP de carta seleccionada, teclado, Estado,
resúmenes de partidas y cifras de subida de nivel siguen en pixel art. No son
valores dibujados dentro de los naipes ni indicadores animados.

Esto es un pendiente de cobertura de la opción HD, no un defecto del modo
pixel original. La corrección debe conservar el aspecto original con HD off.

## Evidencia positiva que sí corresponde al código actual

El backend `PortRegisterBgTextTile` registra una dirección concreta y la firma
del tile de 32 bytes. Actualmente lo invocan los helpers `PortCopyDeckDigit`
de los tres archivos de barajas, exclusivamente para `gDeckCountDigitTiles`.
Los sprites EXP identifican los dígitos mediante `gPortTextChar`, estilo 2.

Se revisaron:

- EU: `khcom-claude/build/codex-text-options-hd/20261014_043109_832196`.
- US/JP: `khcom-claude/build/codex-text-options-hd/20261014_043159_645014`.

Sus informes registran salida 0, captures true y cambios de los diez dígitos
EXP en ambas muestras 12345/67890. El log EU registra cinco glifos pequeños
en frames 324/404 y 42 en frame 944 del menú de barajas. Se inspeccionó la
tira EU `eu_324_digits.png`: los dígitos HD ahora son blancos, sobre el panel
azul, y conservan el espacio de sus celdas. Esto supera el diagnóstico antiguo
de números verdes/fondo desaparecido de `TEXT_OPTIONS_VERIFICATION.md`.

Se inspeccionaron `eu-hd1/out_00944.bmp` y `jp-hd1/out_00944.bmp`. Los números
principales 15/99 y 0261/0275 son HD; las barajas desactivadas conservan sus
tonos grises. En la misma imagen, las cantidades inferiores junto a los
iconos de categoría (12, 02, 01, 00) conservan su tipografía pixel.

## Pendientes concretos del código actual

| Texto | Fuente actual | Motivo por el que la opción no lo sustituye | Verificación necesaria tras corregir |
|---|---|---|---|
| Cantidades por categoría de baraja y colección | `src/card/card_deckmenu2.c:4806` y `:4823`; Riku `card_deckmenu2_riku.c:744` | DMA de `gDeckCategoryDigitTiles`, sin registro HD. Diferencia visible en captura EU/JP frame 944 | Baraja, Añadir/Quitar y filtros; cero y no cero, Sora/Riku, regional |
| Cantidades de colección por valor y CP de carta seleccionada | `src/card/card_deckmenu2.c:5430` y `:5459`; intercambio `card_deckexchange.c:1631`; Riku `card_deckmenu2_riku.c:970` | DMA de `gDeckValueDigitTiles`/variantes de cero, sin registro HD | Cuadrícula de valores y selección de cartas, CP cero/no cero, atenuado y regional |
| Número de baraja en teclado | `src/card/card_deckmenu2.c:6601` | Transferencia de 64 bytes de `gDeckKeyboardDeckNumberTiles`, sin registro HD | Editar nombre en barajas 1/2/3; revisar ambas celdas de la cifra, sin ocultar arte |
| Estadísticas Estado: nivel, HP/max HP, CP, EXP/siguiente, AP/DP o puntos moguri | `src/menu/mode_status.c:24`, especialmente `:51` y llamadas `:62–81` | `CopyNumberTiles` copia `gStatusDigitTiles` al BG sin registrar caracteres | Estado Sora/Riku, valores de varios dígitos y cambio de pestaña, HD off/on |
| Nivel y tiempo de resúmenes de nueva partida/carga/guardado | `src/map/map.c:3747`, `:3776`, `:4246`, `:4274`, `:6252`, `:6278` | DMA de `gSaveSlotDigitTiles` al BG sin registro HD | Ranuras vacías/ocupadas y selección; usar copias de partidas, no guardar sobre las del usuario |
| Cifras del panel de pausa del mapa | `src/map/map.c:5297` y `:5314` | `gMapMenuDigitTiles` escribe directamente a VRAM OBJ; el registro BG rechaza direcciones ≥0x10000 y no hay tag equivalente por estas celdas | START en campo y volver; cifras de 3/5 dígitos, HUD/oclusión/animación HD off/on |
| Estadísticas del panel de subida de nivel | `src/card/card_level_up.c:1403` | `DrawLevelUpStatDigits` llama a DrawSprite con la cifra, sin asignar carácter/estilo. El PPU solo reconoce los tags explícitos | Entrada/reposo/salida y selección Sora/Riku, cada estadística, conservar panel aprobado |

Las referencias a líneas son de HEAD auditado y sirven para localizar el
bloque, no para autorizar una edición concurrente. No se propone cambiar los
paneles, headers o contornos aprobados.

Los tiles de estas familias usan paletas diferentes (por ejemplo categoría
1/12/13 y Estado 1/7/8); no es correcto asignarles a todos la paleta EXP
15/14/13. El backend ya admite índices codificados y obtiene métricas del
tile. Hay que validar ceros, variantes atenuadas, firma obsoleta, prioridades
y las celdas de 64 bytes antes de ampliar registros.

## Alcance de pruebas y documentos

`test_text_options_hd.py --deck` cubre EXP y un menú Sora de barajas, con
captura en frame 944. No navega Añadir/Quitar, teclado, Estado, guardado,
subida de nivel ni Riku. Su umbral ≥10 glifos pequeños puede cumplirse con
los contadores principales aunque sigan faltando categorías y otros números.
No demuestra cobertura total de HD text.

`HDTEXT_INVENTARIO.md` conserva candidatos pendientes que siguen presentes en
el código; necesita reflejar por separado qué subset de barajas fue convertido.
`TEXT_OPTIONS_VERIFICATION.md` es anterior a la corrección visual y conserva
pendientes ya resueltos para EXP. `status.json` concatena notas antiguas que
dicen oclusión general pendiente con el cierre al 100%; `REPARTO.md` mantiene
la tarea HD de números/oclusión sin marcar. Son inconsistencias documentales,
además de las omisiones concretas anteriores.

La evidencia `ppu_scenes.txt` declara 12 escenas **recorded**, y
`test_ppu.py` usa `KHCOM_SHOTS`/config HD por defecto off. Es una regresión del
frame pixel; no demuestra por sí sola la oclusión del overlay HD presentado.
La prueba general debe capturar con HD on/off los tutoriales y confirmaciones
que cubren nombres/cartas/cifras, incluyendo mundo/HUD/capa de pantalla.
Esta auditoría no atribuye un nuevo fallo visual de oclusión sin reproducirlo.

## Criterio para el cierre real

Completar o justificar explícitamente con el usuario cada familia de texto
pendiente, manteniendo los dibujos/valores de naipes y las animaciones como
arte original. Obtener capturas HD on/off de los caminos afectados y de
oclusión, en EU/US/JP y Sora/Riku cuando cambie el código. Actualizar después
inventario y evidencia; el porcentaje debe reflejar esa cobertura demostrada.
