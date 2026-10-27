#!/usr/bin/env python3
"""Static check of shared FireRed Nurse Joy callback wiring; not a GBA build."""
from pathlib import Path
r = Path(__file__).resolve().parents[1]
s = (r/'data/scripts/pkmn_center_nurse.inc').read_text()
code = (r/'src/following_pokemon.c').read_text()
specials=(r/'data/specials.inc').read_text()
header=(r/'include/following_pokemon.h').read_text()

heal = s.split('EventScript_PkmnCenterNurse_TakeAndHealPkmn::',1)[1].split('EventScript_PkmnCenterNurse_FollowerRecallDelay::',1)[0]
for token in ('specialvar VAR_RESULT, Follower_BeginHealingRecall',
              'call_if_eq EventScript_PkmnCenterNurse_FollowerRecallDelay',
              'dofieldeffect FLDEFF_POKECENTER_HEAL',
              'waitfieldeffect FLDEFF_POKECENTER_HEAL',
              'special HealPlayerParty',
              'special Follower_EndHealingRecall',
              '\treturn'):
    assert token in heal, token
assert heal.index('Follower_BeginHealingRecall') < heal.index('FLDEFF_POKECENTER_HEAL') < heal.index('HealPlayerParty') < heal.index('Follower_EndHealingRecall')
assert s.count('EventScript_PkmnCenterNurse_FollowerRecallDelay::') == 1
assert 'delay 21' in s.split('EventScript_PkmnCenterNurse_FollowerRecallDelay::',1)[1]
assert 'Follower_BeginHealingRecall' not in s.split('EventScript_PkmnCenterNurse_Goodbye::',1)[1].split('Movement_Bow::',1)[0]
for x in ('Follower_BeginHealingRecall','Follower_EndHealingRecall'):
    assert specials.count('\tdef_special '+x+'\n') == 1
    assert x in header and 'u16 '+x+'(void)' in code
assert specials.index('def_special Follower_EndInteraction') < specials.index('def_special Follower_BeginHealingRecall') < specials.index('def_special Follower_EndHealingRecall') < specials.index('gSpecialsEnd::')
assert 'if (sHealingRecallState != FOLLOWER_HEAL_IDLE)' in code
assert '!ScriptContext1_IsScriptSetUp() && !ScriptContext2_IsEnabled()' in code
assert 'sHealingRecallState = FOLLOWER_HEAL_IDLE;' in code.split('void Follower_ResetForMapLoad(void)',1)[1].split('static bool8 Follower_TryEnqueue',1)[0]
assert 'Follower_Remove();' in code.split('u16 Follower_BeginHealingRecall(void)',1)[1].split('u16 Follower_EndHealingRecall(void)',1)[0]
print('PASS: shared healing entry, recall delay, deferred return, special table, map reset')
