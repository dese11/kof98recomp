# Registro verificado — 26 de octubre de 2026

## Procedencia y entorno (hito 0)

- Upstream del juego: `17113d96149937e963bf1f357d924118c87ee67f`.
- Toolkit original: `c75d01e5c42bdb080febc5e448c95fcbfe4eb1da`.
- Submódulos: Musashi `313ebf1bd9f4d0d93341eb5ce21fd8a119e9dbdd`,
  ymfm `81aec25ccbb98f4873a255f7551ac4dadac59b4a`,
  z80 `d64fe10a2274e5e40019b1086bf7d8990cbc5f23`.
- Las fuentes versionadas del checkout limpio coinciden byte por byte con
  la copia local de referencia, incluido el toolkit. La copia original no
  tiene metadatos Git; la coincidencia no acredita su historia anterior.
- Motor/juego MIT; Musashi y Z80 MIT; ymfm BSD-3-Clause; stb MIT o dominio
  público; font8x8 dominio público. Se conservan LICENSE y NOTICE.
- Contenedor Fedora 43 `fedora-box`, x86_64, GCC 15.2.1, CMake 3.31.11,
  Ninja 1.13.1, SDL2 2.32.74 y glibc 2.42. Compilación con `-j 4`.
- Host consultado mediante `host-spawn`: RetroArch Flatpak 1.22.2, instalación
  system, rama stable, runtime `org.kde.Platform/x86_64/6.11`, glibc 2.42.
  `org.kde.Sdk//6.11` no está instalado. No se ha certificado la ABI de un core.
- La configuración de RetroArch usa los subdirectorios `cores`, `system`,
  `saves`, `states` de su carpeta de configuración; `.info` está configurado
  en `/app/share/libretro/info`. No se modificó la configuración ni permisos.
- Se renovó la sesión GitHub de `dese11` mediante OAuth de GitHub CLI y se
  configuró Git HTTPS. Ambos forks se crearon y se verificó su upstream:
  https://github.com/dese11/kof98recomp y https://github.com/dese11/neogeorecomp.
  La identidad local de commits usa la dirección noreply pública de GitHub.

## Implementación inicial (hito 1)

Toolkit de desarrollo: `a274d93bbc6c799203e6c251fc56a64dfacd7267`, fijado también en `toolkit.ref`.

- `runtime.h`/`runtime.c`: carga, frame con entrada de dos jugadores, buffers
  de vídeo/audio, reset, descarga y contadores de ejecución recompilada.
- Targets separados: motor con PIC sin SDL; frontend SDL/headless conservado.
- Propiedad y liberación de ROM, tablas, vídeo, sonido, logs y assets del juego;
  limpieza también tras carga fallida y descarga repetida. Entradas y cola de
  audio se limpian al reset; cambiar el driver Z80 conserva las muestras ya
  producidas, como en la referencia.
- Build local genera 57 050 entradas recompiladas; ninguna se versiona.
- El harness del motor comprueba recargas alternando 320 y 400 píxeles,
  reset, entradas de ambos jugadores en el bus, errores y hashes de salidas.

## Evidencia y límites

- Baseline standalone headless: 1 200 frames widescreen, 268 275 bloques
  nativos, 11 753 534 instrucciones interpretadas; 12 entradas sin cubrir.
- PNG del frame 1199 y WAV completos de esa sesión coinciden byte por byte
  antes y después de la extracción inicial.
- SHA-256 PNG: `9304f1ab691dadb5ee5f56626725f61ff3382c67a6860586607d00e74234a330`.
- SHA-256 WAV: `f7bb0c849ad7bb8a92f400ae70e0950602084426b3338564ee0692ccad932a82`.
- Pruebas públicas sin ROM y sin SDL: carga inválida aprobada; conformance
  conserva el baseline 59/60, igual al intérprete. El caso `move` ya falla
  también en la referencia; no se presenta como 60/60.
- Standalone de 4 200 frames con `tests/start_fight.txt`: captura del frame
  4199 y WAV completos idénticos antes/después; 1 901 690 bloques nativos y
  11 876 396 instrucciones interpretadas. SHA-256 PNG:
  `2341d30b7b097b85098496cb3a604388c40d946295accd020db6a12dec2a8411`;
  WAV: `007859ed74b916489716ecf1b2b3a99e712f9d3527eea5b7c5aebf668cccf441`.
