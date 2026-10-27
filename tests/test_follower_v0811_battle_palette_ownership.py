#!/usr/bin/env python3
"""Source-derived regression checks for the FireRed post-battle OBJ palette path.

Compile the actual InitObjectEventPalettes function body and the actual
ReloadMapObjectWithOffset special-palette gate in a small host harness.
This is not a full GBA ROM/emulator test.
"""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
text = (ROOT / 'src/event_object_movement.c').read_text()


def func_body(name):
    match = re.search(r'\bvoid\s+' + re.escape(name) + r'\s*\([^)]*\)\s*\{', text)
    assert match, name
    start = match.end() - 1
    depth = 0
    for i in range(start, len(text)):
        if text[i] == '{':
            depth += 1
        elif text[i] == '}':
            depth -= 1
            if depth == 0:
                return text[match.start():i + 1]
    raise AssertionError('unterminated function: ' + name)


init = func_body('InitObjectEventPalettes')
assert 'FreeAndReserveObjectSpritePalettes();' in init
assert 'gReservedSpritePaletteCount = 12;' in init
assert 'gReservedSpritePaletteCount = 8;' not in init
assert '#define FOLLOWER_PALETTE_SLOT 11' in (ROOT / 'src/following_pokemon.c').read_text()

start = text.index('static void ReloadMapObjectWithOffset(u8 objectEventId, s16 x, s16 y)\n{')
end = text.index('\nstatic ', start + 20)
reload = text[start:end]
match = re.search(
    r'if\s*\(([^;{}]+?)\)\s*\{\s*'
    r'LoadSpecialObjectReflectionPalette\(graphicsInfo->paletteTag, graphicsInfo->paletteSlot\);\s*\}',
    reload,
)
assert match, 'post-battle object reload must guard special NPC palette writes'
gate = match.group(1).strip()
assert 'objectEvent->graphicsId != OBJ_EVENT_GFX_FOLLOWER' in gate, gate
assert 'graphicsInfo->paletteSlot == 10' in gate, gate
assert 'LoadSpecialObjectReflectionPalette' not in reload[match.end():], 'unexpected palette loader'

c = r'''
#include <assert.h>
#include <stdio.h>
#define OBJ_EVENT_PAL_TAG_NONE 0x11FF
typedef unsigned char u8;
#define OBJ_EVENT_GFX_FOLLOWER 251
static unsigned char gReservedSpritePaletteCount;
static unsigned short sCurrentSpecialObjectPaletteTag;
static unsigned char sCurrentReflectionType;
static const unsigned short t0[12] = {0};
static const unsigned short t1[12] = {0};
static const unsigned short *const gObjectPaletteTagSets[2] = {t0, t1};
static int patchStart, patchEnd, paletteWrites;
static void FreeAndReserveObjectSpritePalettes(void) {gReservedSpritePaletteCount = 12;}
static void PatchObjectPaletteRange(const unsigned short *tags, int min, int max)
{assert(tags != 0); patchStart = min; patchEnd = max;}
struct Graphics {int paletteSlot, paletteTag;};
struct Object {int graphicsId;};
static void LoadSpecialObjectReflectionPalette(int tag, int slot)
{(void)tag; (void)slot; paletteWrites++;}
''' + init + '\n' + r'''
static void VerifyReloadGate(struct Graphics *graphicsInfo, struct Object *objectEvent)
{
    if (''' + gate + r''')
    {
        LoadSpecialObjectReflectionPalette(graphicsInfo->paletteTag, graphicsInfo->paletteSlot);
    }
}
int main(void)
{
    struct Graphics gfx = {11, 0xFFFF};
    struct Object follower = {OBJ_EVENT_GFX_FOLLOWER};
    struct Object npc = {15};
    InitObjectEventPalettes(0);
    assert(gReservedSpritePaletteCount == 12 && patchEnd == 10);
    InitObjectEventPalettes(1);
    assert(gReservedSpritePaletteCount == 12 && patchStart == 0 && patchEnd == 6);
    assert(sCurrentReflectionType == 1);
    VerifyReloadGate(&gfx, &follower);
    assert(paletteWrites == 0); // follower invalid paletteTag never passed through NPC loader
    gfx.paletteSlot = 10;
    VerifyReloadGate(&gfx, &npc);
    assert(paletteWrites == 1); // existing special-NPC behaviour retained
    gfx.paletteSlot = 9;
    VerifyReloadGate(&gfx, &npc);
    assert(paletteWrites == 1);
    puts("v0.8.11 post-battle OBJ palette ownership: PASS");
    return 0;
}
'''
with tempfile.TemporaryDirectory() as tmp:
    source = Path(tmp) / 'palette_check.c'
    executable = Path(tmp) / 'palette_check'
    source.write_text(c)
    subprocess.run(['clang', '-std=c11', '-O0', '-Wall', '-Wextra', '-Werror',
                    '-Wno-unused-function', str(source), '-o', str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
print('Source-derived battle palette tests: PASS')
