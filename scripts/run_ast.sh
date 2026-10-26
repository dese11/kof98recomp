#!/usr/bin/env bash
set -euo pipefail
project_dir="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
export KOF98_ASSETS="${KOF98_ASSETS:-$project_dir/assets}"
exec "$project_dir/build/kof98" --widescreen \
  --ast-path "$project_dir/soundtracks/kof98-cd-ast" "$@"
