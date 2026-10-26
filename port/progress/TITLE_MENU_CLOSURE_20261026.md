# Título, Opciones HD y avisos — verificación del 26/10

## Cambios

- El sombreado conserva el alfa del velo de Opciones: la ilustración HD sigue detrás durante los fundidos.
- Palabras largas, títulos y opciones de confirmación se ajustan a sus cajas, conservando límites UTF-8.
- Brillo del título y L/R sólo en GBA original/vista Original, o mediante la opción experimental explícita del
  juego y del launcher. Fuera de esos casos se restaura la paleta neutra.
- Se termina la infografía de logros del aviso de estados rápidos que Claude dejó sin commit; su WIP se conserva.
- SELECT usa el estilo y separación de PULSA START; la tinta queda una fila nativa por encima de la línea negra
  (siete píxeles visibles de separación en ambos avisos a 1920×1080). El logo HD sin «Re:» viene de la importación existente;
  una captura sin `hd/sprites` había mostrado correctamente el fallback GBA, no una regresión del importador.
- Franja roja y línea negra llegan al borde derecho. La fila vacía se eliminó haciendo coincidir la cuadrícula
  de geometría con la del presentador. La extensión roja no recibe después el rosa de la paleta original.

## Pruebas

`test_ui_compositing.py`, `test_ui_wrapping.py` y `test_title_margin_bands.py` compilan funciones de producción.
Las regresiones fallaron antes de los arreglos y pasan después. La última cubre la línea negra de un sprite
reemplazado por HD sin volver a dibujar las letras originales. `check_sources.py`: OK, incluido el shim y glifos JP.

Compilación EU y launcher: OK. Sobre una copia aislada, con vídeo/audio dummy:

- L/R ocultos producen la misma captura píxel a píxel; al habilitar el control experimental cambian el brillo.
- Confirmación de estados rápidos: 20 capturas (240×160 Original y 320×240 CRT; HD/GBA clásico; cinco idiomas EU)
  revisadas sin textos, insignias, opciones ni pie recortados. No se contó japonés como probado en ejecución EU.
- Título con logo HD importado: 1920×1080, 2560×1080, 3840×1080 y 1920×997, salidas 0 sin bios/CRASH/FATAL.
  En todos, la línea negra llega al último píxel y rojo/negro son contiguos. En 1920×1080, rojo a ambos lados de
  la unión y en el extremo: RGB (140,8,49); negro: RGB (0,0,0).

Evidencia privada reproducible: `D:/01Claude/ReKHCoM-build/root-title-verified/report.json` y `title.png` por tamaño.
SHA-256 del ejecutable de esa pasada: `c520dff8d5927af91b454ed26e459e9a6cbf88d80df4a7a15c0c6c17733a053c`.
Es evidencia de este arreglo, no el paquete final de lanzamiento. Opciones: informes de `root-ui-confirm` y
`build/ui-runtime`; sus manifiestos conservan las fuentes. Toda prueba final posterior debe registrar su propio SHA.

Pedido posterior del usuario: unificar todas las luces de menús HD con el indicador suave de PULSA START,
manteniendo los originales cuando los menús HD están apagados. Se entrega como cambio separado, sin reabrir
los arreglos de color, separación ni composición de esta pasada.
