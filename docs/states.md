# Estados de KOF98 Recomp (formato 1)

El core 0.3.0 implementa estados completos entre frames. La misma API queda
disponible para otros frontends en `neogeorecomp/runtime.h`. El standalone SDL
continúa funcionando; su menú todavía no ofrece guardar/cargar estados.

## Uso en RetroArch

Arrancar con `scripts/run_libretro.sh`. En la configuración actual del usuario,
F2 guarda y F4 carga el slot seleccionado. También se pueden usar las acciones
Guardar estado / Cargar estado del menú rápido. Los archivos se escriben en el
directorio de estados configurado por RetroArch; el core solo entrega/recibe
buffers y nunca decide una ruta de guardado.

La ROM/BIOS, el modo AST/original, el paquete AST y la panorámica deben coincidir
con la sesión en que se guardó. Cambiar de 320 a 400 píxeles requiere cerrar y
cargar el contenido con la opción correcta antes de restaurar. Un estado
incompatible o alterado se rechaza sin cambiar la partida. El volumen AST es
una preferencia del frontend y conserva su valor actual al cargar.

Los estados se pueden restaurar tras reset, descargar/cargar el juego y cerrar
el proceso, usando el mismo contenido y una versión compatible del formato.
La portabilidad entre arquitecturas y futuras revisiones del motor todavía no
está certificada; el core declara `PLATFORM_DEPENDENT`. No hay conversión desde
estados de otros emuladores ni promesa de compatibilidad con formatos futuros.

## Inventario de estado

| Módulo | Qué se conserva |
|---|---|
| Planificador | Ciclos maestros, comienzo del frame y líneas de interrupción |
| 68000 | Registros, flags, stacks, PC, STOP y contexto auxiliar de Musashi necesario en los límites de ejecución |
| Bus | RAM de trabajo/backup, banco de programa, vectores BIOS/cartucho y bloqueo de backup |
| LSPC | VRAM, registros de acceso, animación y temporizador de interrupciones |
| I/O | Entradas latcheadas, modo AES/MVS y registros del calendario |
| Video | Ambas paletas y selección de paleta/capa fija |
| Audio | Registros/flags/IRQ del Z80, su RAM/bancos/latches, reloj, contadores de muestras, registros y temporizadores YM2610 |
| Síntesis | Estado de FM, SSG, ADPCM y resampler mediante `ymfm::save_restore` |
| AST | Comando/pista, fase fraccional PCM, argumentos pendientes, ganancia y progreso del fade |

Los registros se codifican con anchos explícitos en little-endian. No se copian
estructuras C/C++ completas, padding, punteros, callbacks, mapas de direcciones,
FILE, rutas, tablas de funciones recompiladas ni ROM. Los contadores de perfil
son diagnósticos y no se rebobinan. Los recursos se siguen cargando localmente.

El FIFO de audio se drena al entregar cada frame; el contrato de snapshots exige
ese límite y comprueba que no quedan muestras pendientes. El framebuffer y los
buffers de salida no son estado futuro: se invalidan al restaurar. Se reconstruyen
los mapas del bus, los colores derivados y la caché PCM. Los márgenes se vuelven
a calcular al renderizar la primera línea del siguiente frame. Sus columnas
usan memoria estable; se corrigió un `realloc` que invalidaba punteros ya
entregados al renderer y hacía depender la primera imagen del layout del heap.

El calendario arranca en 2000-01-01 UTC y avanza por ciclos emulados, sin leer
`time()` ni la zona horaria del host. KOF98 AES mantiene la imagen y el audio de
referencia. Esta política evita una entrada externa sin registrar al restaurar
o ejecutar otra instancia; futuros títulos MVS deberán validar su uso del reloj.

## Formato y errores

Cabecera de 48 bytes: magic `NGSTATE\0`, versión, longitud, huellas FNV64 del
contenido cargado y del paquete AST, ancho/modo, CRC32 del payload y reservado.
La huella del paquete se calcula al cargar sobre los archivos WAV y la
correspondencia de comandos; no depende de las rutas. La huella de ROM incluye
las regiones realmente cargadas, BIOS y programa parcheado.

El payload actual ocupa, con cabecera, **282 658 bytes** (aprox. 276 KiB). La
longitud permanece fija durante la sesión y se conoce antes del primer frame.
Las huellas y CRC detectan incompatibilidades/alteraciones accidentales; no son
una firma criptográfica ni un formato para importar datos de fuentes arbitrarias.

Se comprueba todo antes de modificar el motor: cabecera, contenido/opciones,
longitud, CRC y límites de campos. Si hay que recuperar otra pista AST, se abre
su archivo antes de aplicar el estado. La aplicación posterior no hace nuevas
aperturas ni necesita reservar memoria. Cambiar el orden/significado de campos
o el esquema de YMFM exige revisar e incrementar la versión del formato.

## Pruebas reproducibles

Con ROM locales y pruebas activadas al configurar:

```sh
cmake --build build -j 2
ctest --test-dir build --output-on-failure
./build/kof98_states_test /ruta/al/set /ruta/al/paquete-ast
python3 tests/states_process.py ./build/kof98_states_test /ruta/al/set /ruta/al/paquete-ast
```

`kof98_states_test` prueba límites durante el arranque (también interpretado),
intro, entrada a menús, selección y combate. Guarda, ejecuta un futuro, introduce
otras entradas, restaura y compara bytes de estado, imagen y audio al repetir.
Incluye reset/recarga, geometría incorrecta, buffers cortos, corrupción y un
campo inválido con CRC correcto. Prueba 500 pares guardar/restaurar y 120
correcciones de seis frames. Las pruebas sintéticas ejercitan también fades,
loops, one-shot, comandos con argumentos, archivos AST ausentes y caché fría.

`states_process.py` restaura en un proceso nuevo, con alias distintos para las
carpetas de ROM y música, y comprueba el CRC con `zlib` de forma independiente.
Los estados temporales se eliminan y nunca se versionan. Las pruebas públicas
no necesitan ROM, pistas ni código generado del juego.

Mediciones Release iniciales en Ryzen 7 4800HS: aproximadamente 3,4 ms por par
guardar/restaurar. La comprobación doble de una corrección de seis frames ronda
20–22 ms, incluyendo ambas simulaciones, varias serializaciones y hashes de
imagen/audio. No equivale al coste de un rollback real ni garantiza su límite
máximo; el siguiente hito debe medir percentiles y carga de frontend/red.

Se verificó guardar, pausar, reiniciar y cargar mediante RetroArch Flatpak 1.22.2
real con configuración privada y drivers nulos. Su contenedor `.state` incluía
el payload del core y un CRC correcto; el registro confirmó la restauración.
No sustituye una prueba visual/auditiva de los controles físicos F2/F4.

## Siguiente hito

El `.info` anuncia estados `serialized`. La integración de run-ahead, rewind y
rollback de red necesita pruebas específicas antes de anunciar `deterministic`.
La base ya supera correcciones locales y restauración entre procesos x86_64;
falta validar latencia/coste bajo frontend y dos equipos con pérdida y jitter.
No se incorporó GGPO ni se implementó un protocolo de red adicional en esta fase.
