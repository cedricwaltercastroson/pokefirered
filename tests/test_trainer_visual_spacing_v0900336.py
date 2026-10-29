"""Verify one-tile NPC-yield visual clearance survives final dialogue.

Compiles the actual follower render-spacing function against a minimal native
GBA sprite harness. This tests pixel offsets, not emulator rendering.
"""
import re
import subprocess
import tempfile
from pathlib import Path

root = Path(__file__).resolve().parents[1]
source = (root / 'src/following_pokemon.c').read_text()
assert 'sNpcYieldVisualClearance = TRUE;' in source
assert 'sNpcYieldPlayerX = player->currentCoords.x;' in source
assert 'sNpcYieldPlayerY = player->currentCoords.y;' in source
end_script = source.split('void Follower_EndScriptedPlayerMovement(void)', 1)[1].split('u16 Follower_BeginHallOfFameRecall', 1)[0]
assert 'sNpcYieldVisualClearance = FALSE' not in end_script, 'Dialogue release must preserve clearance'
assert 'sNpcYieldVisualClearance = FALSE;' in source
assert 'sNpcYieldVisualClearance && gPlayerAvatar.objectEventId < OBJECT_EVENTS_COUNT' in source

m = re.search(r'static void Follower_ApplyHorizontalSpacing\(const struct ObjectEvent \*follower\)\s*\{', source)
assert m, 'missing render-spacing function'
start = m.start()
depth = 0
fn = None
for i in range(source.index('{', m.start()), len(source)):
    if source[i] == '{': depth += 1
    if source[i] == '}':
        depth -= 1
        if depth == 0:
            fn = source[start:i + 1]
            break
assert fn

header = r'''
#include <assert.h>
#include <stdio.h>
typedef unsigned char u8;
typedef short s16;
typedef u8 bool8;
#define TRUE 1
#define FALSE 0
#define MAX_SPRITES 64
#define OBJECT_EVENTS_COUNT 16
#define FOLLOWER_SIDE_SPACING_PIXELS 8
#define DIR_WEST 3
#define DIR_EAST 4
#define DIR_SOUTH 1
struct Coords {s16 x, y;};
struct ObjectEvent {u8 active, spriteId, facingDirection;struct Coords currentCoords;};
struct Sprite {s16 x2;};
struct ObjectEvent gObjectEvents[OBJECT_EVENTS_COUNT];
struct Sprite gSprites[MAX_SPRITES];
struct {u8 objectEventId;} gPlayerAvatar;
static bool8 sNpcYieldVisualClearance, sNpcYieldInProgress;
static s16 sNpcYieldPlayerX,sNpcYieldPlayerY;
'''
main = r'''
int main(void) {
 struct ObjectEvent *player=&gObjectEvents[0], *follower=&gObjectEvents[1];
 gPlayerAvatar.objectEventId=0;
 player->active=1;player->currentCoords.x=5;player->currentCoords.y=5;
 follower->active=1;follower->spriteId=1;
 follower->currentCoords.x=4;follower->currentCoords.y=5;
 follower->facingDirection=DIR_WEST;
 // Normal trailing, left of player facing west: legacy +8px offset.
 Follower_ApplyHorizontalSpacing(follower);
 assert(gSprites[1].x2==8);
 // Following a one-tile sidestep from an NPC encounter, the normal
 // +8px offset wrongly pulls the follower toward the player again.
 sNpcYieldVisualClearance=1;sNpcYieldPlayerX=5;sNpcYieldPlayerY=5;
 Follower_ApplyHorizontalSpacing(follower);
 assert(gSprites[1].x2==-8);
 // The gap persists after the NPC stops and the dialogue script releases.
 sNpcYieldInProgress=0;
 Follower_ApplyHorizontalSpacing(follower);
 assert(sNpcYieldVisualClearance && gSprites[1].x2==-8);
 // If follower is on right, push it outward in the opposite direction.
 follower->currentCoords.x=6;follower->facingDirection=DIR_EAST;
 Follower_ApplyHorizontalSpacing(follower);
 assert(gSprites[1].x2==8);
 // Only the player's next tile movement releases the visual correction.
 player->currentCoords.x=7;
 Follower_ApplyHorizontalSpacing(follower);
 assert(!sNpcYieldVisualClearance && gSprites[1].x2==-8);
 // Inert when no additional clearance is required.
 follower->facingDirection=DIR_SOUTH;
 Follower_ApplyHorizontalSpacing(follower);
 assert(gSprites[1].x2==0);
 puts("PASS: render-only NPC yield spacing and post-dialogue lifetime");
 return 0;
}
'''
with tempfile.TemporaryDirectory() as d:
    c = Path(d) / 'spacing.c'
    exe = Path(d) / 'spacing'
    c.write_text(header + fn + main)
    subprocess.run(['gcc', '-std=c99', '-Wall', '-Wextra', '-Werror', str(c), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('PASS: source guards for NPC yield render clearance')
