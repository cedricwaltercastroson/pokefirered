"""v0.8.25.1 — ensure non-modern RAM linker retains dynamic weather state."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
entries = (root / 'sym_ewram.txt').read_text().splitlines()
assert entries[-1].strip() == '.include "src/field_weather_util.o"'
assert sum('src/field_weather_util.o' in line for line in entries) == 1
assert 'static EWRAM_DATA struct DynamicOverworldWeather sDynamicWeather' in (root / 'src/field_weather_util.c').read_text()
script = (root / 'ld_script.txt').read_text()
assert '<EWRAM>' in script
assert 'sym_ewram.txt' in (root / 'Makefile').read_text()
print('PASS: dynamic weather EWRAM section registered in Rev1 linker layout')
