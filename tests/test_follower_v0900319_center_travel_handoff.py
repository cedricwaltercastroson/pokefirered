#!/usr/bin/env python3
"""Guard completed-travel handoff plus immediate post-Center exit return.

The compiled regression checks safe placement after a manual outdoor step,
not atop the Pokemon Center doorway at the end of its exit animation.
"""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
source = (root / "src/following_pokemon.c").read_text()
runtime = (root / "tests/follower_runtime_smoke.c").read_text()

helper = source.split("static void Follower_CancelCompletedTravelHandoffForWarp(void)\n{", 1)[1].split("\n}", 1)[0]
assert "if (sFlyReturnAwaitStep && !sFlyRecallActive)" in helper
assert "sFlyReturnAwaitStep = FALSE;" in helper
assert "sFlyReturnHasAnchor = FALSE;" in helper
assert "sFlyReleaseSoundPending = FALSE;" in helper
assert "if (sFieldTravelAwaitStep && !sFieldTravelRecallActive)" in helper
assert "sFieldTravelAwaitStep = FALSE;" in helper
assert "sFieldTravelHasAnchor = FALSE;" in helper
assert "sFieldTravelReleaseSoundPending = FALSE;" in helper

rear = source.split("void Follower_BeginRearOnlyWarp(void)\n{", 1)[1].split("\n}", 1)[0]
door = source.split("void Follower_BeginDoorWarp(void)\n{", 1)[1].split("\n}", 1)[0]
assert "Follower_CancelCompletedTravelHandoffForWarp();" in rear
assert "Follower_CancelCompletedTravelHandoffForWarp();" in door
assert "sSafariWarpAwaitStep = FALSE;" in door
assert "sSafariWarpHasAnchor = FALSE;" in door
assert "sSafariWarpAwaitStep = TRUE;" in rear
assert "sInterruptedTravelBuildingEntry = TRUE;" in helper
assert "if (sInterruptedTravelBuildingEntry" in source
assert "sInterruptedTravelBuildingEntry = FALSE;" in source
assert "if (!doorRecallFirstSpawn" in source
assert "if (sInterruptedTravelBuildingEntry)" in source
assert "!Follower_IsSafeSpawnTile(sDoorRecallAnchorX, sDoorRecallAnchorY)" in source
assert "MetatileBehavior_IsWarpDoor(behavior)" in source
assert "assert(gObjectEvents[1].currentCoords.y == 61); // Vacated outdoor tile." in runtime
assert "// v0.9.0.3.19: Fly/field Teleport -> Pokemon Center -> immediate exit." in runtime
assert "Follower_BeginRearOnlyWarp();" in runtime
assert "Follower_BeginBuildingExitWarp();" in runtime
assert "Follower_EndBuildingExitWarp();" in runtime
print("PASS: completed travel handoff and safe non-door post-Center exit return")
