#!/usr/bin/env python3
"""Check one-shot post-Quest Log follower handoff; runtime test in follower_runtime_smoke.c."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
ql = (root / 'src/quest_log.c').read_text()
fc = (root / 'src/following_pokemon.c').read_text()
fh = (root / 'include/following_pokemon.h').read_text()
assert 'void Follower_ResumeAfterQuestLog(void);' in fh
hook = ql.split('static void Task_EndQuestLog(u8 taskId)\n{', 1)[1].split('\n#undef tState', 1)[0]
for needle in ('ClearPlayerHeldMovementAndUnfreezeObjectEvents();',
               'ScriptContext2_Disable();',
               'gGlobalFieldTintMode = QL_TINT_NONE;',
               'TimeOfDay_RestoreAfterQuestLog();',
               'Follower_ResumeAfterQuestLog();'):
    assert needle in hook, needle
assert hook.index('ScriptContext2_Disable();') < hook.index('Follower_ResumeAfterQuestLog();')
assert hook.index('TimeOfDay_RestoreAfterQuestLog();') < hook.index('Follower_ResumeAfterQuestLog();')
resync = fc.split('void Follower_ResumeAfterQuestLog(void)\n{', 1)[1].split('\nstatic bool8 Follower_IsSafeSpawnTile', 1)[0]
assert 'if (QL_IS_PLAYBACK_STATE' in resync
for flag in ('sDoorWarpRecall', 'sFlyReturnAwaitStep', 'sFieldTravelAwaitStep',
             'sBattleRecallAwaitStep', 'sSafariWarpAwaitStep'):
    assert f'{flag} = FALSE;' in resync
assert 'Follower_Update(&gObjectEvents[playerId]);' in resync
assert fc.count('Follower_ResumeAfterQuestLog(void)') == 1
assert ql.count('Follower_ResumeAfterQuestLog();') == 1
# No continuous polling during normal follower frame updates.
update = fc.split('void Follower_Update(struct ObjectEvent *player)\n{', 1)[1]
assert 'Follower_ResumeAfterQuestLog()' not in update
print('PASS: Quest Log finishes scripts, night palettes, then one-shot follower resync')
