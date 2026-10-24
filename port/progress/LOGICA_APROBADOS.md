# LÃ³gica de los arreglos aprobados por el usuario (vista amplia)

Reglas que se repiten en todo lo que el usuario ha aprobado. Aplicarlas antes
de proponer algo nuevo; ante un caso nuevo, derivar la soluciÃ³n de ellas.

**Regla vigente de entrega (24/10):** aprender de todas las valoraciones aceptadas, pero activar arte nuevo para
jugadores solo desde la versiÃ³n **con honores** (`build_version`) de la app. Cada entrega entra por PR y la
fusiona un integrador reservado; ver [COLA_COLABORATIVA.md](COLA_COLABORATIVA.md). Las menciones histÃ³ricas a
"aprobado" o "Claude integra" no sustituyen esta regla. No elevar valoraciones ni heredar â˜… a una versiÃ³n hija.

**Mejora continua en cada PR (usuario 24/10):** todo PR de arte trae tambiÃ©n su cambio a este archivo: el criterio
nuevo que dejÃ³ una valoraciÃ³n y la **correcciÃ³n** de la regla cuando una pieza ya aprobada recibe comentarios
nuevos. Una aprobaciÃ³n con comentarios posteriores deja de ser modelo: su regla se reescribe con lo que pidiÃ³ el
usuario y se revisan las escenas hermanas que la copiaron.

## Principios

1. **Solo pÃ­xeles del juego.** Nada se dibuja nuevo: se reutilizan los tiles de
   la propia sala/escena, volteados o repetidos (bits 0x400 hflip / 0x800 vflip
   de la entrada). Si de verdad no hay tile adecuado, se puede dibujar uno, con
   el mismo estilo (el usuario lo autorizÃ³ el 18/10).
2. **Un borde natural cierra el mundo.** Donde la sala termina en corte recto
   se continÃºa con el *propio borde* de la sala (su festÃ³n inferior) volteado,
   vacÃ­o fuera y muro dentro. Nunca un corte recto ni un zigzag inventado.
3. **Nunca repetir adornos Ãºnicos.** Marcos de ventana, relieves y figuras no se
   copian ni se cortan por la mitad: el espacio nuevo se rellena con la pared
   lisa de esa columna. Mejora aprobada con honores en 109 (21/10): sÃ­ se pueden
   intercalar piezas ambientales completas del propio escenario (puestos y
   grupos de jarrones), con separaciones variadas y anclaje correcto al suelo.
   Primero reparar el borde; despuÃ©s decorar. Sin nuevos objetos de juego.
4. **Pared lisa = capa base.** En estas salas BG3 lleva la pared de rayas y
   BG2 solo los adornos; fuera de la sala: BG2 vacÃ­o (0) y BG3 = el tile de
   BG3 mÃ¡s repetido de esa columna (`MapPlainBaseTile`). Elegir por frecuencia
   dentro de la columna, no "el primer tile".
5. **DetecciÃ³n genÃ©rica, no por nÃºmero de sala.** Buscar en los datos de la
   sala la tira que cumple una forma (24 tiles seguidos con borde arriba y
   vacÃ­o justo debajo) en vez de fijar coordenadas; asÃ­ sirve a otras salas
   del mismo tileset. Si no se encuentra, no se toca nada (comportamiento
   original).
6. **GBA intacta.** Todo vive bajo `PLATFORM_PC` y solo fuera de la ventana
   original; se puede apagar (`KHCOM_TOPCLOSE=0`).

## MÃ©todo (rÃ¡pido)
Workflow completo (tiles, pÃ­xeles, pixel art, pruebas en x3 entero): WORKFLOW_ARTE_AMPLIO.md.


1. Reproducir la sala exacta y capturar (ver PRUEBAS_VISUALES.md).
2. Volcar los tiles: `KHCOM_ROOMDUMP=1` â†’ `roomdump.txt` (BG1/2/3) y
   `python port/tools/room_ascii.py roomdump.txt --bg 1 --rows .. --cols ..`
   para ver la forma del borde, las tiras repetibles (`--strip`) y quÃ© tiles
   son comunes.
