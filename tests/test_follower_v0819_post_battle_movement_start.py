#!/usr/bin/env python3
"""v0.8.19: post-battle follower restored on step start, not step completion."""
from pathlib import Path
src = (Path(__file__).resolve().parents[1] / 'src/following_pokemon.c').read_text()
prime = src.split('void Follower_PrimeForOverworldDisplay(void)', 1)[1].split('void Follower_BeginMapConnectionTransition', 1)[0]
update = src.split('void Follower_Update(struct ObjectEvent *player)', 1)[1]
segment = update.split('    if (sBattleRecallAwaitStep)', 1)[1].split('    if (!exists || species', 1)[0]
assert 'Follower_Remove();' in prime
assert 'sBattleRecallAwaitStep = TRUE;' in prime
assert 'ScriptContext1_IsScriptSetUp() || ScriptContext2_IsEnabled()' in segment
assert 'gPlayerAvatar.preventStep' in segment
assert 'player->currentCoords.x == sBattleRecallAnchorX' in segment
assert 'Follower_IsSafeSpawnTile(sBattleRecallAnchorX, sBattleRecallAnchorY)' in segment
assert 'battleRecallFirstSpawn = TRUE;' in segment
# League-specific stationary post-text return may wait for a settled tile;
# ordinary map first-step return must NOT acquire that new restriction.
assert 'tileTransitionState' in segment.split('if (Follower_IsLeagueScriptMap()', 1)[1].split('if (!battleRecallFirstSpawn', 1)[0]
assert 'tileTransitionState' not in segment.split('if (!battleRecallFirstSpawn', 1)[1]
assert 'heldMovementFinished' not in segment
assert 'x = sBattleRecallAnchorX;' in update
assert 'y = sBattleRecallAnchorY;' in update
print('PASS: post-battle first-step spawn guard')
