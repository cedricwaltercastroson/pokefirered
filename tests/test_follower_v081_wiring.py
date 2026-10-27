#!/usr/bin/env python3
"""Static integration assertions. Does not replace agbcc or emulator testing."""
from pathlib import Path
r = Path(__file__).resolve().parents[1]
follower = (r/'src/following_pokemon.c').read_text()
header = (r/'include/following_pokemon.h').read_text()
item = (r/'src/item_use.c').read_text()
world = (r/'src/overworld.c').read_text()
ql = (r/'src/quest_log_objects.c').read_text()
assert '|| QL_IS_PLAYBACK_STATE' not in follower
assert 'void Follower_AfterBicycleDismount(void)' in follower
assert 'void Follower_PrimeForOverworldDisplay(void)' in follower
for symbol in ('Follower_AfterBicycleDismount', 'Follower_PrimeForOverworldDisplay'):
    assert f'void {symbol}(void);' in header
itemcb=item.split('static void ItemUseOnFieldCB_Bicycle(u8 taskId)\n{')[1].split('\n}',1)[0]
assert itemcb.index('GetOnOffBike(') < itemcb.index('ScriptContext2_Disable();') < itemcb.index('Follower_AfterBicycleDismount();')
assert itemcb.index('Follower_AfterBicycleDismount();') < itemcb.index('DestroyTask(taskId);')
prime=world.split('static bool8 map_post_load_hook_exec(void)\n{',1)[1].split('\n}',1)[0]
assert 'Follower_PrimeForOverworldDisplay();' in prime
assert prime.rindex('Follower_PrimeForOverworldDisplay();') < prime.rindex('return TRUE;')
assert 'gObjectEvents[playerId].active' in follower.split('void Follower_PrimeForOverworldDisplay(void)',1)[1].split('void Follower_BeginMapConnectionTransition',1)[0]
assert ql.count('FOLLOWER_LOCAL_ID') == 2
assert ql.count('OBJ_EVENT_GFX_FOLLOWER') == 2
assert 'questLog->unk_008[i].active = FALSE;' in ql
assert 'if (questLogObjectEvents[i].active && !questLogObjectEvents[i].isPlayer' in ql
assert ql.index('if (questLogObjectEvents[i].active') < ql.index('gObjectEvents[i].active = questLogObjectEvents[i].active;')
print('PASS: bike return callback, pre-display hook, replay filtering, prior snapshot compatibility')
