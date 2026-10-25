## Regla vigente de continuidad — usuario, 18/10/2026

**Claude y Codex/ChatGPT pueden continuar commiteando y/o pusheando si el otro
no está trabajando activamente.** Diez minutos sin commits nuevos ni movimiento
verificable del otro se consideran inactividad; desde ese momento pueden tomar
también sus pendientes. La autorización es recíproca y no requiere pedirla otra vez.

Antes de tomar un bloque, revisar commits, cambios/fechas de archivos y procesos
de build/pruebas del checkout correspondiente: una compilación en curso cuenta
como actividad aunque no produzca commits. Registrar hora, evidencia y nuevo
responsable en PENDIENTES_VIGENTES.md antes de editar. Conservar trabajo sin
commitear y WIP; no publicarlo como terminado ni duplicar una implementación.
Al regresar el otro, debe leer el reparto actualizado y continuar lo que esté
libre. La regla permite avanzar sin esperar indefinidamente a un agente apagado.

## Encargo inmediato del usuario — prerelease y táctil (18/10/2026)

**Responsable de este bloque: Codex, este chat.** El usuario solicita integrar
los commits terminados de Claude, push y prerelease de lo acumulado, incluyendo
PS5; añadir Ocultar controles táctiles en Experimental para Android/Web.
Esto transfiere a Codex la preparación/publicación DE ESTA prerelease (41–42),
sin reasignar implementación de guardados/savestates de Claude ni tareas de PS5.
Guardados de Claude e9f053595 integrados como aa8ef4434. No rehacerlos.
Publicada: v1.9.3-nightly.20261018.3, siete paquetes y Pages actualizados.
Informe: NIGHTLY3_PUBLICACION.md. 41, 42 y 45 cerrados. Arte sigue al final.

## Corrección vigente: no duplicar responsables

Claude ya terminó guardados en **e9f053595**, checkout khcom-claude-int.
Claude conserva implementación de guardados/savestates. Codex retiró
su borrador duplicado y toma sus tareas cortas 25–26, 28–29, 36–37 y 40.
Extensiones de arte 12–24 quedan al final de prioridad.
Antes de empezar comprobar propietario y commits de su checkout, no solo main.

# Repartija para Claude y Codex — cola única

**Primero terminar/recoger los workers existentes; después seguir la lista.**
No abrir nuevos frentes de workers durante el cierre de esta tanda.

La lista larga vigente, con 48 puntos, responsables y estados, es
[PENDIENTES_VIGENTES.md](../PENDIENTES_VIGENTES.md). Sustituye repartos anteriores.
No mantener otra cola de pendientes aquí.
Cada tarea nueva o retomada registra lógica, pruebas y criterio de cierre en
LOGICA_PENDIENTES_20261018.md o en su documento técnico enlazado. Editor/cheats
30/47 y mando Xbox 48 ya tienen implementación de Claude en main; sus reglas y
alcance de verificación están en SAVE_EDITOR_LOGICA.md y CONTROLADOR_LOGICA.md.
Límite 999 confirmado por el usuario; conservar formato original.

## Encargo de Claude

Puntos 3, 5–11, 30–35, 38–39 y 43–44: recoger sus workers; guardados,
persistencia y último slot; savestate de cierre superior; checkerboard slot3;
cola de funciones/plataformas. 41–42 de esta prerelease fueron transferidos
a Codex y ya publicados; los bugs pendientes siguen identificados en la cola.
Las funciones de largo plazo no se presentan como terminadas por esta entrega.

## Encargo de Codex

Puntos 1–2, 4, 12–29, 36–37 y 40: consolidar cierres, verificar el bug de
sincorazones, escenas amplias, jefes/arenas, issues/RetroAchievements,
menús y revisión Web/audio. #28 ya cerró 36/36 rutas de submenú; lógica y evidencia
en LOGICA_PENDIENTES_20261018.md. Escenas amplias ya en main 0cb04a8dd;
no rehacerlas ni adjudicarles aprobación visual definitiva.

## Cierres reales

- Este chat no tiene subagentes activos.
- «Revisar notas de Claude»: cierre confirmado en a1d4147e2, sin workers
  propios pendientes. Darkside guardado incompleto como 6e7400189.
- Chat del bug de sincorazones: solicitud de cierre enviada, aún sin informe.
- CIERRE_WORKERS.md recoge resultados; PENDIENTES_VIGENTES.md recoge la cola.

Capturas de escenas: khcom-wide-review/build/wide-runtime-review (061),
wide-runtime-beach-final (057), wide-runtime-monstro-final (074).
1/44 escenas aprobada definitiva: 004 León; 057/061/074 mejorables.

## PS5 cerrado e incluido en futuras releases — 18/10/2026

El usuario confirma que el worker terminó el bloque PS5. PR #15 fusionado e
integración de build/release ya en main (13fba3337, 66f4e8c54). Punto 38 cerrado.
**Claude/publicación:** incluir PS5 en futuras entregas mediante PPSA98511.zip,
compilado desde el main/versionado de esa release en el host Linux/VM preparado.
El build maestro ya tiene --platform ps5. Conservar etiqueta alpha y separar
pruebas físicas pendientes de la implementación entregada. No rehacer el port.
