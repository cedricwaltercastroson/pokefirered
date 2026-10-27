#!/usr/bin/env python3
"""v0.8.23 source-level invariants complementing host C runtime simulation."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
code = (root / "src/following_pokemon.c").read_text()
field = (root / "src/field_effect.c").read_text()
header = (root / "include/following_pokemon.h").read_text()

assert "void Follower_BeginFlyRecall(void);" in header
assert '#include "following_pokemon.h"' in field
fly = field.split("static void UseFlyEffect_1(struct Task * task)\n{", 1)[1].split("static void UseFlyEffect_2", 1)[0]
assert "Follower_BeginFlyRecall();" in fly
assert fly.index("Follower_BeginFlyRecall();") < fly.index("SetPlayerAvatarStateMask(PLAYER_AVATAR_FLAG_ON_FOOT)")
fly_recall = code.split("void Follower_BeginFlyRecall(void)\n{", 1)[1].split("void Follower_AfterBicycleDismount", 1)[0]
assert "sFlyRecallActive = TRUE;" in fly_recall
assert "if (visible)\n        PlaySE(SE_BALL_TRADE);" in fly_recall
assert "if (sFlyRecallActive)" in code
assert "sFlyRecallActive = FALSE;" in code.split("void Follower_ResetForMapLoad", 1)[1]

update = code.split("void Follower_Update(struct ObjectEvent *player)\n{", 1)[1]
recall = update.split("if (usingSpecialMovement || specialTransitionPending)", 1)[1].split("if (species == SPECIES_NONE", 1)[0]
assert "if (exists && objectId < OBJECT_EVENTS_COUNT" in recall
assert "PlaySE(SE_BALL_TRADE);" in recall
assert "sBikeReleaseSoundPending = TRUE;" in recall
assert "if (surfingTravel)\n            sBikeReleaseSoundPending = FALSE;" in recall
spawn = update.split("objectId = SpawnSpecialObjectEventParameterized", 1)[1].split("Follower_RefreshPalette();", 1)[0]
assert "if (objectId != OBJECT_EVENTS_COUNT)" in spawn
assert "if (sHealingReleaseSoundPending)" in spawn
assert "else if (sBikeReleaseSoundPending)" in spawn
assert "PlaySE(SE_BALL_OPEN);" in spawn
assert "sBikeReleaseSoundPending = FALSE;" in spawn
assert "sBikeReleaseSoundPending = FALSE;" in code.split("void Follower_ResetForMapLoad", 1)[1]
print("PASS: v0.8.23 Fly/Bicycle/Surf recall once, successful bicycle release, existing healing audio")
