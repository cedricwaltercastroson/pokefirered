from pathlib import Path
r = Path(__file__).resolve().parents[1]
s = (r/'src/following_pokemon.c').read_text()
w = (r/'src/field_fadetransition.c').read_text()
h = (r/'include/following_pokemon.h').read_text()
assert 'void Follower_BeginDoorWarp(void)' in s and 'void Follower_BeginDoorWarp(void)' in h
assert 'sDoorRecallHasAnchor = FALSE;' in s
assert 'if (sDoorWarpRecall && !QL_IS_PLAYBACK_STATE)' in s
assert 'doorRecallFirstSpawn = TRUE;' in s
assert 'x = sDoorRecallAnchorX;' in s and 'y = sDoorRecallAnchorY;' in s
assert 'gPlayerAvatar.tileTransitionState == T_TILE_TRANSITION' in s
assert 'ScriptContext1_IsScriptSetUp() || ScriptContext2_IsEnabled()' in s
assert 'if (GetCurrentMapType() == MAP_TYPE_INDOOR' in w
assert '|| !Follower_IsOverworldMapType(destination->mapType)' in w
assert 'Follower_BeginRearOnlyWarp();' in w
assert 'void DoDoorWarp(void)\n{\n    // Animated door entrance' in w
# The existing v0.8.15 battle gate remains unchanged; this patch does NOT
# replace or alter its battle-return strategy prior to in-game door validation.
assert 'sBattleRecallAwaitStep = TRUE;' in s
print('PASS: doorway state protection, generic non-overworld routing and battle separation')
