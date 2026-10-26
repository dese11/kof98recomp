#!/usr/bin/env python3
"""Private build output: bind a local core to its input ROMs (never publish it)."""
import argparse
import hashlib
from pathlib import Path

GAME = {
    '242-pn1.bin': 0x100000, '242-p2.bin': 0x400000,
    **{f'242-c{i}.bin': 0x800000 for i in range(1, 9)},
    '242-s1.bin': 0x20000, '242-mg1.bin': 0x40000,
    **{f'242-v{i}.bin': 0x400000 for i in range(1, 5)},
}
BIOS = {'neo-po.bin': 0x20000, 'sfix.sfix': 0x20000,
        'sm1.sm1': 0x20000, '000-lo.lo': 0x20000}


def generate(root, output):
    lines = ['/* Generated locally from private ROMs. Do not distribute. */',
             'static const struct rom_identity k_rom_identity[] = {']
    for system, files in enumerate((GAME, BIOS)):
        for name, size in files.items():
            data = (root / name).read_bytes()
            if len(data) != size:
                raise ValueError(f'{name}: expected {size} bytes, got {len(data)}')
            digest = hashlib.sha256(data).hexdigest()
            lines.append(f'    {{"{name}", {size}, "{digest}", {system}}},')
    lines.append('};\n')
    output.parent.mkdir(parents=True, exist_ok=True)
    temp = output.with_suffix('.tmp')
    temp.write_text('\n'.join(lines), encoding='ascii')
    temp.replace(output)


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('rom_dir', type=Path)
    p.add_argument('output', type=Path)
    args = p.parse_args()
    generate(args.rom_dir, args.output)
