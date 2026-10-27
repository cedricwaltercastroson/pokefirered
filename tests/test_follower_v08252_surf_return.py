#!/usr/bin/env python3
"""v0.8.25.2: Surf dismount's authoritative callback and deferred return."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
follow = (root / 'src/following_pokemon.c').read_text()
field = (root / 'src/field_player_avatar.c').read_text()
header = (root / 'include/following_pokemon.h').read_text()
smoke = (root / 'tests/follower_runtime_smoke.c').read_text()
assert 'void Follower_AfterSurfDismount(void);' in header
assert 'void Follower_AfterSurfDismount(void)' in follow
assert 'sSurfReturnAwaitStep = TRUE;' in follow
assert 'sSurfReleaseSoundPending = TRUE;' in follow
assert 'if (sSurfReturnAwaitStep)' in follow
assert 'surfReturnFirstSpawn = TRUE;' in follow
assert 'else if (surfReturnFirstSpawn && sSurfReleaseSoundPending)' in follow
assert 'sSurfReturnAwaitStep = FALSE;' in follow
hook = field.split('static void Task_WaitStopSurfing(u8 taskId)\n{', 1)[1].split('static bool8 (*const sFishingStateFuncs', 1)[0]
assert 'Follower_AfterSurfDismount();' in hook
assert hook.index('ScriptContext2_Disable();') < hook.index('Follower_AfterSurfDismount();')
assert hook.index('DestroyTask(taskId);') < hook.index('Follower_AfterSurfDismount();')
assert 'mockSpawnFailure = TRUE;' in smoke
assert 'Follower_AfterSurfDismount();' in smoke
assert 'mockLastSe == SE_BALL_OPEN' in smoke
print('PASS: Surf callback after landing, rear-only queued return, one-time release sound')
