"""Run AST tests with tiny synthetic, freely generated audio fixtures."""
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import wave
with tempfile.TemporaryDirectory(prefix='ng-ast-test-') as tmp:
    root=Path(tmp)
    for number in (13,14):
        with wave.open(str(root/f'track{number}.wav'),'wb') as f:
            f.setparams((2,2,44100,0,'NONE','not compressed'))
            f.writeframes(struct.pack('<8h',1000,-1000,2000,-2000,3000,-3000,4000,-4000))
    (root/'soundtrack.txt').write_text('NGAST 1 kof98 cbf43926\nmusic 30 13 1 track13.wav\nmusic 31 14 0 track14.wav\nnative 20\narg 0a 1\narg 0e 1\narg 14 2\n')
    subprocess.run([sys.argv[1],str(root)],check=True)
    manifest=(root/'soundtrack.txt').read_text()
    invalid=[manifest.replace('cbf43926','00000000'),
             manifest.replace('NGAST 1','NGAST 9'),
             manifest.replace('track13.wav','missing.wav'),
             manifest.replace('track13.wav','../track13.wav'),
             manifest+'music 30 13 1 track13.wav\n']
    for text in invalid:
        (root/'soundtrack.txt').write_text(text)
        subprocess.run([sys.argv[1],str(root),'reject'],check=True)
    (root/'soundtrack.txt').write_text(manifest)
    (root/'track13.wav').write_bytes(b'RIFF\x00\x00\x00\x00WAVE')
    subprocess.run([sys.argv[1],str(root),'reject'],check=True)
