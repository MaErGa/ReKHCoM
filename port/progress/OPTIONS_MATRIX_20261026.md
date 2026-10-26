# Auditoría de opciones y controles

`test_options_matrix.py` recorre HD, GBA mejorado y GBA original con Sora/Riku y con/sin mod. Cada caso usa una
carpeta nueva, configuración nueva y partida vacía. El mod se enlaza temporalmente; nunca se modifica. `--out`
permite conservar evidencia en otro disco. Un solo proceso y un trabajador PPU por caso.

- Descubre las filas del menú real mediante `KHCOM_OPTIONS_AUDIT`; una fila visible desconocida falla la prueba.
- Acciona las opciones visibles mediante el mismo `Change()` de la interfaz y comprueba persistencia; una opción
  deshabilitada debe conservar su valor. Guarda también qué filas estaban ocultas o deshabilitadas.
- Entra en Controles, Logros, Escenas, Revanchas y Trucos; cancela los avisos de preset, título y salida.
- Acepta/cancela la fuente, confirma un preset, confirma resolución, recorre las opciones de Trucos (incluido
  el botón experimental de brillo), confirma estados rápidos y verifica que el banco no cambia durante partida.
- Desde título cambia al banco 2 y regresa al 1, conservando ajustes distintos en ambos.
- Las aserciones de resolución cubren la ruta del menú con renderer dummy; no certifican un monitor físico.
  La apertura de Revanchas no certifica las batallas, y las acciones destructivas de progreso de Trucos tienen
  sus pruebas aparte. Esta matriz no atribuye cobertura a filas que la versión/plataforma oculta.

`test_ui_input.py` añade captura real SDL de las 13 asignaciones de teclado y las 13 de mando. Comparte una copia
inmutable del ejecutable por barrido mediante `case_bin`, para no copiarlo por cada caso. Sus pruebas de transición
rápida exigen 100 pares de aperturas/cierres reales tanto en campo como durante un evento, no solo 100 intentos.

Evidencia inicial: 120/120 casos de la matriz principal pasaron con el ejecutable EU de #248. Las extensiones se
ejecutaron después sobre main `204367b2a` (#244 + #248). Detectaron una excepción nativa al cambiar GBA original a
mejorado con mod presente: el handler BIOS interceptaba lecturas de una DLL. La prueba conserva esa señal como
fallo; no se permite concluir la auditoría hasta repetirla con el arreglo y el ejecutable final de integración.

Ejemplo reproducible (ROM y mod propios):

```powershell
python port/tools/test_options_matrix.py --build build/pc-eu --hd D:/mod-importado --roms roms --out D:/qa/options-final
python port/tools/test_ui_input.py --build build/pc-eu --cycles 100 --out D:/qa/input-final
```

Los reportes incluyen SHA256 del ejecutable, logs, INI y resultados por caso. La evidencia de esta sesión está en
`D:/01Claude/ReKHCoM-build/battery-evidence-20261026/`; los BMP de la primera matriz se convirtieron a PNG sin pérdida
para recuperar espacio en C. Pendiente: barrido final unificado y repetición de las rutas afectadas en US/JP.
