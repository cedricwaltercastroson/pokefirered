#!/usr/bin/env python3
"""Audit trainer scripts and callbacks for first-stationary-frame follower recovery.

The callback harness in test_double_trainer_follower_return_v0900330.py
executes the original C callback bodies. This file checks broad event coverage.
"""
from pathlib import Path
import collections
import re

root = Path(__file__).resolve().parents[1]
battle = (root / "src/battle_setup.c").read_text()
follower = (root / "src/following_pokemon.c").read_text()
header = (root / "include/following_pokemon.h").read_text()
macros = (root / "asm/macros/event.inc").read_text()
forest = (root / "data/maps/ViridianForest/scripts.inc").read_text()

expected = {
    "trainerbattle_single", "trainerbattle_no_intro", "trainerbattle_double",
    "trainerbattle_earlyrival", "trainerbattle_rematch", "trainerbattle_rematch_double",
}
seen = collections.Counter()
for path in (root / "data").rglob("*.inc"):
    for macro in re.findall(r"^\s*(trainerbattle_[a-z_]+)\b", path.read_text(errors="replace"), flags=re.M):
        assert macro in expected, (path, macro)
        seen[macro] += 1
assert expected == set(seen), seen
assert sum(seen.values()) >= 500, seen
for macro in expected:
    assert re.search(r"\.macro\s+" + macro + r"\s", macros), macro

forest_ids = re.findall(r"trainerbattle_single\s+(TRAINER_BUG_CATCHER_\w+)", forest)
assert set(forest_ids) == {
    "TRAINER_BUG_CATCHER_RICK", "TRAINER_BUG_CATCHER_DOUG",
    "TRAINER_BUG_CATCHER_SAMMY", "TRAINER_BUG_CATCHER_ANTHONY",
    "TRAINER_BUG_CATCHER_CHARLIE",
}
assert "void Follower_NotifyTrainerBattleReturn(void);" in header
assert "void Follower_NotifyTrainerBattleReturn(void)" in follower
assert "Follower_NotifyGymRematchReturn();" in follower.split("void Follower_NotifyTrainerBattleReturn(void)", 1)[1].split("}", 1)[0]
assert "void StartTrainerBattle(void)" in battle
assert "gMain.savedCallback = CB2_EndTrainerBattle;" in battle
assert "gMain.savedCallback = CB2_EndRematchBattle;" in battle
assert "Follower_NotifyLocalTrainerBattleReturn();" in battle
assert "if (!(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_MULTI | BATTLE_TYPE_INGAME_PARTNER)))" in battle
assert "Follower_NotifyBattleReturn();" in battle.split("static void Follower_NotifyLocalTrainerBattleReturn(void)", 1)[1].split("}", 1)[0]
assert "sGymRematchReturnPending || sWildBattleReturnPending" in follower
assert "ScriptContext1_IsScriptSetUp() || ScriptContext2_IsEnabled()" in follower
assert "Follower_IsSafeSpawnTile(x, y)" in follower
assert "if (battleRecallFirstSpawn && objectId != OBJECT_EVENTS_COUNT)" in follower
assert "SetMainCallback2(CB2_WhiteOut);" in battle
print("PASS: checked", sum(seen.values()), "trainer events across", len(seen), "macro types; all five Viridian Forest Bug Catchers use the repaired shared callback")
