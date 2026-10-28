'''Regression: v0.9.0.3.19 Gym Leader parties must not override wild encounters.

Compiles and executes the conditional from the actual CreateNPCTrainerParty source
against the real battle flags and Gym Leader IDs. No ROM emulator is involved.
'''
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = (ROOT / "src/battle_main.c").read_text()
START = SOURCE.index('static u8 CreateNPCTrainerParty(struct Pokemon *party, u16 trainerNum)\n{')
PARTY_FUN = SOURCE[START:]
RETURN = PARTY_FUN.index('return CreateGymRematchParty(party, trainerNum);')
BEGIN = PARTY_FUN.rfind('    if (', 0, RETURN)
assert BEGIN >= 0
GUARD = PARTY_FUN[BEGIN: RETURN + len('return CreateGymRematchParty(party, trainerNum);')]
assert 'gBattleTypeFlags & BATTLE_TYPE_TRAINER' in GUARD
assert 'BATTLE_TYPE_BATTLE_TOWER' in GUARD
assert 'BATTLE_TYPE_EREADER_TRAINER' in GUARD
assert 'BATTLE_TYPE_TRAINER_TOWER' in GUARD
assert 'TRAINER_GYM_REMATCH_BROCK' in GUARD and 'TRAINER_GYM_REMATCH_GIOVANNI' in GUARD
assert 'ZeroEnemyPartyMons();' not in GUARD
assert SOURCE.index('CreateNPCTrainerParty(&gEnemyParty[0], gTrainerBattleOpponent_A);') < SOURCE.index('static u8 CreateNPCTrainerParty(struct Pokemon *party, u16 trainerNum)\n{')
assert 'gBattleTypeFlags = 0;' in (ROOT / 'src/battle_setup.c').read_text()
assert 'CreateMonWithNature(&gEnemyParty[0], species, level, 32' in (ROOT / 'src/wild_encounter.c').read_text()

# Keep the extracted guard's exact C logic, replacing only its return value to
# measure whether rematch-party creation is selected by a battle's type.
GUARD = GUARD.replace('return CreateGymRematchParty(party, trainerNum);', 'return 1;')
c_src = '''#include "constants/battle.h"
#include "constants/opponents.h"
static unsigned int gBattleTypeFlags;
static int selects_gym_rematch_party(unsigned short trainerNum) {
''' + GUARD + '''
return 0;
}
int main(void) {
    unsigned short gym;
    for (gym = TRAINER_GYM_REMATCH_BROCK; gym <= TRAINER_GYM_REMATCH_GIOVANNI; gym++) {
        gBattleTypeFlags = BATTLE_TYPE_TRAINER;
        if (!selects_gym_rematch_party(gym)) return 1;
        gBattleTypeFlags = 0; // Normal grass/surf/fishing wild encounter
        if (selects_gym_rematch_party(gym)) return 2;
        gBattleTypeFlags = BATTLE_TYPE_ROAMER;
        if (selects_gym_rematch_party(gym)) return 3;
        gBattleTypeFlags = BATTLE_TYPE_SAFARI;
        if (selects_gym_rematch_party(gym)) return 4;
        gBattleTypeFlags = BATTLE_TYPE_WILD_SCRIPTED;
        if (selects_gym_rematch_party(gym)) return 5;
        gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_TRAINER_TOWER;
        if (selects_gym_rematch_party(gym)) return 6;
        gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_EREADER_TRAINER;
        if (selects_gym_rematch_party(gym)) return 7;
        gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_BATTLE_TOWER;
        if (selects_gym_rematch_party(gym)) return 8;
    }
    gBattleTypeFlags = BATTLE_TYPE_TRAINER;
    if (selects_gym_rematch_party(TRAINER_GYM_REMATCH_BROCK - 1)) return 9;
    if (selects_gym_rematch_party(TRAINER_GYM_REMATCH_GIOVANNI + 1)) return 10;
    return 0;
}
'''
cc = os.environ.get('CC') or shutil.which('cc') or shutil.which('gcc')
assert cc, 'C compiler required to test the actual predicate'
with tempfile.TemporaryDirectory() as folder:
    src = Path(folder) / 'gym_guard.c'
    exe = Path(folder) / 'gym_guard'
    src.write_text(c_src)
    subprocess.run([cc, '-std=c99', '-Wall', '-Wextra', '-Werror', '-I', str(ROOT / 'include'), str(src), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('PASS: gym rematch party selected for eight trainer rematches; rejected for wild, roaming, safari, scripted wild, trainer tower, e-Reader and out-of-range trainer IDs')
