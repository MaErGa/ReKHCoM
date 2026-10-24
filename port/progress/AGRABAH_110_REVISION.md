# Agrabah 110: pilares y terraza

24/10/2026. Revisión preliminar, pendiente de valoración humana. Afecta al
fondo gEventBg110 compartido por los eventos 110 y 111; se reconoce por el
contenido BG3 af4371eb y BG2 69a3c4e9, no por el evento activo.

## Nubes y fuente

Envío 20261024-054123-fb2267, imagen padre e767407ba71849c18d41033041e74f9d.
SHA-256 del snapshot comprobado:
4c1d36184501b5c67ba76858881c86f191a245334479289f436413abbdeb1379.
Nubes: 06badb66-76bd-4531-9137-14fd98543390 (extender pilares) y
1fec3e2c-b10f-4612-b075-b9f3feeb39a8 (patrones mal ajustados).

El mapa nativo de 512x512 tiene sus propios giros en x=80 y x=208. Al salir
por la izquierda, el frente de la terraza es horizontal entre y=208 y y=335.
La receta genérica lo desplazaba por tramos diagonales y producía una escalera.
La nueva selección conserva el plano horizontal, repite su propia pared y
separa la arena superior del suelo inferior. Las dos columnas cortadas por
el borde y=0 continúan con sus propios fustes (BG2, x=0..47 y x=112..159),
sin repetir basas, vasijas ni personajes. La pared derecha conserva los
períodos distintos de sus dos planos: 192 arriba y 160 en el frente inferior.

Se muestrean índices de los tiles originales con la paleta activa del juego.
No hay imágenes nuevas, RGB fijo, OAM, colisiones ni cambios de cámara. El
muestreo especial se aplica solo fuera del mapa y fuera de los 240x160 GBA.
Los dos perfiles de 110 se activan solo bajo Preliminar V1: no hay alta en
kPreliminarApproved ni cambio del comportamiento por defecto del jugador.

## Evidencia local

Build EU con ninja -j2. Prueba serial:

    python port/tools/test_preliminar_v1.py --build build/pc --out build/art110-verified --only escena-110,evento-111-escena-110 --subject 110

Cuatro anchos 1920x1080, 2560x1080, 3440x1440, 3840x1080, escala x3 y cuadros
100/300/1000, con y sin esta receta desde la misma instantánea. Capturas,
reportes JSON y galería quedan en build/art110-verified (ignorados). Revisión
visual de 3440x1440 y 3840x1080: pilares continuos y frente sin escalones falsos.
check_sources.py pasa. Esta evidencia no certifica una partida completa ni
las regiones US/JP.

Resultado: **24/24** pares EU pasan, GBA idéntica y márgenes distintos. Hija
`179b6da22e5c4bd39181d5ab14d7b9d3`, padre y ambas respuestas comprobados tras
la importación en la app de revisión. Sigue esperando valoración humana.
