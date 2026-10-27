"""v0.9.0.3.6 source-level invariants and executable clock boundary checks."""
from pathlib import Path
import re
import subprocess
import struct
import tempfile

ROOT=Path(__file__).resolve().parents[1]
src=(ROOT/'src/fieldmap.c').read_text()
assert 'PalletTown_IsNightAtSeconds' in src
assert 'PalletTown_ShouldLoadNightPalettes' in src
assert 'gGlobalFieldTintMode != QL_TINT_NONE' in src
assert 'gQuestLogState == QL_STATE_PLAYBACK' in src
assert 'gQuestLogState == QL_STATE_PLAYBACK_LAST' in src
assert 'MAP_GROUP(PALLET_TOWN)' in src and 'MAP_NUM(PALLET_TOWN)' in src
assert 'paletteData = sPalletNightGeneral' in src
assert 'paletteData = sPalletNightSecondary' in src
assert 'Fieldmap_ApplyGlobalTintToPaletteEntries' in src
assert 'PalletTown_ShouldLoadNightPalettes()' in src
assert 'gPlttBufferFaded[' not in src[src.index('// v0.9.0.3.6:'):src.index('static void LoadTilesetPalette')]
assert 'DayNight_' not in (ROOT/'src/overworld.c').read_text()
assert not (ROOT/'src/day_night.c').exists()
assert (ROOT/'src/roamer.c').read_text().count('SPECIES_MEW') > 0

# Load and compare both new preauthored palette banks against original sources.
for name, orig in [('general','data/tilesets/primary/general/palettes'),('pallet_town','data/tilesets/secondary/pallet_town/palettes')]:
    night=(ROOT/f'data/tilesets/night/pallet_town/{name}.gbapal').read_bytes()
    day=b''.join((ROOT/orig/(f'{i:02d}.gbapal')).read_bytes() for i in range(16))
    assert len(day)==512==len(night), name
    o=struct.unpack('<256H',day)
    n=struct.unpack('<256H',night)
    assert sum(a!=b for a,b in zip(o,n))>100
    assert all((a&32768)==(b&32768) for a,b in zip(o,n))
    # Moonlit, authored palette variants may raise a blue component to
    # change hue; absolute channel-by-channel dimming was the retired system.
    visible=[(a,b) for a,b in zip(o,n) if a not in (0,0x7c1f,0x001f,0x03ff,0x7c00)]
    assert all(a!=b for a,b in visible)
    def luma(c):
        return 0.2126*(c&31)+0.7152*((c>>5)&31)+0.0722*((c>>10)&31)
    ratio=[luma(b)/luma(a) for a,b in visible if luma(a)>0]
    assert 0.60 < sum(ratio)/len(ratio) < 0.92, ratio
    assert min(ratio) > 0.55, min(ratio)
    # Cool-toned neutrals and green vegetation are no longer flat RGB multiples.
    assert sum((((b>>10)&31)*a > ((a>>10)&31)*b) for a,b in visible if a and b) > 35

# Compile and execute the original unmodified clock helper, not a hand-coded copy.
m=re.search(r'static bool8 PalletTown_IsNightAtSeconds\(u32 totalSeconds\)\s*\{[^}]+\}',src)
assert m
c='''#include <stdint.h>\ntypedef uint32_t u32; typedef uint8_t bool8;\n'''+m.group(0)+'''
int main(void) {
    if (PalletTown_IsNightAtSeconds(0)) return 1; // 08:00 = day
    if (PalletTown_IsNightAtSeconds(3599)) return 2; // 19:59 = day
    if (!PalletTown_IsNightAtSeconds(3600)) return 3; // 20:00 = night
    if (!PalletTown_IsNightAtSeconds(7199)) return 4; // 07:59 = night
    if (PalletTown_IsNightAtSeconds(7200)) return 5; // 08:00 = day
    if (PalletTown_IsNightAtSeconds(14400)) return 6; // another 2 days = day
    return 0;
}
'''
with tempfile.TemporaryDirectory() as folder:
    path=Path(folder); (path/'clock.c').write_text(c)
    subprocess.run(['gcc','-Wall','-Wextra','-Werror','-std=c11','clock.c','-o','clock'],cwd=path,check=True)
    subprocess.run([str(path/'clock')],check=True)

# Future releases must contain only README.md among Markdown docs.
assert [p for p in ROOT.rglob('*.md') if p.name!='README.md']==[]
print('PASS: Gen2-style Pallet Town map-load source palettes, quest log guards, 2-hour clock, archive documentation')
