# KOF98 Recomp para RetroArch Flatpak

El core local `kof98_libretro.so` usa el motor recompilado y conserva el
standalone SDL `build/kof98`. El proyecto original `kof98recomp/` y su AppImage
se mantienen como referencia. El core 0.3.0 ofrece savestates versionados y
restauración comprobada de imagen/audio. Run-ahead, rewind y netplay requieren
validación específica; no se anuncia todavía compatibilidad de red. No hay
cheats ni persistencia automática de RAM/cartucho. Ver `states.md`.

## Compilar

Linux x86_64, C/C++, CMake >= 3.21, Ninja y Python >= 3.11. SDL2 es opcional
para el standalone y nunca se enlaza al core. Desde el checkout:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DKOF98_ROM_DIR=/ruta/absoluta/al/set-extraido \
  -DKOF98_BUILD_RUNTIME_TESTS=ON -DKOF98_BUILD_LIBRETRO=ON
cmake --build build -j 2
ctest --test-dir build --output-on-failure
```

Se recomienda `-j 2` en este equipo de 16 GiB: cuatro procesos GCC optimizando
el C generado agotaron RAM y swap. No es necesario recompilar el juego cuando
solo cambian frontend o empaquetado. El generador preserva los archivos C cuyo
contenido no cambia. Se regenera cuando cambian las P ROM y se registra la
identidad del set completo al compilar. No hay core exclusivamente interpretado.

La cabecera libretro está fijada en `libretro/README.md`. El toolkit se fija
mediante submódulo y `toolkit.ref`; las fuentes públicas no incluyen ROM, BIOS,
música, manifiestos privados, C generado ni binarios derivados. El core se
compila localmente con el contenido del usuario y no se publica como binario.

Build público sin ROM (compila y prueba el contrato del adaptador, no un core
jugable):

```sh
cmake -S . -B build-public -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DKOF98_BUILD_RUNTIME_TESTS=ON -DCMAKE_DISABLE_FIND_PACKAGE_SDL2=TRUE
cmake --build build-public -j 2
ctest --test-dir build-public --output-on-failure
```

## Instalar y ejecutar

Con RetroArch cerrado, desde este checkout:

```sh
python3 scripts/install_libretro.py install \
  --rom-dir /ruta/absoluta/al/set-extraido \
  --ast-dir /ruta/absoluta/al/paquete-kof98-cd-ast
