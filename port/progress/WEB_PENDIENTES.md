## v1.9 PUBLICADA — 17/10/2026

- Release publica estable Windows: https://github.com/MatyMeatBoy/ReKHCoM/releases/tag/v1.9
- Android Alpha3 ARMv7 actualizado adjunto a la misma release. Build EU/US/JP,
  parser48 y touch PASS; NO se comprobo en ARM fisico. Mantener etiqueta alpha.
- Web Alpha3: motores EU/US/JP nuevos; Wasm validado/compilado en Node, JS sin
  errores de sintaxis. Pages desplegado; verificacion HTTP/SHA en reporte adjunto.
  https://matymeatboy.github.io/ReKHCoM/
- Windows: PPU206, Riku7, launcher11/import6, fondos circular/rectangular, input
  opt-in/hold/foco/overlay y guardado aislado PASS. Paquetes sin ROM/save/config.
- Devolver acumuladas: runtime OFF inerte; ON devuelve1 y3 cartas asentadas y
  recupera mano21. Fixture3: KHCOM_BATTLE=0:1:0, modo200:hudhp:100, STOCK300/450/600,
  UNSTOCK760-800. Intento con300/360/420 solo acumulo2 por combate/animacion:
  no asumir que cada input del harness logra acumular una carta.
- Instalacion local del BAT actualizada; hashes de saves, INI y estados intactos.
  Copia del savestate anterior + EXE exacto guardada PRIVADA para diagnostico.
- Git main y tagv1.9 actualizados al cierre; snapshot previo preservado como tag
  v1.9-preview-20261017. Nunca subir fixtures privados de build/ ni arte extraido.
- Linux/Steam Deck/RG34XX siguen SIN binario nativo: ABI32/64 pendiente.
  Otros cheats/editor, menus relativos experimentales y zoomx3 reportado pendientes.
  No anunciar esas tareas como terminadas ni compatibilidad de dispositivos no probados.

# Navegador — pendientes y evidencia

Actualizado: 16/10/2026. Prioridad actual del usuario: navegador en GitHub Pages.
Versión: alpha independiente de la estable Windows v1.8.3. No sustituir la
estable ni publicar la futura PC v1.9 sin cerrar sus verificaciones.

## Alpha 2 — publicada

- Fullscreenchange, F11/Escape y ResizeObserver sincronizan SDL/canvas entre frames.
- Táctil centrado 16:9; pruebas C reales de centro y hit test en 5 proporciones.
- HD con KHMenu/ComicHearts y layout compartido idéntico al Windows anterior.
  Prueba real de SDL/WebGL sin ROM dibuja inglés, acentos y subtítulos.
- Guardados serializados; exportar banco activo; reset espera persistencia.
- Interfaz English principal + Español; idioma del juego por región, independiente.
- Caché por hash para shell, motores y fuentes; 17/17 archivos públicos verificados.
- Prerelease: https://github.com/MatyMeatBoy/ReKHCom/releases/tag/web-alpha2 .

## Hecho / observado

- [x] Compilar el juego C real a WebAssembly (EU, US, JP), no un launcher vacío.
- [x] Loader de ROM local con región, tamaño y SHA-1; sin subir ni empaquetar ROM.
- [x] Reutilizar renderizador, audio, zoom, menús y optimizaciones de PC/Android.
- [x] Arranque EU y diálogos de introducción observados por el usuario en Chrome.
- [x] Usuario confirma texto SD correcto en diálogo de Donald.
- [x] Corregir acceso JS a HEAPU8: export explícito en Emscripten 6.
- [x] Corregir declaraciones C implícitas que generaban firmas Wasm incompatibles.
- [x] Alpha1 evitó cuadros vacíos forzando SD. Alpha2 dispone de backend HD real
  y vuelve a SD únicamente si no se cargan/rasterizan las fuentes.
- [x] Evitar duplicar ROM en MEMFS; canvas sin multiplicación por DPR; espera sin
  busy-spin; no segundo timer de SDL RenderPresent; overlay táctil opcional.
- [x] Implementar SRAM/config en IndexedDB, importación/exportación .sav y pausa
  al ocultar pestaña. Implementado no equivale a verificado: ver pruebas abajo.

## Publicación inmediata

- [x] Recompilar los tres módulos con la protección SD final; cero avisos de firma.
- [x] Prueba ejecutable ABI Wasm/Asyncify con memoria fija y callback convertido.
- [x] Publicar rama gh-pages con allowlist; conservar ROM/SRAM/config privados.
- [x] Verificar URL pública y añadir enlace al README y al About del repositorio.
- [x] Registrar commit, hashes, tamaño y evidencia exacta del despliegue.

