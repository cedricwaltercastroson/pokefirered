#!/usr/bin/env python3
"""Static v0.8.20 regression wiring checks; complements host runtime smoke."""
from pathlib import Path
s = (Path(__file__).resolve().parents[1] / 'src/following_pokemon.c').read_text()
spin_setup = s.split('gridMetatileBehavior = MapGridGetMetatileBehaviorAt',1)[1].split('if (sSpinTileRecall)',1)[0]
spin = s.split('if (sSpinTileRecall)',1)[1].split('if (sHealingRecallState',1)[0]
assert 'Follower_IsSpinOrStopTile(playerMetatileBehavior, FALSE)' in spin_setup
assert 'Follower_IsSpinOrStopTile(gridMetatileBehavior, FALSE)' in spin_setup
assert 'gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_FORCED' in spin
assert 'ScriptContext1_IsScriptSetUp()' in spin
assert 'sSpinTileRecall = FALSE;' in spin
assert '!spinStopTile' in spin
heal = s.split('if (sHealingRecallState != FOLLOWER_HEAL_IDLE)',1)[1].split('if (sScriptedMovementState',1)[0]
assert 'sHealingReleaseSoundPending = TRUE;' in heal
spawn = s.split('objectId = SpawnSpecialObjectEventParameterized',1)[1].split('sLastPlayerX = player->currentCoords.x;',1)[0]
assert 'objectId != OBJECT_EVENTS_COUNT' in spawn
assert 'if (sHealingReleaseSoundPending)' in spawn
assert 'PlaySE(SE_BALL_OPEN);' in spawn
assert 'sHealingReleaseSoundPending = FALSE;' in spawn
begin = s.split('u16 Follower_BeginHealingRecall(void)',1)[1].split('u16 Follower_EndHealingRecall(void)',1)[0]
assert 'sHealingReleaseSoundPending = FALSE;' in begin
assert 'PlaySE(SE_BALL_OPEN);' not in begin
print('PASS: spin fallback and deferred successful healing release audio')
