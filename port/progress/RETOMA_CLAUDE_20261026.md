# Retoma de los pedidos que Claude no llegó a procesar — 26/10/2026

Fuente: mensajes del usuario posteriores al límite de sesión de Claude, sesión
`b6535195-3ee4-4426-8bf8-b2fdcdd1f67d`. El usuario delegó decisiones y pidió workers.
Se conserva todo WIP. El usuario autorizó explícitamente el 26/10 publicar la **2.0 stable** después de terminar
todos los pendientes, depurar en general y obtener resultados correctos en las comprobaciones. No volver a pedir
esa autorización; no publicar mientras haya fallos sin resolver.

## Plan y verificaciones

- [x] Recuperar el traspaso de main y los mensajes realmente pendientes; separar notas antiguas de bugs nuevos.
- [x] Diagnóstico Android: APK sin imagen US; evitar resultados falsos del arnés (#239, `065f10ba4`).
- [x] Empaquetado Linux desde cero incluye host64 + SDL + tres imágenes (#240, `0419243fb`).
- [x] Linux pantalla completa ya resuelto (#227); no duplicar.
- [x] Entregar en Atlas título HD con textos, hija `ed3cd58de3e54bde89c268abbc8356b1`, sin inventar aprobación.
- [x] Controles: restablecimiento de bindings, captura e input; Nintendo vuelve a automático (#241, `3ab48f7a2`).
- [x] Gatillos con histéresis y tooltip L2/R2 en logros; letra centrada en botón GBA (#243, `24a57d9a1`).
- [ ] Galería: escenas normales y HD reproducen desde título/partida, vuelven a lista/selección, miniatura tipo
  cartel separada del nombre; checkpoints y videos liberan recursos al acabar/cancelar/fallar.
- [x] Launcher: bandera japonesa en Undub; misma portada de Steam también como hero superior (#242, `df028860e`).
- [x] Presentación: Brillo solo en GBA original o con opción experimental; deshabilitar L/R a la vez que el botón.
- [x] Corregir texto fuera de cajas, centrar el aviso GBA, completar franja inferior HD y fundidos de Opciones.
- [x] Terminar infografía de estados rápidos que Claude dejó en WIP; texto claro sobre logros.
  Estas tres entregas van en #244; pruebas en `TITLE_MENU_CLOSURE_20261026.md`.
- [ ] Integrar PR pequeños verificados; completar gates y paquete limpio desde el commit de cierre.

## Decisiones

- Se retoma la funcionalidad existente y se mantienen sus preferencias; no se rediseña la estética.
- El brillo simula la pantalla física de GBA: en modos modernos queda neutro y sin atajo. La opción experimental
  explica su finalidad y vuelve a habilitar botón y atajos; modo GBA original conserva ambos.
- Previews de galería son capturas estáticas y caché acotada; no reproducir ni decodificar videos al pasar filas.
- Las pruebas largas previas pertenecen a su SHA, no a la versión final. El runner vivo se conserva.
- Un APK incompleto es error de entrada, no evidencia de un combate colgado. Repetir el caso con app completa.
- El WIP de `v20-wt` se conserva intacto; cualquier rescate se hace como cambio explícito y probado en otra rama.
