"""v0.9.0.3.6.5: Crystal-derived night hue families, safe Pallet-only toggle."""
from pathlib import Path
import struct
ROOT = Path(__file__).resolve().parents[1]
NIGHT = ROOT / 'data/tilesets/night/pallet_town'
BASELINES = {'general':'primary/general', 'pallet_town':'secondary/pallet_town'}

for name, base in BASELINES.items():
    lines = (NIGHT / f'{name}.pal').read_text().splitlines()
    assert lines[:3] == ['JASC-PAL','0100','256'] and len(lines) == 259
    colours=[tuple(map(int,l.split())) for l in lines[3:]]
    assert all(len(c)==3 and all(0<=n<=248 and n%8==0 for n in c) for c in colours)
    encoded=struct.pack('<256H',*(r//8|((g//8)<<5)|((b//8)<<10) for r,g,b in colours))
    assert encoded==(NIGHT/f'{name}.gbapal').read_bytes()
    day=b''.join((ROOT/'data/tilesets'/base/'palettes'/f'{i:02d}.gbapal').read_bytes() for i in range(16))
    original=struct.unpack('<256H',day)
    moonlit=struct.unpack('<256H',encoded)
    preserved={0,0x7c1f,0x001f,0x03ff,0x7c00}
    assert all(a==b for a,b in zip(original,moonlit) if a in preserved)
    live=[(a,b) for a,b in zip(original,moonlit) if a not in preserved]
    assert all(a!=b for a,b in live)
    def lum(c):
        return .2126*(c&31)+.7152*((c>>5)&31)+.0722*((c>>10)&31)
    ratio=sum(lum(b) for a,b in live)/sum(lum(a) for a,b in live)
    assert .68 < ratio < .84, (name, ratio)
    # Crystal's night greens become teal and lavender (B >= G), not vivid green.
    greens=[b for a,b in live if ((a>>5)&31) > (a&31)*1.20 and ((a>>5)&31) > ((a>>10)&31)*1.12]
    assert len(greens)>=5
    assert sum(((b>>10)&31)>((b>>5)&31) for b in greens) / len(greens) > .85
    # The original source remains separate; shadows and highlights survive.
    assert max(lum(b) for a,b in live)-min(lum(b) for a,b in live)>13

fieldmap=(ROOT/'src/fieldmap.c').read_text()
assert '#define PALLET_NIGHT_PREVIEW_MODE 3' in fieldmap
assert 'PalletTown_TryToggleLivePreview' in fieldmap
assert 'gQuestLogState == QL_STATE_PLAYBACK_LAST' in fieldmap
assert 'MAP_GROUP(PALLET_TOWN)' in fieldmap
assert 'paletteData = sPalletNightGeneral' in fieldmap
assert 'paletteData = sPalletNightSecondary' in fieldmap
assert not (ROOT/'src/day_night.c').exists()
assert [p for p in ROOT.rglob('*.md') if p.name != 'README.md'] == []
print('PASS: Crystal-inspired Pallet night palettes, JASC assets, preserved markers, hue/luminance, no new rendering hooks')
