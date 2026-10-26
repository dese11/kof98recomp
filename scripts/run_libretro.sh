#!/usr/bin/env bash
set -euo pipefail
launcher="$HOME/.var/app/org.libretro.RetroArch/config/retroarch/kof98-recomp/run.sh"
if [ ! -x "$launcher" ]; then
    echo "Instala primero el core con scripts/install_libretro.py; consulta docs/libretro.md." >&2
    exit 1
fi
exec "$launcher" "$@"
