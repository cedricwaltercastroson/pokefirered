#!/usr/bin/env python3
"""Exercise real trainer end callbacks for automatic Double Battle follower return.

Host C harness: no GBA/emulator required. Exercises double/single/VS Seeker,
Gym Leader, loss, link/multi exclusions, and actual follower notifier reuse.
"""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
setup = (ROOT / 'src/battle_setup.c').read_text()
follower = (ROOT / 'src/following_pokemon.c').read_text()
header = (ROOT / 'include/following_pokemon.h').read_text()


def extract(source, name):
    match = re.search(r'\b(?:static\s+)?(?:void|bool8)\s+' + name + r'\s*\([^;]*?\)\s*\{', source)
    assert match, name
    level = 0
    for pos in range(match.end() - 1, len(source)):
        if source[pos] == '{':
            level += 1
        elif source[pos] == '}':
            level -= 1
            if level == 0:
                return source[match.start():pos + 1]
    raise AssertionError(name)


assert 'void Follower_NotifyDoubleTrainerBattleReturn(void);' in header
assert 'sGymRematchReturnPending || sWildBattleReturnPending' in follower
assert 'Follower_IsSafeSpawnTile(x, y)' in follower
assert 'sGymRematchReturnPending = FALSE;' in follower
assert 'Follower_NotifyDoubleTrainerBattleReturn()' in extract(setup, 'CB2_EndTrainerBattle')
assert 'Follower_NotifyDoubleTrainerBattleReturn()' in extract(setup, 'CB2_EndRematchBattle')
assert 'Follower_NotifyDoubleTrainerBattleReturn()' not in extract(setup, 'CB2_EndWildBattle')
assert 'Follower_NotifyDoubleTrainerBattleReturn()' not in extract(setup, 'CB2_EndScriptedWildBattle')

harness = r'''
#include <assert.h>
#include <stdint.h>
typedef uint8_t bool8;
typedef uint16_t u16;
typedef uint32_t u32;
#define TRUE 1
#define FALSE 0
#define TRAINER_BATTLE_EARLY_RIVAL 1
#define RIVAL_BATTLE_HEAL_AFTER 1
#define TRAINER_SECRET_BASE 5
#define TRAINER_GYM_REMATCH_BROCK 800
#define TRAINER_GYM_REMATCH_GIOVANNI 807
#define BATTLE_TYPE_DOUBLE (1u << 0)
#define BATTLE_TYPE_FIRST_BATTLE (1u << 4)
#define BATTLE_TYPE_LINK (1u << 1)
#define BATTLE_TYPE_MULTI (1u << 6)
#define BATTLE_TYPE_INGAME_PARTNER (1u << 22)

static u32 gBattleTypeFlags;
static int sTrainerBattleMode, sRivalBattleFlags;
static u16 gTrainerBattleOpponent_A;
static int gSpecialVar_Result, gBattleOutcome;
static int notifications, lastNotification, whiteouts, statUpdates, questLogCalls;
static int sBattleReturnPending, sGymRematchReturnPending, sWildBattleReturnPending, sOakLabRivalReturnPending;
static void CB2_WhiteOut(void) {}
static void CB2_ReturnToFieldContinueScriptPlayMapMusic(void) {}
static void (*lastCallback)(void);
static void SetMainCallback2(void (*cb)(void)) { lastCallback = cb; if (cb == CB2_WhiteOut) whiteouts++; }
static bool8 IsPlayerDefeated(int outcome) { return outcome != 0; }
static void HealPlayerParty(void) {}
static void RefillMovesPlayerParty(void) { statUpdates++; }
static void SetBattledTrainerFlag(void) {}
static void QuestLogEvents_HandleEndTrainerBattle(void) { questLogCalls++; }
static void ClearRematchStateOfLastTalked(void) {}
static void ResetDeferredLinkEvent(void) {}
static void Follower_NotifyBattleReturn(void)
{
    notifications++; lastNotification = 1;
    sBattleReturnPending = TRUE;
    sGymRematchReturnPending = FALSE;
    sWildBattleReturnPending = FALSE;
}
static void Follower_NotifyGymRematchReturn(void)
{
    Follower_NotifyBattleReturn();
    lastNotification = 2;
    sGymRematchReturnPending = TRUE;
}
'''

code = '\n\n'.join((extract(follower, 'Follower_NotifyTrainerBattleReturn'),
                   extract(follower, 'Follower_NotifyDoubleTrainerBattleReturn'),
                   extract(follower, 'Follower_NotifyOakLabRivalBattleReturn'),
                   extract(setup, 'Follower_NotifyLocalTrainerBattleReturn'),
                   extract(setup, 'CB2_EndTrainerBattle'),
                   extract(setup, 'CB2_EndRematchBattle')))
