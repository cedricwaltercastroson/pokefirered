#!/usr/bin/env python3
"""Regress the short native fade and its ownership/continuity guarantees."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
field = (root / 'src/fieldmap.c').read_text()
palette = (root / 'src/palette.c').read_text()
overworld = (root / 'src/overworld.c').read_text()

setting = re.search(r'^#define TOD_FADE_DELAY\s+(\d+)\b', field, re.M)
assert setting, 'missing fade delay setting'
delay = int(setting.group(1))
assert delay == 1, 'fade duration regressed: require delay=1 (~1 second total)'
assert 'gPaletteFade.deltaY = 2;' in palette
assert 'gPaletteFade.objPaletteToggle ^= 1;' in palette
assert 'gPaletteFade.delayCounter < gPaletteFade_delay' in palette
assert 'TimeOfDay_TryUpdateOutdoorPalette();' in overworld

start = field.index('void TimeOfDay_TryUpdateOutdoorPalette(void)')
end = field.index('static void LoadTilesetPalette(', start)
update = field[start:end]
assert update.count('BeginNormalPaletteFade(PALETTES_ALL, TOD_FADE_DELAY') == 2
assert 'TOD_FADE_DELAY, 0, 16, RGB_BLACK' in update
assert 'TOD_FADE_DELAY, 16, 0, RGB_BLACK' in update
assert update.count('ScriptContext2_Enable();') == 1
assert update.count('ScriptContext2_Disable();') == 1
assert update.index('ScriptContext2_Disable();') < update.index('if (gSaveBlock1Ptr == NULL')
assert 'TimeOfDay_RefreshBackgroundPalettes();\n        if (BeginNormalPaletteFade(' in update, (
    'a deliberate wait at full black creates a pause between fade stages')
assert 'gWeatherPtr->currWeather != gWeatherPtr->nextWeather' in update
assert 'gPaletteFade.active || ScriptContext1_IsScriptSetUp()' in update
assert 'QL_STATE_PLAYBACK_LAST' in update

# A small simulator of normal software-fade cadence in src/palette.c. Its 2-colour
# coefficient updates, alternating BG/OBJ pass, delay counter and four finishing
# frames are what make a two-legged delay=1 fade roughly one second at 60 FPS.
def fade_frames(delay):
    y = 0
    obj_pass = 0  # 0=BG, 1=OBJ
    wait = delay  # BeginNormalPaletteFade starts with the counter at delay
    finishing = 0
    frames = 0
    while frames < 240:
        frames += 1
        if finishing:
            finishing += 1
            if finishing == 6:  # four finish counter frames, then inactive
                return frames
            continue
        if obj_pass == 0:
            if wait < delay:
                wait += 1
                continue
            wait = 0
        obj_pass ^= 1
        if obj_pass == 0:
            if y == 16:
                finishing = 1
            else:
                y = min(16, y + 2)
    raise AssertionError('fade did not finish')

fast_frames = fade_frames(1) * 2
old_frames = fade_frames(5) * 2
assert 48 <= fast_frames <= 78, (fast_frames, old_frames)
assert fast_frames <= old_frames * .60, (fast_frames, old_frames)
print(f'PASS: fast native day/night fade ~{fast_frames/60:.2f}s '
      f'({fast_frames} frames) vs legacy ~{old_frames/60:.2f}s '
      f'({old_frames} frames), no added black-frame pause')
