# Moguri: submenús y punto de reanudación

Reproducir desde la raíz con `python port/tools/atlas_moguri_replay.py`. Receta, origen y SHA256 del guardado, controles y fotogramas en ATLAS_MOGURI_REPLAY.json. Requiere el binario EU de build/pc-atlas y los recursos locales indicados. Copia el guardado a build/atlas-moguri-after por pantalla: el original no se modifica. No es un savestate de memoria; vuelve de forma reproducible al modo exacto con KHCOM_MODE. Guarda log, configuración y dos capturas por pantalla (1420/1500).

Comprar y Canjear ahora se reconocen como menús completos. Sus márgenes continúan la fila de fondo BG del borde; no repiten cada cuatro píxeles el selector ni arrastran sprites. El margen izquierdo de Comprar conserva el rojo oscuro pedido por el usuario en la captura45d140e4. La cabecera superior y la banda inferior continúan en su altura original.

Validación: EU compilado con un proceso y límite768MB, capturas aisladas software1920x1080, español. No se han probado por esta entrega todos los idiomas ni aperturas de sobres/confirmaciones. La propuesta se envía a Atlas para revisión; no hereda la aprobación del menú principal.

## Evitar repetir los errores de Añadir/Quitar

Consultar MENU_NAIPES_TEXTO_PARPADEO.md y LOGICA_APROBADOS.md, sección «Añadir naipes: selector y cantidades HD». El documento antiguo y test_value_digit_blink.py usan conteo constante de glifos; ese criterio NO demuestra oclusión correcta y no debe sustituir la regla posterior: tinta y sombra HD se recortan por la silueta BG/OBJ, ventanas y prioridades del mismo frame, incluidas coberturas parciales. Al tocar texto/números, comprobar frames antes/durante/después del selector, desplazamiento, cambio de valor y fundido; conservar una pareja GBA/HD con iguales entradas. No declarar correcto solo porque aparezcan55 glifos.

Para retomar: revisar primero aprobados nuevos y PRs; después abrir las capturas y esta receta antes de repetir exploración. No reescribir la lógica aprendida en cada intento.
