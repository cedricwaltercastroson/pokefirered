#!/usr/bin/env python3
"""Wild encounters use the gym/League safe stationary return, not script/safari overrides."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
battle = (root / 'src/battle_setup.c').read_text()
follower = (root / 'src/following_pokemon.c').read_text()
header = (root / 'include/following_pokemon.h').read_text()
assert 'void Follower_NotifyWildBattleReturn(void);' in header
assert 'void Follower_NotifyWildBattleReturn(void)' in follower
wild_end = battle.split('static void CB2_EndWildBattle(void)\n{', 1)[1].split('static void CB2_EndScriptedWildBattle(void)', 1)[0]
assert wild_end.index('IsPlayerDefeated') < wild_end.index('Follower_NotifyWildBattleReturn()')
assert 'SetMainCallback2(CB2_WhiteOut);' in wild_end
assert 'SetMainCallback2(CB2_ReturnToField);' in wild_end
assert 'Follower_NotifyBattleReturn();' not in wild_end
for name in ('DoStandardWildBattle', 'StartRoamerBattle', 'DoGhostBattle'):
    block = battle.split(('static void ' if name != 'StartRoamerBattle' else 'void ') + name + '(void)\n{', 1)[1].split('\n}', 1)[0]
    assert 'gMain.savedCallback = CB2_EndWildBattle;' in block, name
scripted = battle.split('static void CB2_EndScriptedWildBattle(void)\n{', 1)[1].split('static void CB2_EndMarowakBattle(void)', 1)[0]
assert 'Follower_NotifyWildBattleReturn()' not in scripted
assert 'gMain.savedCallback = CB2_EndSafariBattle;' in battle
assert 'Follower_NotifyWildBattleReturn()' not in battle.split('static void CB2_EndTrainerBattle(void)\n{', 1)[1]
notify = follower.split('void Follower_NotifyBattleReturn(void)\n{', 1)[1].split('void Follower_NotifyGymRematchReturn', 1)[0]
assert 'sWildBattleReturnPending = FALSE;' in notify
block = follower.split('if (sBattleRecallAwaitStep)\n    {\n        // A script', 1)[1].split('if (!exists || species != sFollowerSpecies', 1)[0]
for required in ('sWildBattleReturnPending', 'Follower_IsLeagueScriptMap()', 'sGymRematchReturnPending',
                 'ScriptContext1_IsScriptSetUp()', 'ScriptContext2_IsEnabled()',
                 '!gPaletteFade.active', 'IsWeatherNotFadingIn()',
                 '!ObjectEventIsHeldMovementActive(player)', 'Follower_IsSafeSpawnTile(x, y)'):
    assert required in block, required
assert 'sWildBattleReturnPending = FALSE;' in follower.split('if (battleRecallFirstSpawn && objectId != OBJECT_EVENTS_COUNT)', 1)[1]
print('PASS: wild callbacks use fade-safe stationary follower return; scripted and Safari paths untouched')