3. Probar la idea con variables de entorno antes de fijarla; recortar y ampliar
   la zona; comparar con lo que marcÃ³ el usuario (azul = quitar, naranja = usar).
4. Entregar copia jugable + .bat; esperar aprobaciÃ³n; solo entonces dejarlo
   activo por defecto y anotarlo aquÃ­.

## Errores que el usuario descartÃ³

- Continuar el muro hacia arriba repitiendo la fila superior (eco de textura).
- Zigzag con las escaleras de las esquinas (piezas que no encajan).
- Elegir como "pared lisa" una celda con marco de ventana: se ve media ventana
  flotando. La pared lisa se decide por tile y por frecuencia, en BG3.

## Aprobados

- 17: borde superior de la sala de Riku (Hollow Bastion B1, sala 3): aprobado
  totalmente el 18/10 (`src/map/map_cell.c`: `MapFindEdgeStrip`,
  `MapTopEdgeTile`, `MapPlainBaseTile`).
- 061 hielo, 074 Monstro y 057 playa (18/10) y 004: **aprobadas antes de tiempo** (usuario 24/10). Ya no son
  modelo; ver Â«Aprobaciones tempranas corregidasÂ» abajo. Guard Armor y las ventanas 1-17 siguen valiendo
  (REPARTO.md, ESCENAS_REVISION_AMPLIA.md).

- 59 retratos: aprobado el 18/10. La vista amplia exponÃ­a las posiciones de espera
  X=449/-299 antes de aplicar la silueta. Ocultar la espera en `msgface_2` y
  recortar su entrada/salida al Ã¡rea original de la ventana desplazada. Aplicar
  en la funciÃ³n comÃºn, sin excepciones por personaje/escena. Commit `acd85c818`;
  detalle en RETRATOS_59_LOGICA.md.

- 60 tÃ­tulo: aprobado el 18/10. Prolongar las filas planas de BG0 y del OBJ
  del pie de brillo, con paleta viva, mezcla alpha y alineaciÃ³n PixelEdge.
  No tocar letras, ilustraciÃ³n ni pÃ­xeles del rectÃ¡ngulo GBA. Detalle:
  TITULO_60_LOGICA.md; commit `6039fc04d`.

- 69 lÃ³gica mapa GBA: aprobado el 19/10. Las mejoras propias de ReKHCoM que
  cambian lÃ³gica del juego van como opciÃ³n en Trucos â†’ Modo debug, apagadas por
  defecto, con prefijo "[ReKHCoM]" y un panel sobre el arte de Sora que las
  explica con diagrama, en los 6 idiomas. Fuera de eso, la lÃ³gica del juego no
  se toca (63: nada de copias propias de objetos). Commit `8a74f004b`.

- Guard Armor, escena 008 (las piezas caen y se arman antes del combate): aprobado "con
  honores" el 20/10. El patio del combate cubre tambiÃ©n el evento: la extensiÃ³n se elige
  por los cuatro mapas del fondo, sin exigir el modo combate (`port_ppu.c`, `guardPatio`).
  LecciÃ³n: buscar todas las apariciones de un fondo (WORKFLOW_ARTE_AMPLIO.md, paso 1).

- Agrabah, calle de Aladdin, escenas 107/108: aprobada **Â«con honoresÂ»** el
  21/10. Continuar por separado la diagonal de calle y la de las fachadas de
  las azoteas; entre ambas, techo liso del propio mapa. Conservar todas las
  filas del bordillo, incluidos los medios tiles de cada paridad. No repetir
  ventanas, jarrones, puestos ni esquinas. Identificar los mapas por sus datos
  descomprimidos (EU) o directos (US/JP), con paleta activa. Receta en
  `AGRABAH_107_REVISION.md`. Codex commitea; Claude integra.

- Agrabah 109: aprobada **con honores** el 21/10. Borde diagonal completado con una tira donante intacta; puestos y grupos de jarrones completos intercalados con separaciones variadas, como arte de fondo y con paletas BG/OBJ activas. GBA y OAM intactos. Receta en AGRABAH_109_REVISION.md; mejoras incorporadas a WORKFLOW_ARTE_AMPLIO.md para la tanda Preliminar V1.

