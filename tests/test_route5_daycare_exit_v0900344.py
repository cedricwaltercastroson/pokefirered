#!/usr/bin/env python3
"""Route 5 Day Care uses the locked Viridian Gym follower-only exit policy."""
from pathlib import Path
import json
import re
import struct
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
route = json.loads((root / 'data/maps/Route5/map.json').read_text())
daycare = json.loads((root / 'data/maps/Route5_PokemonDayCare/map.json').read_text())
assert any((w['x'],w['y'],w['dest_map']) == (23,25,'MAP_ROUTE5_POKEMON_DAY_CARE') for w in route['warp_events'])
assert len(daycare['warp_events']) == 3
assert all(w['dest_map'] == 'MAP_ROUTE5' and w['dest_warp_id'] == 1 for w in daycare['warp_events'])
layout_defs = json.loads((root/'data/layouts/layouts.json').read_text())['layouts']
layout = next(x for x in layout_defs if x['id']==route['layout'])
raw = (root/layout['blockdata_filepath']).read_bytes()
def collision(x,y):
    metatile = struct.unpack_from('<H',raw,2*(y*layout['width']+x))[0]
    return (metatile >> 10) & 3
assert collision(23,25)!=0, 'facade/door collision'
assert collision(23,26)==0, 'porch should be walkable'
assert collision(23,27)!=0, 'raised porch ends at a wall'

source = (root/'src/following_pokemon.c').read_text()
start = source.index('static bool8 Follower_IsRoute5DayCareExteriorThreshold(')
stop = source.index('// Choose a safe adjacent tile',start)
function = source[start:stop]
assert 'mapNum == MAP_NUM(ROUTE5)' in function
assert 'mapGroup == MAP_GROUP(ROUTE5)' in function
assert 'x == 23 + 7' in function
assert '(y == 25 + 7 || y == 26 + 7)' in function
assert source.count('Follower_IsRoute5DayCareExteriorThreshold(')==2
branch = source.split('if (sDoorRecallOutdoorExitPending)',1)[1].split('if (sInterruptedTravelBuildingEntry)',1)[0]
assert 'Follower_IsViridianGymExteriorThreshold(mapNum, mapGroup,' in branch
assert '|| Follower_IsRoute5DayCareExteriorThreshold(mapNum, mapGroup,' in branch
assert 'sDoorRecallAnchorX = player->currentCoords.x;' in branch
assert 'sDoorRecallAnchorY = player->currentCoords.y;' in branch
assert 'return;' in branch
# Actual C helper compiled to check map isolation and threshold boundaries.
small = '''
#include <assert.h>
typedef unsigned char u8;
typedef short s16;
typedef unsigned char bool8;
#define MAP_ROUTE5 (16 | (3 << 8))
#define MAP_VIRIDIAN_CITY (3 | (2 << 8))
#define MAP_NUM(map) (MAP_##map & 0xFF)
#define MAP_GROUP(map) (MAP_##map >> 8)
''' + function + '''
int main(void) {
  assert(Follower_IsRoute5DayCareExteriorThreshold(16,3,30,32));
  assert(Follower_IsRoute5DayCareExteriorThreshold(16,3,30,33));
  assert(!Follower_IsRoute5DayCareExteriorThreshold(16,3,30,34));
  assert(!Follower_IsRoute5DayCareExteriorThreshold(16,3,29,32));
  assert(!Follower_IsRoute5DayCareExteriorThreshold(16,2,30,33));
  assert(!Follower_IsRoute5DayCareExteriorThreshold(15,3,30,33));
  return 0;
}
'''
with tempfile.TemporaryDirectory() as directory:
    cfile=Path(directory)/'daycare.c'; executable=Path(directory)/'daycare'
    cfile.write_text(small)
    subprocess.run(['gcc','-std=gnu89','-Wall','-Wextra','-Werror',str(cfile),'-o',str(executable)], check=True)
    subprocess.run([str(executable)],check=True)
print('PASS: Route 5 Day Care tile geometry, shared guarded recall, map isolation, compiled threshold checks')
