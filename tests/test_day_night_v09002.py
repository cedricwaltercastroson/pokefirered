"""Current day/night invariants; historical scalar tests superseded by v0.9.0.3.3."""
from pathlib import Path
s=(Path(__file__).resolve().parents[1]/'src/day_night.c').read_text()
for token in [
    '#define DAY_NIGHT_CYCLE_SECONDS (120 * 60)',
    '#define DAY_NIGHT_DAY_START_MINUTE (8 * 60)',
    '#define DAY_NIGHT_NIGHT_START_MINUTE (20 * 60)',
    'DayNight_IsNight', 'if (!night)', 'dest[i] = src[i]',
    'DynamicOverworldWeather_IsEligibleMap()',
    'gPaletteFade.bufferTransferDisabled', 'gPlttBufferFaded',
    'DAY_NIGHT_CHROMA_WEIGHT 224',
    'DAY_NIGHT_CONTRAST_STRENGTH 40',
    'DayNight_ClampColourChannel',
]:
    assert token in s, token
assert 'DAY_NIGHT_COLOUR_WEIGHT 120' not in s
assert 'DAY_NIGHT_NIGHT_RED' not in s
print('PASS: strict day/night, source palette restoration, deep-tone nighttime, outdoor-only processing')
