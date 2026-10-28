#!/usr/bin/env python3
"""Gym rematches use League-style safe post-text follower return, not step-only."""
from pathlib import Path
r = Path(__file__).resolve().parents[1]
battle = (r / 'src/battle_setup.c').read_text()
follower = (r / 'src/following_pokemon.c').read_text()
header = (r / 'include/following_pokemon.h').read_text()
assert 'void Follower_NotifyGymRematchReturn(void);' in header
assert 'void Follower_NotifyGymRematchReturn(void)' in follower
trainer = battle.split('static void CB2_EndTrainerBattle(void)\n{', 1)[1].split('static void CB2_EndRematchBattle(void)',1)[0]
assert 'gTrainerBattleOpponent_A >= TRAINER_GYM_REMATCH_BROCK' in trainer
assert 'gTrainerBattleOpponent_A <= TRAINER_GYM_REMATCH_GIOVANNI' in trainer
assert 'Follower_NotifyGymRematchReturn();' in trainer
assert 'Follower_NotifyBattleReturn();' in trainer
assert 'CB2_WhiteOut' in trainer
block = follower.split('if (sBattleRecallAwaitStep)',1)[1].split('if (!exists || species != sFollowerSpecies',1)[0]
# Wild battles share the same post-fade return as the Gym rematch path as of v25.
assert 'if (Follower_IsLeagueScriptMap() || sGymRematchReturnPending || sWildBattleReturnPending)' in block
for required in ('ScriptContext1_IsScriptSetUp()', 'ScriptContext2_IsEnabled()',
                 '!gPaletteFade.active', 'IsWeatherNotFadingIn()',
                 'gPlayerAvatar.tileTransitionState != T_TILE_TRANSITION',
                 '!ObjectEventIsHeldMovementActive(player)',
                 'Follower_IsSafeSpawnTile(x, y)',
                 'sBattleRecallAnchorX = x;', 'sBattleRecallAnchorY = y;'):
    assert required in block, required
assert 'sGymRematchReturnPending = FALSE;' in follower.split('if (battleRecallFirstSpawn && objectId != OBJECT_EVENTS_COUNT)',1)[1]
assert 'Follower_NotifyGymRematchReturn();' not in battle.split('static void CB2_EndWildBattle(void)\n{',1)[1].split('static void CB2_EndScriptedWildBattle',1)[0]
print('PASS: Gym rematches get the League-style settled safe rear spawn; other trainers keep their existing behavior')
