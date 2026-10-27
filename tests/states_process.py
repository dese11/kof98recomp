#!/usr/bin/env python3
"""Snapshots survive a new address space and a different pack path."""
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import zlib

harness, rom = map(Path, sys.argv[1:3])
ast = sys.argv[3] if len(sys.argv) > 3 else '-'
with tempfile.TemporaryDirectory(prefix='kof98-state-process-') as tmp:
    state = Path(tmp) / 'snapshot.dat'
    alias=Path(tmp)/'ROM'; alias.symlink_to(rom.resolve(),target_is_directory=True)
    music_alias=Path(tmp)/'AST'
    if ast!='-': music_alias.symlink_to(Path(ast).resolve(),target_is_directory=True)
    for mode in ('write', 'read'):
        content=rom if mode=='write' else alias
        music=ast if mode=='write' or ast=='-' else str(music_alias)
        subprocess.run([str(harness), str(content), music, mode, '400', str(state)], check=True)
    data = state.read_bytes()[8:]  # test file begins with the expected output hash
    magic, version, length = struct.unpack_from('<8sII', data)
    assert magic == b'NGSTATE\x00' and version == 1
    assert len(data) == length * 2
    assert zlib.crc32(data[48:length]) == struct.unpack_from('<I', data, 40)[0]
print('cross-process state/video/audio and independent wire checksum passed')
