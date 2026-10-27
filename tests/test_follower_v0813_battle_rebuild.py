#!/usr/bin/env python3
"""v0.8.13 sprite lifetime protections inherited by the deferred v0.8.15 path."""
from pathlib import Path
r = Path(__file__).resolve().parents[1]
source = (r / 'src/following_pokemon.c').read_text()
engine = (r / 'src/event_object_movement.c').read_text()
world = (r / 'src/overworld.c').read_text()
prime = source.split('void Follower_PrimeForOverworldDisplay(void)\n{', 1)[1].split('\n}', 1)[0]
update = source.split('void Follower_Update(struct ObjectEvent *player)\n{', 1)[1]
# Discard a potentially stale sprite before drawing, but do NOT show another
# until the player has walked and battle fade is completely finished.
assert 'Follower_Remove();' in prime
assert 'Follower_ClearTrail();' in prime
assert 'sHasLastPosition = FALSE;' in prime
assert 'sAwaitingSafeSpawn = FALSE;' in prime
assert 'sBattleRecallAwaitStep = TRUE;' in prime
assert 'sBattleRecallAnchorX = player->currentCoords.x' in prime
assert prime.index('Follower_Remove();') < prime.index('sBattleReturnPending = FALSE;')
assert 'if (sBattleRecallAwaitStep)' in update
assert 'gPlayerAvatar.tileTransitionState == T_TILE_TRANSITION' in update
assert 'ScriptContext1_IsScriptSetUp() || ScriptContext2_IsEnabled()' in update
assert 'battleRecallFirstSpawn = TRUE;' in update
assert 'x = sBattleRecallAnchorX;' in update
assert 'y = sBattleRecallAnchorY;' in update
assert 'sPaletteRefreshAfterLoad = !battleRecallFirstSpawn;' in update
# Normal load, Quest Log, stairs, and other path fallback remain unchanged.
assert '!sBattleReturnPending' in source
assert 'Follower_IsSafeSpawnTile(x, y)' in source
assert 'UpdateSpritePaletteWithWeather(FOLLOWER_PALETTE_SLOT);' in source
assert 'graphicsInfo->paletteSlot == 10 && objectEvent->graphicsId != OBJ_EVENT_GFX_FOLLOWER' in engine
assert 'gReservedSpritePaletteCount = 12;' in engine
assert 'Follower_PrimeForOverworldDisplay();' in world
print('PASS: fresh battle return only after movement, no stale sprite or palette, v0.8.11 slot ownership')
