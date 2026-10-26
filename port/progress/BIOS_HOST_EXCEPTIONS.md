# Excepciones BIOS y DLL del sistema (26/10)

El manejador Windows recuperaba accesos fuera del bus GBA incluso si la instrucción pertenecía a una DLL del
sistema. Redirigir sus registros puede alterar su recuperación de excepciones u ocultar un fallo real del host.
La procedencia ya se comprobaba para un intervalo de direcciones bajo, pero no para todo el espacio de memoria.

Ahora solo el ejecutable principal puede pedir recuperación BIOS o división por cero GBA. Se conserva todo su
código, incluido el port; no se limita al tramo del juego original. Un single-step pendiente se termina antes de
comprobar la procedencia, conservando la restauración de registros y flags. Linux no cambia.

`python port/tools/test_bios_host_guard.py` compila el manejador real y una DLL i686 de prueba. Antes falla la
primera aserción: una lectura de la DLL a `0x12181218` se acepta y modifica el contexto. Después verifica:

- Lecturas, escrituras y división por cero de la DLL continúan la búsqueda sin modificar registros.
- El ejecutable conserva la redirección del bus, la restauración del registro tras single-step y la división GBA.
- Una excepción real emitida dentro de la DLL llega a su manejador posterior; una lectura real del ejecutable
  sigue recuperándose como acceso al bus GBA.

La prueba es silenciosa y no abre ROM ni partida. Requiere el compilador Windows i686 habitual junto al workspace.
Los fixtures quedan en `build/bios-host-guard`.

La primera reproducción de Options con esta protección reveló además un fallo del harness: aplicaba un cambio
de resolución desde dentro de Draw y el presenter aún conservaba el canvas anterior. Ese arreglo se gestiona
en el harness de Options; esta corrección no cambia Present ni oculta ese fallo.
