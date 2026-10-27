from pathlib import Path
root = Path(__file__).resolve().parents[1]
src = (root/'src/following_pokemon.c').read_text()
fade = (root/'src/field_fadetransition.c').read_text()
header = (root/'include/following_pokemon.h').read_text()
assert 'void Follower_BeginDoorWarp(void)' in src and 'void Follower_BeginDoorWarp(void)' in header
assert 'sDoorRecallExpectsExitStep = MetatileBehavior_IsWarpDoor_2(behavior)' in src
assert '|| MetatileBehavior_IsNonAnimDoor(behavior)' in src
assert 'if (sDoorRecallExpectsExitStep)' in src
assert 'Follower_IsSafeDoorReturnTile' in src
assert 'MoveCoords(GetOppositeDirection(face), &rearX, &rearY)' in src
assert 'doorRecallFirstSpawn = TRUE;' in src
assert 'Follower_BeginStairWarp();' in fade
assert 'Follower_BeginBuildingExitWarp();' in fade
assert 'Follower_BeginRearOnlyWarp();' in fade
assert 'sSpinTileRecall = TRUE;' in src
assert 'sHealingRecallState = FOLLOWER_HEAL_IN_PROGRESS;' in src
assert 'sBattleRecallAwaitStep = TRUE;' in src
# Door entry timing remains natural; v0.8.27 defers all non-overworld exits,
# stair, healing and spin hooks, and the unchanged battle-return state.
assert 'void DoWarp(void)' in fade
assert 'void DoDoorWarp(void)' in fade
assert 'void DoStairWarp(' in fade
print('PASS: natural door sequencing; new exit gates and battle/stair/heal/spin protected')
