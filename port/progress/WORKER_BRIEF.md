# Brief del worker de arte (leer solo esto para empezar)

Ronda 1 (24/10): tres workers tardaron ~25 min en prepararse y casi no entregaron. Este flujo quita esa preparación.

1. **Preparar (una vez; se reutiliza):** `python port/tools/worker_setup.py <nombre>` deja
   `D:/01Claude/ReKHCoM-build/wt-<nombre>` en `claude/worker-<nombre>` desde origin/main, con ROMs y el juego EU
   compilado. Imprime la línea de PATH (ninja). No hace falta leer EMPEZAR_AQUI ni configurar el toolchain.
2. **Tomar UNA pieza:** `python port/tools/art_queue.py --quiet next --owner <nombre>` (reintenta solo).
3. **Brief de la pieza:** `python port/tools/art_piece.py <id>`: nubes con texto, caja y recorte, comentario general,
   imagen revisada, qué código dibuja esa escena y los comandos de captura y respuesta. Las reglas están en
   LOGICA_APROBADOS.md; leer solo la sección de esa escena o familia.
4. **Cambiar, capturar una vez** (EU; la resolución del brief), mirar la imagen uno mismo.
5. **Responder en la app directo (sin PR):** `import_image.py <png> --parent <id> --revision-info <json>` con el
   `cloud_id` de cada nube atendida. `answer_scene.py` marca TODAS las nubes: solo si se atendieron todas.
   Si el cambio necesita código aún no integrado, el texto de la hija dice «PENDIENTE DE PR #N».
6. **Primero la imagen a la app, después el PR** (ronda 2: 112/120 quedaron en un PR sin imagen y al usuario le llegaron 2 de 3).
   `update --status pr_abierto` ahora exige `--sent <id de la hija>` (o `--no-image` si la pieza no tiene imagen). Enviar apenas
   esté lista, sin esperar al resto de las piezas.
   **Código por PR, chico, por pieza:** commit en la rama del worker, `gh pr create --head <rama>` (el repo por
   defecto ya es MatyMeatBoy/ReKHCoM), `art_queue.py --quiet update <id> --owner <nombre> --status pr_abierto --pr URL`.
   Si mejora una regla, el PR toca también LOGICA_APROBADOS.md. Nunca merge ni push a main: integra el coordinador.
7. **No se puede terminar** (decisión visual grande, comentario ambiguo): `art_queue.py --quiet release <id>
   --owner <nombre> --evidence "qué falta decidir"` y pasar a la siguiente con `next`.
8. Repetir 2-7 hasta que `next` diga «No pending piece». Informe final: piezas, ids de las hijas, PRs, y
   FRICCIÓN (qué costó y por qué), para mejorar este brief.

Siempre: sin ventanas ni sonido (`SDL_VIDEO_DRIVER=dummy`, `SDL_AUDIO_DRIVER=dummy`), nada de baterías ni
repeticiones US/JP salvo que la pieza lo pida, no tocar `jugar/`, `ReKHCoM-vX` ni QuiverLauncher.


Fricción conocida: en bash las rutas van como `/d/01Claude/...`, no `D:/...`. Un worker que muere por límite de sesión deja la pieza
`en_curso`: el coordinador la libera con `release <id> --owner <el que la tenía>` y la relanza.
`art_piece.py` lee la mesa de `$ART_DESK` (desde un worktree en D: exportarla, p. ej. `/d/01Claude/ReKHCoM-build/w3-desk`). `prelim_shot.sh` usa `build/pc/`; si tu build está en `build/pc-eu/`, copia el script con `sed`. `next` ya no te devuelve una pieza que tú liberaste.
