"""v0.9.0 integration assertions; run with python tests/test_day_night_v0900.py."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = (root / 'src/day_night.c').read_text()
world = (root / 'src/overworld.c').read_text()
linker = (root / 'ld_script.txt').read_text()
weather = (root / 'src/field_weather_util.c').read_text()
headers = (root / 'include/day_night.h').read_text()

assert '#define DAY_NIGHT_CYCLE_SECONDS (120 * 60)' in source
assert 'gSaveBlock2Ptr->playTimeHours' in source
assert 'gSaveBlock2Ptr->playTimeMinutes' in source
assert 'gSaveBlock2Ptr->playTimeSeconds' in source
assert 'DynamicOverworldWeather_IsEligibleMap()' in source
assert 'DayNight_TintPalette((volatile u16 *)PLTT, gPlttBufferFaded' in source
assert 'DayNight_ApplyOverworldPalette' in headers
assert world.count('DayNight_ApplyOverworldPalette();') == 1
vblank = world.split('static void VBlankCB_Field(void)\n{',1)[1].split('\n}',1)[0]
assert vblank.index('TransferPlttBuffer();') < vblank.index('DayNight_ApplyOverworldPalette();') < vblank.index('TransferTilesetAnimsBuffer();')
assert 'src/day_night.o(.text)' in linker and 'src/day_night.o(.rodata)' in linker
assert 'sym_ewram' not in source
assert 'case MAP_POKEMON_TOWER_3F:' not in weather
assert 'case MAP_SAFARI_ZONE_CENTER:' not in weather
assert 'case MAP_VIRIDIAN_FOREST:' not in weather
assert (root / 'include/global.h').is_file()
print('PASS: v0.9.0 clock, field-only palette hook, linker integration, protected maps, save compatibility')
