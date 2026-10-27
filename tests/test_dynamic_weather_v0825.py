"""v0.8.25: Source and map eligibility regression checks."""
from pathlib import Path
import json
import re

root = Path(__file__).resolve().parents[1]
weather = (root / 'src/field_weather_util.c').read_text()
engine = (root / 'src/field_weather.c').read_text()
weather_header = (root / 'include/field_weather.h').read_text()

assert 'DynamicOverworldWeather_Update();' in engine
assert 'void DynamicOverworldWeather_Update(void);' in weather_header
assert 'SetWeather(nextWeather)' in weather  # native gradual transition
assert 'gPaletteFade.active' in weather
assert 'gWeatherPtr->currWeather != gWeatherPtr->nextWeather' in weather
assert 'gSaveBlock1Ptr->weather != sDynamicWeather.weather' in weather
assert 'gMapHeader.weather != WEATHER_SUNNY' in weather
assert 'NextDynamicWeatherRandom' in weather
assert re.search(r'\bRandom\s*\(', weather) is None  # do not perturb the battle RNG

allowed = set(re.findall(r'case MAP_([A-Z0-9_]+):', weather.split('static bool8 IsDynamicWeatherMap(void)\n{', 1)[1].split('// Independent PRNG:', 1)[0]))
map_file_weather = {}
for file in (root / 'data/maps').glob('*/map.json'):
    data = json.loads(file.read_text())
    # Only map group 3 (ordinary Kanto towns and routes) is allowed.
    # All other maps, including outdoor-looking dungeons, are excluded.
    name = re.sub(r'([a-z0-9])([A-Z])', r'\1_\2', file.parent.name).upper()
    name = re.sub(r'ROUTE(\d+)', r'ROUTE\1', name)
    name = re.sub(r'_(\d+)_F\b', r'_\1F', name)
    map_file_weather[name] = data.get('weather')

# All allowlisted symbols are the 38 original Kanto outdoor group maps.
groups = (root / 'include/constants/map_groups.h').read_text()
region = groups.split('// gMapGroup_TownsAndRoutes', 1)[1].split('// gMapGroup_IndoorPallet', 1)[0]
original_region = set(re.findall(r'#define MAP_([A-Z0-9_]+)\s+\(', region))
safari = {'SAFARI_ZONE_CENTER', 'SAFARI_ZONE_EAST', 'SAFARI_ZONE_NORTH', 'SAFARI_ZONE_WEST'}
assert allowed == original_region | safari, (allowed ^ (original_region | safari))
assert len(allowed) == 42
assert 'POKEMON_TOWER_3F' not in allowed
assert 'VIRIDIAN_FOREST' not in allowed
assert 'SAFARI_ZONE_CENTER' in allowed
assert 'PALLET_TOWN_PLAYERS_HOUSE_1F' not in allowed
assert map_file_weather['PALLET_TOWN'] == 'WEATHER_SUNNY'
assert map_file_weather['SAFARI_ZONE_CENTER'] == 'WEATHER_NONE'
assert map_file_weather['VIRIDIAN_FOREST'] == 'WEATHER_SHADE'
assert map_file_weather['POKEMON_TOWER_3F'] == 'WEATHER_FOG_HORIZONTAL'
print('PASS: v0.9.0.3.8 dynamic weather 42-map allowlist and native weather transition wiring')