- Harness aprobado: cuatro sesiones de 4 200 frames (16 800 en total),
  alternando 320/400/320/400. Hash combinado vídeo/audio repetido por ancho:
  `7bb518b96a1bed7a` y `85679d3374b1f792`. Cada sesión produjo 3 942 400
  pares estéreo; bloques nativos 1 860 045 y 1 859 996 respectivamente.
  El audio llega en bloques de 933..944 pares por frame en esta prueba;
  el frontend debe aceptar tamaño variable. Reset, dos jugadores en el bus,
  errores de carga y descarga repetida comprobados automáticamente.
- `readelf -d` del harness: libc, libm, libstdc++, libgcc y cargador; sin SDL.
- Builds y evidencia de comparación en `/tmp/kof98-libretro-*` durante esta
  sesión; son temporales. Los comandos reproducibles están en `libretro.md`.
- No hay validación de ventana visible, audio escuchado ni mandos físicos.
  Tampoco hay savestates, persistencia, rewind, run-ahead, netplay o core aún.

## Continuación

Completar el adaptador libretro y el contrato de contenido/identidad de ROM
(hito 2); después comprobar ABI y cargarlo realmente en el Flatpak (hito 3).
Instalar el SDK compatible o demostrar compatibilidad del binario concreto.
Para los commits del desarrollo consultar `git log -1` y `toolkit.ref`.
No se suben builds, ROM, BIOS, código generado, WAV ni capturas del usuario.

## Agregado AST — 26 de octubre de 2026

Toolkit AST: `7c9eb568db3de0478f724a7ab4416a8cd106cf8f`, fijado en el submódulo y en `toolkit.ref`.

- Se inspeccionó el 7z local de KOF98 CD: pista de datos MODE1/2352 y 39
  pistas de audio. Importador reproducible en `scripts/import_ast.py`;
  paquete privado en `soundtracks/kof98-cd-ast/` (652 968 420 bytes).
- Driver CD 1.7, SHA-256
  `27cbfc301498ca7653fd7d3edd44c3108763906eb7fe43badd1e3eb325b5aaf7`;
  M1 AES requerido CRC32 `4e7a6b1b`. La correspondencia de comandos se
  extrajo del CD del usuario y se comprobó ejecutando su driver en una
  herramienta local. El manifiesto y las grabaciones no se versionan.
- 41 comandos alcanzan todas las pistas 02..40. Las voces, efectos y
  argumentos siguen en el driver AES. La orden de parar música anterior
  conserva el eco del comando que espera el 68000.
- Mezcla PCM estéreo en el reloj del YM2610, conversión racional desde
  44 100 Hz, volumen 0..100, one-shot/loop según el CD, reset y descarga.
  Uso: `--ast-path DIR --ast-volume 70`; sin la ruta queda el audio original.
- Prueba de 4 200 frames: AST activó automáticamente pistas 39, 40, 05 y 22;
  captura final idéntica a la referencia y los mismos 1 901 690 bloques
  nativos. El WAV conserva 3 942 400 pares; cambia con AST y coincide
  byte por byte con la referencia al desactivar AST. Con volumen externo
  cero sigue habiendo efectos/voces; se comprobó además con mezcla sintética.
- Cuatro recargas con AST (16 800 frames) repiten hashes:
  320 píxeles `1d1ca6489e15566d`; 400 píxeles `7bb9b345ab477a13`.
  Cada sesión produce 3 942 400 pares. Reset y parámetros pendientes probados.
- Pruebas sintéticas con loops, one-shot, fade/restauración, mezcla, argumentos
  que parecen canciones y paquetes inválidos aprobadas. También pasan con
  AddressSanitizer, UndefinedBehaviorSanitizer y LeakSanitizer de Clang.
- La sesión AST al 70% no produjo muestras saturadas. El motor sigue sin SDL
  ni codecs de audio adicionales. No se ha certificado escucha ni control físico.
- Build permanente Release con `-j 4` en `kof98-libretro/build/`, usando las
  ROM de la referencia sin copiarlas ni modificar su build. Launcher AST:
  `scripts/run_ast.sh`; admite `--soundtrack original` para volver al AES.
