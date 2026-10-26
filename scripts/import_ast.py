#!/usr/bin/env python3
"""Build a private KOF98 AST pack from a user's Neo Geo CD BIN/CUE or 7z.

Only this tool is distributable. Extracted drivers, mapping data and audio
stay in the output pack, which must never be committed or uploaded.
"""
import argparse
import hashlib
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import wave

CD_DRIVER_SHA256 = '27cbfc301498ca7653fd7d3edd44c3108763906eb7fe43badd1e3eb325b5aaf7'
AES_M1_CRC32 = '4e7a6b1b'
ARGUMENTS = {0x0a: 1, 0x0e: 1, 0x14: 2, 0x16: 1, 0x17: 1,
             0x18: 1, 0x1a: 1, 0x1b: 1, 0x1c: 1, 0x1d: 1, 0x1e: 1}

def cue_tracks(cue):
    tracks = []
    file = None
    for line in cue.read_text(encoding='utf-8-sig').splitlines():
        line = line.strip()
        if line.startswith('FILE '):
            match = re.fullmatch(r'FILE "([^"\n]+)" BINARY', line)
            if not match:
                raise ValueError('Only separate BINARY BIN files are supported')
            name = match[1]
            if Path(name).name != name or '\\' in name or name in ('.', '..'):
                raise ValueError('CUE file names must stay in the CUE directory')
            file = cue.parent / name
            if file.is_symlink() or not file.is_file():
                raise ValueError(f'Missing regular BIN file: {file}')
        elif line.startswith('TRACK '):
            m = re.fullmatch(r'TRACK (\d+) (AUDIO|MODE1/2352)', line)
            if not m or file is None:
                raise ValueError('Unsupported track layout')
            tracks.append({'number': int(m[1]), 'kind': m[2], 'file': file, 'index': None})
        elif line.startswith('INDEX 01 '):
            m = re.fullmatch(r'INDEX 01 (\d+):(\d+):(\d+)', line)
            if not m or not tracks or int(m[2]) >= 60 or int(m[3]) >= 75:
                raise ValueError('Invalid INDEX 01')
            tracks[-1]['index'] = (int(m[1])*60+int(m[2]))*75+int(m[3])
    if any(t['index'] is None for t in tracks) or len({t['file'] for t in tracks}) != len(tracks):
        raise ValueError('Expected one BIN file per track and an INDEX 01 for each')
    if [t['number'] for t in tracks] != list(range(1, 41)) or tracks[0]['kind'] != 'MODE1/2352' or any(t['kind'] != 'AUDIO' for t in tracks[1:]):
        raise ValueError('Expected this KOF98 CD edition: data track 01, audio tracks 02..40')
    return tracks

