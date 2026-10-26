# Punto de reanudación: 26/10

Detención solicitada por el usuario después de enviar Maléfica y atender el comentario de moguri. No continuar automáticamente la cola ni crear tareas programadas.

- PR228, Comprar/Canjear aprobados: integrado en main,5b8d91148e05948acbc69c1d0bcf18da461fb5df.
- PR229, rama codex/atlas-maleficent-continuity: tres adaptaciones nativas pendientes en Atlas306aea394d144032932cf1595bf4cbdd,57768c3432cb475d9e2f67887c8bbaf6,2ca1f374795c4c4f9c78ec82f553fe2d. Conserva originales;41 estados offline, paleta16, borde idéntico; EU y dos capturas runtime pasan. Receta y fuentes en port/art/proposals/maleficent-continuity/README.md de esa rama. No confundir esta adaptación pendiente con los estudios aprobados PR222.
- Canjear revisión2, rama codex/atlas-moguri-tab-fix: respuesta a nube69f4acf0, cuatro fases y receta en ATLAS_MOGURI_SUBMENUS.md. Consultar la valoración de la hija antes de integrar.

Al retomar, mirar aprobados nuevos primero y enviar/integrar su PR, con reserva de integrador. No repetir la navegación manual: usar atlas_moguri_replay.py y atlas_moguri_tabs_replay.py. Las recetas usan copias aisladas del guardado y no son snapshots RAM. Para texto/selector aplicar la máscara bidimensional vigente en LOGICA_APROBADOS, sin volver al viejo criterio de conteo constante de glifos.

El resto de Atlas sigue pendiente, incluido el castillo095 y las cifras START. No se declara terminada toda la cola.