- Jafar, sala de lava (Preliminar V2): aprobada el 24/10 ("se veÃ­a genial, quedaba muy bueno el efecto"). Encendida
  para todos; `KHCOM_JAFAR_WIDE=0` la apaga. Pared, columnas, orilla y lava siguen por sus perÃ­odos de tiles; los
  pilares mÃ³viles solo en la sala; un solo genio. Receta en BATTLE_SCENERY_TECHNIQUE.md punto 12 (`port_jafar.c`).

- IntegraciÃ³n (22/10): 107/108 y 109 encendidas por defecto para los jugadores; `KHCOM_AGRABAH_WIDE=0` /
  `KHCOM_AGRABAH109_WIDE=0` las apagan en las pruebas. Regla: lo aprobado (o con honores) va encendido, nunca detrÃ¡s
  de una variable de prueba.


## Tablets y plegables (aprobado 24/10)

En pantallas mÃ¡s altas que 16:9 la vista Auto usa las filas de una pantalla 16:9 del mismo ancho (`PortConfigAutoView`,
port_config.c): mÃ¡s grande, sin franja negra sobre el fondo, el HUD cerca de la acciÃ³n. `KHCOM_TALL_FIT=0` lo apaga.

## Revisiones valoradas el 24/10 (app, 13:06â€“13:12 Chile)

La valoraciÃ³n se conserva por versiÃ³n; una aprobaciÃ³n anterior no aprueba una hija nueva.
Las recetas ya estÃ¡n en main. La activaciÃ³n para jugadores sigue la referencia con honores de la app.

| Pieza / versiÃ³n | ValoraciÃ³n humana | Criterio que sÃ­ quedÃ³ aceptado |
|---|---|---|
| 044 / `0ac326ad582f4214b308a5d0c1bc6629` | Aprobada | Continuar las casas y la arcada; conservar una sola torre del reloj y quitar la tira suelta de fachada. |
| 045 / `2f0e9dee739642468bded660684f5355` | Aprobada | CÃ©sped de la extensiÃ³n con el tono de la sombra del bosque; completar el pino cortado con su otra mitad. |
| 053 / `553e59d173b14e00bc61c1d513188719` | Con honores | Colocar matas completas del propio mapa sobre hierba lisa, separadas y con densidad parecida; ninguna mata partida por el donante. |
| 054 / `18f40e19e5814241a966b3348a542977` | Con honores | Prolongar el muelle por su diagonal hacia abajo a la izquierda, repitiendo dos pilotes completos por tramo. |
| 098 / `de7e4e0b84f346c3a5cb9fd905010b50` | Aprobada | Cada muro y el ajedrezado superior siguen su propia diagonal: papel y barandilla a la izquierda, pilar y celosÃ­a a la derecha. |
| 104 / `52df9ff43f4e48b4a722240c723bc2f8` | Aprobada | Donantes con los brillos enteros; algas prolongadas y cerradas; suelo y borde continuos sin repetir el jarrÃ³n. |
| Darkside 03 r6 / `2938cbbb50a345088e08d0365dd0b9f3` | Aprobada | Continuar torso y mechones/ala cortados; tramado solo en lo aÃ±adido y pÃ­xeles originales intactos. |

AplicaciÃ³n al siguiente caso: medir el perÃ­odo y direcciÃ³n de cada estructura; comprobar que el donante contiene
la pieza entera, tambiÃ©n encima del corte y en la Ãºltima fila ocupada. No trasladar las coordenadas de un fondo
a otro. Estas valoraciones documentan casos aceptados, no una aprobaciÃ³n automÃ¡tica de recetas futuras.

## Aprobaciones tempranas corregidas (usuario 24/10, 13:17â€“13:20 Chile)

El usuario dio por aprobadas 004/057/061/074 demasiado pronto y volviÃ³ a comentarlas sobre la versiÃ³n integrada.
Lo que se habÃ­a aprendido de ellas no basta; estas son las reglas corregidas (valen para toda escena nueva):

