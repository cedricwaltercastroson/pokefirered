#!/usr/bin/env python3
"""v0.8.24 source wiring: warp-pad transition and Fly's deferred release."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
src = (root / 'src/following_pokemon.c').read_text()
fade = (root / 'src/field_fadetransition.c').read_text()
fly = (root / 'src/field_effect.c').read_text()
header = (root / 'include/following_pokemon.h').read_text()

assert 'void Follower_BeginWarpPadRecall(void);' in header
assert 'void Follower_EndWarpPadRecall(void);' in header
start = fade.split('void DoTeleportWarp(void)\n{', 1)[1].split('\n}', 1)[0]
assert start.index('Follower_BeginWarpPadRecall();') < start.index('ScriptContext2_Enable();')
arrival = fade.split('static void Task_TeleportWarpIn(u8 taskId)\n{', 1)[1].split('static void Task_WaitFade', 1)[0]
assert arrival.index('ScriptContext2_Disable();') < arrival.index('Follower_EndWarpPadRecall();')
assert 'sWarpPadRecall = TRUE;' in src
assert 'if (sWarpPadRecall)' in src
assert 'if (sWarpPadAwaitRelease)' in src
assert 'warpPadRecallFirstSpawn' in src
assert 'MetatileBehavior_IsWarpPad(MapGridGetMetatileBehaviorAt(x, y))' in src
assert 'if (!sWarpPadRecall && !sWarpPadAwaitRelease)' in src
assert 'sWarpPadReleaseSoundPending' in src
assert 'sWarpPadAnchorX -= dx;' in src

fly_takeoff = fly.split('static void UseFlyEffect_1(struct Task * task)\n{', 1)[1].split('static void UseFlyEffect_2', 1)[0]
assert fly_takeoff.index('Follower_BeginFlyRecall();') < fly_takeoff.index('SetPlayerAvatarStateMask(PLAYER_AVATAR_FLAG_ON_FOOT);')
reset = src.split('void Follower_ResetForMapLoad(void)\n{', 1)[1].split('static bool8 Follower_TryEnqueue', 1)[0]
assert 'sFlyRecallActive = FALSE;' in reset
assert 'sFlyReturnHasAnchor = FALSE;' in reset
assert 'sFlyReturnAwaitStep = FALSE;' not in reset
assert 'sFlyReleaseSoundPending = FALSE;' not in reset
prime = src.split('void Follower_PrimeForOverworldDisplay(void)\n{', 1)[1].split('void Follower_BeginMapConnectionTransition', 1)[0]
assert prime.index('if (sFlyReturnAwaitStep)') < prime.index('if (sDoorWarpRecall')
assert 'sFlyReturnHasAnchor = TRUE;' in prime
update = src.split('void Follower_Update(struct ObjectEvent *player)\n{', 1)[1]
assert 'if (sFlyReturnAwaitStep)' in update
assert 'flyReturnFirstSpawn = TRUE;' in update
assert 'sFlyReturnAnchorX = player->currentCoords.x;' in update
assert 'sFlyReturnAwaitStep = FALSE;' in update
assert 'else if (flyReturnFirstSpawn && sFlyReleaseSoundPending)' in update
assert 'sFlyReturnAnchorX -= dx;' in src
print('PASS: v0.8.24 warp-pad callback wiring + queued Fly arrival release')
