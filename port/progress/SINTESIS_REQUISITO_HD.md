# Síntesis: requisito HD dinámico, implementación pendiente de captura

Tarea `sintesis-requisito-hd`, owner `codex-sintesis`.
Referencia `dbdb82b983fc4a178d46ce70edf44c6b`, envío
`20261025-011932-34d062`. Base del worktree: `d47dbeab6`.

## Qué cambia

Sólo `SELMAP_EVKEY_0/2` usan el adaptador PC de
`src/card/synthesis_requirement_hd.inc`. Se guarda una copia del requisito
de cada carta al iniciar la tarea. Después de los DrawSprite originales,
se identifican las entradas de dorso y requisito de esa carta. Copias
persistentes de sus ObjTiles cambian únicamente `src`, usado como identidad
por `PortOamSetHdPart`; los tiles VRAM, frames, palettes, índices, flags,
orden y afines son los originales. No se modifica el recurso compartido
ni se cambia cómo se paga una puerta. El recorrido existente dibuja HD y
suprime los píxeles GBA de esas mismas piezas; la PPU no recibe cambios.

El requisito usa `rule/value`; TOTAL usa `drawnTotal`, que ya actualiza
`UpdateEventKeyTotal` con el saldo real. Rangos cubiertos: 0..99, uno/dos
dígitos, al menos/al máximo/igual/total. Fuera de ese rango o si falta alguna
de las dos piezas necesarias, queda la carta GBA completa. NONE sin condición
no agrega un número. Un `kind` específico conserva la ilustración y el
recorrido de cartas existentes: no se inventa un símbolo genérico encima.
Los dorsos distinguen verde/rojo/azul; GOLD conserva la silueta blanca usada
por `gMapCardBackDefs`, y los indicadores de color originales siguen intactos.

No encontré el script original de la propuesta en el `build/hdm-check`
consultado: allí están `req_r1.json` y las imágenes. El nuevo generador
reconstruye la silueta abierta y banda inferior con geometría propia,
supersampling y salida 4x. Los dígitos usan el KHMenu ya incluido en el repo;
no son un recorte del mod de Re:CoM y requieren valoración visual propia.

## Assets y entrega pendiente

`port/assets/synthesis-requirement/`: 405 PNG transparentes y manifiesto.
`port/tools/synthesis_requirement_hd.py`: generador y staging explícito.
`port/include/port/synthesis_requirement_hd_entries.inc`: identidades y cajas
OAM. Se incorpora por el generador de la tabla HD, sin filas falsas de ROM
en el TSV ni cambios al importador de Re:CoM.

Los PNG **no se activan ni empaquetan automáticamente**: son una propuesta
pendiente de captura hija y revisión. Se deben colocar en el directorio de
sprites de la prueba aislada; assets ausentes dejan GBA. La incorporación al
paquete de jugadores se decidirá después de la revisión según la política
de honores. No se ha publicado PR ni se ha enviado una imagen estática como
si fuera evidencia del juego.

## Verificación sin compilación ni juego

```powershell
python port/tools/synthesis_requirement_hd.py --check
python port/tools/gen_hd_sprites_inc.py --check
git diff --check
```

El primer comando comprueba 405 identidades, cajas, dimensiones RGBA y alpha,
y que las cajas incluyan todas las partes de los frames OAM de los YAML.
No demuestra todavía el resultado del renderer ni la sincronía del saldo.

## Prueba serial futura: sólo tras autorización del coordinador

`port/tools/test_synthesis_requirement_hd.py --build <build-nuevo> --out <dir-nuevo>`
prepara capturas aisladas sin ejecutar; `--hd <sprites-existentes>` mezcla el
resto de los assets HD. Añadir `--run` sólo después de recibir autorización.
`--baseline <build-anterior>` añade una comparación píxel a píxel con HD
apagado. Todo se ejecuta en serie; nunca modifica la partida del usuario.

Fixture: sala 10901 en frame 60, freeze 120, doorpose 200, A 240..242;
capturas 300/380/470/510 a 1920x1080. Comprueba logs de las dos piezas nuevas
en 470 y ausencia de ellas en HD off. Revisar visualmente que la carta GBA
ya no se duplique, el foco atraviese el centro transparente y la mano/panel
conserven su oclusión. El fixture sólo cubre la primera puerta `1 >=`.

Antes de cerrar faltan casos reales para `<=`, igualdad, TOTAL con pago que
cambie 10→9, diferentes colores, carta de `kind` específico, varios requisitos,
cancelación/reapertura, transición/mosaico y assets ausentes. Comparar HD off
con el binario base. Después capturar y subir hija real en Atlas con parent
`dbdb82b983fc4a178d46ce70edf44c6b`, atendiendo nube
`13c9d20d-c78f-4ba3-8431-cd894028c7c0`. Sólo entonces preparar el PR.

## Límites honestos

No compilado ni ejecutado por instrucción del usuario (RAM/prueba serial).
La separación y oclusión reutilizan código existente, pero necesitan
comprobación visual runtime. La forma y fuente nuevas son una propuesta,
no una reproducción exacta del montaje previo. No se tocó `port_ppu.c`,
`port_hdtext_layout.inc` ni las fuentes/render de texto.
