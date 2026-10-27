#!/usr/bin/env python3
"""v0.8.27: building exits and ALL non-overworld warps are deferred, rear-only."""
from pathlib import Path
r = Path(__file__).resolve().parents[1]
f = (r / 'src/following_pokemon.c').read_text()
e = (r / 'src/field_fadetransition.c').read_text()
h = (r / 'include/following_pokemon.h').read_text()
runtime = (r / 'tests/follower_runtime_smoke.c').read_text()
warp = e.split('void DoWarp(void)\n{', 1)[1].split('void DoDiveWarp(void)', 1)[0]
map_types = e.split('static bool8 Follower_IsOverworldMapType(u8 mapType)\n{', 1)[1].split('}\n', 1)[0]
for outdoor in ('MAP_TYPE_TOWN', 'MAP_TYPE_CITY', 'MAP_TYPE_ROUTE', 'MAP_TYPE_OCEAN_ROUTE'):
    assert outdoor in map_types, outdoor
for indoor in ('MAP_TYPE_UNDERGROUND', 'MAP_TYPE_INDOOR'):
    assert indoor not in map_types, indoor
assert 'GetCurrentMapType() == MAP_TYPE_INDOOR' in warp
assert 'Follower_IsOverworldMapType(destination->mapType)' in warp
assert warp.index('Follower_BeginBuildingExitWarp();') < warp.index('Follower_BeginRearOnlyWarp();')
assert warp.count('Follower_BeginRearOnlyWarp();') == 1
assert '|| !Follower_IsOverworldMapType(destination->mapType)' in warp
assert warp.index('Follower_BeginRearOnlyWarp();') < warp.index('Follower_BeginSafariZoneWarp();')
assert 'MAPSEC_KANTO_VICTORY_ROAD' not in warp  # Generic cave rule, not a hard-coded entrance.
fall_warp = e.split('void DoDiveWarp(void)\n{', 1)[1].split('void DoStairWarp(', 1)[0]
assert 'Follower_BeginRearOnlyWarp();' in fall_warp  # Fall/hole cave warp bypass
assert 'Follower_BeginSafariZoneWarp();' in warp  # Preserve outdoor Safari special case.
task = e.split('static void Task_DoorWarp(u8 taskId)\n{', 1)[1].split('static void Task_StairWarp(u8 taskId)', 1)[0]
assert task.index('walkrun_is_standing_still()') < task.index('Follower_BeginRearOnlyWarp();')
assert task.index('ObjectEventIsHeldMovementActive(&gObjectEvents[followerId])') < task.index('Follower_BeginRearOnlyWarp();')
assert 'Follower_BeginRearOnlyWarp();' in task and 'Follower_BeginDoorWarp();' not in task
assert 'void Follower_EndBuildingExitWarp(void);' in h
assert 'void Follower_BeginRearOnlyWarp(void);' in h
for task_name, end_name in (
    ('static void sub_807DFBC(u8 taskId)\n{', 'static void task_map_chg_seq_0807E20C(u8 taskId)'),
    ('static void task_map_chg_seq_0807E20C(u8 taskId)\n{', 'static void task_map_chg_seq_0807E2CC(u8 taskId)'),
    ('static void task_map_chg_seq_0807E2CC(u8 taskId)\n{', 'static void Task_TeleportWarpIn(u8 taskId)'),
):
    section = e.split(task_name, 1)[1].split(end_name, 1)[0]
    assert 'Follower_EndBuildingExitWarp();' in section
    assert section.index('ScriptContext2_Disable();') < section.index('Follower_EndBuildingExitWarp();')
assert 'if (!sDoorRecallExitReady)' in f
assert 'void Follower_EndBuildingExitWarp(void)' in f
assert 'sDoorRecallExitReady = TRUE;' in f
assert 'if (sSafariWarpAwaitStep)' in f
assert 'if (!Follower_IsSafeSpawnTile(sSafariWarpAnchorX, sSafariWarpAnchorY))' in f
assert 'if (!Follower_IsSafeDoorReturnTile(sDoorRecallAnchorX,' in f
assert 'Follower_BeginBuildingExitWarp();' in runtime
assert 'Follower_EndBuildingExitWarp();' in runtime
assert 'Follower_BeginRearOnlyWarp();' in runtime
assert 'mockBlockedX = 100; mockBlockedY = 100;' in runtime
print('PASS: non-overworld cave/indoor routing, deferred building exit, Safari isolation')
