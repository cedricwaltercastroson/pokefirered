#!/usr/bin/env python3
"""Source wiring for v0.8.7; runtime test is follower_runtime_smoke.c."""
from pathlib import Path
r = Path(__file__).resolve().parents[1]
fade = (r / 'src/field_fadetransition.c').read_text()
fol = (r / 'src/following_pokemon.c').read_text()
head = (r / 'include/following_pokemon.h').read_text()
assert 'void Follower_BeginStairWarp(void);' in head
assert 'void Follower_EndStairWarp(void);' in head
stair = fade.split('void DoStairWarp(u16 metatileBehavior, u16 delay)\n{',1)[1].split('\n}',1)[0]
assert stair.index('Follower_BeginStairWarp();') < stair.index('taskId = CreateTask(Task_StairWarp, 10);')
for marker in ['static void task_map_chg_seq_0807E2CC(u8 taskId)\n{', 'static void sub_807EC34(u8 taskId)\n{']:
    block = fade.split(marker,1)[1].split('\n}',1)[0]
    assert block.index('ScriptContext2_Disable();') < block.index('Follower_EndStairWarp();')
update = fol.split('void Follower_Update(struct ObjectEvent *player)\n{',1)[1]
assert update.index('if (sStairWarpRecall)') < update.index('if (sHealingRecallState != FOLLOWER_HEAL_IDLE)')
reset = fol.split('void Follower_ResetForMapLoad(void)\n{',1)[1].split('\n}',1)[0]
assert 'sStairWarpRecall = FALSE' not in reset
prime = fol.split('void Follower_PrimeForOverworldDisplay(void)\n{',1)[1].split('\n}',1)[0]
assert 'if (exists && species == sFollowerSpecies' in prime
assert 'Follower_IsSafeRearTileForExisting(x, y, objectId)' in prime
assert 'if (follower->currentCoords.x != x || follower->currentCoords.y != y)' in prime
print('PASS: stair recall survives map reset; exit release; surviving battle follower reused without forced respawn')