./scripts/run_libretro.sh
```

`--ast-dir` es opcional; preparación del paquete en `ast.md`. El instalador lee
las rutas efectivas `libretro_directory` y `system_directory` de la configuración
Flatpak. Copia únicamente este core, sus metadatos y los dos PNG de márgenes;
crea un enlace al paquete musical para no duplicar los aproximadamente 623 MiB.
Las ROM y BIOS permanecen donde las tiene el usuario. La ruta del paquete AST
debe seguir existiendo y ser visible dentro del sandbox.

El lanzador instalado queda en:

```text
~/.var/app/org.libretro.RetroArch/config/retroarch/kof98-recomp/run.sh
```

Usa `flatpak` en el host y `host-spawn flatpak` desde Distrobox. Acepta argumentos
adicionales de RetroArch, por ejemplo `./scripts/run_libretro.sh --verbose`.
El directorio de `.info` de esta instalación se selecciona con un archivo
suplementario exclusivo del lanzador. La configuración principal permanece
intacta; el lanzador desactiva guardar esa configuración temporal al salir.
Las opciones del core se guardan mediante los mecanismos propios de RetroArch.

También se puede cargar `kof98_libretro.so` con «Cargar núcleo» y después
seleccionar `242-pn1.bin`. Para incluir sus metadatos sin cambiar el directorio
global de información, usar el lanzador anterior.

Desinstalar solamente los archivos que instaló este proyecto:

```sh
python3 scripts/install_libretro.py uninstall
```

Se conservan archivos modificados por el usuario. Nunca se borra el paquete
musical original ni el set de ROM/BIOS. Para una configuración no estándar,
ambas acciones aceptan `--config /ruta/retroarch.cfg`.

## Contenido y rutas

Seleccionar **`242-pn1.bin`** del set AES `kof98h` extraído. No hay soporte ZIP.
La carpeta también debe contener:

- `242-p2.bin`, `242-c1.bin` a `242-c8.bin`;
- `242-s1.bin`, `242-mg1.bin`, `242-v1.bin` a `242-v4.bin`.

Las cuatro ROM de sistema (`neo-po.bin`, `sfix.sfix`, `sm1.sm1`, `000-lo.lo`)
se buscan como grupo completo, en este orden:

1. `<system>/kof98/`;
2. `<system>/`;
3. la carpeta del contenido.

El primer grupo completo debe coincidir en tamaño y SHA-256 con el utilizado
al compilar. Un grupo completo pero distinto produce un error explícito; no se
mezclan BIOS de distintas carpetas. También se valida el tamaño y SHA-256 de
cada ROM del juego antes de cargar. Cambiar de set/BIOS requiere recompilar.
No se aceptan parches externos del programa; la panorámica propia se aplica
tras comprobar su identidad. No se anuncia soporte para variantes MVS u otros
juegos con este core.

Recursos de los márgenes: `<system>/kof98/assets/`, `assets/` junto al contenido,
o `<core assets>/kof98/assets/`, en ese orden. Si faltan, el motor utiliza su
reflejo de respaldo y deja un aviso. El core no usa variables de entorno ni
rutas del checkout para localizar sus recursos.

AST: `<system>/kof98/ast/` o `ast/` junto al contenido, con `soundtrack.txt` y
los WAV locales. El paquete se valida al cargar; un paquete instalado pero
inválido produce un error, también en modo automático. Voces y efectos
continúan en el sintetizador AES. Los fades AST siguen siendo aproximados.

## Opciones y controles

- **Pantalla panorámica**: activada por defecto, 400×224. Desactivada: 320×224.
  Amplía la escena, conserva píxeles cuadrados y los márgenes propios.
  Cerrar y volver a cargar el contenido aplica el cambio; «Reiniciar» conserva
  la geometría de la sesión cargada.
- **Música**: automático por defecto (AST si encuentra el paquete), original
  AES o AST obligatorio. Cerrar y volver a cargar para cambiar.
- **Volumen AST**: 0–100, predeterminado 70; se aplica durante la partida y
  no cambia el volumen de voces/efectos.

Dos puertos RetroPad, con la misma asignación para ambos:

| RetroPad | Neo Geo AES |
|---|---|
| Cruceta | Direcciones |
| B (abajo) | A: golpe débil |
| A (derecha) | B: patada débil |
| Y (izquierda) | C: golpe fuerte |
| X (arriba) | D: patada fuerte |
| Start | Start del jugador |
| Select | Select AES del jugador |

RetroArch gestiona teclados, mandos físicos, remapeos, pausa y presentación.
Desconectar un puerto libera todas sus entradas. No se simulan monedas MVS.

## API y verificación

El core implementa la API obligatoria completa, vídeo software XRGB8888,
pitch `ancho*4`, 224 líneas, `12000000/(768*264)` Hz y audio s16 estéreo
`4000000/72` Hz en bloques variables. No abre ventanas, no lee dispositivos,
no duerme y no enlaza SDL. Solo se exportan las funciones `retro_*`.
`retro_serialize_size()` devuelve 282 658 bytes con contenido cargado y cero
sin contenido. Guardar/restaurar usa un formato versionado y validado; no se
expone RAM como un guardado persistente de cartucho que todavía no existe.

El motor es una placa por proceso, no reentrante. Los buffers vencen al
siguiente frame/reset/descarga. Reset conserva la semántica de hardware previa;
descargar y cargar inicia una sesión limpia. Se registran los contadores de
bloques nativos al descargar para acreditar ejecución recompilada.

Pruebas: harness `dlopen` contra el `.so` real, entradas de ambos jugadores,
320/400, audio en bloques y por muestra, reset, recargas y puertos desconectados;
errores de contenido y SHA-256 con datos sintéticos; regresión del standalone.
Una prueba automatizada no acredita por sí sola que se escuchó el audio ni que
funciona un mando físico. La aceptación visual y de mandos debe registrarse
por separado en `LIBRETRO_PROGRESS.md`.

Para comprobar ABI y carga en el Flatpak instalado:

```sh
readelf -d build/kof98_libretro.so
nm -D --defined-only build/kof98_libretro.so
host-spawn flatpak run --command=/ruta/al/build/kof98_libretro_test \
  org.libretro.RetroArch /ruta/al/build/kof98_libretro.so /ruta/al/set
```

El último entorno inspeccionado utiliza RetroArch 1.22.2,
`org.kde.Platform/x86_64/6.11`, glibc 2.42. El SDK 6.11 no estaba instalado.
Se admite el build del contenedor solo tras comprobar sus dependencias,
versiones GLIBC/GLIBCXX y carga real en ese runtime. Para otro entorno,
preferir su SDK correspondiente y volver a verificar la ABI; no copiar libc
ni el cargador del host dentro del paquete.

Referencias oficiales:

- https://docs.libretro.com/development/cores/developing-cores/
- https://docs.libretro.com/guides/cli-intro/
- https://docs.flatpak.org/en/latest/first-build.html
