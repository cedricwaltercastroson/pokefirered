#!/usr/bin/env python3
"""v0.5.2 static wiring checks; emulator and agbcc build not substitutes."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
world = (root / 'src/overworld.c').read_text()
obj = (root / 'src/event_object_movement.c').read_text()
follow = (root / 'src/following_pokemon.c').read_text()
hdr = (root / 'include/following_pokemon.h').read_text()
loader = world.split('void LoadMapFromCameraTransition(u8 mapGroup, u8 mapNum)\n{', 1)[1].split('static void mli0_load_map(', 1)[0]
assert loader.index('Follower_BeginMapConnectionTransition();') < loader.index('ApplyCurrentWarp();') < loader.index('Follower_FinishMapConnectionTransition();')
assert loader.index('Follower_FinishMapConnectionTransition();') < loader.index('Follower_RefreshPalette();')
assert 'Follower_RebaseForCameraUpdate(dx, dy);' in obj
assert 'void Follower_RebaseForCameraUpdate(s16 dx, s16 dy);' in hdr
assert 'RemoveFollowerObjectEventById(i);' in follow
assert 'sTrail[index].x -= dx;' in follow and 'sTrail[index].y -= dy;' in follow
handoff = follow.split('void Follower_BeginMapConnectionTransition(void)\n{', 1)[1].split('void Follower_FinishMapConnectionTransition', 1)[0]
assert 'Follower_Remove();' not in handoff
assert 'Follower_ClearTrail();' not in handoff
assert 'sHasLastPosition = FALSE;' not in handoff
assert 'sMapConnectionSpawnDelay' not in follow
assert 'RemoveFollowerObjectEvent(void)' in obj
assert 'FlagSet(' not in obj.split('void RemoveFollowerObjectEventById(u8 objectEventId)', 1)[1].split('static void RemoveObjectEventInternal(', 1)[0]
print('PASS: continuous seamless connection handoff and trail rebase wiring')
