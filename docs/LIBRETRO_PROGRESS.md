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
