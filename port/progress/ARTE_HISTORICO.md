# Arte histórico: recuperación para revisar

Pedido del usuario 24/10/2026: incorporar a la mesa el arte modificado desde la primera build, conservando
aprobados y honores para poder repasarlos. El changelog comienza en v0.1 (12/10). Este inventario separa la
evidencia recuperada de los huecos que requieren otra captura; no afirma que se conservaran todas las builds.

## Tanda recuperada de capturas reales

17 familias nuevas: 15 pantallas de menús, HUD de combate y título HD; 62 vistas adicionales de idioma/vida quedan
en la ficha como evidencia. Son imágenes completas emitidas por el juego, sin montar ni redibujar. Se revisaron
visualmente los 17 frames canónicos. La escena visible manda sobre el nombre del arnés: `hdm_worldcards_es`
muestra **Mapamundi**, y así se cataloga. No es una captura de Naipes mundo.

Fuentes: `khcom-claude-int/build/hdm_*_es` y `build/titlehd`, binario/log/configuración conservados. Las rutas
relativas a la carpeta contenedora y sus SHA-256 están en [ARTE_HISTORICO.json](ARTE_HISTORICO.json). El SHA
identifica el binario de la carpeta; no demuestra un commit de origen. Los logs de algunas capturas indican
`ReKHCoM v2.0 (built Oct 23 2026)`. Son evidencia histórica, no una validación de main actual.

| Familia de revisión | Captura nativa |
|---|---|
| Menú START · cabecera y márgenes | `khcom-claude-int/build/hdm_menu_es/out_01700.bmp` |
| Ver barajas · cartas, textos y cabecera | `khcom-claude-int/build/hdm_deck_es/out_01940.bmp` |
| Naipes mapa · cartas y cabecera | `khcom-claude-int/build/hdm_mapcards_es/out_01980.bmp` |
| Mapamundi · panel, mundos y cabecera | `khcom-claude-int/build/hdm_worldcards_es/out_02000.bmp` |
| Estado · panel y cabecera | `khcom-claude-int/build/hdm_status_es/out_02020.bmp` |
| Diario · panel y cabecera | `khcom-claude-int/build/hdm_journal_es/out_02040.bmp` |
| Cargar partida · panel y cabecera | `khcom-claude-int/build/hdm_load_es/out_00760.bmp` |
| Guardar · panel y cabecera | `khcom-claude-int/build/hdm_save_es/out_01760.bmp` |
| Elige un mundo · panel y cabecera | `khcom-claude-int/build/hdm_worldselect_es/out_01900.bmp` |
| Vórtice · panel y cabecera | `khcom-claude-int/build/hdm_worldwarp_es/out_01760.bmp` |
| Tienda moguri · panel y cabecera | `khcom-claude-int/build/hdm_shop_es/out_01900.bmp` |
| Baraja de Riku · cartas y cabecera | `khcom-claude-int/build/hdm_rikudeck_es/out_01100.bmp` |
| Bonus élite · cartas y cabecera | `khcom-claude-int/build/hdm_premire_es/out_02000.bmp` |
| Subida de nivel · panel y cabecera | `khcom-claude-int/build/hdm_levelup_es/out_01000.bmp` |
| Síntesis de estancias · cartas y cabecera | `khcom-claude-int/build/hdm_synthesis_es/out_00470.bmp` |
| HUD de combate · cartas y medidores HD | `khcom-claude-int/build/hdm_gauge50_es/out_00700.bmp` |
| Título · logo HD y barras | `khcom-claude-int/build/titlehd/present_01700.bmp` |

