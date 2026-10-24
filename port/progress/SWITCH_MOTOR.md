# Motor nuevo de Switch (plan, 24/10/2026)

Pedido del usuario: Switch a velocidad completa (#35: 20-25 fps antes de presentar por GPU; sin medidas después).
Referencia: ggdrn/KH_COM_VITA llega a 60 fps en Vita con la lógica en un núcleo y la PPU en otros dos (3-6 ms por
cuadro), escala por GPU (vitaGL).

## Publicación (decisión del usuario, 24/10)

La **v2.0 estable sale sin Switch**. La Switch se queda en la última prerelease de la 2.0 (el motor actual). Vuelve a
un release cuando el motor nuevo esté hecho y comprobado (medido en una Switch real). Al armar la v2.0: sacar el
`.nro` de `stage_release.py` MAIN para esa publicación y decirlo en las notas.

## Cómo es hoy

- El juego y toda la capa del port (PPU incluida) son una **imagen invitada** AArch64 de punteros de 32 bits
  (`arm64_32`, Mach-O en 0x10000000) servida por el puente (`port/ps5/shared/bridge_list.h`). El host
  (`port/switch/host/switch_host.c`, libnx) mapea la imagen, presenta las texturas por GPU (SDL2), suena y lee mandos.
- La PPU corre **en un solo núcleo**: el invitado no tiene hilos ni TLS, y `PORT_PPU_THREADS` (filas fuera de la
  imagen GBA repartidas entre núcleos, `RenderOuterRows`) es solo de escritorio porque usa `_Thread_local`.
- El salto de cuadros mantiene la velocidad del juego cuando no se llega a dibujar (`frame skip` en el log).

## Fases

0. **Medir en la consola** (primero, barato): una línea por segundo en el log de las consolas, como la de la Vita de
   ggdrn: µs de lógica, PPU y presentación, cuadros saltados. Con el log de una Switch real se sabe qué parte pesa.
   Sin eso el motor se construye a ciegas.
1. **PPU en varios núcleos en el invitado**:
   - Puente: `thread_start(fn, arg, core)`, `sem_create/post/wait` (libnx `threadCreate` en los núcleos 0-2; el host
     llama la función del invitado con el mismo trampolín que `guest_main`). En el host de prueba Linux, pthreads.
   - Sin TLS: los búferes de línea marcados `PPU_TLS` pasan a un contexto por núcleo (`PpuCtx`) que recorren las
     funciones de dibujo (refactor mecánico de `port_ppu.c`). En PC se comprueba píxel a píxel
     (`compare_ppu_builds.py`, `KHCOM_PPU_VERIFY`, baterías) antes de habilitarlo en consolas.
   - Primero las filas fuera de la imagen (como en PC); después las 160 filas de la imagen guardando por línea el
     estado que usa la PPU (registros, paleta: el siguiente paso también para PC a x1, RENDIMIENTO.md).
2. **Solo si 1 no alcanza**: la PPU en la GPU (deko3d / shaders), proyecto grande.

## Hecho (24/10/2026, pedido del usuario: "rebuildear el motor de switch antes de la 2.0 y hacer pruebas")

Medido primero (fase 0): a 1280x720 el lienzo es 320x180 (x4) y la imagen GBA ocupa 160 de las 180 filas, así que
repartir solo las filas de los márgenes (lo de PC) no servía. Se hizo el paso siguiente de RENDIMIENTO.md:

- **Las 160 filas de la imagen en varios núcleos** (`port_ppu.c`, `PpuInnerKeep`/`PpuInnerDraw`): primero corren
  los eventos de línea del cuadro (VCount, HBlank, DMA de HBlank) y cada línea guarda lo que su dibujo lee y ellos
  pueden cambiar: los registros de pantalla (0x00-0x5F), el punto de referencia afín, y la paleta y la OAM cuando
  difieren de la línea anterior. Después las filas se dibujan desde esas copias en todos los núcleos, como las de los
  márgenes. Las lecturas de la PPU (`IO16`, `IO32`, `PLTT16`, `OAM16`) pasan por vistas por hilo (`sPpuIoView`...).
  Quedan línea a línea los cuadros con estado entre filas (`PpuRowsIndependent`, el arte con fundido del título) y,
  si un mosaico vertical se enciende a mitad de cuadro, se dibuja lo guardado y se sigue línea a línea.
  Encendido por defecto solo en Switch (`PORT_SWITCH`); en PC `KHCOM_PPU_INNER=1` (para la v2.0 queda apagado en
  PC). El log dice cada 600 cuadros en cuántos se repartió.
- **Verificación píxel a píxel:** `KHCOM_PPU_VERIFY=1` dibuja además cada cuadro línea a línea y compara (cuadro,
  fondo, máscaras, oclusión, glifos HD). `sh build/inner_verify.sh` (combates, Jafar, Darkside, eventos 004/016/056/
  107/120/177, título; 1280x720 y 1920x1080; EU/US/JP): 0 diferencias, repartido en 597 de 600 cuadros.
- **Hilos en la imagen invitada** (sin TLS del sistema): entrada del puente `thread` (`shared/thread_host.h`: libnx
  `threadCreate` en los núcleos 1 y 2 + semáforos en Switch, pthreads en el host de prueba Linux, "un núcleo" en
  PS4/PS5/Android 64). El hilo entra en la imagen por `switch_thread` (crt0_a64.S) sobre una pila de la imagen
  (`rk_tstacks`, 3 x 512 KiB alineadas): la pila dice el número de hilo, y con él su pila del host para las
  llamadas al puente (`gHostSp[]`) y su bloque de variables `_Thread_local` (`gTlsBase[]`): `__tlv_bootstrap` es
  nuestro (lld deja los descriptores para dyld; `rk_threads_boot` los apunta a él al arrancar). Android 64 reusa el
  invitado sin `PORT_PPU_THREADS`.
- Arreglado de paso: los botones táctiles nuevos usaban `SDL_expf`/`SDL_sqrtf`, que el shim de consolas no tenía: el
  invitado de Switch y el de Android 64 no compilaban.

## Cómo se prueba

- Correctitud: el host de prueba Linux AArch64 en la VM ARM64 (`build_switch.py`, `--shot`), mismas capturas que
  con un núcleo; en PC la misma PPU con contexto por núcleo, idéntica píxel a píxel.
- Rendimiento: solo la consola. Un emulador (Ryujinx) sirve para cazar crashes, no para medir velocidad.
- Lo mismo aplica a la Vita (mismo host compartido, 3 núcleos de usuario).
