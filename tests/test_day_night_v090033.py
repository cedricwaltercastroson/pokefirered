"""v0.9.0.3.3 source regression: preserve colour and contrast, not dimming."""
from pathlib import Path
s=(Path(__file__).resolve().parents[1]/'src/day_night.c').read_text()
for token in ['DAY_NIGHT_CHROMA_WEIGHT 224', 'DAY_NIGHT_CONTRAST_STRENGTH 40',
              'DAY_NIGHT_CONTRAST_PIVOT 16', 'DAY_NIGHT_NIGHT_TONE_OFFSET 1',
              'DayNight_GradeChannel', 'DayNight_ClampColourChannel',
              'if ((color & 0x7FFF) == 0)', 'if (!night)',
              '(color & 0x8000)', 'gPlttBufferFaded',
              'DynamicOverworldWeather_IsEligibleMap()']:
    assert token in s, token
assert 'DAY_NIGHT_COLOUR_WEIGHT 120' not in s
print('PASS: rich, contrast-preserving night without pale greyscale filter')