- El harness recompilado con AST se ejecutó también dentro del sandbox real
  `org.libretro.RetroArch`: cuatro sesiones aprobadas con los mismos hashes,
  contadores y pares de audio que fuera del Flatpak. Esto comprueba las
  dependencias del harness y el acceso al pack; no equivale a cargar un core
  `.so` ni a aceptar el frontend visual/audio de RetroArch.
- Build público sin ROM/SDL: aprobados AST sintético, carga inválida y el
  baseline CPU. El CMake mantiene las pruebas AST independientes del disco.
- Límites: loop de pista completa; fades con envolvente lineal aproximada.
  Falta exponer original/AST, volumen y ruta del paquete en el adaptador
  libretro y probarlos realmente en el Flatpak. Véase `ast.md`.

## Hitos 2–3: MVP libretro local — 26 de octubre de 2026

El usuario confirmó la escucha correcta del AST en el standalone antes de
iniciar este hito. Se implementó `libretro/core.c` y el target local
`kof98_libretro.so`, con la API obligatoria, dos RetroPads, reset, carga/descarga,
vídeo XRGB8888, audio estéreo por bloques y fallback por muestra. Solo exporta
las 25 funciones obligatorias `retro_*`; no enlaza SDL ni bibliotecas de imagen
externas. `retro_serialize_size()==0`, serialización falsa, memoria no expuesta:
no hay savestates, rewind, run-ahead, netplay ni guardados persistentes aún.

Opciones: panorámica 400×224 activada, música automática (AST si hay paquete),
AES original/AST obligatorio y volumen 0–100 en caliente. Cambiar panorámica o
soundtrack requiere cerrar y cargar el contenido; reset conserva las opciones
de la sesión. Assets dinámicos, sin rutas compiladas del checkout en el core.
El toolkit añade únicamente el control de volumen en vivo; commit fijado:
`175cd96efe9edb71d2a134314731427afbf8740c`.

Contenido: `242-pn1.bin` y el set AES completo. El build genera en privado las
identidades de 20 archivos (16 de juego y 4 de sistema), tamaño y SHA-256. El
core las valida antes de iniciar; BIOS por grupo en system/kof98, system o
contenido. La carpeta seleccionada se conserva aunque P1 sea un enlace: una
prueba negativa detectó y permitió corregir la resolución inicial al destino
del enlace. Los archivos P son dependencias del generador; copiar únicamente
salidas diferentes evita recompilar todo el programa al cambiar los assets o
el frontend. Se comprobó una regeneración sin recompilar ninguno de los 16
fragmentos idénticos. La cabecera oficial libretro está vendorizada y fijada.

Build Release GCC 15.2.1: los cuatro compiladores de los fragmentos agotaron RAM
y swap. Se terminó con `-j2`, recomendado en este host de 16 GiB. Se exploró
reutilizar objetos idénticos comprobando código y cabeceras byte por byte;
Ninja invalidó su metadata y finalmente recompiló los fragmentos normalmente.
No se depende de esa copia temporal para el build reproducible.

Verificación:

- Suite pública sin ROM/SDL: 4/4 (AST, errores de ciclo de vida, SHA-256 con
  longitudes que cruzan los bloques/relleno, contrato libretro sin contenido).
- Suite local: AST, ciclo de vida de 16 800 frames y contrato del `.so` de
  16 800 frames aprobados. Tras corregir la resolución de enlaces, también
  pasaron errores de ROM faltante, tamaño, identidad y BIOS incompleta.
- El contrato prueba las 25 exportaciones con `dlopen`, ambas entradas,
  puertos desconectados/reconectados, tamaños/timing/pitch, batch y fallback,
  reset y recargas, ausencia de serialización y APIs opcionales coherentes.
- Harness del `.so` real dentro del Flatpak con AST y assets: 4×4 200 frames,
  3 942 400 pares de audio por sesión y hashes repetibles idénticos al motor:
  320 `1d1ca6489e15566d`, 400 `7bb9b345ab477a13`. Bloques recompilados positivos
  (aprox. 1,86 millones por sesión); comandos AST 21, 28, 23 y 3c observados.
