"""Verify v0.9.0.3.14 Safari day/night selection using actual C functions."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
field = (ROOT / 'src/fieldmap.c').read_text()


def function(name):
    match = re.search(r'(?:static )?(?:const u16 \*\s*|bool8\s+|u8\s+)' + name + r'\s*\([^)]*\)\s*\{', field)
    assert match, name
    start = match.start()
    depth = 0
    for i in range(match.end() - 1, len(field)):
        if field[i] == '{':
            depth += 1
        elif field[i] == '}':
            depth -= 1
            if depth == 0:
                return field[start:i + 1]
    raise AssertionError(name)


# The four Safari layouts all reuse both already approved night palette banks.
layouts = {e['id']: e for e in json.loads((ROOT / 'data/layouts/layouts.json').read_text())['layouts']}
maps = {data['id']: data for p in (ROOT / 'data/maps').glob('*/map.json') for data in [json.loads(p.read_text())]}
safari = ['MAP_SAFARI_ZONE_CENTER', 'MAP_SAFARI_ZONE_EAST', 'MAP_SAFARI_ZONE_NORTH', 'MAP_SAFARI_ZONE_WEST']
for map_name in safari:
    layout = layouts[maps[map_name]['layout']]
    assert layout['primary_tileset'] == 'gTileset_General', (map_name, layout)
    assert layout['secondary_tileset'] == 'gTileset_FuchsiaCity', (map_name, layout)

# All approved night palettes remain EXACTLY identical to last visual candidate.
approved = {
    'data/tilesets/night/pallet_town/general.gbapal': '2e5321cf3fb6731373ac74ab36782767cef5a482632f51f9d244141cc0c5611a',
    'data/tilesets/night/pallet_town/pallet_town.gbapal': '5b88014375800233d125a8226f441583af0e3a1d677b3b74d695dab71ea8d2b5',
    'data/tilesets/night/outdoor/fuchsia_city/fuchsia_city.gbapal': '8e17a7960da7f46a237f000d657a42f361f1c30f4e45583a4947f4b9f6eff8fe',
}
for rel, expected in approved.items():
    assert hashlib.sha256((ROOT / rel).read_bytes()).hexdigest() == expected, rel

# Ensure normal Kanto coverage, dynamic weather and the selected 20% rain odds are unchanged.
weather = (ROOT / 'src/field_weather_util.c').read_text()
assert 'roll < 80' in weather and 'roll < 97' in weather
assert 'sNightFuchsiaCity' in field and 'sNightGeneral' in field
assert 'TimeOfDay_RefreshBackgroundPalettes();' in field
assert 'TimeOfDay_RestoreAfterQuestLog(void)' in field

names = re.findall(r'static const u16 (sNight\w+)\[\]', field)
assert len(names) >= 12
palette_declarations = '\n'.join(f'static const u16 {n}[] = {{{100 + i}}};' for i,n in enumerate(names))

c = '''#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include "constants/map_groups.h"
typedef uint8_t u8; typedef uint16_t u16; typedef uint8_t bool8;
#define TRUE 1
#define FALSE 0
#define TOD_DAY 1
#define TOD_NIGHT 2
#define TOD_DEBUG_FORCE_PERIOD 0
#define QL_TINT_NONE 0
#define QL_TINT_BACKUP_GRAYSCALE 3
#define QL_STATE_PLAYBACK 1
#define QL_STATE_PLAYBACK_LAST 2
struct Tileset { int marker; };
struct MapLayout { struct Tileset *primaryTileset; struct Tileset *secondaryTileset; };
struct MapHeader { struct MapLayout *mapLayout; };
struct Location { u8 mapNum; u8 mapGroup; };
struct SaveBlock1 { struct Location location; };
struct SaveBlock1 saved;
struct SaveBlock1 *gSaveBlock1Ptr = &saved;
struct MapHeader gMapHeader;
struct Tileset gTileset_General, gTileset_FuchsiaCity;
struct Tileset gTileset_PalletTown, gTileset_ViridianCity, gTileset_PewterCity,
 gTileset_CeruleanCity, gTileset_LavenderTown, gTileset_VermilionCity,
 gTileset_CeladonCity, gTileset_CinnabarIsland, gTileset_IndigoPlateau,
 gTileset_SaffronCity;
static bool8 weatherEligible = TRUE;
static bool8 isNight = TRUE;
u8 gGlobalFieldTintMode = QL_TINT_NONE;
u8 gQuestLogState = 0;
static bool8 DynamicOverworldWeather_IsEligibleMap(void) { return weatherEligible; }
static bool8 TimeOfDay_IsNight(void) { return isNight; }
''' + palette_declarations + '\n' + function('TimeOfDay_IsEligibleMap') + '\n' + function('TimeOfDay_GetDesiredState') + '\n' + function('TimeOfDay_GetNightPalette') + '''
static void SetMap(u16 id) {
    saved.location.mapNum = id & 0xff;
    saved.location.mapGroup = id >> 8;
}
int main(void) {
    static struct MapLayout safari = {&gTileset_General, &gTileset_FuchsiaCity};
    static struct MapLayout alien = {&gTileset_General, &gTileset_PalletTown};
    u16 ids[] = { MAP_SAFARI_ZONE_CENTER, MAP_SAFARI_ZONE_EAST,
                  MAP_SAFARI_ZONE_NORTH, MAP_SAFARI_ZONE_WEST };
    unsigned i;
    gMapHeader.mapLayout = &safari;
    for (i = 0; i < sizeof(ids)/sizeof(ids[0]); i++) {
        SetMap(ids[i]);
        weatherEligible = TRUE;
        isNight = TRUE;
        assert(TimeOfDay_IsEligibleMap());
        assert(TimeOfDay_GetDesiredState() == TOD_NIGHT);
        assert(TimeOfDay_GetNightPalette(safari.primaryTileset) == sNightGeneral);
        assert(TimeOfDay_GetNightPalette(safari.secondaryTileset) == sNightFuchsiaCity);
        isNight = FALSE;
        assert(TimeOfDay_GetDesiredState() == TOD_DAY);
        isNight = TRUE;
        // Quest Log uses its own palettes except final grayscale backup.
        gQuestLogState = QL_STATE_PLAYBACK;
        assert(TimeOfDay_GetDesiredState() == TOD_DAY);
        gQuestLogState = QL_STATE_PLAYBACK_LAST;
        gGlobalFieldTintMode = QL_TINT_BACKUP_GRAYSCALE;
        assert(TimeOfDay_GetDesiredState() == TOD_NIGHT);
        gQuestLogState = 0;
        gGlobalFieldTintMode = QL_TINT_NONE;
        weatherEligible = FALSE;
        assert(!TimeOfDay_IsEligibleMap());
        weatherEligible = TRUE;
        gMapHeader.mapLayout = &alien;
        assert(!TimeOfDay_IsEligibleMap());
        gMapHeader.mapLayout = &safari;
    }
    // Rest houses and the Secret House stay unchanged, even with matching tilesets.
    SetMap(MAP_SAFARI_ZONE_CENTER_REST_HOUSE);
    weatherEligible = FALSE; // weather allowlist excludes all indoor Safari maps
    assert(!TimeOfDay_IsEligibleMap());
    SetMap(MAP_SAFARI_ZONE_SECRET_HOUSE);
    assert(!TimeOfDay_IsEligibleMap());
    // No save block or map layout must be a safe non-eligible state.
    gMapHeader.mapLayout = 0;
    assert(!TimeOfDay_IsEligibleMap());
    gMapHeader.mapLayout = &safari;
    gSaveBlock1Ptr = 0;
    assert(!TimeOfDay_IsEligibleMap());
    return 0;
}
'''

with tempfile.TemporaryDirectory() as td:
    cfile = Path(td) / 'safari_day_night.c'
    binary = Path(td) / 'safari_day_night'
    cfile.write_text(c)
    subprocess.run(['gcc', '-O2', '-std=gnu89', '-Wall', '-Wextra', '-Werror',
                    '-I', str(ROOT / 'include'), str(cfile), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)

print('PASS: all four Safari outdoor maps use existing General/Fuchsia Crystal night palettes;')
print('PASS: day, night, Quest Log final backup, special maps, weather eligibility and fail-safe checks')