| Escena / versiÃ³n comentada | Comentario | Regla corregida |
|---|---|---|
| 004 / `22fc4ea176864111880e02b851a58445` | Â«falta acabar patrÃ³n hacia la derechaÂ» | Un patrÃ³n sigue **hasta el borde de la pantalla en los dos lados**; no basta con cerrar el lado mÃ¡s visible. Comprobar el ancho mÃ¡ximo (3840) en ambos mÃ¡rgenes. 004 (r3c): perfil propio; la pared repite con (6,-3) a lo largo de su base (periodo 6 medido en el mapa), el suelo 8x4; alfombra, cajas y manchas quedan Ãºnicas; el borde del zÃ³calo se mide por tile (2y+x). |
| 057 / `e49d2f465f9346d28ba013d9a81de938` | Â«el patrÃ³n no se sigue hacia arriba bienÂ» (4 nubes); Â«pelones a los que falta la mitad derecha del pastoÂ»; Â«mejorar derecha del pastoÂ» | Repetir **hacia arriba** con el perÃ­odo vertical medido del propio patrÃ³n, no con la fila superior. Cada mata de pasto entera (las dos mitades): el donante debe contenerla completa, como 053. El lado derecho con la misma estructura que el izquierdo. |
| 061 / `f31927d3f7014eb3a51e52fcf7323003` | Â«falta un cachitoÂ»; Â«falta terminar el patrÃ³n de pasilloÂ» | Sin huecos: revisar cada esquina y uniÃ³n entre la extensiÃ³n y el mapa. Un pasillo sigue con su propio perÃ­odo hasta el borde, sin cortarse antes. |
| 074 / `303416c343704456aea87be42f0508bf` | Â«el patrÃ³n no estÃ¡ bien, creo que habÃ­a una versiÃ³n mejorada, mejorarÂ» | Antes de rehacer, buscar en la app las versiones anteriores de la pieza y partir de la mejor valorada; el patrÃ³n debe repetir su perÃ­odo real. |

Regla general que sale de las cuatro: medir el perÃ­odo de cada estructura en **horizontal y vertical**, extenderla
**en todas las direcciones** hasta los bordes de la pantalla mÃ¡s ancha, y no dar por terminado un lado sin revisar
los demÃ¡s. Las escenas hermanas que usaron estas recetas se revisan con estas reglas al tocarlas.


## Comentarios nuevos sobre arenas y Darkside 06 (usuario 24/10, 05:33-13:07 Chile; worker claude-sonnet-w3)

| Pieza / versiÃ³n comentada | Comentario | Regla |
|---|---|---|
| Darkside arena / `85b593a92f164922966b64436ccf06ab` | Â«no estÃ¡ completando la arenaÂ» (huecos negros entre copias a 32:9; Â«abajo tambiÃ©n le falta dibujoÂ») | La entrega se captura con **todas** las capas V2 de la arena encendidas: `arena_before_shots.py --v2` ahora activa tambiÃ©n `KHCOM_DARKSIDE_BEACH=1` (la playa de `port_darkside.c`). Antes de comentar un hueco de arena, comprobar que la captura se hizo con las variables de revisiÃ³n completas. |
| Darkside 06 r6 / `ad293ae9c5324daf883db049dd30af46` | Â«ahora quedÃ³ demasiado arribaÂ» | Mover una pieza hasta la posiciÃ³n literal de una nube anterior (23 px) puede pasarse: r7 la baja 11 px (`darkside_revise.py --round r7`). Ante un ajuste de posiciÃ³n pedir/medir el punto medio, no el extremo. |
| Oogie arena / `b7df95116b0543719423d2b29c7aa17f` | Â«no deberÃ­a seguir un patrÃ³n, sino dibujar como pixel artist las puntas que faltan en el GBA de los botones del suelo, a ambos lados; dejar en penumbras el resto como un vacÃ­oÂ»; los fondos y otro patrÃ³n Â«quedaron genialesÂ» | Pendiente (bloqueado, arte nuevo): los botones del suelo se completan dibujando sus puntas faltantes en pixel art, no repitiendo patrÃ³n; fuera de eso, penumbra/vacÃ­o. Los patrones de fondo marcados Â«genialesÂ» se conservan. |
| Trickmaster arena / `bcf3aec4ee7f40748959a87960d4ed5d` | Â«solo debe haber una chimenea en el patrÃ³n como en la escena preliminarÂ» | Pendiente de aclarar cuÃ¡l se conserva (el mapa trae dos chimeneas por perÃ­odo de 256 px). |
| MalÃ©fica 01 / `25aa0ffe25b14be5855a107bd882b6f6` | Â«terminar de dibujar a MalÃ©fica hasta la cola; ver si estÃ¡ en los archivos o dibujar como artista pixel artÂ» | Pendiente (bloqueado, arte nuevo): buscar el resto del sprite en la ROM; si no existe, dibujar. |
| MalÃ©fica arena / `ae681fffa79a4feea3b4ea1ef137a512` | Â«arena bienÂ» (aprobada); sprites se comentan por separado | Sin cambios: la arena de MalÃ©fica queda tal cual. |

