# Cierre v2.0 — reanudar aquí (26/10)

Pedido directo del usuario: implementar sin Atlas. Rama codex/hd-ui-v20-completion continúa claude/title-hd sin tocar su WIP ajeno. Las mejoras a aprobados de Atlas también se implementan directamente. Mantener RAM baja, pruebas silenciosas y compilación serial. No workers.

- [x] PR229, Maléfica nativa aprobada: main d1712a484b6eff18f89a261cd898198456d7fe0c;41 estados offline y GIF, dos capturas de combate. Comentarios de transición de color/animación conservados.
- [ ] Consultar y aplicar comentarios nuevos sobre los aprobados (incluido Canjear: revertir regresión del PR230 pendiente; leer texto exacto en API).
- [ ] Completar título y pantallas Nueva partida/Cargar/Sora/Riku/Brillo L-R con recursos HD. Claude dejó el primer botón, fondo y barra. Su árbol khcom-recom-art conserva dos herramientas sin commit; no sobrescribirlo.
- [ ] Todo el título y Opciones/settings HD solo con HUD HD; HD apagado conserva originales. Transiciones/fundidos y oclusión coherentes.
- [ ] Audio: barras de volumen y aviso de reinicio al seleccionar Undub, sin texto «reiniciar» en lista. Implementación inicial en port_ui.c; falta compilar/verificar y ajustar espacio.
- [ ] Galería: problema identificado, scenes_seen.dat solo registra escenas posteriores a la instalación; falta importar el progreso real de los saves anteriores sin desbloquear mundos no visitados. Esperar guardado Agrabah que el usuario pondrá en raíz y trabajar sobre copia.
- [ ] Steam: logo de biblioteca modificado sin «Re:» (zona naranja en captura1a25eb37) y banner horizontal con gameplay real de arena en x3 integer2. Reutilizar integración Steam existente, actualizar recursos empaquetados; no capturar un mockup.
- [ ] Pruebas dirigidas, issues163/202 con evidencia; nunca cerrar por el mero pedido si la causa sigue abierta.
- [ ] Compilar y empaquetar2.0 desde commit final: Windows/Linux/Deck, Android, PS4/Vita/PS5 y web; sin Switch según política vigente. Verificar release_check antes/después y actualización, fuentes core y mod necesarios. Usuario autorizó salida2.0.
- [ ] Con la entrega2.0, responder a duartepedro03 en issue163 con el arreglo de galería, enlace de versión y disculpa breve. También notas pendientes en140 y44, según PENDIENTES_VIGENTES. No enviar antes de disponer de entrega concreta.

Tomas remotas: hd-ui-v20-completion y gallery-save-progress, owner codex-atlas. Registrar rama, pruebas y PR al entregar; integrador remoto reservado para cada integración. La anterior detención pedida queda revocada por este nuevo encargo.
