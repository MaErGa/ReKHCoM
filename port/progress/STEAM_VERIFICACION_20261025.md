# Añadir a Steam — diagnóstico 25/10/2026

La sospecha del traspaso anterior sobre la lambda `value()` no se confirma:
la expresión obtiene correctamente la tercera y cuarta comilla de la línea.
`SteamUserConfigs` selecciona la cuenta activa de esta instalación en una
lectura real, sin modificar `loginusers.vdf` ni `shortcuts.vdf`.

`python port/tools/test_steam_accounts.py` compila la función original del
launcher en un ejecutable pequeño y prueba cuatro instalaciones ficticias:
MostRecent prioritario, timestamp más reciente, marcador anterior al timestamp
y fallback sin loginusers (excluye userdata/0). Los cuatro casos pasan.
La comprobación opcional de la instalación real compara con ActiveUser del
registro y no imprime nombres ni identificadores reales de cuenta.

El handoff ya confirma que existe una entrada ReKHCoM válida para la cuenta
activa. Steam recarga la biblioteca al iniciarse; no se reinicia ni se cierra
Steam mientras el usuario juega Deadlock. No se afirma que una biblioteca
abierta se haya refrescado. No hay cambio funcional al parser basado en una
hipótesis refutada; queda una prueba reproducible para evitar esa regresión.
