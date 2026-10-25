# Continuidad para Claude: HUD, escalas y v1.5

## Estado y autorización

El usuario autoriza corregir, compilar y pushear incrementalmente, lanzar v1.5,
retirar v1.4 después de validar la nueva y rehacer el tráiler. Pidió esta guía
para continuar sin depender del historial de Codex. No es necesario convertirla
en una skill instalada: esta guía y archivo/REPARTO.md viajan con el código.

Trabajar en `khcom-claude`, rama local `claude/windows-5`. Publicar con
`git push origin HEAD:codex/v1.5-fixes` (los nombres local/remoto difieren).
`khcom` es otro checkout con cambios HUD: no sobrescribirlo. No borrar partidas,
ROMs, configuraciones ni paquetes locales anteriores. README.md tiene una edición
previa ajena a este bloque; las imágenes sueltas también son anteriores.

## Lo que pidió el usuario

Sus capturas eran de v1.4: prefería el antiguo x4 frente al normal x5. La referencia
concreta implementada es 4 píxeles de pantalla por píxel GBA a 1920×1080. No invertir
los números basándose en la duda del usuario sobre x1. El menú muestra el factor
real y omite opciones que darían el mismo factor.

Submenús: tamaño independiente del zoom del mundo, entero y menor que antes;
adaptar encuadre 16:9/32:9. Cabeceras arriba, pies abajo y contenido separado.
El usuario llama «slider» a la transición al volver del submenú; las extensiones
de textura deben acompañar la animación original sin residuos ni parpadeos.

## Cambios entregados

- deaabb7: zoom normal x4 a 1080p, x5 a 1440p, opciones válidas únicas, minimapa
  de campo con PortHudBegin(1,-1), dimensiones máximas 1280×540, configuración v4.
- b0359b4: en DrawAffineBg, el fondo de batalla charbase0/screenbase12 continúa
  su última fila al salir de la textura por abajo fuera de las 160 líneas GBA.
  El wrap de la textura estaba activado: comprobar solo !wrap dejaba el negro.
  Se mantiene composición normal de paletas, sprites y efectos. Revisado en
  Ciudad de Paso x3, Agrabah x2, País de las Maravillas x3, y Ciudad 5120×1440.
- Bloque de síntesis: PortScreenBg(1,TRUE/FALSE) en inicio/destrucción del menú
  declara BG1 como UI aunque SetBgMapBlocks haga que parezca mapa del mundo.
  Esto recupera el panel y elimina los bloques cian causados por prolongar la UI
  como escenario. El estado se guarda con PortPpuVisitState.
- MAPSEL_RAISE=8 ya existía; reponer scroll inmediatamente después de tres
  SetBgMapBlocks evita que panel y sprites pierdan su desplazamiento relativo.
  La barra solo calcula lastPage cuando >0 (había división por cero).
- Sombreado: BG0 del spotlight extiende su borde en vez de leer padding de VRAM;
  eliminado negro forzado fuera del rectángulo 240×160 durante la animación.
  Cuando síntesis posee BG1, se presenta el escenario de UI completo sobre negro,
  sin mostrar Sora/puerta a otra escala detrás de la carta iluminada.
- MenuScale en port_main.c: x5 a 1080p, x6 a 1440p, acotado para ventanas pequeñas.
  Aplica a overlay y menús estrechos; películas conservan su ajuste anterior.

## Arquitectura y errores que no repetir

`gPortFrame`: mundo a su escala, origen GBA centrado. `gPortOverlay`: menús de
160 líneas a escala independiente; PortScreenBegin/End marca sus sprites.
`sScreenBg` decide qué fondos forman ese overlay. No inferir siempre «mapa del
mundo» por usar SetBgMapBlocks: el panel de síntesis también lo usa.

`PortHudBegin(ax,ay)` marca HUD de esquina; PortPpuRenderHud lo compone a su propia
escala. `port_hdtext.c` dibuja texto HD separado (capas mundo=0, HUD=1, overlay=2).
Si mueves una capa, debes mover también texto, cursor, números y oclusión.

`DrawMenuHeader` ya etiqueta sprites como header y registra el recurso;
`PORT_BAND` repite barras hacia márgenes, pero no les asigna una semántica vertical.
`PortMenuBandSource` existe para allmap. `SideBar`, `DrawHeaderLeft`,
`DrawEdgeBands`, `BuildNarrowSides` generan extensiones de menús.
Los menús pueden sustituir sprites por BG al quedar quietos (diferencias regionales).
Una solución de cabeceras debe contemplar ambas fases y la salida animada.

**Se retiró el intento BeginMenuCanvas/EndMenuCanvas:** recortaba filas 0–16 y
152–160 de la imagen compuesta para moverlas a los bordes. Cortaba contenido ajeno,
perdía partes del panel y producía glitches. No restaurarlo. La captura que reclamó
el usuario corresponde a esa fase. La versión entregada conserva el contenido
centrado a escala menor: el desacoplamiento vertical sigue pendiente.

