#!/usr/bin/env python3
"""Quest Log follower colour handoff regression, using the real palette refresh C.

This tests one-shot wiring and both unfaded/display buffers; it is not a ROM test.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
quest = (ROOT / 'src/quest_log.c').read_text()
follower = (ROOT / 'src/following_pokemon.c').read_text()

end = quest.split('static void Task_EndQuestLog(u8 taskId)\n{', 1)[1].split('\n#undef tState', 1)[0]
required = (
    'CpuCopy16(sPalettesBackup, gPlttBufferUnfaded, PLTT_SIZE);',
    'gGlobalFieldTintMode = QL_TINT_NONE;',
    'TimeOfDay_RestoreAfterQuestLog();',
    'Follower_ResumeAfterQuestLog();',
    'Follower_RefreshPalette();',
    'DisableWildEncounters(FALSE);',
)
for text in required:
    assert text in end, f'Quest Log palette handoff missing: {text}'
assert [end.index(s) for s in required] == sorted(end.index(s) for s in required), 'Exit handoff ordering changed'
assert end.count('Follower_RefreshPalette();') == 1, 'Should refresh exactly once on recap exit'
assert quest.count('Follower_RefreshPalette();') == 2, 'Only final recap-ready and recap-exit refreshes are allowed'


def extract(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 0
    for i in range(opening, len(source)):
        if source[i] == '{':
            depth += 1
        elif source[i] == '}':
            depth -= 1
            if depth == 0:
                return source[start:i + 1]
    raise AssertionError(signature)

getter = extract(follower, 'static const u16 *Follower_GetSelectedPalette(void)')
refresh = extract(follower, 'void Follower_RefreshPalette(void)')

preamble = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint8_t bool8;
#define FALSE 0
#define TRUE 1
#define SPECIES_NONE 0
#define FOLLOWER_PALETTE_SLOT 11
#define WEATHER_FOG_HORIZONTAL 8
#define WEATHER_PAL_STATE_SCREEN_FADING_IN 1
#define WEATHER_PAL_STATE_SCREEN_FADING_OUT 2
struct Weather { u8 palProcessingState, fadeScreenCounter; u16 fadeDestColor; };
struct PaletteFade { u8 y; u16 blendColor; };
struct FollowerArt { const void *frames; const u16 *palette; };
static u16 gPlttBufferUnfaded[512], gPlttBufferFaded[512];
static const u16 normalColors[16] = {0x001f,0x03e0,0x7c00,0x7fff,0x4210,0x1111,0x2222,0x3333,0x4444,0x5555,0x6666,0x7777,0x1234,0x6789,0x5432,0x1a2b};
static const u16 shinyColors[16]  = {0x7c00,0x001f,0x03e0,0x7fff,0x1021,0x1022,0x1023,0x1024,0x1025,0x1026,0x1027,0x1028,0x1029,0x1030,0x1031,0x1032};
static const struct FollowerArt sFollowerArt[2] = {{0,0}, {0,normalColors}};
static const u16 *sFollowerShinyPalettes[2] = {0,shinyColors};
static u16 sFollowerSpecies = 1;
static bool8 sFollowerShiny = FALSE;
static struct Weather weather;
static struct Weather *gWeatherPtr = &weather;
static struct PaletteFade gPaletteFade;
static unsigned tintCalls, weatherCalls;
static int GetCurrentWeather(void) {return 0;}
static void LoadPalette(const void *p,u16 offset,u16 size) {
  memcpy(gPlttBufferUnfaded+offset,p,size);
  memcpy(gPlttBufferFaded+offset,p,size);
}
static void ApplyGlobalFieldPaletteTint(u8 palIndex) {
  assert(palIndex==FOLLOWER_PALETTE_SLOT);
  ++tintCalls;  // Normal field tint must be inactive after Quest Log.
}
static void UpdateSpritePaletteWithWeather(u8 palIndex) {
  assert(palIndex==FOLLOWER_PALETTE_SLOT);
  ++weatherCalls;
}
static void BlendPalette(u16 offset, u16 size, u8 coeff, u16 color) {
  (void)offset;(void)size;(void)coeff;(void)color;
}
'''
main = r'''
int main(void) {
  const u16 start = 0x100 + FOLLOWER_PALETTE_SLOT * 16;
  for (int isShiny = 0; isShiny < 2; ++isShiny) {
    const u16 *expected = isShiny ? shinyColors : normalColors;
    sFollowerShiny = isShiny;
    // Reproduce Quest Log after faded copy is still grayscale but original
    // colours are restored only into the unfaded source palette.
    for (int i=0; i<16; ++i) {
      gPlttBufferUnfaded[start+i] = expected[i];
      gPlttBufferFaded[start+i] = 0x4210;
    }
    Follower_RefreshPalette();
    for (int i=0; i<16; ++i) {
      assert(gPlttBufferUnfaded[start+i] == expected[i]);
      assert(gPlttBufferFaded[start+i] == expected[i]);
    }
  }
  assert(tintCalls==2 && weatherCalls==2);
  puts("PASS: real follower palette refresh repairs grayscale displayed slot for normal and shiny");
  return 0;
}
'''
with tempfile.TemporaryDirectory() as tmp:
    src = Path(tmp) / 'palette_test.c'
    exe = Path(tmp) / 'palette_test'
    src.write_text(preamble + getter + '\n' + refresh + '\n' + main)
    subprocess.run(['gcc', '-std=gnu99', '-Wall', '-Wextra', '-Werror', str(src), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('PASS: Quest Log exit performs one-shot follower colour repair after resync')