main = r'''
static void reset(int flags, int trainer, int defeated)
{
    gBattleTypeFlags=flags; gTrainerBattleOpponent_A=trainer; gBattleOutcome=defeated;
    sTrainerBattleMode=0; sRivalBattleFlags=0;
    notifications=0; lastNotification=0; whiteouts=0; statUpdates=0; questLogCalls=0;
    sBattleReturnPending=sGymRematchReturnPending=sWildBattleReturnPending=sOakLabRivalReturnPending=0;
    lastCallback=0;
}
int main(void)
{
    reset(BATTLE_TYPE_DOUBLE, 10, 0);
    CB2_EndTrainerBattle();
    assert(notifications==1 && lastNotification==2 && sGymRematchReturnPending);
    assert(lastCallback == CB2_ReturnToFieldContinueScriptPlayMapMusic);
    assert(statUpdates==1 && questLogCalls==1);

    reset(0, 10, 0);
    CB2_EndTrainerBattle();
    assert(notifications==1 && lastNotification==2 && sGymRematchReturnPending);

    reset(0, TRAINER_GYM_REMATCH_BROCK, 0);
    CB2_EndTrainerBattle();
    assert(notifications==1 && lastNotification==2 && sGymRematchReturnPending);

    reset(BATTLE_TYPE_DOUBLE | BATTLE_TYPE_LINK, 10, 0);
    CB2_EndTrainerBattle();
    assert(notifications==1 && lastNotification==1);
    reset(BATTLE_TYPE_DOUBLE | BATTLE_TYPE_MULTI, 10, 0);
    CB2_EndTrainerBattle();
    assert(notifications==1 && lastNotification==1);
    reset(BATTLE_TYPE_DOUBLE | BATTLE_TYPE_INGAME_PARTNER, 10, 0);
    CB2_EndTrainerBattle();
    assert(notifications==1 && lastNotification==1);

    reset(BATTLE_TYPE_DOUBLE, 10, 1);
    CB2_EndTrainerBattle();
    assert(notifications==0 && whiteouts==1 && !sBattleReturnPending);

    reset(BATTLE_TYPE_DOUBLE, 10, 0);
    CB2_EndRematchBattle();
    assert(notifications==1 && lastNotification==2 && sGymRematchReturnPending);
    reset(0, 10, 0);
    CB2_EndRematchBattle();
    assert(notifications==1 && lastNotification==2 && sGymRematchReturnPending);
    reset(BATTLE_TYPE_DOUBLE, 10, 1);
    CB2_EndRematchBattle();
    assert(notifications==0 && whiteouts==1);

    reset(BATTLE_TYPE_DOUBLE, TRAINER_SECRET_BASE, 0);
    CB2_EndTrainerBattle();
    assert(notifications==1 && lastNotification==2 && sGymRematchReturnPending);
    reset(BATTLE_TYPE_DOUBLE, TRAINER_SECRET_BASE, 0);
    CB2_EndRematchBattle();
    assert(notifications==1 && lastNotification==2 && sGymRematchReturnPending);
    // Route 22 also uses early-rival mode, but has no tutorial flag.
    reset(0, 10, 0);
    sTrainerBattleMode=TRAINER_BATTLE_EARLY_RIVAL;
    CB2_EndTrainerBattle();
    assert(notifications==1 && lastNotification==2 && sGymRematchReturnPending && !sOakLabRivalReturnPending);
    // Only Oak's actual first battle arms the special lab cutscene latch.
    reset(BATTLE_TYPE_FIRST_BATTLE, 10, 0);
    sTrainerBattleMode=TRAINER_BATTLE_EARLY_RIVAL;
    CB2_EndTrainerBattle();
    assert(notifications==1 && lastNotification==2 && sGymRematchReturnPending && sOakLabRivalReturnPending);
    reset(0, 10, 1);
    sTrainerBattleMode=TRAINER_BATTLE_EARLY_RIVAL;
    sRivalBattleFlags=RIVAL_BATTLE_HEAL_AFTER;
    CB2_EndTrainerBattle();
    assert(notifications==1 && lastNotification==2 && sGymRematchReturnPending && !sOakLabRivalReturnPending);
    reset(BATTLE_TYPE_FIRST_BATTLE, 10, 1);
    sTrainerBattleMode=TRAINER_BATTLE_EARLY_RIVAL;
    sRivalBattleFlags=RIVAL_BATTLE_HEAL_AFTER;
    CB2_EndTrainerBattle();
    assert(notifications==1 && lastNotification==2 && sGymRematchReturnPending && sOakLabRivalReturnPending);
    reset(0, 10, 1);
    sTrainerBattleMode=TRAINER_BATTLE_EARLY_RIVAL;
    CB2_EndTrainerBattle();
    assert(notifications==0 && whiteouts==1);
    return 0;
}
'''
with tempfile.TemporaryDirectory() as tmp:
    src = Path(tmp) / 'test.c'
    out = Path(tmp) / 'test'
    src.write_text(harness + '\n' + code + '\n' + main)
    subprocess.run(['gcc', '-std=c99', '-O2', '-Wall', '-Wextra', '-Werror', str(src), '-o', str(out)], check=True)
    subprocess.run([str(out)], check=True)
print('PASS: single/double/early-rival/VS Seeker Trainer callbacks select safe return; link/multi/whiteout exclusions retained')
