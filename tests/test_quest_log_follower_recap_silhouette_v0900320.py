#!/usr/bin/env python3
"""Regression for black follower silhouette in Quest Log's saved-game recap.

The follower's dedicated OBJ palette must be rebuilt AFTER the weather and
screen fades complete, while BACKUP_GRAYSCALE tint is still in effect.
Only a host-side palette contract test; real GBA gameplay is unverified.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
quest = (ROOT / 'src/quest_log.c').read_text()
follower = (ROOT / 'src/following_pokemon.c').read_text()

ready = quest.split('static void Task_QuestLogScene_SavedGame(u8 taskId)\n{',1)[1].split('\n#define tTimer',1)[0]
assert 'if (!gPaletteFade.active && IsWeatherNotFadingIn())' in ready
assert ready.index('if (!gPaletteFade.active && IsWeatherNotFadingIn())') < ready.index('Follower_RefreshPalette();') < ready.index('DrawQuestLogSceneDescription();')
assert ready.count('Follower_RefreshPalette();') == 1
assert 'Follower_RefreshPalette();' in quest.split('static void Task_EndQuestLog(u8 taskId)\n{',1)[1].split('\n#undef tState',1)[0]


def extract(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 0
    for i in range(opening, len(source)):
        if source[i] == '{': depth += 1
        elif source[i] == '}':
            depth -= 1
            if depth == 0: return source[start:i + 1]
    raise AssertionError(signature)

getter = extract(follower, 'static const u16 *Follower_GetSelectedPalette(void)')
refresh = extract(follower, 'void Follower_RefreshPalette(void)')

preamble = r'''
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint8_t bool8;
#define SPECIES_NONE 0
#define FOLLOWER_PALETTE_SLOT 11
#define QL_TINT_BACKUP_GRAYSCALE 3
#define QL_TINT_NONE 0
#define WEATHER_FOG_HORIZONTAL 8
#define WEATHER_PAL_STATE_SCREEN_FADING_IN 1
#define WEATHER_PAL_STATE_SCREEN_FADING_OUT 2
struct Weather {u8 palProcessingState, fadeScreenCounter; u16 fadeDestColor;};
struct PaletteFade {u8 y; u16 blendColor;};
struct FollowerArt {const void *frames; const u16 *palette;};
static const u16 normal[16]={0x001f,0x03e0,0x7c00,0x7fff,0x4210,0x1111,0x2222,0x3333,0x4444,0x5555,0x6666,0x7777,0x1234,0x6789,0x5432,0x1a2b};
static const u16 shiny[16]={0x7c00,0x001f,0x03e0,0x7fff,0x1021,0x1022,0x1023,0x1024,0x1025,0x1026,0x1027,0x1028,0x1029,0x1030,0x1031,0x1032};
static const struct FollowerArt sFollowerArt[2]={{0,0},{0,normal}};
static const u16 *sFollowerShinyPalettes[2]={0,shiny};
static u16 sFollowerSpecies=1;
static bool8 sFollowerShiny=0;
static u16 gPlttBufferUnfaded[512],gPlttBufferFaded[512],sPalettesBackup[512];
static u8 gGlobalFieldTintMode=QL_TINT_BACKUP_GRAYSCALE;
static struct Weather weather;
static struct Weather *gWeatherPtr=&weather;
static struct PaletteFade gPaletteFade;
static int GetCurrentWeather(void){return 0;}
static void LoadPalette(const void *p,u16 offset,u16 size){memcpy(gPlttBufferUnfaded+offset,p,size);memcpy(gPlttBufferFaded+offset,p,size);}
static u16 gray(u16 c){
 unsigned r=c&31,g=(c>>5)&31,b=(c>>10)&31;
 unsigned l=(r*30+g*59+b*11)/100;
 return (u16)(l|(l<<5)|(l<<10));
}
static void ApplyGlobalFieldPaletteTint(u8 palIndex){
 const u16 base=(16+palIndex)*16;
 if(gGlobalFieldTintMode==QL_TINT_BACKUP_GRAYSCALE){
  memcpy(sPalettesBackup+base,gPlttBufferUnfaded+base,32);
  for(unsigned i=0;i<16;i++)gPlttBufferUnfaded[base+i]=gray(gPlttBufferUnfaded[base+i]);
  memcpy(gPlttBufferFaded+base,gPlttBufferUnfaded+base,32);
 }
}
static void UpdateSpritePaletteWithWeather(u8 palIndex){(void)palIndex;}
static void BlendPalette(u16 a,u16 b,u8 c,u16 d){(void)a;(void)b;(void)c;(void)d;}
'''
main = r'''
int main(void){
 const u16 base=(16+FOLLOWER_PALETTE_SLOT)*16;
 for(int k=0;k<2;k++){
  const u16 *expected=k?shiny:normal;
  sFollowerShiny=k;
  // The last field fade left the follower's displayed palette black.
  for(unsigned i=0;i<16;i++)gPlttBufferFaded[base+i]=0;
  Follower_RefreshPalette();
  for(unsigned i=0;i<16;i++){
   assert(sPalettesBackup[base+i]==expected[i]);
   assert(gPlttBufferUnfaded[base+i]==gray(expected[i]));
   assert(gPlttBufferFaded[base+i]==gray(expected[i]));
  }
  assert(gPlttBufferFaded[base+1]!=0);
  // Restore from backup at the end of playback, then repaint real colours.
  gGlobalFieldTintMode=QL_TINT_NONE;
  Follower_RefreshPalette();
  for(unsigned i=0;i<16;i++)assert(gPlttBufferFaded[base+i]==expected[i]);
  gGlobalFieldTintMode=QL_TINT_BACKUP_GRAYSCALE;
 }
 puts("PASS: recap follower grayscale survives black fade; exit restores normal/shiny colours");
 return 0;
}
'''
with tempfile.TemporaryDirectory() as tmp:
    src=Path(tmp)/'ql_palette.c'; exe=Path(tmp)/'ql_palette'
    src.write_text(preamble+getter+'\n'+refresh+'\n'+main)
    subprocess.run(['gcc','-std=gnu99','-Wall','-Wextra','-Werror',str(src),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS: final recap refresh is gated by both fades and occurs before saved-game caption')
