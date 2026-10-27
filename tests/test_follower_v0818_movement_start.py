from pathlib import Path
root = Path(__file__).resolve().parents[1]
src = (root/'src/following_pokemon.c').read_text()
start = src.index('    if (sDoorWarpRecall)', src.index('void Follower_Update('))
end = src.index('    if (sBattleRecallAwaitStep)', start)
door = src[start:end]
assert 'if (gPaletteFade.active || !IsWeatherNotFadingIn()' in door
assert 'if (player->currentCoords.x == sDoorRecallAnchorX' in door
assert 'if (sDoorRecallExpectsExitStep)' in door
assert 'if (!sDoorRecallExpectsExitStep' in door
assert 'ScriptContext1_IsScriptSetUp() || ScriptContext2_IsEnabled()' in door
assert 'gPlayerAvatar.tileTransitionState == T_TILE_TRANSITION' in door  # stationary/fade-only gate
assert 'Follower_IsSafeDoorReturnTile' in door
assert 'doorRecallFirstSpawn = TRUE;' in door
assert 'sDoorRecallHasAnchor = FALSE;' in src
# Catch reintroductions of a universal stop-moving gate ahead of both cases.
prefix = door[:door.index('if (player->currentCoords.x == sDoorRecallAnchorX')]
assert 'tileTransitionState' not in prefix
assert 'heldMovementFinished' not in prefix
# v0.8.18.2 intentionally refines only the animated entry-door task;
# its destination-map movement-start gate remains exactly as tested here.
assert 'void DoDoorWarp(void)' in (root/'src/field_fadetransition.c').read_text()
print('PASS: early doorway movement handoff, fade guard, and protected systems')
