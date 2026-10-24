# Ver barajas: sincronía de la extensión de pestañas

Las filas originales se dibujan antes de VBlank; la presentación de márgenes sucede después de DMA.
PortMenuTabPixel leía VRAM y paleta ya actualizadas para el siguiente cuadro, mezclando dos cuadros.
Se guardan los 16 colores de cada extremo BG al terminar el render, antes de VBlank; Present usa esa copia.
Las pestañas hechas de sprites conservan su ruta. No cambia arte, navegación ni dibujo original.

Reproducción aislando mandos externos: entrar Mode_Deck, subir al filtro, derecha y volver izquierda.
42 fotogramas EU a1536x864; solo cambia la extensión izquierda del cuadro661 (x0..119, y97..172).
71 filas discrepantes en la unión pasan a0; región original idéntica en los42 y estado estable idéntico.
Comando reproducible: test_menu_tab_sync.py --reference <build-anterior> --build build/pc-eu.
Build EU y check_sources pasan. Claude integra. El parpadeo HD de Añadir naipes es otra tarea.
