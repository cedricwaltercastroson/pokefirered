#!/usr/bin/env python3
"""Check the night-only grass OBJ palette and the actual transition-time refresh."""
from pathlib import Path
import re, subprocess, tempfile, hashlib
root = Path(__file__).resolve().parents[1]
effect = (root/'src/field_effect.c').read_text()
field = (root/'src/fieldmap.c').read_text()
assert 'TimeOfDay_IsNightForFieldEffects()' in effect
assert 'FieldEffect_RefreshTimeOfDayPalette();' in field
assert 'gGlobalFieldTintMode != QL_TINT_NONE' in field
assert 'gQuestLogState == QL_STATE_PLAYBACK' in field
assert 'gQuestLogState == QL_STATE_PLAYBACK_LAST' in field
assert 'FLDEFF_PAL_TAG_GENERAL_1' in effect
assert '.paletteTag = FLDEFF_PAL_TAG_GENERAL_1' in (root/'src/data/field_effects/field_effect_objects.h').read_text()
assert 'gFieldEffectObjectTemplate_TallGrass' in (root/'src/data/field_effects/field_effect_objects.h').read_text()
# Night variant must not mutate the user's original BG banks.
for name, sha in {
    'general':'2e5321cf3fb6731373ac74ab36782767cef5a482632f51f9d244141cc0c5611a',
    'pallet_town':'5b88014375800233d125a8226f441583af0e3a1d677b3b74d695dab71ea8d2b5'
}.items():
    assert hashlib.sha256((root/f'data/tilesets/night/pallet_town/{name}.gbapal').read_bytes()).hexdigest()==sha
pat=r'static const u16 sNightGeneralFieldEffect1Palette\[16\]\s*=\s*\{(.*?)\};'
m=re.search(pat,effect,re.S); assert m
entries=re.findall(r'RGB\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)',m.group(1))
assert len(entries)==16
assert all(0<=int(ch)<=31 for triple in entries for ch in triple)
assert all(int(b)>=int(g)>=int(r) for r,g,b in entries[1:6]) # Violet-slate grass, no green cast.
assert all(int(b)>=int(g)>=int(r) for r,g,b in entries[12:16])
# Compile the real night/day refresh function with mocked palette allocator and weather engine.
fn='void FieldEffect_RefreshTimeOfDayPalette(void)'
pos=effect.index(fn); start=effect.index('{',pos); depth=0
for end in range(start,len(effect)):
    if effect[end]=='{': depth+=1
    if effect[end]=='}':
        depth-=1
        if not depth: break
real_func=effect[pos:end+1]
c='''#include <stdint.h>
#include <assert.h>
#include <stddef.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint8_t bool8;
#define RGB(r,g,b) ((r)|((g)<<5)|((b)<<10))
#define FLDEFF_PAL_TAG_GENERAL_1 0x1005
static const u16 gFieldEffectObjectPalette1[16] = {RGB(31,0,29), RGB(23,31,17)};
'''+m.group(0)+'''
static int slot=4, night=1, load_count=0, weather_count=0;
static const u16 *source; static int offset,bytes;
static u8 IndexOfSpritePaletteTag(int tag) {assert(tag==FLDEFF_PAL_TAG_GENERAL_1);return slot;}
static bool8 TimeOfDay_IsNightForFieldEffects(void) {return night;}
static void LoadPalette(const u16 *p,int dest,int n) { source=p;offset=dest;bytes=n;load_count++; }
static void UpdateSpritePaletteWithWeather(u8 n) {assert(n==4);weather_count++;}
'''+real_func+'''
int main(void) {
    slot=0xFF; FieldEffect_RefreshTimeOfDayPalette(); assert(!load_count&&!weather_count);
    slot=4; night=1; FieldEffect_RefreshTimeOfDayPalette();
    assert(load_count==1&&weather_count==1&&source==sNightGeneralFieldEffect1Palette&&offset==320&&bytes==32);
    assert(source[1]==RGB(17,21,24));
    night=0; FieldEffect_RefreshTimeOfDayPalette();
    assert(load_count==2&&weather_count==2&&source==gFieldEffectObjectPalette1&&offset==320&&bytes==32);
    return 0;
}
'''
with tempfile.TemporaryDirectory() as d:
    source_file=Path(d)/'grass.c'; executable=Path(d)/'grass'
    source_file.write_text(c)
    subprocess.run(['gcc','-std=gnu11','-Wall','-Wextra','-Werror',str(source_file),'-o',str(executable)],check=True)
    subprocess.run([str(executable)],check=True)
print('PASS: night grass blue-slate grade, lazy shared OBJ slot, day palette restoration, weather processing, Quest Log safety, locked BG hashes')
