# Galería: retorno seguro y vistas previas

Decisión de implementación autorizada por el usuario: reproducir desde título o partida y regresar a la misma
lista, selección y pestaña. Una captura temporal en RAM conserva memoria del juego, hardware y SRAM; se toma y
restaura únicamente en el límite del bucle principal. Las escenas no evalúan logros ni escriben partidas. La
reproducción HD suspende el overlay mientras su pausa está activa y lo recupera incluso si el video falla.

1. Regresión real antes del arreglo: `test_scene_gallery.py` sobre main bf09cf0 reproduce evento 10, vuelve al
   título y falla porque nunca regresa a la lista (log en `<work>/gallery-regression-before/run.log`).
2. Compartir el visitante de memoria ya usado por rollback; guardar una sola captura temporal acotada y liberarla
   después del retorno. Conservar SRAM y excluir evaluación de logros durante la reproducción.
3. Separar entrada configurada A/B de pestañas; mantener selección al volver. Recuperar overlay al acabar HD.
4. Añadir cartel lateral con nombre y captura estática, sin tapar filas. Mantener una sola miniatura residente;
   nunca precargar videos completos ni decodificar en cada cuadro.
5. Verificar reproducción, salto, regreso y repetición; captura real del menú, fuentes y `check_sources.py`.


## Estado para revisión (26/10)

- Piloto de la matriz: **24/24**. Modos HD / GBA mejorado / GBA original, Sora y Riku desde partida, GBA natural,
  ReKHCoM con motor nativo y cancelación de pausa, ReKHCoM con HD y salto, HD con archivo inválido (dos intentos).
- Cada reproducción completada compara estado de juego, mapa, configuración, archivos de logros/escenas y versión/rango
  Re:CoM; todos iguales. SRAM del save de prueba intacta, sin `bios:`, crash ni acceso a un canvas liberado.
- Previews antes de reproducir: **199/199** fondos GBA del catálogo; un solo still HD bajo demanda (MF/ffmpeg), después
  de 12 cuadros de cursor quieto. 160×90 residente + caché en disco; se liberan lector, sample y buffer del decoder.
- Hallazgo regional: EU omite los dos eventos dummy 127/128. Los límites de progreso Riku y previews usan ahora constantes
  del enum; no se confunde el prefijo numérico del símbolo con su valor. El arnés conserva IDs canónicos en los reportes
  y traduce a EU al invocar el ejecutable.
- Catálogo de 194 eventos no dummy + cinco películas. Tres listas GBA/HD/ReKHCoM: motor cuando no hay HD importado;
  mapeos, extras y offsets son los mismos de gameplay (`PortHdGalleryMovieName`, `MovieFile`, `PlayExtras`). Las colas
  de película ya contenidas en un evento HD completo no aparecen dos veces. No se cambia `hd_scenes` ni `hd_movies`.
- Compatibilidad: sin soporte de checkpoint (web), GBA se reproduce desde título y vuelve a título/lista; desde partida
  se muestra la limitación. Extracción de still HD previa disponible en Windows y Linux con decoder; los hosts sin ese
  extractor conservan el marcador, no se afirma soporte inexistente.
- **Pendiente antes de afirmar cierre general**: matriz grande sobre el SHA final de integración; pilotos dedicados de
  películas/extras, QA a 1920 y ultrapanorámico, y revisión independiente. No es evidencia de todas las combinaciones.
- Datos locales: `<work>/gallery-matrix-pilot2/{manifest.json,results.jsonl,logs,shots}`. 24 casos, dos reproducciones por
  caso, salida 0 y comparaciones registradas; los MP4 de codec son fixtures sintéticos, no footage del juego.

## Revisi�n posterior del candidato

- Cuatro pilotos adicionales pasan sobre el ejecutable EU SHA256 `78150ce92df6bb3e56629faac1c979e0b66f219b75d6e63082cd4b3f3b494406`: evento 56 con HD corrupto y fallback nativo; intro 513 nativa con salto; pel�cula 514 nativa con final natural; final Riku 517 HD cancelando la pausa. Dos reproducciones por caso, estado/SRAM intactos. Evidencia en `<work>/gallery-movie-pilots`.
- Un HD corrupto en ReKHCoM ya no oculta su cola cinematogr�fica nativa: se invalidan las entradas que comparten recurso, se reconstruye la lista y se conserva el evento seleccionado. El piloto confirma que vuelve a estar disponible el alias 515.
- La auditor�a incluye metadata Re:CoM y nombres oficiales de logros EU/US/JP; esto ampl�a las huellas comparadas, sin afirmar ejecuci�n de las tres regiones. El fallback sin checkpoint reinicia la captura para cada escena.
- El arn�s limita los renderers a dos, fuerza un hilo PPU y rechaza reanudar resultados con otro ejecutable, ROM, save o fixtures. Cada resultado registra el SHA del binario.
- Cuatro pilotos de extras pasan con Sora/Riku y versi�n l�gica Re:CoM desactivada/activada, aun con `hd_scenes=hd_movies=0`: la galer�a usa HD sin cambiar preferencias, y los extras 001/614 aparecen solamente con Re:CoM activo. El primer intento con SRAM totalmente vac�a detect� inicializaci�n leg�tima del arranque; el arn�s exige ahora un save inicializado y los cuatro casos se repitieron sobre la copia estable. Evidencia en `<work>/gallery-extras-pilots`.