Estas capturas entran en **Pre** con versión «Histórica» cuando no existe una aprobación exacta de sus píxeles.
Una aprobación antigua de márgenes no aprueba automáticamente su cabecera/cartas HD posteriores. Las familias
ya presentes en **Aprobados**/**Honores**, sus comentarios y sus versiones se conservan; la herramienta nunca
las revalora ni publica un envío humano. Las escenas/jefes que ya están en la mesa no se duplican.

## Repetir y verificar la carga

Desde un checkout con el importador, usar `--workspace` para la carpeta que contiene los worktrees y `--data`
para la mesa compartida real. La ejecución sin `--apply` lee SQLite en modo solo lectura, verifica todos los
hashes y genera un plan antes de cualquier importación. No crea una mesa al planificar.

```text
python port/tools/art_review/catalog_history.py --workspace .. --data ../khcom-codex/build/art-review --report build/history-plan.json
python port/tools/art_review/catalog_history.py --workspace .. --data ../khcom-codex/build/art-review --apply --report build/history-imported.json
python -m unittest discover -s port/tools/art_review -p test_catalog_history.py
```

Repetir `--apply` preserva las familias existentes por clave o píxeles y no crea hijas. Un archivo ausente queda
en `missing`; un hash distinto detiene todo el plan antes de escribir. Si aparece una versión más reciente,
entregarla por el flujo normal de hija y revisión. No sobrescribir el manifiesto para esconder una discrepancia.
Los archivos grandes, capturas, binarios y base de revisión permanecen fuera de git.

## Cobertura que aún requiere evidencia/captura

Estos puntos siguen abiertos para las siguientes tandas; no son trabajo terminado por tener un nombre en la
lista. Intentar regenerar con fixtures/harness existentes antes de declararlos irrecuperables.

| Pieza | Qué falta exactamente |
|---|---|
| Naipes mundo · selección de carta | El archivo hdm_worldcards_es muestra MAPAMUNDI al verlo, no la selección de Naipes mundo. Mapamundi sí se recuperó como captura nativa; falta localizar/capturar Naipes mundo y no trasladar etiquetas del arnés a ciegas. **25/10: capturada con la build vigente** (test_hd_menus `worldcards` ES, pieza `0e705199e35740d7a0d5f85dba579621`; no es la build histórica). Referencia: `REPARTO.md; PENDIENTES_HISTORIAL_2026-10.md`. |
| Retratos de diálogo · recorte/entrada y salida de Sora/Riku | Aprobación documentada 18/10; no se encontraron build/portrait59-output ni portrait59-regression en los worktrees inspeccionados. Reproducir o recuperar esos archivos antes de asignar la aprobación a píxeles concretos. Referencia: `port/progress/RETRATOS_59_LOGICA.md`. |
| Título original · barras aprobadas18/10 | La captura encontrada es de título HD posterior. Falta build/title60-regression; conservar aprobación textual sin inventar equivalencia con el logo HD. Referencia: `port/progress/TITULO_60_LOGICA.md`. |
| Cierre superior Hollow Bastion · borde natural | La captura v2 conocida es un recorte sin binario/log enlazados en esa carpeta. Falta evidencia nativa completa del caso exacto aprobado. Referencia: `port/progress/LOGICA_APROBADOS.md; khcom-claude-int/build/evidence/top_edge_plain_wall_v2_17.png`. |
| Idioma y copyright · márgenes | No se localizó captura nativa con procedencia suficiente en los conjuntos inspeccionados; no sustituir por una pantalla de título. Referencia: `port/progress/REPARTO.md (17 ventanas)`. |
| Añadir/quitar naipes · subpáginas de Ver barajas | Hay recortes/hojas/archivos .last que apuntan a carpetas ya ausentes; falta captura nativa del menú completo con binario/log. Referencia: `khcom-claude-int/build/menucap; CHANGELOG.md v1.8.3`. |
| Mapas de campo y HUD/minimapa · todas las salas | roomtest_eu/rooms.png es hoja de contacto. Faltan capturas individuales conservadas con caso/build para revisar cada sala; una sola escena no cubre todos los mapas. Referencia: `CHANGELOG.md v0.1–v0.8; port/progress/REPARTO.md`. |
| HUD y arenas originales · anclaje/entrada/salida | Las familias de jefes existentes conservan sus entregas; falta catálogo separado de transiciones, arenas comunes y todas las fases originales. La captura HD importada no demuestra esa matriz. Referencia: `CHANGELOG.md v0.1–v1.9.2; port/progress/BATTLE_SCENERY_AUDIT.md`. |
| Películas y fondos HD · encuadre/transiciones | Los logs de prueba no bastan como entrega visual; recuperar capturas nativas por caso y versión sin incorporar assets del juego a git. Referencia: `CHANGELOG.md v0.2–v2.0`. |
| HUD HD piloto / HD-2D / números y marcos anteriores | Se localizaron atlas, crops y composiciones de diseño; no representan una captura completa verificable de cada entrega histórica. No confundir con el HUD Re:CoM capturado el 23/10. Referencia: `CHANGELOG.md v0.8–v1.8; port/progress/HUD_HD_EXPERIMENTAL.md`. |
| Launchers, Opciones, logros y controles · plataformas | Hay capturas sueltas históricas, pero falta manifiesto uniforme de binario/log/caso y revisión visual por plataforma. Botones táctiles/primer arranque ya tienen familia viva y se preservan. Referencia: `CHANGELOG.md; port/progress/PENDIENTES_VIGENTES.md`. |
| Todas las revisiones visuales entre primera build y hoy | El changelog registra 0.1(12/10)–v2.0. No se conservaron capturas nativas de cada versión. Este catálogo recupera evidencia disponible; no declara reconstruido todo el historial ni traslada aprobaciones entre builds. Referencia: `CHANGELOG.md`. |

Las nuevas capturas de main deben decir «actual» con SHA real y no fingir que son la versión antigua aprobada.
Tomar cada pieza en la cola online antes de capturar para evitar duplicar el trabajo de Claude. El historial
de estas tandas y los PR se registra en PENDIENTES_VIGENTES y en la tarea `catalogo-historico`.