Publicación verificada: https://matymeatboy.github.io/ReKHCom/ .
Evidencia: WEB_PUBLICACION.json (12/12 hashes y tipos MIME correctos).

## Prioridad alta: compatibilidad y partidas

- [ ] Probar en la URL pública: seleccionar ROM, empezar, diálogo, combate y menús.
- [ ] Probar US y JP en navegador; compilar no garantiza juego regional correcto.
- [ ] Importar una SRAM existente, cargar, guardar en juego, recargar, volver a
  seleccionar ROM y verificar persistencia y contenido de la partida.
- [ ] Exportar .sav y comparar con SRAM del motor; verificar nombres/bancos EU/US/JP.
- [x] Manejar fallo de sync con banner persistente y exportación local; probado con fallo simulado.
- [ ] Verificar IndexedDB privado/cuota en cada navegador real.
- [ ] Verificar exportación del banco activo con partida real. Alpha2 ya consulta
  el banco nativo y escribe el nombre correspondiente; nombres probados en Node.
- [x] Reset espera el sync y se cancela si falla; sync serializado probado en Node.
- [ ] Verificar reset con partida real y cierre abrupto del navegador. Pagehide no
  garantiza que el navegador permita completar I/O; seguir ofreciendo exportación.
- [ ] Pausa/resume de pestaña sin inputs atascados ni ráfaga de audio atrasado.

## Texto HD portátil

- [x] Crear backend Web con Canvas2D y las fuentes del port; Windows mantiene GDI.
- [ ] Mantener métricas, anchos, baseline, colores/fades y ocultación de cada capa.
- [ ] Diálogos, subtítulos, menús, acentos y japonés; EXP sigue pixel como pidió usuario.
- [ ] Verificar Ver barajas / añadir-quitar naipes y animaciones, sin solapamiento.
- [x] Caché de glifos limitada; liberar recursos al cambiar escala/contexto.
- [x] Activar HD únicamente cuando las fuentes carguen; fallback SD ante fallo.

## Audio y rendimiento

- [ ] Medir FPS, tiempos PPU/present, memoria y latencia en Chrome/Firefox/Safari.
- [ ] Medir x1 como benchmark y x4 por defecto; no prometer 60 FPS por compilar.
- [ ] Comprobar buzz, saltos, pausa de AudioContext, reconexión y cambio de salida.
- [ ] Medir coste de Asyncify/compatibilidad de function pointers; optimizar con
  evidencia. No eliminar el soporte a ciegas: películas tienen bucle propio.
- [ ] Evaluar reducir reserva de memoria manteniendo compatibilidad de punteros.
  256 MiB de espacio lineal inicial no son una medida de RAM residente.
- [ ] Evaluar Worker/OffscreenCanvas para UI fluida; threads solo si aportan una
  mejora medida. Multihilo requiere revisar COOP/COEP y alojamiento.
- [ ] No dejar logs crecer sin límite ni hacer sync/compilar shaders cada frame.

## Móvil y controles

- [ ] Android Chrome de gama baja e iOS Safari físicos: memoria, audio y temperatura.
- [ ] Safe areas, orientación, pantalla completa, redimensionado y zoom del navegador.
- [ ] Botones con tamaño táctil cómodo, diagonales y varios dedos sin bloquear scroll
  fuera del canvas; revisar silueta de rodar y símbolo de naipes.
- [ ] Mandos: conexión, desconexión, remapeo y pérdida de foco.
- [ ] Accesibilidad del launcher: teclado, lector de pantalla y estado de errores.

## Distribución y mantenimiento

- [x] Integrar Web en build_all.py para compilar todas las regiones con un comando.
- [x] Versionar módulos, shell y fuentes por hash para evitar mezclar cachés.
- [ ] Empaquetado reproducible y publicación automatizada sin ROMs privadas.
- [ ] Evaluar descarga offline/PWA después de validar guardados y actualizaciones.
- [x] Captura pública Alpha2 en inglés, README y notas de release para jugadores.
- [x] Mantener licencia SDL/créditos y fuentes en web/ZIP.

## PC / Android siguen pendientes

Consultar RELEASE_V19.md y archivo/REPARTO.md. Recompilar y verificar PC tras guardas de
stock US/JP y prototipos; preparar v1.9 sin regresiones. Android ARM alpha está
compilada y x86 emulado probado; falta dispositivo ARM físico. No afirmar prueba
de ARM ni texto HD portátil. Conservar ROMs, partidas y savestates del usuario.
