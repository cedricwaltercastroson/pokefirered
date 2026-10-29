#!/usr/bin/env python3
"""v0.9.0.3.40 — one adjacent map tile, zero extra pixel drift.

Runs a real extracted follower render function under host C; the companion
v38 regression executes the actual trainer alert / movement functions.
Does not replace a GBA ROM or emulator visual test.
"""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = (ROOT / 'src/following_pokemon.c').read_text()
TRAINER = (ROOT / 'src/trainer_see.c').read_text()

def extract(name):
    matches = list(re.finditer(r'(?:static\s+)?void\s+' + re.escape(name) + r'\s*\(', SOURCE))
    assert matches, name
    pat = matches[-1]
    brace = SOURCE.index('{', pat.end())
    depth = 0
    for n in range(brace, len(SOURCE)):
        if SOURCE[n] == '{':
            depth += 1
        if SOURCE[n] == '}':
            depth -= 1
            if depth == 0:
                return SOURCE[pat.start():n + 1]
    raise AssertionError(name)

ALERT = SOURCE[SOURCE.index('bool8 Follower_BeginTrainerAlertYield'):SOURCE.index('bool8 Follower_IsCollisionExempt')]
assert 'sTrainerAlertYieldExtraDirection' not in SOURCE
assert 'for (pass =' not in ALERT
assert ALERT.count('ObjectEventSetHeldMovement(follower, GetWalkFastMovementAction(') == 1
assert 'Follower_WaitTrainerAlertYield' in TRAINER
assert 'Follower_BeginTrainerAlertYield' in TRAINER
assert 'sprite->x2 = 0;\n            return;' in extract('Follower_ApplyHorizontalSpacing')

c = r'''
#include <assert.h>
#include <stdio.h>
typedef unsigned char u8;
typedef short s16;
typedef u8 bool8;
#define TRUE 1
#define FALSE 0
#define MAX_SPRITES 64
#define OBJECT_EVENTS_COUNT 16
#define DIR_EAST 4
#define DIR_WEST 3
#define FOLLOWER_SIDE_SPACING_PIXELS 8
struct Coords {s16 x,y;};
struct ObjectEvent {u8 active,spriteId,facingDirection;struct Coords currentCoords;};
struct Sprite {s16 x2;};
struct ObjectEvent gObjectEvents[OBJECT_EVENTS_COUNT];
struct Sprite gSprites[MAX_SPRITES];
struct {u8 objectEventId;} gPlayerAvatar;
static bool8 sTrainerAlertVisualHold;
static s16 sTrainerAlertPlayerX,sTrainerAlertPlayerY;
''' + extract('Follower_ApplyHorizontalSpacing') + r'''
int main(void)
{
    struct ObjectEvent *player=&gObjectEvents[0],*follower=&gObjectEvents[1];
    struct Sprite *sprite=&gSprites[1];
    gPlayerAvatar.objectEventId=0;
    player->active=TRUE;
    player->currentCoords.x=10;player->currentCoords.y=10;
    follower->active=TRUE;follower->spriteId=1;
    follower->currentCoords.x=9;follower->currentCoords.y=11;
    follower->facingDirection=DIR_WEST;
    sTrainerAlertVisualHold=TRUE;sTrainerAlertPlayerX=10;sTrainerAlertPlayerY=10;
    sprite->x2=99;
    Follower_ApplyHorizontalSpacing(follower);
    assert(sprite->x2==0); /* exactly one tile, no 8px outward extension */
    assert(sTrainerAlertVisualHold);
    player->currentCoords.y=9;
    Follower_ApplyHorizontalSpacing(follower);
    assert(!sTrainerAlertVisualHold);
    assert(sprite->x2==FOLLOWER_SIDE_SPACING_PIXELS); /* normal spacing resumes */
    puts("PASS: one-tile follower render clearance during trainer alert");
    return 0;
}
'''
with tempfile.TemporaryDirectory() as temp:
    file = Path(temp) / 'single_tile.c'
    out = Path(temp) / 'single_tile'
    file.write_text(c)
    subprocess.run(['gcc', '-std=gnu89', '-Wall', '-Wextra', '-Werror', str(file), '-o', str(out)], check=True)
    subprocess.run([str(out)], check=True)
print('PASS: no two-step alert branch and no visual extension during alert hold')
