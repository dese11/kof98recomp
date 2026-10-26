#!/usr/bin/env python3
"""Validate SHA-256 padding boundaries and failures using entirely synthetic data."""
import hashlib
import subprocess
import sys
import tempfile
from pathlib import Path

compiler, repo = sys.argv[1], Path(sys.argv[2])
with tempfile.TemporaryDirectory(prefix='kof98-identity-') as tmp:
    root = Path(tmp)
    files = {'neo-po.bin': b'', 'sfix.sfix': b'a', 'sm1.sm1': b'abc', '000-lo.lo': b'x' * 55}
    files.update({f'vector-{n}': bytes(i % 251 for i in range(n)) for n in (56, 63, 64, 65, 127, 128, 129, 10000)})
    header = 'static const struct rom_identity k_rom_identity[] = {\n'
    for name, data in files.items():
        (root / name).write_bytes(data)
        header += f'{{"{name}", {len(data)}, "{hashlib.sha256(data).hexdigest()}", 0}},\n'
    (root / 'rom_identity.h').write_text(header + '};\n')
    (root / 'main.c').write_text('''
#include "content.h"
#include <stdio.h>
int main(int argc, char **argv) {
    char bios[1024], error[1400];
    if (argc!=2) return 2;
    int result=kof98_validate_content(argv[1], NULL, bios, sizeof(bios), error, sizeof(error));
    if (result) puts(error);
    return result ? 1 : 0;
}
''')
    executable = root / 'vectors'
    subprocess.run([compiler, '-std=c11', '-O1', '-DKOF98_HAVE_RECOMP', '-I', str(root),
                    '-I', str(repo / 'libretro'), str(repo / 'libretro/content.c'),
                    str(root / 'main.c'), '-o', str(executable)], check=True)
    subprocess.run([str(executable), str(root)], check=True)
    victim = root / 'vector-65'
    victim.write_bytes(b'z' * 65)
    bad = subprocess.run([str(executable), str(root)], capture_output=True, text=True)
    assert bad.returncode == 1 and 'Identidad distinta: vector-65' in bad.stdout, bad
    victim.write_bytes(b'')
    bad = subprocess.run([str(executable), str(root)], capture_output=True, text=True)
    assert bad.returncode == 1 and 'Tamano incorrecto: vector-65' in bad.stdout, bad
    victim.unlink()
    bad = subprocess.run([str(executable), str(root)], capture_output=True, text=True)
    assert bad.returncode == 1 and 'Falta vector-65' in bad.stdout, bad
print('SHA-256 padding, identity, size and missing-file vectors passed')
