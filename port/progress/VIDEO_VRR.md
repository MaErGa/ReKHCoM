# Presentación VRR optativa, Windows DXGI

Pedido usuario24/10: soporte G-SYNC/FreeSync automático al habilitar la opción, con alternativa segura.
El campo existente vsync conserva0=No y1=VSync; agrega2=VRR. Predeterminado sigue1: no migra ni modifica
la elección del usuario. Lanzador y menú Windows permiten elegirlo. La ayuda recuerda que monitor y
controlador deben tener VRR habilitado. El juego no cambia opciones globales del controlador ni monitor.

Con2, consulta la swapchain prestada por SDL (D3D11 o12) y exige Windowed + ALLOW_TEARING. Si cumple,
solicita intervalo0: SDL emplea los flags DXGI apropiados. Si no cumple o falla la petición, solicita
VSync1. No usa intervalo-1 (VSync adaptativo no es VRR). No identifica compatibilidad por marca ni Hz.
Comprueba una vez por segundo si cambió la elegibilidad (p.ej. exclusiva/borderless) sin reaplicar el
modo estable ni llenar el log. Aplicar Opciones fuerza reaplicar la preferencia. SDL conserva la propiedad
de la swapchain: no se libera ni retiene. No se agregan SDK de fabricante, DLL ni paquetes nuevos.

El log dice eligible/unavailable: compatibilidad de la ruta, nunca afirma VRR activo. El estado real de
G-SYNC/FreeSync y el rango del panel dependen del monitor/controlador. Otros backends conservan VSync.
El reloj original59.7275Hz, audio, cámara y lógica permanecen iguales. No se modifica resolución ni
se fuerza pantalla completa. Pantalla completa exclusiva conserva la alternativa VSync.

Validación: build EU juego+lanzador, check_sources;11 casos del selector de producción con DXGI/SDL
simulados (D3D11/12, bandera ausente, exclusiva, error consulta, backend sin swapchain, fallo de intervalo0,
poll estable y cambio de elegibilidad). Arranques reales ocultos0/1/2 conservan configuración;2 vuelve
correctamente aVSync en SDL software. Prueba dirigida del menú cambia1→2 y lo guarda. El recorrido completo
de opciones falla en DODGE también con ejecutable de referencia previo; no se contabiliza como pruebaVRR.
No se confirma activación de VRR en pantalla física mediante el renderizador oculto.

Fuentes primarias revisadas:
- https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/variable-refresh-rate-displays
- https://wiki.libsdl.org/SDL3/SDL_SetRenderVSync
- SDL3.4.18 src/render/direct3d11/SDL_render_d3d11.c (UpdatePresentFlags / CheckFeatureSupport).

Claude integra por PR; no publicar releases ni cambiar ajustes del usuario.
