# Lógica de continuación — pendientes Codex

Este documento es la guía de trabajo junto a la cola única
[`PENDIENTES_VIGENTES.md`](../PENDIENTES_VIGENTES.md). Cada bloque indica el método,
la evidencia necesaria y el criterio para cambiar su estado. No sustituye código,
capturas ni cierres verificables. Editor/cheats: [`SAVE_EDITOR_LOGICA.md`](../SAVE_EDITOR_LOGICA.md).
Controles: [`CONTROLADOR_LOGICA.md`](../CONTROLADOR_LOGICA.md).

## 4 — Sincorazones pequeños fuera de pantalla

- La lógica de culling vive en `src/map/map_tasks.c`; usar sus límites
  `MAP_ENM_OFF_*` basados en `VIEW_*` y los márgenes originales. No copiar la
  fórmula en el test: extraer/compilar las funciones reales con stubs mínimos,
  o probar el binario en runtime, para evitar que prueba y producto diverjan.
- Hay tres comportamientos que validar por separado: familia 00 elimina y
  desactiva collider al salir por X o por `y + z`; familias 01 y 04 guardan
  `wasOnScreen`, sobreviven antes de entrar y solo se destruyen tras haber
  estado visibles y salir por X. Comprobar límites exactos y ±1, entrada,
  salida y regreso en GBA 240×160 y vistas PC 16:9/ultrawide.
- Runtime: duplicar el savestate del usuario a una carpeta de prueba; jamás
  escribir el original. Capturar estado antes/durante/después, distinguir que
  el enemigo no esté cargado al iniciar combate de que desaparezca sin animación
  estando en pantalla, y repetir en EU/US/JP. Mantener builds y estados locales.
- Evidencia actual: la corrección de `MAP_ENM_OFF_*` está en main; el archivo
  `fieldmaps_wide.txt` son capturas cuyo `mode_battle` ya había comenzado, así
  que no demuestran por sí solas el culling. El parser de KHST no puede leer
  ejecutables release sin símbolos `khdata`/`khbss`. `port/tools/test_field_enemy_culling.py`
  extrae macros y tres funciones de producción sin duplicar su lógica, compila
  con clang y pasa límites ±1, `y + z`, `wasOnScreen` y su transición en GBA,
  16:9 y 32:9. Runtime del `slot1.khst` original, usando una copia y el
  ejecutable EU Nightly 2 compatible: capturas 1/60/120/180/240 muestran el
  Heartless entrando y moviéndose; no se reproduce la desaparición. El binario
  de main rechaza ese estado por build ID distinto. Capturas y copia locales en
  `build/codex-culling-runtime/`. Cerrar como corrección validada con el límite
  de que esta sesión no reprodujo el defecto exacto del reporte; reabrir si se
  obtiene una secuencia que lo reproduzca.

## 25 — Remapeo y gatillos

- Lógica: traducir entradas SDL a acciones del juego sin confundir índice/signo
  de eje; LT/RT analógicos parten en reposo neutral, cruzan umbral, se liberan y
  no repiten en un solo ciclo. Persistir el binding con el formato actual.
- Verificar harness SDL virtual en EU/US/JP, overlay abierto/cerrado, pérdida y
  reconexión; después probar un mando físico y registrar modelo/SDL/reportes.
- Cerrar lo físico cuando se pueda nombrar dispositivo y resultado. La prueba
  virtual no se presenta como cobertura física. Avance anterior: 3fd15b641.
- 18/10/2026: SDL3 3.4.18 inicializa gamepad, pero enumera cero dispositivos en
  esta máquina. Windows sí ve periféricos USB/HID, sin que SDL los exponga como
  gamepad. No cerrar el tramo físico; conservar el harness virtual y registrar
  identidad/resultados cuando haya un mando SDL disponible.

## 27 — RetroAchievements

- Estado vigente y evidencia: RETROACHIEVEMENTS.md. rcheevos 12.5.0 fijado,
  cuenta/catálogo experimental en Windows y 16 retos locales propios en EU/US/JP.
  Guardar por región, observar cambios reales en datos nativos, establecer base
  al cargar saves previos y suspender concesiones por modificadores/savestates.
- Corregida la primera aproximación: RA usa direcciones aplanadas IWRAM/EWRAM/
  SRAM, y el port tiene globals relocados fuera de esas ventanas. El lector de
  bus no permite prometer los logros oficiales. No evaluar ni enviar premios
  oficiales hasta validar referencias y cadenas de punteros del set real.
- UI blanca/negra/roja; avisos de logros centrados arriba, fundido/deslizamiento,
  escala de salida independiente del zoom y cola. Probar vista previa marcada
  sin alterar el progreso, a 1080p x3 y en ventanas pequeñas/ultrawide.
- test_achievements.py pasa: puente nativo EU/US/JP, persistencia, no duplicados,
  invalidación, layout en seis resoluciones y contrato rcheevos con servidor
  sintético. Hash EU real comprobado. Login real, proyección oficial y Level 1
  siguen pendientes; fixtures no son validación online ni de hardware físico.

## 26 — Issues #2 y #3

- Lógica: no implementar una petición amplia hasta identificar la función de
  historia o el elemento HUD. Mantener en GitHub una pregunta concreta, revisar
  respuesta nueva y asociar captura/versión si describen un fallo.
