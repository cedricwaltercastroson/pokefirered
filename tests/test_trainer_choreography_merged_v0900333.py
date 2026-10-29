#!/usr/bin/env python3
"""Regression for merging v31 trainer return and v32 cutscene choreography.

Compile the actual follower callback bodies against a small host stub. This
catches the missing v31 symbol that the v32 overlay introduced.
"""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
header = (root / 'include/following_pokemon.h').read_text()
follower = (root / 'src/following_pokemon.c').read_text()
battle = (root / 'src/battle_setup.c').read_text()
movement = (root / 'src/script_movement.c').read_text()

assert re.search(r'^void Follower_NotifyTrainerBattleReturn\(void\);$', header, re.M)
assert re.search(r'^bool8 Follower_DeferNpcScriptStep\(u8 npcId, u8 movementActionId\);$', header, re.M)
assert 'Follower_NotifyLocalTrainerBattleReturn();' in battle
assert 'Follower_DeferNpcScriptStep(objEventId, nextMoveActionId)' in movement
assert 'sGeneralScriptFollowing = TRUE;' in follower
assert follower.count('void Follower_NotifyTrainerBattleReturn(void)') == 1
assert re.search(r'bool8 Follower_DeferNpcScriptStep\(u8 npcId, u8 (?:action|movementActionId)\)', follower)

match = re.search(r'void Follower_NotifyTrainerBattleReturn\(void\)\s*\{(.*?)\}', follower, re.S)
assert match and 'Follower_NotifyGymRematchReturn();' in match.group(1)

# Real production function, compiled with the same C89 vintage as agbcc.
code = r'''
typedef unsigned char u8;
typedef u8 bool8;
static int gym_return, step_return;
void Follower_NotifyGymRematchReturn(void) { gym_return++; }
void Follower_NotifyBattleReturn(void) { step_return++; }
''' + match.group(0) + r'''
int main(void)
{
    Follower_NotifyTrainerBattleReturn();
    if (gym_return != 1 || step_return != 0)
        return 1;
    return 0;
}
'''
with tempfile.TemporaryDirectory() as temp:
    source = Path(temp) / 'merged.c'
    exe = Path(temp) / 'merged'
    source.write_text(code)
    subprocess.run(['gcc', '-std=gnu89', '-Werror=implicit-function-declaration',
                    '-Wall', '-Wextra', '-Werror', str(source), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('PASS: v31 trainer callback + v32 choreography coexist, symbol declared/defined and compiled')
