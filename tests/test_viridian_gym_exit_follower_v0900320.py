#!/usr/bin/env python3
"""Regression wiring for the specific Viridian Gym exterior landing.

Run with the separately compiled tests/follower_runtime_smoke.c for runtime
state-machine coverage; these assertions verify real map/layout coordinates.
"""
from pathlib import Path
import json
import struct

root = Path(__file__).resolve().parents[1]
gym = json.loads((root / 'data/maps/ViridianCity_Gym/map.json').read_text())
city = json.loads((root / 'data/maps/ViridianCity/map.json').read_text())
layout = json.loads((root / 'data/layouts/layouts.json').read_text())
city_layout = next(m for m in layout['layouts'] if m['id'] == city['layout'])
gym_exits = [w for w in gym['warp_events'] if w['dest_map'] == 'MAP_VIRIDIAN_CITY']
assert len(gym_exits) == 3
assert all(w['dest_warp_id'] == 2 for w in gym_exits)
assert city['warp_events'][2]['x'] == 36
assert city['warp_events'][2]['y'] == 10
assert city['warp_events'][2]['dest_map'] == 'MAP_VIRIDIAN_CITY_GYM'

block = (root / city_layout['blockdata_filepath']).read_bytes()
def map_entry(x, y):
    return struct.unpack_from('<H', block, 2 * (y * city_layout['width'] + x))[0]

# Exterior door is non-walkable; x36/y11 porch is accessible, y12 wall isn't.
assert ((map_entry(36, 10) >> 10) & 3) != 0
assert ((map_entry(36, 11) >> 10) & 3) == 0
assert ((map_entry(36, 12) >> 10) & 3) != 0
assert ((map_entry(35, 11) >> 10) & 3) == 0

src = (root / 'src/following_pokemon.c').read_text()
assert 'Follower_IsViridianGymExteriorThreshold' in src
assert 'mapNum == MAP_NUM(VIRIDIAN_CITY)' in src
assert 'mapGroup == MAP_GROUP(VIRIDIAN_CITY)' in src
assert 'x == 36 + 7' in src
assert '(y == 10 + 7 || y == 11 + 7)' in src
assert src.index('Follower_IsViridianGymExteriorThreshold(mapNum, mapGroup,') > src.index('if (sDoorRecallOutdoorExitPending)')

runtime = (root / 'tests/follower_runtime_smoke.c').read_text()
assert 'v0.9.0.3.20 Viridian Gym: fade-only exit opens onto a narrow porch.' in runtime
assert 'assert(gObjectEvents[1].currentCoords.x == 42);' in runtime
assert 'assert(gObjectEvents[1].currentCoords.y == 18);' in runtime
print('PASS: real Viridian Gym doorway/porch coordinates and targeted follower guard')
