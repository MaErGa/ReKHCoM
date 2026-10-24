# Añadir naipes: cantidad HD estable con selector

Pedido usuario24/10, captura6b1f1e62-a4ef-4d22-8597-4fa3d5c0c31d.
Reproducción: fixture de sala, colección privada con cheat3, Mode_Deck, R, A, abajo.
El centro transparente de los dígitos podía mostrar el selector de detrás: el filtro por un solo
píxel ocultaba la letra HD completa, aunque su tinta original seguía visible. Ahora conserva una
máscara de tinta8x8 del tile original y busca tinta visible, manteniendo ventanas y prioridades.
Una celda totalmente tapada sigue oculta; no se alteran sprites ni lógica de naipes.

EU: build + check_sources; 21 cuadros480..540 cada3: antes53/55 glifos numéricos, después55 constantes.
21 pares con HD apagado son idénticos. Captura final revisada y enviada a app antes del PR.
Regresión reproducible: python port/tools/test_value_digit_blink.py --build build/pc-eu.
Claude integra. No release ni cambios a partidas del usuario.
