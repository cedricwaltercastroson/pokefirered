"""Static wiring checks for v0.8.9 (the runtime test validates state)."""
from pathlib import Path
s = Path('src/following_pokemon.c').read_text()
assert 'static bool8 sSpinTileRecall;' in s
assert 'sSpinTileRecall = FALSE;' in s
assert 'MetatileBehavior_IsSpinTile(behavior)' in s
assert 'MetatileBehavior_IsStopSpinning(behavior)' in s
assert 'Follower_IsSpinOrStopTile(gPlayerAvatar.lastSpinTile, FALSE)' in s
assert 'gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_FORCED' in s
assert 'gPlayerAvatar.tileTransitionState == T_TILE_TRANSITION' in s
assert 'player->heldMovementFinished' in s
assert 'Follower_ClearTrail();' in s
assert 'sHasLastPosition = FALSE;' in s
assert 'movementAction = GetSpinMovementAction(direction);' not in s
print('v0.8.9 spin recall/stop state checks OK')
