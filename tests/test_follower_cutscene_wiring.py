#!/usr/bin/env python3
"""Static integration assertions (does not simulate FireRed's scripting task)."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
h=(r/'include/following_pokemon.h').read_text()
c=(r/'src/following_pokemon.c').read_text()
s=(r/'src/script_movement.c').read_text()
for sym in ('Follower_BeginScriptedPlayerMovement','Follower_EndScriptedPlayerMovement'):
    assert 'void '+sym+'(void);' in h
    assert 'void '+sym+'(void)' in c
assert 'gObjectEvents[objEventId].isPlayer' in s
assert 'if (!failed && gObjectEvents[objEventId].isPlayer)' in s
assert s.index('failed = ScriptMovement_TryAddNewMovement') < s.index('Follower_BeginScriptedPlayerMovement();') < s.index('return failed;')
assert 'Follower_EndScriptedPlayerMovement();' in s.split('void ScriptMovement_UnfreezeObjectEvents(void)',1)[1].split('void ScriptMovement_StartMoveObjects',1)[0]
assert c.index('if (sHealingRecallState != FOLLOWER_HEAL_IDLE)') < c.index('if (sScriptedMovementState != FOLLOWER_SCRIPT_IDLE)') < c.index('usingSpecialMovement =')
assert 'sScriptedMovementState = FOLLOWER_SCRIPT_IDLE;' in c.split('void Follower_ResetForMapLoad(void)',1)[1].split('static bool8 Follower_TryEnqueue',1)[0]
assert 'FOLLOWER_SCRIPT_WAIT_FOR_RELEASE' in c
assert 'ScriptContext1_IsScriptSetUp()' in c
assert 'ScriptContext2_IsEnabled()' in c
assert 'Follower_ClearTrail();' in c.split('void Follower_BeginScriptedPlayerMovement(void)',1)[1].split('void Follower_EndScriptedPlayerMovement(void)',1)[0]
assert 'Follower_Remove();' in c.split('void Follower_BeginScriptedPlayerMovement(void)',1)[1].split('void Follower_EndScriptedPlayerMovement(void)',1)[0]
print('PASS: cutscene callbacks, player-only trigger, script-release gating, warp reset')
