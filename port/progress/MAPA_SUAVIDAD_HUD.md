# Caminata: quitar mezcla de HUD transparente que desperdiciaba el cuadro

Pedido usuario24/10: con textos HD y lógica GBA Mejorado, la cámara de campo parece menos suave que combate.
Se midió el render antes de cambiar cámara, temporización o velocidad. En la sala fixture10901 el HUD visible
solo ocupa28x29 píxeles (indicador de piso), pero se mezclaban384x216, ampliados a toda la ventana1536x864.
Ahora la copia de HUD usa solo el rectángulo con alfa visible. Escala HUD entera y mismo origen: no hay cambio visual.
El búfer, capas delante, texto HD y coordenadas HUD siguen completos. HUD vacío no envía un draw transparente.

Prueba oculta SDL software, no certificación de VSync/G-SYNC en pantalla física. Config: view4, zoom6,
HDtext1, gbaMapLogic2, frameSkip1, vsync1. Misma sala y entradas, controles aislados,181 cuadros medidos:
main antes: trabajo mediano18.13ms; intervalo mediano18.185ms, p9524.02ms,3 intervalos>25ms.
Con recorte: trabajo9.21ms; intervalo16.7426ms, p9516.9965ms,0 intervalos>25ms.
El combate queda a16.7426ms; no se acelera el juego. El cartucho conserva59.7275Hz y la cámara original.

La imagen completa es idéntica en8 pares del mismo ejecutable (mapa1536/1920/3840 y combate1920,
2 cuadros cadauno), una vez retirado el aviso animado de carga. test_hud_crop.py reproduce la comparación
con KHCOM_HUD_FULL_BLIT=1 como referencia. Build EU y check_sources. EvidenciaJSON adjunta.

VSync puede añadir cadencia en una pantalla fija: esto quita un coste real, no demuestra que explique todo
el movimiento percibido. No se probaron VRR ni cambio de monitor; no se cambia ninguna opción del usuario.
Claude integra el PR; las copias jugables y partidas originales se conservan.
