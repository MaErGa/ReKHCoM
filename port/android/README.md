# Android — estudio del port (16/10/2026)

Estado real: **Android Alpha 2 publicada**, con APK ARMv7 de tres regiones.
Combate EU/US/JP comprobado en emulador API24 x86 (frame600 y salida frame900),
parser SRAM y controles tactiles C verificados. ARM fisico, rendimiento real y
picker SAF en dispositivos siguen pendientes. No cubre telefonos ARM64-only.
Windows v1.8.3 sigue siendo la unica estable.

Desarrollo: https://github.com/MatyMeatBoy/ReKHCoM/tree/android-alpha2
Las compilaciones de prueba no son releases publicas; estable vigente: v1.8.3.

Importa `.sav`/`.srm` por contenido: SRAM 32/64KiB y 128KiB solo si repite la
misma mitad. Verifica firma/checksum, region compatible y destino de banco 1..5.
Crea copia previa al reemplazar. Exporta SRAM normal de 64KiB. No importa
savestates ni convierte partidas entre regiones. EU usa VER13; US/JP VER12 y
requieren elegir la region correcta. Las opciones 4:3 y GBA 3:2 admiten zoom.
HD y quickstates permanecen desactivados en Android.

## Compilar y retomar

`python port/tools/build_android.py` configura y compila EU/US/JP, prepara el
launcher nativo y ensambla una APK debug firmada. `--native-only` omite Gradle.
`python port/tools/build_all.py --platform android` llama al mismo proceso.
SDK35/build-tools35.0.0, NDK30.0.16248370, JDK17, CMake3.22.1 y SDL3.4.18 están
instalados localmente bajo ../toolchain/android con licencias autorizadas.

CMake convierte símbolos COFF a ELF y conserva secciones de globals. El backend
reserva espacio virtual GBA sin reemplazar mapeos ajenos y da páginas físicas
solo a regiones utilizadas. SRAM usa archivos privados y renombrado atómico.
No se incluye el manejador x86 de fallos; accesos BIOS inesperados quedan por
verificar en ejecución. HD desactivado hasta implementar fuentes portables;
estados rápidos desactivados porque contienen punteros con ASLR.

Launcher: SAF, SHA-1 por streaming y tamaño exacto para tres ROMs, importación de
SRAM de 64KiB con copia previa y exportación, zoom x4 y selección de idioma.
Cada biblioteca regional se carga por separado en el proceso :game.
La app no pide permisos de Internet ni acceso global a archivos.
Pausa/reanudación y liberación de toques implementadas; pendientes de prueba.
Manifiesto y hashes de cada APK bajo build/releases/android-build-manifest.json.
Build actual: 1.9-android-alpha2, versionCode 19002; APK de pruebas firmada debug.

## Herramientas y motor recomendados

| Pieza | Elección | Motivo |
|---|---|---|
| IDE | Android Studio; también build por línea de comandos | SDK Manager, profiler, emuladores y dispositivos |
| Código del juego | C + Android NDK/Clang + CMake/Ninja | Reutilizar el port y sus correcciones |
| Ventana, audio y mando | SDL3, con SDLActivity | Ya es la capa utilizada en PC |
| Presentación 2D | SDL_Renderer con backend GLES, escalado nearest | Subir el framebuffer y dibujar controles en pocas llamadas |
| Launcher | Activity nativa Java/Kotlin, vistas simples | Selector de ROM, ajustes, partidas, sin un segundo motor |
| Archivos | Storage Access Framework / ACTION_OPEN_DOCUMENT | Abrir desde Archivos y proveedores, sin permisos globales |
| Texto HD | FreeType o SDL_ttf, caché de glifos | Sustituir GDI sin rasterizar cada fotograma |
| Medición | Android Studio Profiler, Perfetto, adb; luego dispositivo modesto | Medir CPU, memoria, audio y temperatura |

No recomiendo migrar a Unity/Godot para esta primera adaptación: no resuelve las
dependencias de memoria del núcleo. El PPU de software existente debe conservar
prioridades, ventanas, mezclas y efectos de la GBA. La GPU presentaría/escalaría
su salida y el overlay; no equivale a trasladar automáticamente todo el PPU a GPU.

SDL documenta SDK 35+, NDK r28c+ y API mínima 21 en su guía actual. Fijar versiones
exactas del SDK/NDK/Gradle/SDL en el proyecto al implementarlo; mínimo propuesto
API21 en la compilación actual; ejecución comprobada parcialmente en API24.

## Bloqueos comprobados en el código

1. `port/include/port/port.h`, README y estructuras del juego requieren punteros
   de 32 bits. `port_memory.c` escribe punteros nativos dentro de palabras ROM de
   32 bits. **No basta con cambiar el compilador a arm64.** ARMv7 puede servir de
   primer hito, pero no cubre teléfonos que solo admiten apps de 64 bits.
