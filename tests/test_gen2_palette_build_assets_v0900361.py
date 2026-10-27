"""v0.9.0.3.6.1: ensure the night palette source files are buildable."""
from pathlib import Path
import struct

root = Path(__file__).resolve().parents[1]
for base in ('general', 'pallet_town'):
    directory = root / 'data/tilesets/night/pallet_town'
    src = (directory / f'{base}.pal').read_bytes()
    assert b'\r\n' in src and src.endswith(b'\r\n')
    lines = src.split(b'\r\n')
    assert lines[:3] == [b'JASC-PAL', b'0100', b'256']
    assert len(lines) == 260 and lines[-1] == b''
    data = bytearray()
    for line in lines[3:-1]:
        r, g, b = (int(x) for x in line.split())
        assert all(0 <= x <= 255 for x in (r, g, b))
        rgb555 = (r // 8) | ((g // 8) << 5) | ((b // 8) << 10)
        data.extend(struct.pack('<H', rgb555))
    assert bytes(data) == (directory / f'{base}.gbapal').read_bytes()
print('PASS: 2 editable JASC night palettes exactly regenerate the included GBA palettes')
