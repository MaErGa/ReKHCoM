# Cola online para Claude y Codex

`art_queue.py` requiere Python 3 y Git, sin dependencias Python. Lee el remoto
del repositorio de trabajo; mantiene objetos en un repositorio bare temporal o
en `--state-dir DIR`. No cambia checkout, índice ni rama del trabajo.
Cada invocación debe tener su propio state-dir si corre simultáneamente.

La fuente compartida es `coordination/art-queue`, con `queue.json` legible y
`PENDIENTES.md`, publicados juntos en cada operación. No se escribe en main.
Cada push usa compare-and-swap con `--force-with-lease` y el SHA exacto leído;
si otro carril escribió antes, falla sin publicar sus cambios. Leer y reintentar
la operación completa después de revisar el estado. No usar `--force` manual.

## Inicio y flujo

Todos los flags globales van antes del subcomando. Desde la raíz del repo:

```powershell
python port/tools/art_queue.py init
python port/tools/art_queue.py add escena-054 --piece "Escena 054" --approval "Referencia de aprobación del usuario" --reference "Ruta/review"
python port/tools/art_queue.py --snapshot port/PENDIENTES_COLA_ONLINE.md list
python port/tools/art_queue.py claim escena-054 --owner claude-carril-1
python port/tools/art_queue.py update escena-054 --owner claude-carril-1 --status en_revision --branch arte/escena-054 --evidence "Revisión visual preparada"
python port/tools/art_queue.py update escena-054 --owner claude-carril-1 --status listo_pr --evidence "Validación terminada"
python port/tools/art_queue.py update escena-054 --owner claude-carril-1 --status pr_abierto --pr https://github.com/OWNER/REPO/pull/123
```

`init` solo crea la rama si no existe. Añadir piezas pendientes no implica su
aprobación: registrar la referencia antes de `listo_pr`. Tomar con `claim` y
comprobar `ok: true` **antes de editar una pieza**. No iniciar trabajo si falla.
El claim no permite volver a tomar una tarea ya tomada, aunque sea del mismo
owner. Elegir owners estables y únicos por carril. El estado no caduca.

Estados: `pendiente → en_curso → en_revision → listo_pr → pr_abierto → integrado`.
Se permite pasar de en_curso a listo_pr y volver de revisión/PR a en_curso.
Se puede marcar `bloqueado` y reanudar como en_curso por el mismo owner.
Toda actualización de tarea exige su owner. Cada tarea conserva timestamps,
rama, PR, referencia de aprobación y evidencia acumulada. No hay borrado,
reclaim automático ni reasignación implícita por antigüedad: si un agente cae,
revisar su trabajo, PR y sesión antes de acordar recuperación explícita.

## Un solo integrador

```powershell
python port/tools/art_queue.py integrator-acquire --owner codex-integrador
# Guardar state.integrator.token de la respuesta JSON como TOKEN.
python port/tools/art_queue.py integrator-touch --owner codex-integrador --token TOKEN
# Con el lock vigente: revisar aprobación/PR, actualizar main, integrar PR y verificar.
python port/tools/art_queue.py update escena-054 --owner claude-carril-1 --status integrado --integrator-owner codex-integrador --token TOKEN --evidence "PR #123 merged; commit SHA"
python port/tools/art_queue.py integrator-release --owner codex-integrador --token TOKEN
```

El integrador puede registrar el resultado usando el owner de la tarea y su
propio token. `integrado` exige PR, aprobación, lock vigente y evidencia. Este
comando registra el resultado; no ejecuta ni comprueba el merge en GitHub.
Quien integra debe verificarlo antes. Mantener el lock desde antes de actualizar
main hasta terminar la verificación, y liberarlo aun si se bloquea una pieza.
No se roba el lock por tiempo; touch actualiza su timestamp y solo el token
vigente permite touch/release. Un lock abandonado requiere recuperación explícita.

Después de verificar la sesión detenida, el worktree y los PR, el coordinador
puede recuperar sin editar JSON. No usar antigüedad como evidencia. Leer `list`,
conservar su `commit` exacto y registrar la autorización/handoff:

```powershell
python port/tools/art_queue.py recover-task escena-054 --from-owner claude-carril-1 --to-owner codex-carril-2 --actor coordinador --expected-commit SHA --evidence "Sesión detenida confirmada; PR y trabajo revisados; handoff autorizado en referencia X"
python port/tools/art_queue.py recover-integrator --from-owner integrador-anterior --actor coordinador --expected-commit SHA --evidence "Integración detenida confirmada y main/PR revisados; autorización referencia X"
```

La recuperación falla si cambió el commit observado o el dueño indicado. Deja
auditoría permanente con actor, evidencia, dueño anterior y SHA. La tarea vuelve
a en_curso conservando PR/evidencias; recuperar el integrador lo deja libre.

La exclusión funciona entre clientes que siguen este protocolo. Tener acceso
Git al remoto permite editar el estado por fuera: owners/tokens no reemplazan
permisos de GitHub y no protegen contra clientes maliciosos.

## Lectura y pruebas

stdout siempre es JSON (`ok`, `commit`, `state` o `error`); fallos salen con
código 1. El token viene en el estado para coordinación, no es una credencial
GitHub. `--snapshot PATH` escribe copia Markdown local inmediatamente después
del éxito. El snapshot autoritativo siempre acompaña queue.json en el commit.
Si falla solo la copia local tras publicar, `ok` sigue true con `snapshot_error`.

```powershell
python port/tools/art_queue.py --help
python port/tools/art_queue.py --remote C:/tmp/queue-remote.git --state-dir C:/tmp/agent-cache.git list
python -m unittest discover -s port/tools -p test_art_queue.py -v
```

Las pruebas usan exclusivamente repositorios bare locales y dos clones que
compiten por el mismo claim/lock: exactamente uno publica, el perdedor no
altera el estado; también prueban rechazo de release ajeno y flujo de PR.