2. `port_memory.c` reserva una sección PE hasta 0x0E010000 y depende de direcciones
   GBA absolutas. Android carga bibliotecas ELF/PIE con ASLR: estudiar mapeos
   puntuales sin reemplazar memoria ajena y abortar ante conflictos. No usar
   MAP_FIXED a ciegas. Las regiones pequeñas deben respetar el tamaño de página.
3. `port_biosfault.c` interpreta instrucciones x86 y registros CONTEXT de Windows
   para accesos BIOS/espejos y divisiones por cero. No se puede usar en ARM.
   Registrar y resolver los accesos conocidos mediante helpers explícitos;
   verificar el comportamiento original, en vez de copiar ese manejador.
4. `port_sections.c` y `port_state.c` leen PE/GetModuleHandle. Sustituir por límites
   ELF definidos por linker. Estados rápidos de Windows no son intercambiables;
   preservar la compatibilidad de los `.sav` normales.
5. `gen_pc_assets.py` emite ensamblador COFF con `_symbol` y `.rdata,"dr"`.
   Hace falta salida ELF y relocaciones compatibles con cada ABI.
6. Texto HD usa GDI/windows.h; el launcher usa Win32, OpenGL de escritorio y
   CreateProcess. El launcher Android necesita su propia capa nativa/JNI.
7. Faltan manejo de pausa/reanudación, pérdida de superficie, interrupciones de
   audio, safe areas, multitoque y persistencia al salir al launcher.

Al comenzar no había herramientas Android en PATH. Ya se instalaron de sus fuentes
oficiales bajo toolchain; la lista anterior describe los problemas de origen y
no implica que todos sigan abiertos. La compilación no sustituye la prueba real.

## Flujo del launcher

1. «Cargar ROM» abre ACTION_OPEN_DOCUMENT, CATEGORY_OPENABLE, MIME genérico
   (los proveedores no siempre identifican `.gba`). Validar el contenido real.
2. Leer en streaming hacia un temporal privado, limitar a 32 MiB, calcular SHA-1
   y verificar código/región y los hashes admitidos por PC. Borrar el temporal si
   falla; renombrarlo solo tras completar la validación. No cargar dos copias en RAM.
3. Mostrar Europa/América/Japón e idiomas correspondientes. Guardar `.sav` en
   almacenamiento privado; importar/exportar mediante SAF. No solicitar acceso a
   todos los archivos ni incluir ROMs en APKs.
4. Una actividad SDL en proceso `:game` cargaría solo la biblioteca de la región
   seleccionada. Las tres tienen globals/símbolos coincidentes: no cargarlas juntas.
   Al cambiar de región, guardar y cerrar ese proceso de forma controlada antes
   de iniciar otro; revisar ese ciclo con SDL antes de implementarlo.

## Rendimiento en gama baja: trabajo concreto

- Mantener lógica a 59.7275 Hz. Escalar la presentación, no acelerar el juego.
- El PPU reserva **13 bytes por píxel máximo** (dos buffers RGBA y máscaras
  de 1+1+1+2 bytes), y port_main otros **8 bytes/píxel** para composición/HUD.
  A 2560×1440 son **73.83 MiB**, sin contar otros buffers, ROM, texturas ni audio.
  Es capacidad reservada/calculada, no una medición de RSS o consumo real.
- Para móvil, convertir esas capacidades globales a asignación según el canvas
  activo, crecer con cuidado al cambiar zoom y reutilizar buffers. Por ejemplo,
  el mismo subtotal a 640×360 sería 4.61 MiB. No imponer ese canvas si recorta
  la escena; definir límites coherentes por aspecto y zoom.
- Mapear solo las regiones GBA necesarias: la reserva PE contigua de Windows
  no debe convertirse en una asignación Android enorme.
- Perfil ligero: texto pixelado, sin HD-2D, caché acotada de glifos, márgenes
  simples cuando sea posible, nearest y resolución interna moderada. Exponer
  ajustes sin cambiar partidas ni dificultad.
- Comenzar con el mezclador SDL actual, cola pequeña con protección ante
  underruns. Medir antes de cambiar a Oboe/AAudio; no sumar dos mezcladores.
- Pausar CPU/audio al ir a fondo; descartar todas las pulsaciones activas.
  Estudiar Swappy solo si hace falta integrar frame pacing EGL: no añadirlo
  junto al pacing actual sin definir cuál controla la presentación.
- Medir percentiles de frame time, underruns, PSS, temperatura y batería en
  sesiones de 15–30 min; no prometer «gama baja» basándose en el emulador.

## Controles táctiles

Referencia funcional: distribución GBA landscape de RetroArch (D-pad izquierdo,
A/B diagonales a la derecha, L/R, SELECT/START). Se dibuja arte propio,
sin reutilizar sus imágenes. Ver `assets/` y `preview.html`.

- D-pad con diagonales; A/B y L/R de GBA; START/SELECT independientes.
- Botón de rodar con silueta propia de Sora encogido, pelo en puntas y zapatos;
  flecha curva. Botón de apilar con tres naipes y símbolo de suma.
