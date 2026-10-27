#!/usr/bin/env python3
"""Safari Zone outdoor-to-outdoor warp is rear-only and deferred (v0.8.25.3)."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
follow = (root / 'src/following_pokemon.c').read_text()
field = (root / 'src/field_fadetransition.c').read_text()
header = (root / 'include/following_pokemon.h').read_text()
smoke = (root / 'tests/follower_runtime_smoke.c').read_text()
assert 'void Follower_BeginSafariZoneWarp(void);' in header
assert 'void Follower_BeginSafariZoneWarp(void)' in follow
assert 'sSafariWarpAwaitStep = TRUE;' in follow
assert 'sSafariWarpHasAnchor = FALSE;' in follow
assert 'if (sSafariWarpAwaitStep)' in follow
assert 'if (safariWarpFirstSpawn)' in follow
assert 'sSafariWarpAwaitStep = FALSE;' in follow
assert 'if (!Follower_IsSafeSpawnTile(sSafariWarpAnchorX, sSafariWarpAnchorY))' in follow
assert 'x = sSafariWarpAnchorX;' in follow and 'y = sSafariWarpAnchorY;' in follow
assert 'safariWarpFirstSpawn ||' in follow or '|| safariWarpFirstSpawn)' in follow
assert '// Preserve sSafariWarpAwaitStep across ResetObjectEvents on arrival.' in follow
warps = field.split('void DoWarp(void)', 1)[1].split('void DoDiveWarp(void)', 1)[0]
assert '#include "constants/region_map_sections.h"' in field
assert 'MAPSEC_KANTO_SAFARI_ZONE' in warps
assert 'GetDestinationWarpMapHeader()->regionMapSectionId == MAPSEC_KANTO_SAFARI_ZONE' in warps
assert 'Follower_BeginSafariZoneWarp();' in warps
assert warps.index('Follower_BeginSafariZoneWarp();') < warps.index('ScriptContext2_Enable();')
# Leave unaffected special movement/release paths in place.
assert 'if (sSurfReturnAwaitStep)' in follow
assert 'surfReturnFirstSpawn' in follow
assert 'if (sFlyReturnAwaitStep)' in follow
assert 'if (sWarpPadAwaitRelease)' in follow
assert 'void Follower_AfterSurfDismount(void)' in follow
assert 'Follower_BeginSafariZoneWarp();' in smoke
assert 'mockBlockedX = 90;' in smoke
assert 'mockSpawnFailure = TRUE;' in smoke
print('PASS: Safari warp hook, persistent pending latch, rear-only respawn, existing special movement retained')
