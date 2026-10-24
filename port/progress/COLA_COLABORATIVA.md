# Cola colaborativa de Claude y Codex

Pedido del usuario del 24/10/2026: varios carriles para sacar la cola online, marcar de inmediato qué arte se
tomó, entregar cada pieza por pull request e integrar solo si nadie más está integrando. Esta regla sustituye
el push directo a main, los repartos personales antiguos y la recuperación automática tras 10 minutos.

## Fuentes y autoridad

- **Comentarios y aprobaciones:** app de revisión, `build/art-review/revisiones.sqlite3`, su `cola.json` y
  `COLA_PARA_AGENTES.md`. En esta máquina la mesa compartida vive en `khcom-codex/build/art-review`, junto a los
  otros worktrees. No crear una mesa vacía en cada checkout ni editar sus estados humanos para cerrar trabajo.
- **Tomas y estado entre máquinas:** rama remota `coordination/art-queue`, archivos `queue.json` y
  `PENDIENTES.md`, operados por `port/tools/art_queue.py`. Esta es la reserva online compartida; un archivo
  local o una nota de chat no basta para reservar. Sus pushes usan comparación con el SHA remoto para que dos
  tomas simultáneas no se pisen.
- **Índice legible y pendientes técnicos:** `PENDIENTES_VIGENTES.md`. Su tabla es una fotografía; antes de actuar
  refrescar la rama de coordinación y la mesa. Registrar avances de inmediato con la herramienta y añadir
  evidencia/traspaso en el PR. No usar un contador histórico como estado actual.
- **Criterios aprendidos:** `LOGICA_APROBADOS.md` y `WORKFLOW_ARTE_AMPLIO.md`. Una valoración pertenece a su versión;
  una hija no hereda la aprobación de su madre. El arte nuevo se activa para jugadores desde `build_version`
  con honores. Aprobado sin ★ espera al usuario; una entrega puede subir por PR como preliminar apagada.

## Carriles y secuencia

Mantener varios carriles mientras existan tareas independientes: escenas, jefes/arena, herramientas/documentación
y verificación. Cada carril toma una pieza concreta y trabaja en rama/worktree propio desde `origin/main`.
Dos agentes pueden tocar un archivo común en árboles separados, pero el integrador aplica sus PR de uno en uno
y vuelve a comprobar la composición. No tocar WIP, ejecutables ni partidas del otro.

1. Leer la cola viva y la reserva remota. Elegir una pieza pendiente y tomarla **antes de editar o generar arte**.
   El registro debe identificar dueño, pieza y referencia de la revisión. Si la toma falla por otro dueño o
   conflicto remoto, volver a listar y elegir otra; no empezar esa pieza.
2. Registrar rama, contexto y envío al comenzar. Leer todas las nubes, comentario general y trazos del envío;
   conservar los IDs de imagen, familia y aprobación. No tratar una revisión ya respondida como cierre si
   llegó un comentario nuevo.
3. Trabajar y verificar la pieza en su árbol. Para arte: captura exacta, rectángulo GBA idéntico, márgenes
   distintos donde corresponde, varios anchos/fotogramas/eventos y revisión visual a x3. Si se activa para
   jugadores, probar también el camino normal sin variable preliminar (`test_preliminar_v1.py --approved`
   para escenas compatibles). No afirmar pruebas que no se corrieron.
4. Responder en la mesa con versión hija (`import_image.py --parent <image_id>`) y ficha que responda el envío.
   Registrar `en_revision` si depende del usuario. Mantener la aprobación anterior y registrar lo nuevo como
   preliminar hasta que llegue su valoración; publicar una hija no equivale a habilitarla para jugadores.
5. Commitear solo la pieza, pushear su rama y abrir **un PR por entrega** contra `main` con problema, cambio,
   referencia humana, pruebas y limitaciones reales. Registrar de inmediato `pr_abierto`, URL y evidencia.
   Entregas de documentación/herramientas también van por PR. No acumular un lote de aprobados sin subir.
   Cada PR de arte incluye su mejora de `LOGICA_APROBADOS.md`: el criterio de las valoraciones nuevas y la
   corrección de la regla de toda pieza aprobada que recibió comentarios después (usuario 24/10).
6. Si hay PR verificados y no hay integrador, tomar su reserva remota. Solo quien obtuvo el token integra.
   Si otro ya integra, seguir otro carril. Refrescar `main`, comprobar que el PR sigue vigente y sin conflicto,
   revisar aprobación exacta y pruebas; fusionar **un PR a la vez**. Actualizar/validar la rama cuando un PR
   anterior afecte la misma zona. Nunca forzar `main` ni saltarse un conflicto.
