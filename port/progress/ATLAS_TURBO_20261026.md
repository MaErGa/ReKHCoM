# Atlas · pasada turbo 26/10

Entregas con dos capturas reales cada una; fuentes y valoraciones humanas conservadas. Los PR quedan pendientes de aprobación de su nueva versión.

| Pieza | PR | Cambio |
|---|---|---|
| 087 | #206 | Fase de píxel de bordes diagonales |
| 090 | #210 | Base del muro derecho completa |
| 095 | #211 | Árbol y flores completos; el castillo sigue pendiente, tarea atlas-095-castle |
| 074 | #212 | Fragmento azul sobrante eliminado |
| 097 | #213 | Sombras de actores en espera fuera del marco GBA |
| 112 | #214 | Genio y Aladdín, inicialización de cámara y borde de puerta; depende de #213 |
| 061 | #215 | Puntas del pie del muro restauradas |
| 110 | #216 | Reflejos alternados de la pared hasta arriba |

Ver barajas: dos capturas actuales EU, español, 1860/1940 a 1920x1080. El log confirma `HD text: selected style=0 face=KHMenu`. El valor inicial es hdFont=0 y el selector vigente es KHMenu/Sistema (PR139, posterior al comentario de otras dos fuentes); Comic Hearts se reserva para subtítulos. PR194/196 ya contienen las fuentes core. No se agregó otro cambio de código para esta verificación.

Hija Atlas de Ver barajas: 2783c8219d0d4ca4b178db5ab711b9b2.

Seguir las observaciones antiguas del título, START, naipes mundo, remates 115/116/117/118/120, 057, 081, 099, Trickmaster y jefes, además de las propuestas Agrabah/Oogie ya pendientes. La cola no está terminada. No confundir una hija subida con aprobación o integración. La nube del castillo 095 sigue abierta aunque la app agrupe la respuesta parcial como envío respondido.

Aplicar la lógica vigente; consolidarla sólo en las dos pasadas diarias solicitadas. Sin workers, compilación serial de baja prioridad con tope de 768 MB; capturas silenciosas y sin baterías generales.

START comprobado en dos capturas actuales EU (1620/1700): nivel, VIT y puntos moguri todavía están en píxeles. Se conserva abierto; no se publicó una hija sin mejora. Origen: `src/map/map.c`, `MapMenuWriteDigits3/5`, DMA de dígitos a OBJ_VRAM0 dentro de paneles compuestos. Implementar celdas de texto HD sin cubrir el panel ni perder fade/oclusión. Evidencia local: `build/atlas-menus-20261026/menu/review.png`.
