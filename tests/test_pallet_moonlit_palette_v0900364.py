"""Moonlit palette regression test: Pallet-only, source assets and RGB555 tones."""
from pathlib import Path
import struct
R=Path(__file__).resolve().parents[1]
assert (R/'src/fieldmap.c').read_text().count('#define PALLET_NIGHT_PREVIEW_MODE 3')==1
assert 'PalletTown_TryToggleLivePreview' in (R/'src/fieldmap.c').read_text()
assert not (R/'src/day_night.c').exists()
for bank,base in [('general','primary/general'),('pallet_town','secondary/pallet_town')]:
    d=R/'data/tilesets/night/pallet_town'
    lines=(d/(bank+'.pal')).read_text().splitlines()
    assert lines[:3]==['JASC-PAL','0100','256'] and len(lines)==259
    rgb=[tuple(map(int,line.split())) for line in lines[3:]]
    assert all(len(x)==3 and all(0<=v<=255 for v in x) for x in rgb)
    rebuilt=struct.pack('<256H',*(r//8 | (g//8)<<5 | (b//8)<<10 for r,g,b in rgb))
    assert rebuilt==(d/(bank+'.gbapal')).read_bytes()
    originals=b''.join((R/'data/tilesets'/base/'palettes'/f'{i:02}.gbapal').read_bytes() for i in range(16))
    old=struct.unpack('<256H',originals)
    new=struct.unpack('<256H',rebuilt)
    assert sum(a!=b for a,b in zip(old,new))>100
    assert all(new[i]==old[i] for i in range(256) if old[i] in (0,0x7c1f,0x001f,0x03ff,0x7c00))
    assert all((a&0x8000)==(b&0x8000) for a,b in zip(old,new))
    # Red roofing keeps identifiable burgundy chroma, foliage shifts blue-green.
    assert any((a&31)>(a>>5&31)*1.2 and (b&31)>(b>>5&31)*1.08 for a,b in zip(old,new) if a and b)
    assert any(((b>>10)&31)>((a>>10)&31) for a,b in zip(old,new))
assert [p for p in R.rglob('*.md') if p.name!='README.md']==[]
print('PASS: moonlit JASC/gbapal consistency, readable deep palette, markers, Pallet-only toggle and README-only policy')