- Standalone 1 200 frames: PNG del frame 1199 y WAV idénticos byte por byte
  al baseline. El proyecto original y la AppImage permanecen intactos.
- ELF: NEEDED libstdc++, libm, libgcc_s, libc y cargador; GLIBC máxima 2.33,
  GLIBCXX 3.4 y CXXABI 1.3.9. Sin RPATH/SDL; carga real aprobada en KDE 6.11
  (glibc 2.42). Se utilizó ABI comprobada del contenedor, sin instalar el SDK.
- RetroArch Flatpak 1.22.2 real: 1 200 frames / 20 segundos, 400×224,
  59,19 Hz, 55 555,56 Hz anunciados, GL Wayland y PulseAudio inicializados,
  AST pista 39. Captura GPU privada inspeccionada: intro visible y correcta;
  su resolución 1500×840 es la de presentación, no el buffer nativo.
  Al descargar: 268 275 bloques nativos y 11 753 534 instrucciones interpretadas,
  exactamente los contadores del baseline de 1 200 frames.

Instalación realizada en la ruta de cores configurada del usuario. El instalador
copia únicamente este core, `.info` y dos PNG y enlaza el paquete AST privado;
no duplica las pistas ni mueve las ROM. Lanzador y overlay bajo
`~/.var/app/org.libretro.RetroArch/config/retroarch/kof98-recomp/`.
Se comprobó el SHA-256 de `retroarch.cfg` antes/después: no cambió. El `.info`
usa un directorio exclusivo del lanzador con `--appendconfig`, sin reemplazar
el directorio global de metadatos. Instalar/actualizar/desinstalar se probó en
carpeta temporal; la desinstalación conserva archivos modificados y las pistas.

La primera prueba gráfica se pausó por la preferencia existente de perder foco.
Para completar la captura se desactivó solo en un overlay privado de prueba;
el lanzador de uso normal conserva esa preferencia. No se cambiaron permisos
Flatpak, drivers ni opciones gráficas/audio globales. RetroArch escribió sus
opciones del core y su registro de uso mediante sus mecanismos normales.

Pendiente de aceptación humana: jugar un combate en el frontend con mandos
físicos, ambos jugadores, escuchar continuidad de audio y comprobar pausa/reset
interactivos y recarga desde el menú. La prueba automatizada cubre combate y
entradas de ambos jugadores; no acredita por sí sola los dispositivos físicos.
Savestates y netplay quedan explícitamente para los siguientes hitos.

Uso y desinstalación reproducibles: `docs/libretro.md` y
`scripts/install_libretro.py`. Lanzar: `./scripts/run_libretro.sh` desde el
checkout o ejecutar su ruta absoluta desde cualquier carpeta. No publicar el
core generado, ROM/BIOS, manifests privados, pistas ni capturas.

## Fase de savestates y base para rollback — 26 de octubre de 2026

El usuario confirmó el funcionamiento del MVP en RetroArch, pidió discutir
rollback y luego autorizó comenzar por estados completos. Esta fase implementa
savestates; no declara probado netplay entre equipos ni run-ahead/rewind del
frontend. Detalles de uso, inventario y formato: `states.md`.

- API común: `ng_runtime_state_size/serialize/unserialize`, conectada a las
  tres funciones libretro. Formato 1, 282 658 bytes desde el primer frame,
  codificación little-endian explícita, cabecera de 48 bytes, huellas FNV64 de
  ROM/BIOS/programa parcheado y AST, CRC32, compatibilidad de modo/geometría.
- Estado de CPU nativa y contexto auxiliar Musashi, RAM, bancos/vectores,
  VRAM, paletas, timers, I/O, Z80 y YM2610/ymfm; AST conserva posición fraccional,
  parser, pista y fades. El volumen permanece como preferencia del frontend.
  No se guardan punteros, FILE, rutas, ROM, padding ni contadores de perfil.
- La validación precede a toda modificación; las aperturas AST necesarias se
  preparan antes de aplicar el estado. Rechazos de buffers cortos, versión,
  identidad, geometría, CRC y campos inválidos dejan la partida intacta.