7. Registrar `integrado` con PR y SHA real de merge, refrescar el índice y liberar la reserva de integrador.
   `pr_abierto` no significa integrado, y merge no significa release publicado. No publicar releases sin OK.

## Herramienta compartida

Consultar `python port/tools/art_queue.py --help` y la ayuda del subcomando para los argumentos disponibles.
La herramienta se integra por su propio PR; si todavía no está en tu base, actualizar desde main tras ese
merge. Hasta disponer de ella y de la rama compartida, coordinar las tomas con el integrador actual; no
simular una reserva exitosa ni iniciar otro integrador por una nota local.

Ejemplo (IDs y nombres son ilustrativos; listar antes para usar los reales):

```text
python port/tools/art_queue.py list
python port/tools/art_queue.py claim escena-112 --owner claude-escenas
python port/tools/art_queue.py update escena-112 --owner claude-escenas --status en_curso --branch claude/escena-112 --reference <imagen-y-envio>
python port/tools/art_queue.py update escena-112 --owner claude-escenas --status en_revision --evidence <hija-y-pruebas>
python port/tools/art_queue.py update escena-112 --owner claude-escenas --status listo_pr --approval <honores-exactos> --evidence <pruebas-del-camino-normal>
python port/tools/art_queue.py update escena-112 --owner claude-escenas --status pr_abierto --pr <URL>
python port/tools/art_queue.py integrator-acquire --owner codex-integrador
```

`integrator-acquire` devuelve `state.integrator.token`; conservarlo para renovar, cerrar o liberar. No usar
un token ajeno. Con la reserva obtenida y después de fusionar el PR:

```text
python port/tools/art_queue.py integrator-touch --owner codex-integrador --token <token>
python port/tools/art_queue.py update escena-112 --owner claude-escenas --status integrado --pr <URL> --evidence <SHA-merge-y-pruebas> --token <token> --integrator-owner codex-integrador
python port/tools/art_queue.py integrator-release --owner codex-integrador --token <token>
```

`add ID --piece <nombre> [--approval <ref> --reference <ref>]` agrega trabajo faltante; `claim` debe completarse
antes de editar. `--repo`, `--remote` y `--snapshot` permiten indicar repositorio, remoto y copia local del
estado; `init` prepara la coordinación. Inicializar una sola vez y conservar después el estado existente.
No editar ni pushear manualmente la rama de coordinación; usar la herramienta para preservar la exclusión.

## Estados y relevo

| Estado | Significado y salida |
|---|---|
| `pendiente` | Disponible para tomar. |
| `en_curso` | Dueño trabajando; rama y referencias registradas inmediatamente. |
| `en_revision` | Hija/ficha entregada, espera valoración o revisión; no inventar ★. |
| `listo_pr` | Entrega verificada lista para abrir PR; no demorar su publicación. |
| `pr_abierto` | PR registrado; espera revisión e integración exclusiva. |
| `integrado` | PR fusionado; registrar el SHA comprobado. |
| `bloqueado` | Motivo concreto y siguiente acción/dato necesarios; elegir otro carril independiente. |

Una reserva no se roba por tiempo sin commits. Si un agente se apaga, comprobar sus procesos, rama, PR y WIP;
registrar último avance y evidencia del corte. Conservar el trabajo y acordar/relevar la reserva con su dueño
o con quien coordina la sesión usando las operaciones soportadas por la herramienta. No borrar `queue.json`
ni retirar un lock remoto para resolver un conflicto. Un proceso sin respuesta no habilita dos integradores.

Antes de parar, dejar dueño/estado, rama y último SHA, imagen/envío, PR, pruebas, bloqueos y siguiente acción.
Si aún se integra, terminar el merge en curso o dejar el PR abierto y liberar correctamente el lock; no dejar
una nota que diga "libre" mientras la reserva remota sigue tomada.

## Aprendizaje que ya quedó aceptado

Aplicar los criterios documentados de 044/045/053/054/098/104 y Darkside 03 r6 en `LOGICA_APROBADOS.md`: estructuras
en su propia diagonal, donantes completos, motivos únicos sin duplicación, bordes continuos y GBA intacta.
Anotar un criterio nuevo solo después de una valoración humana y enlazar la versión exacta. Las nubes nuevas
de 074/087/090/095/099/102 siguen pendientes aunque sus entregas anteriores estén respondidas.