## MediciÃ³n: workers vs. trabajo solo (24-25/10/2026)

| Modo | Agentes | Piezas de arte entregadas a la app | Aprobadas | Costo aprox. |
|---|---|---|---|---|
| Solo (Claude, 24/10 14:58-16:24, ~1,5 h) | 1 | 12 hijas (090, 044, 045, 053, 054, 087, 095, 098, 099, 102, 104, 110) | 044, 045, 053â˜…, 054â˜…, 098, 104 aprobadas | 1 contexto |
| Ronda 1 workers | 3 | 1 (Darkside 06 + arena, #70) + 112/120 sin imagen (#71) | 0 aÃºn | ~25 min de preparaciÃ³n cada uno |
| Ronda 2 | 3 | 0 (murieron por lÃ­mite de sesiÃ³n) | 0 | todo perdido |
| Ronda 3 + limpiador | 4 | 1 (004, #75) + imÃ¡genes de 112/120 | 0 aÃºn | ~380 k tokens |
| Ronda 4 (1 worker) | 1 | 1 (087, #76); 17 devueltas como pregunta sin intento | 0 | ~390 k tokens |
| Ronda 5 | 1 | 0 (detenido) | 0 | â€” |

Resultado: **11 agentes de worker â†’ 3 piezas entregadas, 0 aprobadas**; solo â†’ 12 entregadas y 6 aprobadas en ~1,5 h.
Es una **regresiÃ³n clara en tiempo y en esfuerzo** (â‰ˆ1 pieza por ~250 k tokens contra â‰ˆ12 piezas en un contexto).

Por quÃ©:
1. **PreparaciÃ³n repetida**: cada worker reconstruÃ­a worktree, ROMs y build (5-25 min) antes de tocar arte.
2. **Contexto perdido**: el worker no tiene la lÃ³gica aprobada en la cabeza; lee el brief, ve un comentario Â«vagoÂ» y lo libera.
   El agente que viene trabajando las escenas sÃ­ entiende Â«mejorar patrÃ³n superiorÂ» mirando la imagen.
3. **Modelo y esfuerzo bajos** para un trabajo de criterio visual (Sonnet bajo): bueno para mecÃ¡nica, malo para arte.
4. **FricciÃ³n de herramientas** (rutas, `prelim_shot.sh` a `build/pc`, `next` en bucle, `--title`/`--version`, ninja `-f`) consumiÃ³
   rondas enteras; se arreglaba entre rondas en vez de antes.
5. **LÃ­mite de sesiÃ³n**: muchos agentes a la vez agotaron el cupo y mataron la ronda 2 completa.
6. **Sin obligaciÃ³n de intentar**: la regla de liberar Â«si es ambiguaÂ» se volviÃ³ la salida por defecto.

Regla desde ahora: el arte lo hace el agente principal (con contexto), en serie, con la base del generador Python y su
mejora propia; otros agentes (Codex) toman piezas en paralelo por la cola. Los workers solo para tareas mecÃ¡nicas
(capturas, baterÃ­as, limpieza), nunca para decidir arte.