- Se eliminó la consulta a `time()` del RTC: época fija 2000-01-01 UTC y avance
  por ciclos emulados. El modo AES conserva los resultados de referencia.
- La revisión de determinismo detectó un defecto previo de los márgenes:
  `realloc(s_cols)` movía píxeles cuyas direcciones ya estaban en la lista de
  sprites. ASan reprodujo un heap-use-after-free con VRAM sintética y los PNG
  públicos. Ahora son nodos de dirección estable, con prueba fría/caliente y
  descarga repetida. Tras corregirlo, reapareció el hash de referencia 400
  `85679d3374b1f792`; 320 sigue en `7bb518b96a1bed7a`.

Verificación terminada:

- CTest local 9/9 y pública sin ROM/SDL 7/7. Cobertura de formato/CRC/bounds,
  AST sintético, caché de gráficos, ciclo de vida y contrato del `.so` real.
- Checkpoints 0, 1, 20, 180, 600, 1200, 2280, 2299, 2390, 3650, 4200 y 4500:
  guardar, ejecutar, perturbar el futuro, restaurar y repetir produce bytes
  de estado, imagen y audio idénticos. Funciona con 320/400, AES y AST, también
  después de reset, descarga/nueva carga y en un proceso nuevo con otras rutas.
- Pruebas específicas AST: loops/one-shot, fase fraccional, fade a mitad,
  comandos con argumentos, cambio de archivo y apertura fallida sin mutación.
  Las huellas del paquete se calculan al cargar, sin volver a leerlo al guardar.
- 500 pares guardar/restaurar y 120 correcciones de seis frames. Release en
  este host: ~3,4 ms/par; comprobación doble de seis frames ~20–22 ms incluyendo
  ambas simulaciones, serializaciones adicionales y hashes. No es la medida
  del coste máximo de rollback en red; se deben medir percentiles después.
- Contrato dentro del Flatpak con AST aprobado, incluida restauración. La
  prueba de estados/rollback del motor también pasó en su runtime KDE 6.11.
- ASan+UBSan+LeakSanitizer: suite pública 7/7 y harness completo de estados con
  AST aprobados. Se instrumentaron motor/serializador/caché; el C recompilado
  del juego se reutilizó como objetos Release, sin publicarlo. LSan no funciona
  dentro del sandbox con ptrace; las ejecuciones se hicieron fuera de él.
- Standalone: PNG/WAV de 1 200 frames y combate de 4 200 frames idénticos a la
  referencia. Los hashes completos vuelven a coincidir tras corregir la caché.
- RetroArch Flatpak 1.22.2 real: guardar, pausa, reset y cargar por su interfaz
  local de comandos, con configuración/directorio privados y drivers nulos.
  Archivo `.state` de 282 688 bytes, payload del core en offset 16 y CRC
  comprobado con zlib. El log confirmó reset y carga; `retroarch.cfg` no cambió.
  No equivale a probar visualmente F2/F4 ni escuchar una restauración real.

Los intentos gráficos de automatización dependían del foco/contexto Wayland.
La prueba aislada evitó esa dependencia; se desactivó la confirmación de doble
pulsación solo en su configuración. Sus ajustes de red/control no se copiaron
al lanzador normal. Se comprobó que las opciones personales actuales (AST
automático, volumen 50, panorámica desactivada) permanecen sin cambios.

Se instaló core 0.3.0 y `.info` con `savestate=true`, `serialized` y quirk
`PLATFORM_DEPENDENT`. La caché de metadatos exclusiva del lanzador se invalida
al actualizar. La configuración actual usa F2 para guardar y F4 para cargar;
las acciones del menú rápido siguen disponibles. La referencia original y la
AppImage no se modificaron. Binarios/estados/capturas permanecen privados.

Siguiente hito: medir y verificar run-ahead/rewind y rollback de RetroArch con
sesiones reproducibles, después dos equipos x86_64 con latencia/pérdida/jitter.
No marcar `deterministic` en `.info` ni prometer interoperabilidad con GGPO o
Fightcade sin esas comprobaciones. No hay persistencia automática de SRAM aún.

Toolkit de esta fase fijado en `31b1c00188397935cb51fa4d376b46b34a4875ea`; corrección de caché del juego `84469d0`.
