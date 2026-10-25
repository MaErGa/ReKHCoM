# Tirón al entrar al título — 25/10/2026

Reproducido en Windows/D3D11, 1920×1080 a 144 Hz, modo gráfico 2, texto HD
encendido, mod HD apagado, ROM EU y JP presente en roms/, undub apagado.
El cuadro 675 tardó **125,247 ms**, aunque el dibujo consumió solo 3,862 ms.

`mode_title_1` llama por primera vez a `PortUiNeedsModeChoice`, que comprueba
los assets importados. Esa comprobación incluye `PortUndubAvailable`: lee y
calcula el CRC de toda la ROM japonesa (32 MB), aun con undub desactivado.
La comprobación no necesita el juego activo y ya conserva su resultado.

`PortUiInit` prepara ahora esos resultados antes de que arranque la secuencia
del juego. Conserva el aviso de nuevos assets para el título y la verificación
íntegra del cartucho. No añade threads, una segunda ROM en memoria ni un caché
persistente. Los buffers de la comprobación siguen siendo de 64 KB.

Validación con `test_title_startup.py --jp-rom <ROM JP> --gpu`: dos procesos
secuenciales, ocultos y sin audio, un hilo PPU, prioridad baja respetada mediante
`KHCOM_BACKGROUND_TEST=1`. El test comprueba que la verificación ocurre antes
del título y que la secuencia alcanza el cuadro final.

- Sin JP: máximo de la transición **17,01 ms**.
- Con JP: máximo de la transición **16,74 ms**.
- `check_sources.py` y `git diff --check`: OK.

El primer timeout de diagnóstico fue el selector de primer arranque esperando
entrada en una copia sin graphics_mode; se corrigió el fixture con modo 2.
No era evidencia de un cuelgue del título.

Las pruebas en segundo plano ya no elevan su prioridad ni se registran como
MMCSS Games si llevan KHCOM_BACKGROUND_TEST. El juego normal conserva su
política de rendimiento. Se evitó iniciar builds Android por RAM libre ~1 GB
mientras el usuario juega Deadlock.
