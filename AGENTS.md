# Retomar ReKHCoM y procesar la cola

Cuando el usuario diga «ponte al día y procesa la cola pendiente», empieza por
[EMPEZAR_AQUI.md](EMPEZAR_AQUI.md) y
[COLA_COLABORATIVA.md](port/progress/COLA_COLABORATIVA.md).

- Refresca `origin/main` antes de elegir trabajo. Conserva cualquier WIP ajeno;
  si el checkout está ocupado o atrasado, crea un worktree propio desde `origin/main`.
- Consulta `python port/tools/art_queue.py list`. La rama remota
  `coordination/art-queue` es la autoridad de tomas, responsables e integrador.
  La tabla de `PENDIENTES_VIGENTES.md` es una fotografía, no una reserva.
- Mantén varios carriles independientes, como pidió el usuario. Cada agente
  debe obtener un `claim` exitoso antes de editar su pieza. Un conflicto CAS
  exige refrescar y comprobar el dueño; nunca seguir como si la toma hubiera pasado.
- Registra en el momento rama, revisión humana, estado, pruebas y PR. Todos
  los cambios entran por PR; no pushear código directamente a main.
- Integra solo después de obtener el lock remoto de integrador. Si está
  ocupado, trabaja otro carril. Integra un PR a la vez, registra su SHA real
  y libera el lock al terminar; nadie lo roba por antigüedad.
- Para arte, consulta la mesa compartida y la aprobación de la versión exacta.
  Respeta la política vigente en `LOGICA_APROBADOS.md`; una hija no hereda
  aprobación. Conserva comentarios, trazos, imágenes y partidas.
- Habla en español, prueba en silencio y no publiques releases ni comentarios
  externos sin autorización. Deja traspaso antes de terminar.

Los comandos completos, recuperación explícita y pruebas están en
[ART_QUEUE.md](port/tools/ART_QUEUE.md). No iniciar otra cola local vacía.
