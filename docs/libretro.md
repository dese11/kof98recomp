# Desarrollo del core libretro

Especificación de referencia: `INICIO_KOF98_LIBRETRO.md`, conservada en la
carpeta KOF98 original. El checkout de desarrollo es `kof98-libretro/`;
`kof98recomp/` y su AppImage son la referencia funcional y no se modifican.

## Estado

Esta primera entrega extrae el motor reutilizable (hito 1). Todavía no genera
`kof98_libretro.so` ni implementa el contrato libretro. No se ha validado un
core dentro de RetroArch. El registro de entorno y pruebas está en
`LIBRETRO_PROGRESS.md`.

Agregado: reproducción opcional de música AST del Neo Geo CD en el motor
reutilizable y el standalone. Preparación, uso y límites en `ast.md`. El futuro
core debe exponer original/AST, volumen y la ruta de sistema/contenido del pack.

## Compilación local y pruebas

Desde este checkout, con CMake >= 3.21, C/C++, Ninja y SDL2 para el standalone:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DKOF98_ROM_DIR=/ruta/absoluta/al/set-extraido \
  -DKOF98_BUILD_RUNTIME_TESTS=ON
cmake --build build -j 4
ctest --test-dir build --output-on-failure
./build/kof98 --rom-path /ruta/absoluta/al/set-extraido --widescreen
```

Se requieren las ROM del usuario que describe el README original, incluido el
BIOS AES. Se genera código recompilado localmente; no debe publicarse ni
incorporarse a commits o artefactos de CI.

Para comprobar los caminos de error sin ROM ni dependencias SDL:

```sh
cmake -S . -B build-public -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DKOF98_BUILD_RUNTIME_TESTS=ON -DCMAKE_DISABLE_FIND_PACKAGE_SDL2=TRUE
cmake --build build-public -j 4
ctest --test-dir build-public --output-on-failure
```

## API del motor

`<neogeorecomp/runtime.h>` expone carga, frame, reset, descarga y contadores.
El target `neogeorecomp_runtime` tiene PIC y no enlaza SDL, no abre ventanas,
no lee dispositivos y no regula la velocidad. `neogeorecomp` conserva el
frontend SDL/headless y usa la misma API. El cargador posee y libera las ROM,
tablas de despacho, imagen decodificada y sintetizador. El juego puede liberar
sus assets con el callback opcional `ng_game_t.unload`.

Existe una sola placa por proceso; la API no es reentrante ni segura entre
hilos. Los buffers de salida pertenecen al motor y vencen en el siguiente
frame/reset/descarga. Cambiar el ancho exige descargar y volver a cargar,
porque se parchea el programa antes del arranque. Reset conserva la semántica
de reset de hardware previa (RAM de trabajo/paleta no se borra); descarga y
nueva carga inician una sesión limpia. No hay persistencia ni serialización.

El framebuffer usa palabras `0xFFRRGGBB`, 224 filas y pitch `ancho * 4`.
Vídeo: `12000000 / (768 * 264)` Hz. Audio: estéreo s16, `4000000 / 72` Hz;
el entero 55555 del WAV/SDL histórico redondea esta frecuencia. El futuro core
debe anunciar la frecuencia racional real.

## Próximos hitos

1. Adaptador libretro completo, `.info`, dos RetroPads y widescreen por defecto.
2. Contrato de contenido `242-pn1.bin`: validar identidad del programa generado,
   tamaños de todo el set, BIOS desde sistema/contenido y assets dentro del sandbox.
3. Build con `org.kde.Sdk/x86_64/6.11`, inspección ABI y carga real en el Flatpak.
4. Aceptación visual y de mandos: intro, menús, combate, audio, ambos jugadores,
   pausa, reset y recargas. Las pruebas sin ventana no acreditan estos puntos.

RetroArch está instalado en el host, pero el SDK 6.11 aún no. Desde Distrobox
se consulta con `host-spawn flatpak ...`. La configuración actual tiene
`libretro_directory` en `~/.var/app/org.libretro.RetroArch/config/retroarch/cores`
y el directorio `.info` en `/app/share/libretro/info` (solo lectura). Resolver
la instalación de metadatos de usuario al implementar el core, sin reemplazar
la configuración ni modificar permisos globales.

## Seguimiento

Los forks son `dese11/kof98recomp` y `dese11/neogeorecomp`; `upstream` apunta a
`danilopaulinodasilva` y `origin` al fork. La rama de trabajo en ambos es
`libretro/initial-core`. El juego fija el commit del toolkit mediante submódulo
y `toolkit.ref`; al avanzar el toolkit, actualizar ambos juntos.

```sh
git fetch upstream
git -C neogeorecomp fetch upstream
```

Integrar cambios upstream en ramas locales, ejecutar de nuevo las pruebas y
revisar `git diff --cached --stat` y `git diff --cached --name-only` antes de
cada commit o push. No se ha instalado ningún core: no hay nada que desinstalar.

Documentación oficial consultada:

- https://docs.libretro.com/development/cores/developing-cores/
- https://github.com/libretro/libretro-common/blob/master/include/libretro.h
- https://docs.flatpak.org/en/latest/first-build.html