No aumentar PORT_MAX_W/H sin medir memoria: probar 1920×720 rompió el arranque
porque BSS desplazó la sección gbaspace por encima de 0x02000000. 1280×540 funciona.
Para más resolución interna, mover buffers grandes al heap o rediseñar esa reserva.

## Reproducir y comprobar

Desde khcom-claude en PowerShell:

```powershell
../toolchain/bin/ninja.exe -f build/rel-eu/build.ninja -j 4
../toolchain/bin/ninja.exe -f build/rel-us/build.ninja -j 4
../toolchain/bin/ninja.exe -f build/rel-jp/build.ninja -j 4
python port/tools/test_zoom.py
python port/tools/verify_v15.py
```

No encadenar pruebas si falla una compilación; PowerShell no aborta automáticamente.
verify_v15 usa directorios nuevos, drivers dummy, copias de ejecutables/fonts y
configuración propia. CAPTURED significa proceso/capturas sin errores detectados,
**no** aceptación visual. Revisar BMP y log. No pasar KHCOM_BATTLE vacío.

Fixture real de puerta: KHCOM_MODE=60:room:10901,120:fieldfreeze,200:doorpose:0;
KHCOM_KEYS=240-242:A,420-422:A,500-502:B,560-562:B. Capturar frames
280,380,470,580,650. room configura mapa real; doorpose sitúa al jugador frente a
una puerta cerrada y añade cartas. El golpe, entrada, panel y salida siguen la
ruta normal del juego. doorpose:1 busca evento; puede no existir en la sala.
`doortutorial` limpia las dos banderas de tutorial. Elegir sala adecuada si no hay
puerta de evento. `roomgraph`/`roomgoto` permiten inspeccionar y cambiar sala.
El fixture antiguo `spotlight,doorselect` no crea contexto real de puerta; produjo
NULL reads y falsos resultados. No usarlo como prueba de síntesis.

Evidencia local (no incluida en Git):
- build/v15-review/arena-bottom-fixed, arena-agrabah, arena-town-wide, arena-wonderland.
- build/v15-review/natural-door-layers2: panel recuperado y salida, versión intermedia
  aún mostraba Sora bajo la carta; corregido en la captura siguiente.
- build/v15-review/20261015_153501_477517/synthesis-panel: versión actual, frame470
  panel completo sin cian ni Sora detrás; frame650 regreso al campo.
- event-tutorial en el mismo directorio: revisar resultado antes de contarlo.

`test_gameplay_menus.py --build build/rel-eu --version eu --cycles 1` abre seis
submenús con START/DOWN/A reales. Añadir capturas intermedias a la salida para
investigar la animación, no limitarse a una captura del estado quieto.

`test_ppu_conformance.py` necesitaba stubs de funciones HD HUD recién incorporadas;
se añadieron en ppu_conformance.c para poder enlazar. Última ejecución completa:
los casos hardware pasaron, pero fallaron semantic_hd_hud_cover_bg,
semantic_hd_hud_cover_obj y semantic_hd_hud_window. NO se ha demostrado que sean
preexistentes. Comparar con HEAD anterior usando los mismos stubs antes de atribuir.
Reporte build/ppu-conformance/20261015_152355_523533/report.json.

## Próximos pasos, en orden

1. Verificar alineación exacta cursor/contador/panel después de tutorial normal y
   de evento; revisar no solo que exista el panel.
2. Separar cabecera/pie por identidad de capa/recurso, con estado de animación;
   conservar escala de contenido y texto HD. Probar también cambio sprite→BG.
3. Corregir salida de márgenes y fondos en cada menú. Comprobar zoom cercano,
   donde canvas del mundo puede ser menor que el overlay.
4. Matriz 1920×1080, 3440×1440, 5120×1440 y todas las escalas válidas EU/US/JP.
5. Actualizar README/changelog, compilar launcher además del juego y empaquetar
   con port/tools/package_release.py usando --version v1.5, builds rel-eu/us/jp
   y --sdl ../toolchain/SDL3-3.4.18. No incluir ROM ni partidas.
6. Publicar v1.5 y retirar v1.4 cuando la sustituta esté validada. Aún no realizado.
7. Tráiler nuevo: ../Trailer tiene material antiguo y audio, no se ha editado el
   nuevo. Capturar mediante KHCOM_RECORD=desde-hasta:ruta.mov (ProRes/ffmpeg),
   KHCOM_WINDOW=1920x1080 y las nuevas escalas; montar y revisar vídeo final.

Actualizar archivo/REPARTO.md y pushear cada bloque con evidencia y límites reales.
