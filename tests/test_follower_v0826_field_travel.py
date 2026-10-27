#!/usr/bin/env python3
"""Field move Dig/Rope/Teleport recall and delayed rear-only return."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
f=(root/'src/following_pokemon.c').read_text()
e=(root/'src/field_effect.c').read_text()
h=(root/'include/following_pokemon.h').read_text()
t=(root/'tests/follower_runtime_smoke.c').read_text()
for name in ('Begin','End'):
    assert f'void Follower_{name}FieldTravelRecall(void);' in h
    assert f'void Follower_{name}FieldTravelRecall(void)' in f
start_rope=e.split('void StartEscapeRopeFieldEffect(void)\n{',1)[1].split('\n}',1)[0]
start_tele=e.split('void CreateTeleportFieldEffectTask(void)\n{',1)[1].split('\n}',1)[0]
assert start_rope.index('Follower_BeginFieldTravelRecall();') < start_rope.index('ScriptContext2_Enable();')
assert start_tele.index('Follower_BeginFieldTravelRecall();') < start_tele.index('CreateTask(')
rope_end=e.split('static void EscapeRopeExitFieldEffect_Step1(struct Task * task)\n{',1)[1].split('\n}',1)[0]
tele_end=e.split('static void TeleportInFieldEffectTask3(struct Task * task)\n{',1)[1].split('\n}',1)[0]
for cb in (rope_end,tele_end):
    assert cb.index('ScriptContext2_Disable();') < cb.index('Follower_EndFieldTravelRecall();')
    assert cb.index('UnfreezeObjectEvents();') < cb.index('Follower_EndFieldTravelRecall();')
reset=f.split('void Follower_ResetForMapLoad(void)\n{',1)[1].split('static bool8 Follower_TryEnqueue',1)[0]
assert 'sFieldTravelRecallActive = FALSE;' not in reset
assert 'sFieldTravelAwaitStep = FALSE;' not in reset
assert 'sFieldTravelReleaseSoundPending = FALSE;' not in reset
prime=f.split('void Follower_PrimeForOverworldDisplay(void)\n{',1)[1].split('void Follower_BeginMapConnectionTransition',1)[0]
assert prime.index('if (sFieldTravelRecallActive || sFieldTravelAwaitStep)') < prime.index('if (sFlyReturnAwaitStep)')
update=f.split('void Follower_Update(struct ObjectEvent *player)\n{',1)[1]
assert 'if (sFieldTravelRecallActive)' in update
assert 'if (sFieldTravelAwaitStep)' in update
assert 'fieldTravelFirstSpawn = TRUE;' in update
assert 'if (!Follower_IsSafeSpawnTile(sFieldTravelAnchorX, sFieldTravelAnchorY))' in update
assert 'else if (fieldTravelFirstSpawn && sFieldTravelReleaseSoundPending)' in update
assert 'if (fieldTravelFirstSpawn && objectId != OBJECT_EVENTS_COUNT)' in update
assert 'sFieldTravelAnchorX -= dx;' in f
assert 'for (fieldMove = 0; fieldMove < 3; fieldMove++)' in t
assert 'mockSpawnFailure = TRUE;' in t
print('PASS: Dig/Rope/Teleport callbacks, reset latch, pre-display guard and safe release')
