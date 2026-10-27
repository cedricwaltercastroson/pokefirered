#!/usr/bin/env python3
"""Source wiring tests; does not replace a GBA ROM build or emulator run."""
from pathlib import Path
r = Path(__file__).resolve().parents[1]
follower = (r / 'src/following_pokemon.c').read_text()
battle = (r / 'src/battle_setup.c').read_text()
world = (r / 'src/overworld.c').read_text()
header = (r / 'include/following_pokemon.h').read_text()
assert 'void Follower_NotifyBattleReturn(void);' in header
assert 'void Follower_NotifyBattleReturn(void)' in follower
prime = follower.split('void Follower_PrimeForOverworldDisplay(void)\n{', 1)[1].split('\n}', 1)[0]
assert prime.index('if (sBattleReturnPending && !sStairWarpRecall)') < prime.index('Follower_Update(&gObjectEvents[playerId]);')
assert 'sScriptedMovementState == FOLLOWER_SCRIPT_WAIT_FOR_RELEASE' in prime
assert 'sScriptedMovementState == FOLLOWER_SCRIPT_IDLE' in prime
assert 'Follower_ClearTrail();' in prime
assert 'sHasLastPosition = FALSE;' in prime
assert 'Follower_IsSafeRearTileForExisting(x, y, objectId)' in prime
assert 'MoveObjectEventToMapCoords(follower, x, y);' in prime
assert 'if (exists)' in prime
assert prime.index('Follower_Update(&gObjectEvents[playerId]);') < prime.index('sBattleReturnPending = FALSE;')
assert world.index('Follower_PrimeForOverworldDisplay();') < world.index('void CB2_ReturnToField(void)')
for name, next_name, count in (
    ('CB2_EndTrainerBattle', 'static void CB2_EndRematchBattle', 4),
    ('CB2_EndRematchBattle', 'void StartRematchBattle', 2),
    ('CB2_EndWildBattle', 'static void CB2_EndScriptedWildBattle', 1),
):
    marker = ('static void ' + name + '(void)\n{')
    block = battle.split(marker, 1)[1].split(next_name, 1)[0]
    assert block.count('Follower_NotifyBattleReturn();') == count, (name, block.count('Follower_NotifyBattleReturn();'))
    assert 'CB2_WhiteOut' in block
print('PASS: trainer, rematch, wild battle handoff; fade-time follower resync; movement safety guards')
