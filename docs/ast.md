# Música AST de KOF98 CD

El ejecutable AES recompilado puede usar las pistas de audio del Neo Geo CD
manteniendo voces y efectos del cartucho. El soporte vive en el motor común,
sin SDL: queda disponible también para el futuro adaptador libretro.

## Uso en este equipo

El paquete privado ya se preparó en `soundtracks/kof98-cd-ast/` a partir del
archivo 7z facilitado por el usuario. Son 39 WAV PCM16 estéreo de 44 100 Hz,
sin pérdida, unos 623 MiB. Los índices de pregap del CUE se excluyen; no se
incluye la pista de datos como audio.

Desde el checkout de desarrollo:

```sh
bash scripts/run_ast.sh --rom-path ../kof98recomp/roms
```

O de forma explícita, desde cualquier directorio:

```sh
/home/martin/Downloads/Games/KOF98/kof98-libretro/build/kof98 \
  --rom-path /home/martin/Downloads/Games/KOF98/kof98recomp/roms \
  --ast-path /home/martin/Downloads/Games/KOF98/kof98-libretro/soundtracks/kof98-cd-ast \
  --ast-volume 70 --widescreen
```

`--ast-volume` acepta 0..100; el valor predeterminado es 70. Omitir `--ast-path`
o usar `--soundtrack original` conserva el audio original. El launcher acepta
también esta última opción. `--ast-volume 0` conserva efectos y voces mientras
silencia solo la música externa. No hay cambio de modo en caliente: reiniciar
el contenido para cambiar de soundtrack. El build y la AppImage originales
permanecen en su carpeta de referencia.

## Preparación reproducible

Se requieren Python >= 3.11 y `7z` para importar el archivo comprimido. El
reproductor no necesita ffmpeg, codecs adicionales ni conexión de red.

```sh
python3 scripts/import_ast.py \
  "$HOME/Downloads/The King of Fighters '98 - The Slugfest (Japan) (En,Ja).7z" \
  --output soundtracks/kof98-cd-ast
```

El destino debe ser nuevo. El importador también acepta directamente el CUE
con un BIN por pista. Valida la revisión del driver del CD y el formato del
disco, genera la correspondencia de comandos a partir de ese disco y escribe
sumas SHA-256 de los WAV. No adivina tablas de otra edición. El dump aportado
usa PCM little-endian, comprobado durante el desarrollo; `--swap-audio` queda
para dumps de audio con bytes invertidos.

El paquete exige el M1 AES `242-mg1.bin`, CRC32 `4e7a6b1b`. Una revisión
incompatible o un WAV ausente/formato incorrecto hacen fallar la carga con
un error comprensible. Si un archivo desaparece antes de iniciar una pista,
se conserva la música nativa para esa orden. No modificar el paquete durante
la ejecución.

`soundtracks/`, ROM, BIN, WAV, PCM, CUE, ISO y builds están excluidos de Git.
Solo se publican el importador, el reproductor y pruebas con señales sintéticas.
El manifiesto extraído y las grabaciones permanecen locales.

## Funcionamiento y límites

Los drivers AES/CD comparten la versión 1.7 de 1998-06-16. La tabla del CD
identifica 41 órdenes que alcanzan sus 39 pistas. Las órdenes de música con
CDDA se traducen a reproducción externa y el Z80 recibe su orden de parar
música anterior. Su eco hacia el 68000 conserva el comando original.
Voces, efectos, comandos de sistema y bytes de parámetros siguen llegando
al driver AES. Los jingles que el propio CD sintetiza conservan esa ruta.

El audio se mezcla con saturación en el mismo reloj que el YM2610. La
conversión de 44 100 a `4 000 000 / 72` Hz mantiene una fase racional y usa
interpolación lineal. Reset, descarga y nueva carga limpian pista, fase,
fade, buffers y parámetros pendientes.

La bandera del CD decide entre reproducción única y loop de la pista
completa desde INDEX 01. No se han inventado loops internos ni crossfades.
Los fades responden a la velocidad del comando AES, pero su envolvente
externa es una aproximación lineal; no está calibrada como reproducción
analógica exacta del hardware CD. Cancelar fade/restaurar volumen funciona.

No hay savestates: el futuro inventario debe incluir pista, posición racional,
gain/fade, parámetros pendientes e identidad del paquete. No se declara
validación auditiva ni de controles físicos a partir de las pruebas sin ventana.

## Pruebas

```sh
ctest --test-dir build --output-on-failure
./build/kof98_runtime_test /ruta/al/set-AES /ruta/al/paquete-AST
```

Las pruebas públicas generan PCM diminuto sin datos propietarios: mezcla con
efectos, estéreo, interpolación, loop, one-shot, fade/restauración, parámetros
que coinciden con códigos de canciones, reset, recarga y paquetes inválidos.
La prueba local opcional repite sesiones completas con las pistas reales.

Referencias consultadas: documentación de CDDA y drivers de sonido de
[NeoGeo Development Wiki](https://wiki.neogeodev.org/index.php/CDDA) y
[Z80](https://wiki.neogeodev.org/index.php/Sound_driver). La correspondencia
de este juego se verificó ejecutando el driver local del CD; no se copió el
reproductor de otro core ni se incorporó código GPL al motor MIT.

## Estados

El core 0.3.0 conserva pista, fase PCM, argumentos pendientes y fades en sus
savestates. El paquete debe ser el mismo al restaurar; sus archivos se identifican
por contenido, no por ruta. El volumen actual del frontend se conserva. Ver
`states.md` para compatibilidad, pruebas y límites de rollback.
