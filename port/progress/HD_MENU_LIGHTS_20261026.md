# Luces de los menús HD

Pedido: usar la luz con brillo de PULSA START en todos los indicadores de los menús HD, con transiciones suaves, conservando las luces GBA cuando HD está apagado.

Se reutilizan las cinco muestras existentes del cursor HD. Un reloj compartido interpola suavemente el ciclo de ida y vuelta; SELECT, PULSA START y los menús del port usan la misma imagen y fase. No se modifican los PNG importados, los textos, la línea inferior ni las dimensiones de sus barras.

Los indicadores del port entran en su cola de dibujo existente para respetar superposiciones, borrados y fundidos. La cola mantiene su estructura, incluida la restauración de contexto de la galería. La memoria y las texturas se liberan al cerrar/recrear el renderer; si faltan los recursos queda el indicador original.

Verificación:

- `test_hd_menu_lights.py`: dos pruebas, muestras equivalentes en los idiomas EU y ciclo periódico suave.
- `test_hd_menu_light_renderer.py`: funciones C de producción con SDL software, 64 cuadros, unión del ciclo, fundidos negro/blanco, desactivación y liberación. Comprueba explícitamente que el color es cálido R>G>B: evita la inversión RGBA/BGRA al actualizar la textura.
- `test_ui_compositing.py`, `check_sources.py` y compilación PC EU correctos.
- Capturas reales: título y Opciones sin HD idénticos píxel a píxel a la versión anterior; título y Opciones HD y título alemán terminan sin bios/CRASH/FATAL.

Evidencia privada: `D:/01Claude/ReKHCoM-build/hd-light-qa/report.json` y `qa.png` en cada caso. Ejecutable de esta pasada: SHA-256 `ca67e6aa34af2da7a5ac2f1a98f9196645296fd3021daf6b0fc2db17aedf515d`.
Esta evidencia corresponde al cambio; el paquete final requiere sus propias comprobaciones con el SHA integrado.