def cd_driver(track):
    """Read KF98.Z80 from the root of a raw MODE1 ISO9660 data track."""
    with track['file'].open('rb') as f:
        def read(offset, size):
            data = bytearray()
            while size:
                sector, within = divmod(offset, 2048)
                n = min(size, 2048-within)
                f.seek((track['index']+sector)*2352+16+within)
                part = f.read(n)
                if len(part) != n:
                    raise ValueError('Truncated data track')
                data.extend(part)
                offset += n
                size -= n
            return bytes(data)
        descriptor = read(16*2048, 2048)
        if descriptor[:7] != b'\x01CD001\x01':
            raise ValueError('Expected a MODE1 ISO9660 volume')
        extent = struct.unpack_from('<I', descriptor, 158)[0]
        size = struct.unpack_from('<I', descriptor, 166)[0]
        if size > 1024*1024:
            raise ValueError('Unexpected ISO root size')
        directory = read(extent*2048, size)
        pos = 0
        while pos < len(directory):
            n = directory[pos]
            if not n:
                pos = (pos//2048+1)*2048
                continue
            entry = directory[pos:pos+n]
            if n < 34 or len(entry) != n:
                raise ValueError('Invalid ISO directory')
            name = entry[33:33+entry[32]].split(b';')[0]
            if name == b'KF98.Z80':
                extent = struct.unpack_from('<I', entry, 2)[0]
                size = struct.unpack_from('<I', entry, 10)[0]
                if size > 65536:
                    raise ValueError('Unexpected sound driver size')
                return read(extent*2048, size)
            pos += n
    raise ValueError('KF98.Z80 not found on the disc')

def mapping(driver):
    if hashlib.sha256(driver).hexdigest() != CD_DRIVER_SHA256:
        raise ValueError('Unsupported CD sound-driver revision; do not guess its command table')
    rows = []
    for command in range(0x20, 0x60):
        category = driver[0x3038+command-0x20]
        if category not in (2, 6):
            continue
        kind, bcd = driver[0x329e+2*(command-0x20):0x32a0+2*(command-0x20)]
        number = (bcd >> 4)*10+(bcd & 15)
        if category == 6 and kind in (0, 1, 4, 5) and 2 <= number <= 40 and (bcd & 15) < 10:
            rows.append(f'music {command:02x} {number} {int(kind in (0, 4))} track{number:02}.wav')
        else:
            rows.append(f'native {command:02x}')
    return rows

def write_track(track, output, swap_audio):
    offset = track['index']*2352
    size = track['file'].stat().st_size-offset
    if size <= 0 or size % 2352:
        raise ValueError(f'Incomplete CDDA sectors: {track["file"]}')
    with track['file'].open('rb') as source, wave.open(str(output), 'wb') as dest:
        dest.setnchannels(2)
        dest.setsampwidth(2)
        dest.setframerate(44100)
        source.seek(offset)
        while True:
            data = source.read(1024*1024)
            if not data:
                break
            if swap_audio:
                b = bytearray(data)
                b[0::2], b[1::2] = data[1::2], data[0::2]
                data = b
            dest.writeframesraw(data)

def build_pack(cue, output, swap_audio=False):
    tracks = cue_tracks(cue)
    rows = mapping(cd_driver(tracks[0]))
    output.mkdir(parents=True, exist_ok=False)
    manifest = ['NGAST 1 kof98 '+AES_M1_CRC32,
                '# Generated locally from the user CD. Do not distribute this pack.',
                '# command (hex), CD track (decimal), loop (0/1), local PCM16 WAV', *rows]
    manifest += [f'arg {cmd:02x} {count}' for cmd, count in ARGUMENTS.items()]
    for track in tracks[1:]:
        write_track(track, output/f'track{track["number"]:02}.wav', swap_audio)
    (output/'soundtrack.txt').write_text('\n'.join(manifest)+'\n')
    checksum = []
    for file in sorted(output.glob('*.wav')):
        with file.open('rb') as f:
            digest = hashlib.file_digest(f, 'sha256').hexdigest()
        checksum.append(f'{digest}  {file.name}')
    (output/'SHA256SUMS').write_text('\n'.join(checksum)+'\n')
    return sum(row.startswith('music ') for row in rows)

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('source', type=Path, help='local .7z archive or separate BIN/CUE .cue file')
    p.add_argument('--output', type=Path, required=True, help='new private soundtrack pack directory')
    p.add_argument('--swap-audio', action='store_true', help='only for big-endian audio dumps; supplied Redump BIN is little-endian')
    args = p.parse_args()
    source = args.source.expanduser().resolve(strict=True)
    output = args.output.expanduser().absolute()
    if output.exists():
        p.error('Output already exists; use a new directory')
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='kof98-ast-import-') as tmp:
        tmp = Path(tmp)
        cue = source
        if source.suffix.lower() == '.7z':
            unpacked = tmp/'disc'
            subprocess.run(['7z', 'x', '-y', '-bso0', f'-o{unpacked}', str(source), '*.cue', '*.bin', '-r'], check=True)
            cues = list(unpacked.rglob('*.cue'))
            if len(cues) != 1:
                raise ValueError('Archive must contain exactly one CUE')
            cue = cues[0]
        elif source.suffix.lower() != '.cue':
            p.error('Use a .7z or .cue source')
        # A sibling staging directory keeps the final rename on one filesystem.
        with tempfile.TemporaryDirectory(prefix='.kof98-ast-', dir=output.parent) as staging:
            pack = Path(staging)/'pack'
            count = build_pack(cue, pack, args.swap_audio)
            pack.rename(output)
    print(f'AST pack ready: {output} (39 audio tracks, {count} mapped music commands)')

if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        raise SystemExit(f'AST import failed: {error}')
