"""Regression contract for the experimental day/night feature's complete removal.
Run: python3 tests/test_v090035_no_day_night.py
"""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
assert not (root/'src/day_night.c').exists()
assert not (root/'include/day_night.h').exists()
for path in ('src/overworld.c','ld_script.txt'):
    src=(root/path).read_text()
    assert 'DayNight_' not in src and 'day_night' not in src, path
roamer=(root/'src/roamer.c').read_text()
assert 'SPECIES_MEW' in roamer and 'ROUTE21_NORTH' in roamer
assert 'GetRoamerLocation' in roamer
print('PASS: day/night code and hooks removed; Mew stationary roamer still present')
