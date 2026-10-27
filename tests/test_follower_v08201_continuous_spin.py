#!/usr/bin/env python3
"""v0.8.20.1 source wiring checks; executable behavior lives in host smoke."""
from pathlib import Path

src = (Path(__file__).resolve().parents[1] / 'src/following_pokemon.c').read_text()
spin = src.split('if (sSpinTileRecall)', 1)[1].split('if (sHealingRecallState', 1)[0]
spawn = src.split('if (!exists || species != sFollowerSpecies', 1)[1]
smoke = (Path(__file__).resolve().parent / 'follower_runtime_smoke.c').read_text()

assert 'static bool8 sSpinStopHasAnchor;' in src
assert 'sSpinStopHasAnchor = FALSE;' in src
assert 'gridSpinTile = Follower_IsSpinOrStopTile(gridMetatileBehavior, FALSE);' in src
assert 'gridStopTile = Follower_IsSpinOrStopTile(gridMetatileBehavior, TRUE);' in src
assert 'MapGridGetMetatileBehaviorAt(player->previousCoords.x,' in spin
assert '&& !(gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_FORCED)' in spin
assert '!ScriptContext1_IsScriptSetUp() && !ScriptContext2_IsEnabled()' in spin
assert '&& !gPlayerAvatar.preventStep' in spin
assert 'if (Follower_IsSafeSpawnTile(sSpinStopX, sSpinStopY))' in spin
assert 'spinRecallFirstSpawn = TRUE;' in spin
assert 'if (!spinRecallFirstSpawn' in spin
assert 'if (spinRecallFirstSpawn)' in spawn
assert 'x = sSpinStopX;' in spawn and 'y = sSpinStopY;' in spawn
assert 'if (spinRecallFirstSpawn && objectId != OBJECT_EVENTS_COUNT)' in spawn
assert 'sSpinTileRecall = FALSE;' in spawn
assert 'if (sHealingReleaseSoundPending)' in src
assert 'PlaySE(SE_BALL_OPEN);' in src
for item in ('const u8 dirs[4]', 'stale cached arrow', 'mockBlockedX = 121',
             'Recover STOP from previousCoords', 'Scripted movement following a spin'):
    assert item in smoke, item
print('PASS: v0.8.20.1 spin handoff, successful spawn and regression coverage')