- Mismo marco blanco, fondo oscuro y transparencia que el resto de botones;
  respuesta visual al pulsar. Opacidad y tamaño ajustables; áreas táctiles
  mayores que el dibujo, mínimo 48dp, dentro de los insets del dispositivo.
- Multitoque por finger ID. Liberar solo la acción del dedo que se levanta;
  cancelar todo al perder foco. Un dedo debe deslizarse entre direcciones.
- Rodar/apilar usarán PORT_KEY_DODGE / PORT_KEY_STOCK del port, sin inventar
  macros temporizadas de A/B/L/R. Mantener el mismo comportamiento que PC.
- Prototipo HTML muestra pulsaciones simultáneas; no controla el juego todavía.
  El layout sobre la imagen de PC solo sirve para revisar tamaño y posición.

## Compilación conjunta y condiciones de alpha

Ya existe `python port/tools/build_all.py`: configura y compila EU/US/JP para
Windows, crea manifiesto de hashes y opcionalmente empaqueta. `--platform android`
compila las tres bibliotecas y la APK experimental. No publica ni cambia versiones.
`--dry-run` muestra el plan.

La siguiente etapa es compartir lista de fuentes/opciones con CMake: matriz
`plataforma × región × ABI`, carpeta por combinación, una sola versión de producto,
un APK con bibliotecas regionales (o splits por ABI) y un ZIP Windows con las tres
regiones. Artefactos fuera del código generado. Las ROMs privadas solo en el
paso de generación autorizado; jamás en logs, cachés públicas o artefactos CI.

Una alpha jugable necesita: boot y SHA de ROM, juego/carga/guardado real, batalla
con audio, menús y cinemática, controles simultáneos, suspender/reanudar, pruebas
de instalación en un ABI admitido y dispositivo real. Publicar como prerelease
Android junto a v1.8.3 stable, no reemplazar ni retocar su ZIP. Mientras no pase,
el estado es «port en preparación», no «Android disponible».

## Fuentes primarias consultadas

- [SDL3 Android: Activity, CMake y ciclo de vida](https://wiki.libsdl.org/SDL3/README-android)
- [Android: Storage Access Framework](https://developer.android.com/training/data-storage/shared/documents-files)
- [Android NDK: ABIs](https://developer.android.com/ndk/guides/abis)
- [Android: compatibilidad 64 bits](https://developer.android.com/games/optimize/64-bit)
- [Android: páginas de 16 KiB](https://developer.android.com/guide/practices/page-sizes)
- [Android: Frame Pacing / Swappy](https://developer.android.com/games/sdk/frame-pacing)
- [RetroArch: layout GBA landscape](https://github.com/libretro/common-overlays/blob/master/gamepads/gba/gba.cfg)
- [RetroArch: overlays de entrada](https://docs.libretro.com/development/retroarch/input/overlay/)

## Símbolos absolutos: regresión de arranque

En API24, las referencias dinámicas SHN_ABS recibían la base ASLR de la .so.
Esto transformaba el valor 26 de gNumMusicPlayers y acababa escribiendo fuera
 de la tabla de audio (SIGSEGV en SoundMainBTM). Se enlaza con -Bsymbolic para
resolver los símbolos del núcleo localmente. build_android.py inspecciona ELF32
 y rechaza relocaciones que aún referencien símbolos absolutos. Mantener esa
comprobación tanto para ARMv7 como para x86 de pruebas.

Prueba local: importar B8CP.gba desde SAF, iniciar Jugar, selección de idioma,
confirmar con A, START para alcanzar título. x86 solo prueba el backend común;
no ofrece datos de rendimiento ARM ni garantiza compatibilidad en móviles.

## Pruebas de ejecución

`python port/tools/build_android.py --abi x86 --smoke` genera una app aislada
com.rekhcom.android.smoke. Instalar ese APK en el emulador y ejecutar
`python port/tools/test_android_runtime.py --serial emulator-5554` con las
ROMs privadas en roms/. Verifica SHA de cada copia, batalla y frame600/900 en
EU/US/JP; no toca la app del jugador. El APK smoke no se publica.

Las tres regiones pasan en API24 x86. Launcher SAF, SRAM, movimiento, menú,
opciones táctiles y background/foreground se verificaron aparte en la app
normal. Falta hardware ARM; estas pruebas no miden rendimiento de móviles.

El cargador stock US/JP también necesitó comprobar tres cartas antes de leer
sus definiciones: el acceso NULL original dependía de SEH en Windows. Las
combinaciones válidas mantienen sus resultados en las tres regiones.

## HD mod (optional)

Make it on a PC: ReKHCoM for Windows/Linux > Mod HD > "Take the HD mod to another platform" > **Android** >
"Make ReKHCoM-HD.zip". Copy that file to the phone (Downloads, say) and in ReKHCoM on the phone use
**Install ROM...** and pick it: the music, cards, movies and scenes go in at once.