- Verificar estado público, contestar con límites y pasos reproducibles; solo
  después crear alcance técnico/prueba. Cierre si el autor aclara, se implementa
  y comprueba, o confirma que ya no necesita el cambio. Hasta entonces abierto.

## 28 — Menús de juego

- Lógica: configurar únicamente el estado inicial con el fixture `room`; congelar
  apariciones enemigas antes del input. Ejecutar una ruta natural de START,
  DOWN, A por proceso y no inyectar otra sala después de entrar o salir del menú.
  Un encuentro aleatorio o fallo del harness no prueba un fallo de menú.
- Para cada submenú comprobar entrada exacta en la traza, navegación/cambio de
  pestaña con controles naturales, regreso a `Mode_MapFld`, proceso sin crash y
  capturas antes/dentro/después. Riku requiere avanzar el tutorial inicial de su
  baraja con botones; luego abre y cierra la vista de baraja fija.
- Matriz de cierre: EU/US/JP, Sora y Riku, las seis opciones, transiciones de
  entrada/salida, capturas y GBA intacta. Builds, capturas y JSON quedan en
  `build/` ignorado. No cambiar mecánica si falla el harness.
- Hallazgo: el harness anterior ejecutaba seis estados en un proceso, forzaba una
  sala sobre una batalla aleatoria y convertía el crash posterior en 0/6 aunque
  Barajas ya había vuelto al campo. También excluía por error Mundo/Diario de
  Riku; al intentar ambas rutas naturales sí entraron y regresaron.
- Cierre: builds frescas EU/US/JP; Sora 6/6 y Riku 6/6 por región, 36/36. Cada
  caso tiene traza, capturas de pantalla y JSON con salida/entrada bajo
  `build/codex-gameplay-menus/`. No se cambió lógica del juego ni se inyectó
  estado durante los menús.

## 29 — Mapamundi y objetos de sala

- Lógica: inventariar desde la auditoría histórica los floors con una sola variante
  que omite suelo/objeto; distinguir variante omitida por diseño de asset faltante.
  Seguir el patrón nativo de las variantes adyacentes, como hizo Monstro: usar el
  tile complementario que cierra el motivo a ambos lados de la unión.
- Para persistencia de objetos, trazar dueño y vida del objeto en cambio de room,
  mapa y recarga; no duplicar una entidad que ya guarda estado en floor. Primero
  hacer fixtures con transiciones concretas y comparar presencia/posición antes,
  después y tras volver.
- Cerrar por lista de variantes y transiciones con prueba en EU/US/JP, capturas
  enteras a zoom nativo/amplio, juego original sin cambios y evidencia de ida/vuelta.

## 36 — Web regional y SRAM

- Lógica: matriz EU/US/JP usando el mismo release web y ROM local de cada región.
  Cubrir arranque, idioma, diálogo, combate, reset y cierre abrupto; para SRAM
  importar una copia válida, guardar en ambos slots, exportar y comparar checksum,
  reiniciar y reimportar. No incluir ROM/save del usuario en el repositorio.
- Ejecutar en navegador real y registrar versión del navegador, SO, consola,
  resultado y capturas/logs sin datos de partida. Fallos del emulador no se
  atribuyen al transporte de archivos sin comparar bytes/checksum.
- Cerrar cada región solo con flujo completo de ida/vuelta y datos idénticos
  fuera de los campos modificados.

## 37 — Web: render, audio, memoria, mando y móvil

- Separar mediciones: FPS y memoria tras arranque/reposo/combate; texto y márgenes
  en ventanas pequeñas y anchas; audio al perder/recuperar foco; teclado/mando;
  interfaz táctil en teléfono/tablet real. Guardar métricas reproducibles.
- Solo probar Worker/OffscreenCanvas si un perfil atribuye el cuello a render o
  bloqueo de hilo. Comparar misma máquina, navegador, escena y duración antes y
  después, junto con compatibilidad de entrada/audio. No proponerlo por intuición.
- Cerrar cada subcaso por separado; móvil real no se infiere de navegador desktop.

## 40 — PSG en combate

- Lógica: fijar ROM/región, encuentro, semilla/acciones, frecuencia de muestreo y
  estado de audio; grabar referencia de juego original y build actual desde la
  misma ruta. Alinear comienzo, separar música/efectos si la herramienta permite,
  y comparar tempo, notas, envolvente, canales, cortes y ruido.
- Reproducir el mismo patrón dos veces para descartar aleatoriedad. Identificar
  driver/canal PSG y reproducir una sola hipótesis de arreglo por vez; validar que
  música de Coliseo, ya resuelta, no vuelva a anotarse como pendiente.
- Cierre: discrepancia localizada y corregida o explicación de incompatibilidad
  aceptada, WAV/medidas reproducibles, combate no PSG sin regresión y compilación
  EU/US/JP.

## 49–50 — Mando nativo y transiciones de opciones

- 49: usar el arte Gemini elegido por el usuario con transparencia, máscara de
  regiones y nombres Xbox. Conservar propiedad del input y bindings; foco por
  acción/eje, sin dependencias de imágenes externas al ejecutar. Lógica actual,
  importación y copia de revisión en CONTROLADOR_LOGICA.md.
- 50: mantener presentación de juego hasta negro y cambiar entonces al lienzo
  del menú; fundido final de todas las capas. Luz del selector tomada de frames,
  paleta y tiempos originales del debug, conservando tamaños de cuadros/textos.
  Implementación y comprobaciones en MENU_TRANSICIONES_LOGICA.md (75a7efa60).
