#!/usr/bin/env python3
"""v0.8.25.2: outdoor rain follower gamma and native follower ledge SE."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
weather = (root / 'src/field_weather.c').read_text()
weather_util = (root / 'src/field_weather_util.c').read_text()
header = (root / 'include/field_weather.h').read_text()
follow = (root / 'src/following_pokemon.c').read_text()
tests = (root / 'tests/follower_runtime_smoke.c').read_text()

assert '#define FOLLOWER_OBJ_WEATHER_PALETTE  (16 + 11)' in weather
assert 'bool8 DynamicOverworldWeather_IsEligibleMap(void)' in header
assert 'return IsDynamicWeatherMap();' in weather_util
assert 'GetOutdoorRainFollowerGammaIndex(curPalIndex, gammaIndex)' in weather
assert weather.count('GetOutdoorRainFollowerGammaIndex(curPalIndex, gammaIndex)') == 2
assert 'WEATHER_FOG_HORIZONTAL' not in weather.split('static u8 GetOutdoorRainFollowerGammaIndex', 1)[1].split('static void ApplyGammaShift(', 1)[0]

m = re.search(r'static u8 GetOutdoorRainFollowerGammaIndex\(u16 paletteIndex, u8 gammaIndex\)\n\{.*?\n\}', weather, re.S)
assert m, 'Weather rain helper missing'
helper = m.group(0)
# Compile the actual helper with stand-ins, rather than just duplicating logic
# in Python; compare eligible rain, storm and downpour to sunny/fog/dungeons.
code = '''#include <assert.h>
typedef unsigned char u8;
typedef unsigned short u16;
#define FOLLOWER_OBJ_WEATHER_PALETTE (16 + 11)
#define WEATHER_RAIN 3
#define WEATHER_RAIN_THUNDERSTORM 5
#define WEATHER_DOWNPOUR 13
#define WEATHER_SUNNY 2
#define WEATHER_FOG_HORIZONTAL 6
struct Weather {u8 currWeather;};
static struct Weather mockWeather;
static struct Weather *gWeatherPtr = &mockWeather;
static int eligible;
static int DynamicOverworldWeather_IsEligibleMap(void) {return eligible;}
''' + helper + '''
int main(void) {
    const u8 rain[] = {WEATHER_RAIN, WEATHER_RAIN_THUNDERSTORM, WEATHER_DOWNPOUR};
    int i;
    eligible = 1;
    for (i = 0; i < 3; ++i) {
        gWeatherPtr->currWeather = rain[i];
        assert(GetOutdoorRainFollowerGammaIndex(27, 2) == 3);
        assert(GetOutdoorRainFollowerGammaIndex(27, 18) == 18);
        assert(GetOutdoorRainFollowerGammaIndex(26, 2) == 2);
        eligible = 0;
        assert(GetOutdoorRainFollowerGammaIndex(27, 2) == 2);
        eligible = 1;
    }
    gWeatherPtr->currWeather = WEATHER_SUNNY;
    assert(GetOutdoorRainFollowerGammaIndex(27, 2) == 2);
    gWeatherPtr->currWeather = WEATHER_FOG_HORIZONTAL;
    assert(GetOutdoorRainFollowerGammaIndex(27, 2) == 2);
    return 0;
}
'''
with tempfile.TemporaryDirectory() as td:
    p = Path(td)
    (p / 'check.c').write_text(code)
    subprocess.run(['cc', '-std=c99', '-Wall', '-Werror', str(p/'check.c'), '-o', str(p/'check')], check=True)
    subprocess.run([str(p/'check')], check=True)

assert 'if (next->jump)\n            PlaySE(SE_LEDGE);' in follow
assert 'if (!ObjectEventSetHeldMovement(follower, movementAction))' in follow
assert tests.count('mockLastSe == SE_LEDGE') >= 3
print('PASS: v0.8.25.2 gamma slot/outdoor/rain guards and follower-only ledge sound')
