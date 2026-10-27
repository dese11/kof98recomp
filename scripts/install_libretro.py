#!/usr/bin/env python3
"""Install this locally built core for RetroArch Flatpak, without replacing its config."""
import argparse
import hashlib
import json
import re
import shlex
import shutil
from pathlib import Path

APP = 'org.libretro.RetroArch'
ROOT = Path(__file__).resolve().parent.parent


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def config_path(text, key, fallback):
    match = re.search(r'^\s*' + re.escape(key) + r'\s*=\s*"([^"]*)"\s*$', text, re.M)
    value = match.group(1) if match else ''
    path = Path(value).expanduser() if value and value != 'default' else fallback
    if not path.is_absolute():
        raise ValueError(f'{key}: se requiere una ruta absoluta: {path}')
    return path


def install(args):
    cfg = args.config.expanduser().resolve()
    text = cfg.read_text()
    base = cfg.parent
    managed = base / 'kof98-recomp'
    record = managed / 'install.json'
    previous = json.loads(record.read_text()) if record.exists() else {}
    owned = previous.get('files', {})
    new = dict(owned)

    def write(path, data, executable=False):
        name = str(path)
        if path.is_symlink():
            raise ValueError(f'No se reemplaza el enlace existente: {path}')
        existed = path.exists()
        if existed:
            current = digest(path)
            if current != hashlib.sha256(data).hexdigest() and owned.get(name) != current:
                raise ValueError(f'Archivo ajeno o modificado; no se reemplaza: {path}')
        path.parent.mkdir(parents=True, exist_ok=True)
        temp = path.with_name(path.name + '.kof98-new')
        temp.write_bytes(data)
        temp.chmod(0o755 if executable else 0o644)
        temp.replace(path)
        if not existed or name in owned:
            new[name] = digest(path)

    core_dir = config_path(text, 'libretro_directory', base / 'cores')
    system = config_path(text, 'system_directory', base / 'system')
    core = core_dir / 'kof98_libretro.so'
    content = (args.rom_dir.expanduser().resolve() / '242-pn1.bin')
    if not content.is_file():
        raise ValueError(f'Falta el contenido: {content}')
    source = args.build_dir.expanduser().resolve() / 'kof98_libretro.so'
    if not source.is_file():
        raise ValueError(f'Primero compila el core: {source}')
    # Preflight local resources before writing anything.
    ast_target = system / 'kof98' / 'ast'
    ast = args.ast_dir.expanduser().resolve() if args.ast_dir else None
    if ast and not (ast / 'soundtrack.txt').is_file():
        raise ValueError(f'Falta soundtrack.txt: {ast}')
    if ast and (ast_target.exists() or ast_target.is_symlink()):
        if not ast_target.is_symlink() or ast_target.resolve() != ast:
            raise ValueError(f'Ya existe otro paquete AST: {ast_target}')
    write(core, source.read_bytes(), True)
    write(managed / 'info' / 'kof98_libretro.info', (ROOT / 'libretro/kof98_libretro.info').read_bytes())
    # This directory belongs exclusively to this launcher. Rebuild cached
    # capabilities after an update (e.g. newly available save states).
    cache = managed / 'info' / 'core_info.cache'
    if cache.is_file() and not cache.is_symlink():
        cache.unlink()
    for name in ('stone.png', 'stone-red.png'):
        write(system / 'kof98' / 'assets' / name, (ROOT / 'assets' / name).read_bytes())
    links = dict(previous.get('links', {}))
    if ast:
        ast_target.parent.mkdir(parents=True, exist_ok=True)
        if not ast_target.is_symlink():
            ast_target.symlink_to(ast, target_is_directory=True)
            links[str(ast_target)] = str(ast)
    overlay = managed / 'launch.cfg'
    # Only this launcher uses the supplemental info directory. Never persist
    # its temporary directory choice over the user's main RetroArch config.
    write(overlay, (f'libretro_info_path = "{managed / "info"}"\n'
                    'config_save_on_exit = "false"\n').encode())
    launcher = managed / 'run.sh'
    q = shlex.quote
    script = f'''#!/usr/bin/env bash
set -euo pipefail
runner=(flatpak)
if command -v host-spawn >/dev/null 2>&1 && {{ [ -f /run/.containerenv ] || [ -n "${{CONTAINER_ID:-}}" ]; }}; then
    runner=(host-spawn flatpak)
fi
exec "${{runner[@]}}" run {APP} --appendconfig={q(str(overlay))} \\
    -L {q(str(core))} "$@" {q(str(content))}
'''
    write(launcher, script.encode(), True)
    managed.mkdir(parents=True, exist_ok=True)
    record.write_text(json.dumps({'files': new, 'links': links}, indent=2) + '\n')
    print(f'Core instalado: {core}\nAST: {ast_target if ast else "sin agregar"}\nEjecuta:\n{launcher}')


def uninstall(args):
    record = args.config.expanduser().resolve().parent / 'kof98-recomp/install.json'
    state = json.loads(record.read_text())
    remaining = {'files': {}, 'links': {}}
    for name, expected in state.get('files', {}).items():
        path = Path(name)
        if not path.exists() and not path.is_symlink():
            continue
        if path.is_file() and not path.is_symlink() and digest(path) == expected:
            path.unlink()
        else:
            remaining['files'][name] = expected
            print(f'Se conserva el archivo modificado: {path}')
    for name, target in state.get('links', {}).items():
        path = Path(name)
        if path.is_symlink() and str(path.readlink()) == target:
            path.unlink()
        elif path.exists() or path.is_symlink():
            remaining['links'][name] = target
            print(f'Se conserva el enlace modificado: {path}')
    if any(remaining.values()):
        record.write_text(json.dumps(remaining, indent=2) + '\n')
    else:
        record.unlink()
    print('Desinstalacion terminada. Las ROM y el paquete de musica original se conservan.')


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('action', choices=('install', 'uninstall'))
    p.add_argument('--config', type=Path, default=Path.home() / f'.var/app/{APP}/config/retroarch/retroarch.cfg')
    p.add_argument('--build-dir', type=Path, default=ROOT / 'build')
    p.add_argument('--rom-dir', type=Path)
    p.add_argument('--ast-dir', type=Path)
    args = p.parse_args()
    if args.action == 'install' and not args.rom_dir:
        p.error('install requiere --rom-dir')
    try:
        install(args) if args.action == 'install' else uninstall(args)
    except (OSError, ValueError) as error:
        p.exit(1, f'{error}\n')
