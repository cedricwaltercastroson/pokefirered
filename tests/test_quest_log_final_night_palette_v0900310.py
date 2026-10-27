#!/usr/bin/env python3
"""Compile actual production C to verify Quest Log's saved-game palette backup is night-first."""
from pathlib import Path
import re, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
field=(root/'src/fieldmap.c').read_text()
ql=(root/'src/quest_log.c').read_text()

def extract(name):
    pattern=r'(?:static )?(?:bool8|u8|u32|void) '+name+r'\([^)]*\)\s*\{'
    m=re.search(pattern,field)
    assert m,name
    depth=0
    for i in range(m.end()-1,len(field)):
        if field[i]=='{':depth+=1
        if field[i]=='}':
            depth-=1
            if depth==0:return field[m.start():i+1]
    raise AssertionError(name)

assert 'gGlobalFieldTintMode = QL_TINT_BACKUP_GRAYSCALE;' in (root/'src/overworld.c').read_text()
assert 'QuestLog_BackUpPalette(offset, size);' in field
assert 'TimeOfDay_RestoreAfterQuestLog();' in ql
assert ql.index('gGlobalFieldTintMode = QL_TINT_NONE;') < ql.index('TimeOfDay_RestoreAfterQuestLog();')

header=r'''
#include <stdint.h>
#include <assert.h>
#include <string.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
typedef u8 bool8;
#define TRUE 1
#define FALSE 0
#define QL_TINT_NONE 0
#define QL_TINT_GRAYSCALE 1
#define QL_TINT_SEPIA 2
#define QL_TINT_BACKUP_GRAYSCALE 3
#define QL_STATE_PLAYBACK 2
#define QL_STATE_PLAYBACK_LAST 3
#define TOD_DAY 1
#define TOD_NIGHT 2
#define TOD_DEBUG_FORCE_PERIOD 0
#define NUM_PALS_IN_PRIMARY 6
#define NUM_PALS_TOTAL 13
#define RGB_BLACK 0
struct Save2 {u16 playTimeHours; u8 playTimeMinutes, playTimeSeconds;};
struct Save1 {u8 dummy;};
struct Tileset {const u16 *palettes; u8 isSecondary;};
struct MapLayout {const struct Tileset *primaryTileset, *secondaryTileset;};
struct MapHeader {struct MapLayout *mapLayout;};
static struct Save2 save2;
static struct Save2 *gSaveBlock2Ptr=&save2;
static struct Save1 save1;
static struct Save1 *gSaveBlock1Ptr=&save1;
#define VAR_SLEEP_TIME_OFFSET 0x40FF
#define TOD_MINUTES_PER_CYCLE (24 * 60)
#define TOD_DAY_START_MINUTES (8 * 60)
#define TOD_NIGHT_START_MINUTES (20 * 60)
static u16 savedSleepOffset=0,gSpecialVar_0x8004;
static u16 VarGet(u16 var){assert(var==VAR_SLEEP_TIME_OFFSET);return savedSleepOffset;}
static void VarSet(u16 var,u16 value){assert(var==VAR_SLEEP_TIME_OFFSET);savedSleepOffset=value;}

static u16 sourceDay[NUM_PALS_TOTAL*16],sourceNight[NUM_PALS_TOTAL*16];
static u16 out[NUM_PALS_TOTAL*16],backup[NUM_PALS_TOTAL*16];
static struct Tileset primary={sourceDay,FALSE};
static struct Tileset secondary={sourceDay,TRUE};
static struct MapLayout layout={&primary,&secondary};
static struct MapHeader gMapHeader={&layout};
static u8 gGlobalFieldTintMode,gQuestLogState,mapEligible=1;
static bool8 TimeOfDay_IsEligibleMap(void) {return mapEligible;}
static const u16 *TimeOfDay_GetNightPalette(const struct Tileset *ts) {
  assert(ts==&primary||ts==&secondary);return sourceNight;
}
static void LoadPalette(const void *src, u16 offset, u16 size) {
  memcpy(out+offset,src,size);
}
static void LoadCompressedPalette(const u32 *src,u16 offset,u16 size){
  LoadPalette(src,offset,size);
}
static void Fieldmap_ApplyGlobalTintToPaletteEntries(u16 offset, u16 count) {
  if (gGlobalFieldTintMode==QL_TINT_BACKUP_GRAYSCALE) {
    memcpy(backup+offset,out+offset,count*2);
    // Quest Log owns visible grayscale, despite colour-accurate saved backup.
    for (int i=0;i<count;i++) out[offset+i]=0x4210;
  }
}
'''
body=r'''
int main(void) {
 for(int i=0;i<NUM_PALS_TOTAL*16;i++) {sourceDay[i]=i+111;sourceNight[i]=i+1234;}
 // Day at new-save / hour zero, night after one full active-play hour.
 assert(!TimeOfDay_IsNightAtSeconds(0));
 assert(TimeOfDay_IsNightAtSeconds(1800));
 save2.playTimeMinutes=30;
 // Normal Quest Log playback is unchanged and must NOT apply night.
 gQuestLogState=QL_STATE_PLAYBACK;
 gGlobalFieldTintMode=QL_TINT_GRAYSCALE;
 assert(TimeOfDay_GetDesiredState()==TOD_DAY);
 LoadTilesetPalette(&primary,0,NUM_PALS_IN_PRIMARY*16*2);
 assert(out[1]==sourceDay[1]);
 // Final Saved Game QL scene loads colour-correct BG source BEFORE backing it up.
 memset(backup,0,sizeof backup);
 gQuestLogState=QL_STATE_PLAYBACK_LAST;
 gGlobalFieldTintMode=QL_TINT_BACKUP_GRAYSCALE;
 assert(TimeOfDay_GetDesiredState()==TOD_NIGHT);
 assert(!TimeOfDay_IsNightForFieldEffects());
 LoadTilesetPalette(&primary,0,NUM_PALS_IN_PRIMARY*16*2);
 assert(backup[1]==sourceNight[1]&&backup[79]==sourceNight[79]);
 assert(out[1]==0x4210); // display still Quest Log grayscale
 LoadTilesetPalette(&secondary,NUM_PALS_IN_PRIMARY*16,
                    (NUM_PALS_TOTAL-NUM_PALS_IN_PRIMARY)*16*2);
 assert(backup[NUM_PALS_IN_PRIMARY*16]==sourceNight[NUM_PALS_IN_PRIMARY*16]);
 // When the QL restores the backup before the first gameplay frame, it's night.
 memcpy(out,backup,sizeof out);
 assert(out[1]==sourceNight[1]);
 gQuestLogState=0;gGlobalFieldTintMode=QL_TINT_NONE;
 assert(TimeOfDay_GetDesiredState()==TOD_NIGHT);
 assert(TimeOfDay_IsNightForFieldEffects());
 // The saved bed choice is applied before the Quest Log palette backup.
 save2.playTimeHours=0;save2.playTimeMinutes=0;
 gQuestLogState=0;gGlobalFieldTintMode=QL_TINT_NONE;
 gSpecialVar_0x8004=1;TimeOfDay_SleepUntil();
 assert(TimeOfDay_GetDesiredState()==TOD_NIGHT);
 gQuestLogState=QL_STATE_PLAYBACK_LAST;gGlobalFieldTintMode=QL_TINT_BACKUP_GRAYSCALE;
 assert(TimeOfDay_GetDesiredState()==TOD_NIGHT);
 gSpecialVar_0x8004=0;TimeOfDay_SleepUntil();
 assert(TimeOfDay_GetDesiredState()==TOD_DAY);
 // If it's day, the final backup must be exactly day.
 save2.playTimeHours=0;save2.playTimeMinutes=0;
 gQuestLogState=QL_STATE_PLAYBACK_LAST;
 gGlobalFieldTintMode=QL_TINT_BACKUP_GRAYSCALE;
 memset(backup,0,sizeof backup);
 assert(TimeOfDay_GetDesiredState()==TOD_DAY);
 LoadTilesetPalette(&primary,0,NUM_PALS_IN_PRIMARY*16*2);
 assert(backup[1]==sourceDay[1]);
 // Non-final grayscale in the same state cannot bypass the tint guard.
 gGlobalFieldTintMode=QL_TINT_GRAYSCALE;
 save2.playTimeMinutes=30;
 assert(TimeOfDay_GetDesiredState()==TOD_DAY);
 // Ineligible maps never load night, not even the final QL scene.
 gGlobalFieldTintMode=QL_TINT_BACKUP_GRAYSCALE;mapEligible=0;
 assert(TimeOfDay_GetDesiredState()==TOD_DAY);
 return 0;
}
'''
code=header+'\n'.join(extract(n) for n in ['TimeOfDay_GetPlaySeconds','TimeOfDay_GetGameMinuteAtSeconds','TimeOfDay_IsNightAtSeconds','TimeOfDay_SleepUntil','TimeOfDay_IsNight','TimeOfDay_GetDesiredState','TimeOfDay_IsNightForFieldEffects','LoadTilesetPalette'])+body
with tempfile.TemporaryDirectory() as d:
    path=Path(d)/'test.c';exe=Path(d)/'test';path.write_text(code)
    subprocess.run(['gcc','-std=gnu11','-Wall','-Wextra','-Werror',str(path),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS: final Quest Log backs up true night BG palette before displaying grayscale; normal playback and sprites protected; day/indoor restoration')
