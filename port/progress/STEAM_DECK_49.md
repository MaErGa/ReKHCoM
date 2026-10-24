# Steam Deck: issue 49, entrada nativa y Proton

24/10/2026. Carril issue-049, owner art_110, rama
codex/issue-049-steam-native, base origin/main aedffc4c8. Solo PR; el integrador
único integra. No se modifica configuración de Steam ni se publica release.

## Fallos y alcance

[Issue 49](https://github.com/MatyMeatBoy/ReKHCoM/issues/49): el usuario abre
ReKHCoM.sh en Desktop Mode; Add to Steam lo rechaza como Unsupported file
type y el acceso creado desde Steam no inicia. No hay log adjunto que permita
atribuir el segundo síntoma a un fallo de Proton.

Defecto localizado en el paquete: build_linux.py generaba un .desktop con
Exec=sh ReKHCoM.sh, Icon relativo y sin Path. Ese comando depende del cwd del
lanzador y falla si parte desde otra carpeta. La guía además exigía escoger
el .sh que el reporte dice que su integración de Steam no acepta. El wrapper
ya hacía cd al directorio propio y pasaba --data-dir; no se atribuye ese arreglo
preexistente a este PR.

Solución: ReKHCoM.sh --install-desktop registra una aplicación XDG con
Exec=/bin/sh, script absoluto, Path e Icon absolutos, y escapes Desktop Entry
para espacios, %, dólar, comillas, backticks y barras. Se puede instalar sin
ROM ni permisos ejecutables en el script. No se escribe shortcuts.vdf ni se
retocan accesos existentes. --steam-help imprime los campos para reparar un
acceso manual con /bin/sh como target y script absoluto entre comillas como
primer argumento. Al mover la carpeta se vuelve a instalar y se actualiza
el acceso de Steam expresamente. PC Linux y Steam Deck comparten el wrapper;
el builder ya no distribuye el .desktop relativo inválido.

Clases relacionadas cubiertas: directorio de inicio ajeno, rutas con espacios
y metacaracteres, app instalada antes de copiar ROM, argumentos que deben
mantenerse separados y dispatch regional por cabecera. --region inválida o sin
valor falla con uso en vez de intentar un nombre de ejecutable arbitrario.
Los logs siguen junto al juego y ahora identifican explícitamente Linux nativo.

## Relación con el reporte Proton anterior

Valve describe [Proton](https://github.com/ValveSoftware/Proton) como herramienta
para los juegos de Windows. Este acceso llama al shell Linux y al ejecutable
ELF nativo con su intérprete y bibliotecas de 32 bits empaquetados. Forzar
Proton sobre ese acceso es una configuración incorrecta; se documenta desactivar
la compatibilidad forzada. La versión Windows usa ReKHCoM.exe y sí puede usar
Proton. No hay evidencia para declarar que el reporte antiguo de requerir
Experimental tenga esta misma causa. Se conservan como diagnósticos diferentes:
para Windows hacen falta kh_com.log y versión de Proton; para Linux los dos
logs, campos del acceso y versión de SteamOS. No se cambia el motor de Windows.

## Fuentes primarias

- [Steam Desktop FAQ](https://help.steampowered.com/en/faqs/view/671A-4453-E8D2-323C):
  añadir desde las aplicaciones instaladas en Steam Desktop y abrir después en
  Gaming Mode. No garantiza aceptar cualquier tipo de archivo .sh.
- [Desktop Entry: Exec](https://specifications.freedesktop.org/desktop-entry/latest/exec-variables.html):
  reglas de doble escape y %% literal; la sintaxis no es la de un comando shell.
- [Desktop Entry: claves](https://specifications.freedesktop.org/desktop-entry/latest/recognized-keys.html):
  Path establece el directorio de trabajo de la aplicación; Icon puede ser ruta absoluta.

## Validación

Pruebas dirigidas sin ROM real: port/tools/test_linux_launcher.py. Ejecutan el
wrapper real con stubs de los juegos y cabeceras artificiales, verificando cwd,
--data-dir, bibliotecas, argumentos y código de salida. El caso de instalación
usa GLib gio launch y desktop-file-validate si están disponibles para probar
el parser real del .desktop con comillas y barras adicionales en Linux.

La prueba de shell no acredita Steam Deck real, Gamescope, sonido ni la GUI
de Steam. La evidencia de hardware llega como reporte del usuario.

Resultado final: **4/4** en Git sh de Windows y **4/4** en Debian 12 x86_64
(QEMU, 1 CPU/768 MiB, /bin/sh=dash), incluida ejecución por GLib gio y
validación desktop-file-validate. Metacaracteres probados en Linux: espacios,
apóstrofo, dólar, backticks, %f, comillas y barra inversa. Cabeceras artificiales
EU/US/JP despachan al stub correcto; sin ROM se conserva el aviso y el flujo EU.
python -m py_compile, sh -n, check_sources.py y git diff --check pasan.
Evidencia privada local: build/issue049-vm-final/issue049-final.log; sin ROM,
sin ejecución de Steam, sin publicación de paquetes ni comentarios en el issue.
