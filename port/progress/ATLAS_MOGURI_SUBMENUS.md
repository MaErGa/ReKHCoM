# Moguri: submenús y punto de reanudación

Reproducir desde la raíz con `python port/tools/atlas_moguri_replay.py`. Receta, origen y SHA256 del guardado, controles y fotogramas en ATLAS_MOGURI_REPLAY.json. Requiere el binario EU de build/pc-atlas y los recursos locales indicados. Copia el guardado a build/atlas-moguri-after por pantalla: el original no se modifica. No es un savestate de memoria; vuelve de forma reproducible al modo exacto con KHCOM_MODE. Guarda log, configuración y dos capturas por pantalla (1420/1500).

Comprar y Canjear ahora se reconocen como menús completos. Sus márgenes continúan la fila de fondo BG del borde; no repiten cada cuatro píxeles el selector ni arrastran sprites. El margen izquierdo de Comprar conserva el rojo oscuro pedido por el usuario en la captura45d140e4. La cabecera superior y la banda inferior continúan en su altura original.

Validación: EU compilado con un proceso y límite768MB, capturas aisladas software1920x1080, español. No se han probado por esta entrega todos los idiomas ni aperturas de sobres/confirmaciones. La propuesta se envía a Atlas para revisión; no hereda la aprobación del menú principal.

## Evitar repetir los errores de Añadir/Quitar

Consultar MENU_NAIPES_TEXTO_PARPADEO.md y LOGICA_APROBADOS.md, sección «Añadir naipes: selector y cantidades HD». El documento antiguo y test_value_digit_blink.py usan conteo constante de glifos; ese criterio NO demuestra oclusión correcta y no debe sustituir la regla posterior: tinta y sombra HD se recortan por la silueta BG/OBJ, ventanas y prioridades del mismo frame, incluidas coberturas parciales. Al tocar texto/números, comprobar frames antes/durante/después del selector, desplazamiento, cambio de valor y fundido; conservar una pareja GBA/HD con iguales entradas. No declarar correcto solo porque aparezcan55 glifos.

Para retomar: revisar primero aprobados nuevos y PRs; después abrir las capturas y esta receta antes de repetir exploración. No reescribir la lógica aprendida en cada intento.

## Revisión2: pestaña y animación

Padre bbfce11b25f34df0a2004146597afb99, envío20261026-071224-dc7307, nube69f4acf0-5614-4535-a159-2d503e7692ca. El óvalo de la última pestaña toca el borde GBA y su tinta se prolongaba horizontalmente; otras filas copiaban siluetas de la rejilla. El perfil de Canjear continúa el fondo limpio bajo las pestañas por la columna derecha, desde la cabecera hasta antes de la línea inferior. Se toma el BG del mismo frame, manteniendo el fundido. Comprar conserva el rojo ya aprobado.

Retomar con `python port/tools/atlas_moguri_tabs_replay.py`: cuatro fases1420/1433/1447/1462 en una misma ejecución aislada. Carpeta build/atlas-moguri-tabs-r2/canjear, PNG, BMP, runtime.log, kh_com.log, ini y review.png. Se verificó fondo uniforme en la columna de margen de las cuatro fases y centro GBA idéntico al padre en1420. Compilación EU768MB, check_sources y diff --check pasan. La nueva hija requiere valoración; el padre aprobado se integró en PR228.


Revisión directa posterior (usuario26/10, sin Atlas): se retira la sustitución de todo el margen16..158. Solo se reemplaza la tinta junto a las pestañas16..31; desde fila32 cada fila vuelve a continuar su propio BG, incluido el degradado inferior original. Comparar con padre aprobado1420..1462: región inferior debe ser idéntica.
