# Jafar: parpadeo y copia del genio — 25/10/2026

Solicitud: al mover el escenario alterna un recuadro negro con la vista amplia.
Conservar el efecto de extensión/reflejo del escenario, excluyendo a Jafar.

## Diagnóstico y corrección

- `BosJfDrawPillars` y `BosJfMajinUpdateBgClip` reenlazan buffers mutables con
  `SetBgMapBlocks` después del scroll. La función ponía el origen en cero aunque
  VRAM todavía mostraba la ventana desplazada. En PC, el rebind de estas capas
  exactas conserva su origen y mantiene el redraw pendiente.
- El buffer puede contener la próxima pose de pilares/genio mientras VRAM conserva
  la anterior. Las capas exactas de Jafar se reconocen por identidad, tamaño y modo,
  sin depender de una comparación heurística de nueve tiles que falla en ese intervalo.
- La copia del genio se ancla a (768,256), según las constantes del scroll de
  `bos_jf_majin.c`. Escoger la copia más cercana al centro de la cámara podía
  desplazar al jefe 512 píxeles al cruzar el punto medio. Solo se modifica el margen.
- Buscar parientes encontró el mismo rebind tardío en la capa del genio: ambas capas
  quedan cubiertas. Otras arenas conservan su validación habitual.

## Evidencia

- `test_jafar_view.py`: arnés determinista, movimiento lateral, log de cada cuadro,
  aserción de sala/vista; un hilo PPU y prioridad baja en Windows.
- Pre12: 40 cuadros activos pierden BG0 en 1800; reproducción breve: cuadros
  513,515,...,539 (14 fallos en 461 cuadros activos).
- Arreglo a 1920×1080: 1800 cuadros; 1701 activos, cero pérdidas de sala/vista.
- Arreglo a 3840×1080: 560 cuadros; 461 activos, cero pérdidas; capturas inspeccionadas.
- Centro GBA 240×160 (escalado ×3): idéntico píxel por píxel antes/después en
  240,500,900,1200,1500. `check_sources.py` y `git diff --check`: OK.
- Atlas, hija técnica del parent `53e790683ac740ab8df504a7bb8406f3`:
  `e182f7052b8348b2a88b4f2e996a0ed7`; frame 513 antes/después, sin modificar valoraciones.
- Copia EU independiente: `../pruebas/ReKHCoM-jafar-blink-20261025`, con
  `Probar Jaffar corregido.bat` y `Jugar normal.bat`, copia de save/config pre12 y
  enlaces a ROM/assets/HD locales. No se inicia ni reemplaza la partida activa.

## Traspaso

Rama `codex/jafar-blink`. La reserva online `agrabah-jafar-arena` pertenece a
`codex-jafar`. Este arreglo no rediseña el patrón inferior: ese trabajo de arte
sigue pendiente. No afirmar que resuelve el crash de carta Android #83.
Sin builds Android/Linux ni baterías grandes por pedido de cuidar RAM mientras
el usuario juega Deadlock. Todas las pruebas de esta entrega terminadas.
La publicación pre13/PR #90 y los demás pendientes del handoff anterior no se
modifican con este arreglo; no se publicó una release.
