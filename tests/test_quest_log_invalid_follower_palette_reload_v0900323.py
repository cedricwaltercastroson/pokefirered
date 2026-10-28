#!/usr/bin/env python3
"""v0.9.0.3.23: real sprite-reload palette dispatch guard contract.

Quest Log skip and return-to-field may reload active object-event sprites.
The follower uses reserved OBJ slot 11 with SPRITE_INVALID_TAG (0xffff).
Passing that tag to LoadSpecialObjectReflectionPalette() indexes the standard
palette list with FindObjectEventPaletteIndexByTag() == 0xff, reading outside it.
This test compiles the actual palette-selection branch extracted from the
project source against a host spy. It does not simulate the GBA renderer.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / 'src/event_object_movement.c').read_text()
follower = (ROOT / 'src/following_pokemon.c').read_text()

start = source.index('static void ReloadMapObjectWithOffset(u8 objectEventId, s16 x, s16 y)\n{')
end = source.index('static void ResetObjectEventFldEffData', start)
reload = source[start:end]

# This is not the ordinary object setup path. The full reload path has an
# independent >9 palette dispatch that must not attempt the invalid tag.
anchor = 'if (graphicsInfo->paletteSlot > 9'
assert reload.count(anchor) == 1, 'missing or duplicate reload dispatch'
branch_begin = reload.index(anchor)
branch_end = reload.index('    *(u16 *)&spriteTemplate.paletteTag = SPRITE_INVALID_TAG;', branch_begin)
branch = reload[branch_begin:branch_end]
assert 'graphicsInfo->paletteTag != SPRITE_INVALID_TAG' in branch
assert 'graphicsInfo->paletteTag != OBJ_EVENT_PAL_TAG_NONE' in branch
assert 'LoadSpecialObjectReflectionPalette(graphicsInfo->paletteTag, graphicsInfo->paletteSlot)' in branch

# Follower colours are selected dynamically and must be re-staged after
# CreateSprite. Don't rely on the object-palette-tag loader for this slot.
assert reload.index('spriteId = CreateSprite(') < reload.index('if (objectEvent->graphicsId == OBJ_EVENT_GFX_FOLLOWER)')
assert 'if (objectEvent->graphicsId == OBJ_EVENT_GFX_FOLLOWER)\n            Follower_RefreshPalette();' in reload
assert 'info.paletteTag = SPRITE_INVALID_TAG;' in follower
assert 'info.paletteSlot = FOLLOWER_PALETTE_SLOT;' in follower
assert '#define FOLLOWER_PALETTE_SLOT 11' in follower

harness = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#define SPRITE_INVALID_TAG 0xffff
#define OBJ_EVENT_PAL_TAG_NONE 0x11ff
struct GraphicsInfo {uint16_t paletteTag; uint8_t paletteSlot;};
static unsigned calls;
static uint16_t lastTag;
static uint8_t lastSlot;
static void LoadSpecialObjectReflectionPalette(uint16_t tag, uint8_t slot)
{calls++; lastTag=tag; lastSlot=slot;}
static void ReloadPalette(const struct GraphicsInfo *graphicsInfo)
{
__BRANCH__
}
int main(void)
{
  const struct GraphicsInfo follower = {SPRITE_INVALID_TAG, 11};
  const struct GraphicsInfo noPalette = {OBJ_EVENT_PAL_TAG_NONE, 11};
  const struct GraphicsInfo npc = {0x1100, 10};
  const struct GraphicsInfo custom = {0x1123, 12};
  const struct GraphicsInfo normal = {0x1133, 8};
  ReloadPalette(&follower);
  ReloadPalette(&noPalette);
  ReloadPalette(&normal);
  assert(calls == 0);  // no invalid palette tag reaches NPC loader
  ReloadPalette(&npc);
  assert(calls == 1 && lastTag == npc.paletteTag && lastSlot == 10);
  ReloadPalette(&custom);
  assert(calls == 2 && lastTag == custom.paletteTag && lastSlot == 12);
  puts("PASS: reserved follower palette never indexes the NPC palette table");
  return 0;
}
'''.replace('__BRANCH__', branch)
with tempfile.TemporaryDirectory() as tmp:
    src = Path(tmp, 'reload_guard.c')
    binary = Path(tmp, 'reload_guard')
    src.write_text(harness)
    subprocess.run(['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
                    str(src), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('PASS: Quest Log follower sprite-reload palette guards and refresh ordering')
