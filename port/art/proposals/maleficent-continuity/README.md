# Maléfica: continuidad nativa del lomo

Candidato pendiente de revisión, derivado del dibujo y adaptado a los tres bordes originales. No añade una tercera ala. La primera espina sólida continúa el corte alto, desciende al lomo bajo y se mezcla con la paleta púrpura nativa. La columna256 reproduce exactamente la columna255 del original; se mezclan los siguientes siete píxeles con la rampa nativa.

Los tres archivos indices tienen512x384 índices de0 a15;0 transparente, solo hay añadido desde x256. Los PNG native-body/native-mounted son originales extraídos de la ROM local; los montajes y las comparativas tienen igual origen, escala y fondo. El manifest conserva agrupación de41 estados, paleta, registro del dibujo y pruebas: cero píxeles opacos originales cambiados, cero diferencias en la unión y un único borde por grupo. Esto es evidencia offline, no41 capturas runtime.

Reanudar con Python provisto de Pillow/numpy/PyYAML, desde la raíz:

```
python port/tools/art_review/maleficent_animation_review.py --out build/maleficent-native-audit
python port/tools/art_review/maleficent_continuity.py --install-candidate
```

La primera orden reconstruye los cuerpos, brazos y agrupación originales usando la ROM EU y sus símbolos locales. La segunda registra reference.png, adapta el corte por grupo y reconstruye los índices, las comparativas y el candidato de runtime. Sin --install-candidate no toca el código activo. Nunca cambia valoraciones humanas. reference.png es dibujo generado; los originales de las comparativas proceden de ROM, sin recrear el panel ANTES.

Runtime aislado utilizado: B8CP.gba, ventana3840x1080, KHCOM_BATTLE=202:1:0; KHCOM_ARENAS_V2=1; KHCOM_PRELIMINAR_V1=1; teclas60-63:A,300-320:RIGHT,400-420:LEFT; capturas240/330 y salida334; SDL vídeo/audio dummy, controladores0xffff/0xffff, PPU_THREADS1, BACKGROUND_TEST1. Copia del exe en build/maleficent-approved-runtime/maleficent-3840; no se modifica una partida del usuario. La integración depende de la valoración de esta adaptación, separada del estudio PR222.
