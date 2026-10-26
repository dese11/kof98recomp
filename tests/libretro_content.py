#!/usr/bin/env python3
"""Negative content tests use local temporary copies, never modify the input set."""
import subprocess
import sys
import tempfile
from pathlib import Path

harness, core, source = map(Path, sys.argv[1:])
with tempfile.TemporaryDirectory(prefix='kof98-content-') as tmp:
    root = Path(tmp)
    for path in source.iterdir():
        if path.is_file():
            (root / path.name).symlink_to(path.resolve())

    def reject(expected):
        subprocess.run([str(harness), str(core), '--reject', str(root / '242-pn1.bin'), expected], check=True)

    (root / '242-v4.bin').unlink()
    reject('Falta 242-v4.bin')
    (root / '242-v4.bin').write_bytes(b'invalid')
    reject('Tamano incorrecto: 242-v4.bin')
    (root / '242-v4.bin').unlink()
    (root / '242-v4.bin').symlink_to((source / '242-v4.bin').resolve())
    data = bytearray((source / '242-pn1.bin').read_bytes())
    data[0x1234] ^= 1
    (root / '242-pn1.bin').unlink()
    (root / '242-pn1.bin').write_bytes(data)
    reject('Identidad distinta: 242-pn1.bin')
    (root / 'neo-po.bin').unlink()
    reject('BIOS incompleta')
print('content missing/size/identity/BIOS rejection passed')
