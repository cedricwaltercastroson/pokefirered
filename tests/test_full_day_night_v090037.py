"""v0.9.0.3.7 day/night source, coverage and compiled boundary tests."""
from pathlib import Path
import json
import re
import struct
import subprocess
import tempfile
import hashlib

ROOT = Path(__file__).resolve().parents[1]
field = (ROOT / 'src/fieldmap.c').read_text()
overworld = (ROOT / 'src/overworld.c').read_text()
names = ['pallet_town','viridian_city','pewter_city','cerulean_city','lavender_town','vermilion_city','celadon_city','fuchsia_city','cinnabar_island','indigo_plateau','saffron_city']
assert 'TimeOfDay_TryUpdateOutdoorPalette();' in overworld
assert 'PalletTown_TryToggleLivePreview' not in field + overworld
assert 'DynamicOverworldWeather_IsEligibleMap()' in field
assert 'TimeOfDay_IsEligibleMap()' in field
assert 'case MAP_SAFARI_ZONE_CENTER:' in field
assert 'gGlobalFieldTintMode != QL_TINT_NONE' in field
assert 'gQuestLogState == QL_STATE_PLAYBACK' in field
assert 'gQuestLogState == QL_STATE_PLAYBACK_LAST' in field
assert 'gPaletteFade.active' in field
assert 'ScriptContext2_IsEnabled()' in field
assert 'gWeatherPtr->palProcessingState != WEATHER_PAL_STATE_IDLE' in field
assert 'ApplyWeatherGammaShiftToPal(i)' in field
assert 'sLastLoadedTimeOfDay' in field and 'TOD_UNINITIALIZED' in field
assert 'gPlttBufferFaded' not in field[field.index('// v0.9.0.3.14:'):field.index('static void LoadTilesetPalette')]
assert not (ROOT/'src/day_night.c').exists()
assert [p for p in ROOT.rglob('*.md') if p.name != 'README.md'] == []

# Lock the exact user-approved Crystal palette SHA-256 values. These are
# self-contained, with no dependency on a neighbouring development folder.
approved = {
    'general': '2e5321cf3fb6731373ac74ab36782767cef5a482632f51f9d244141cc0c5611a',
    'pallet_town': '5b88014375800233d125a8226f441583af0e3a1d677b3b74d695dab71ea8d2b5',
}
for name, expected_hash in approved.items():
    rel = Path(f'data/tilesets/night/pallet_town/{name}.gbapal')
    raw = (ROOT/rel).read_bytes()
    assert len(raw) == 512
    assert hashlib.sha256(raw).hexdigest() == expected_hash
for name in names:
    if name == 'pallet_town':
        palette=ROOT/'data/tilesets/night/pallet_town/pallet_town.gbapal'
    else:
        palette=ROOT/f'data/tilesets/night/outdoor/{name}/{name}.gbapal'
        text=(ROOT/f'data/tilesets/night/outdoor/{name}/{name}.pal').read_bytes()
        assert text.startswith(b'JASC-PAL\r\n0100\r\n256\r\n')
        assert len(text.split(b'\r\n')) >= 260
        assert f'sNight{"".join(part.title() for part in name.split("_"))}' in field
    assert len(palette.read_bytes())==512
    assert len(struct.unpack('<256H',palette.read_bytes()))==256

weather=(ROOT/'src/field_weather_util.c').read_text()
start=weather.index('static bool8 IsDynamicWeatherMap(void)\n{')
stop=weather.index('// Expose the exact same allowlist')
map_ids=set(re.findall(r'case (MAP_\w+):',weather[start:stop]))
assert len(map_ids)==42  # 38 original outdoor maps + four Safari weather-only maps
night_map_ids = map_ids  # v0.9.0.3.14: the four Safari outdoor maps also use night palettes.
assert len(night_map_ids)==42
layouts={e['id']: e for e in json.loads((ROOT/'data/layouts/layouts.json').read_text())['layouts']}
maps={x['id']: x for f in (ROOT/'data/maps').glob('*/map.json') for x in [json.loads(f.read_text())]}
expected={'gTileset_'+''.join(w.title() for w in n.split('_')) for n in names}
for map_id in night_map_ids:
    layout=layouts[maps[map_id]['layout']]
    assert layout['primary_tileset']=='gTileset_General',map_id
    assert layout['secondary_tileset'] in expected,(map_id,layout['secondary_tileset'])

# Extract the actual production C clock helper, compile it and test the boundaries.
minute_match = re.search(r'static u32 TimeOfDay_GetGameMinuteAtSeconds\(u32 seconds\)\s*\{[^}]+\}',field)
assert minute_match
match = re.search(r'static bool8 TimeOfDay_IsNightAtSeconds\(u32 seconds\)\s*\{[^}]+\}',field)
assert match
c = '''#include <stdint.h>\ntypedef uint32_t u32; typedef uint8_t bool8;\n#define TOD_MINUTES_PER_CYCLE 1440\n#define TOD_DAY_START_MINUTES 480\n#define TOD_NIGHT_START_MINUTES 1200\n'''+minute_match.group(0)+'\n'+match.group(0)+'''
int main(void) {
    if (TimeOfDay_IsNightAtSeconds(0)) return 1;
    if (TimeOfDay_IsNightAtSeconds(1799)) return 2;
    if (!TimeOfDay_IsNightAtSeconds(1800)) return 3;
    if (!TimeOfDay_IsNightAtSeconds(3599)) return 4;
    if (TimeOfDay_IsNightAtSeconds(3600)) return 5;
    if (!TimeOfDay_IsNightAtSeconds(5400)) return 6;
    if (TimeOfDay_IsNightAtSeconds(7200)) return 7;
    return 0;
}
'''
with tempfile.TemporaryDirectory() as td:
    inp=Path(td)/'clock.c'
    inp.write_text(c)
    subprocess.run(['gcc','-Wall','-Wextra','-Werror','-std=gnu89',str(inp),'-o',str(Path(td)/'clock')],check=True)
    subprocess.run([str(Path(td)/'clock')],check=True)
print(f'PASS: {len(night_map_ids)} day/night outdoor maps (including 4 Safari); exact approved palettes; clock, quest log and fade guards')
