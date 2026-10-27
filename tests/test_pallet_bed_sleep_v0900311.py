#!/usr/bin/env python3
"""Compiled persisted 30/30 clock plus Pallet bedroom script/asset contract."""
from pathlib import Path
import json,re,subprocess,tempfile,hashlib,struct
root=Path(__file__).resolve().parents[1]
field=(root/'src/fieldmap.c').read_text()
menu=(root/'src/script_menu.c').read_text()
script=(root/'data/maps/PalletTown_PlayersHouse_2F/scripts.inc').read_text()
maps=json.loads((root/'data/maps/PalletTown_PlayersHouse_2F/map.json').read_text())
orig=None

def extract(name):
    m=re.search(r'(?:static )?(?:bool8|u8|u32|void) '+name+r'\([^)]*\)\s*\{',field)
    assert m,name
    level=0
    for i in range(m.end()-1,len(field)):
        if field[i]=='{':level+=1
        if field[i]=='}':
            level-=1
            if level==0:return field[m.start():i+1]
    raise AssertionError(name)

# Actual bed is metatile block x1..3, y4..6. Other objects remain unaffected.
metatiles=struct.unpack('<108H',(root/'data/layouts/PalletTown_PlayersHouse_2F/map.bin').read_bytes())
bed={(x,y) for y in range(4,7) for x in range(1,4)}
events={(e['x'],e['y']) for e in maps['bg_events'] if e['script']=='PalletTown_PlayersHouse_2F_EventScript_Bed'}
assert events==bed
assert all(metatiles[y*12+x]&0x3ff in range(0x280,0x2a0) for x,y in bed)
assert 'PalletTown_PlayersHouse_2F_EventScript_Bed' in script
assert 'MULTICHOICE_SLEEP_DAY_NIGHT' in menu or 'sScriptMultiChoiceMenu_SleepDayNight' in menu
assert 'multichoice 1, 1, MULTICHOICE_SLEEP_DAY_NIGHT, FALSE' in script
assert script.index('fadescreen FADE_TO_BLACK',script.index('PalletTown_PlayersHouse_2F_EventScript_DoSleep')) < script.index('playfanfare MUS_HEAL',script.index('PalletTown_PlayersHouse_2F_EventScript_DoSleep')) < script.index('waitfanfare',script.index('PalletTown_PlayersHouse_2F_EventScript_DoSleep')) < script.index('special TimeOfDay_SleepUntil',script.index('PalletTown_PlayersHouse_2F_EventScript_DoSleep')) < script.index('fadescreen FADE_FROM_BLACK',script.index('PalletTown_PlayersHouse_2F_EventScript_DoSleep'))
assert 'goto_if_questlog EventScript_ReleaseEnd' in script
assert 'def_special TimeOfDay_SleepUntil' in (root/'data/specials.inc').read_text()
assert (root/'include/constants/vars.h').read_text().count('0x40FF')==3 or 'VAR_SLEEP_TIME_OFFSET       0x40FF' in (root/'include/constants/vars.h').read_text()
assert [p for p in root.rglob('*.md') if p.name!='README.md']==[]
# Verify base palettes unchanged without requiring the old archive to be available.
for k,digest in {
  'general':'2e5321cf3fb6731373ac74ab36782767cef5a482632f51f9d244141cc0c5611a',
  'pallet_town':'5b88014375800233d125a8226f441583af0e3a1d677b3b74d695dab71ea8d2b5',
}.items():
 assert hashlib.sha256((root/f'data/tilesets/night/pallet_town/{k}.gbapal').read_bytes()).hexdigest()==digest

header=r'''
#include <stdint.h>
#include <assert.h>
#include <stddef.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef u8 bool8;
#define TOD_MINUTES_PER_CYCLE (24*60)
#define TOD_DAY_START_MINUTES (8*60)
#define TOD_NIGHT_START_MINUTES (20*60)
#define VAR_SLEEP_TIME_OFFSET 0x40FF
struct Save2 {u16 playTimeHours; u8 playTimeMinutes,playTimeSeconds;};
struct Save1 {u16 vars[256];};
static struct Save2 sav2,*gSaveBlock2Ptr=&sav2;
static struct Save1 sav1,*gSaveBlock1Ptr=&sav1;
static u16 gSpecialVar_0x8004;
static u16 VarGet(u16 id){assert(id==VAR_SLEEP_TIME_OFFSET);return gSaveBlock1Ptr->vars[id-0x4000];}
static bool8 VarSet(u16 id,u16 v){assert(id==VAR_SLEEP_TIME_OFFSET);gSaveBlock1Ptr->vars[id-0x4000]=v;return 1;}
'''
functions='\n'.join(extract(n) for n in ['TimeOfDay_GetPlaySeconds','TimeOfDay_GetGameMinuteAtSeconds','TimeOfDay_IsNightAtSeconds','TimeOfDay_SleepUntil','TimeOfDay_IsNight'])
body=r'''
int main(void){
  assert(!TimeOfDay_IsNight()); // All original saves have 0 offset.
  sav2.playTimeHours=0;sav2.playTimeMinutes=17;sav2.playTimeSeconds=24;
  gSpecialVar_0x8004=1;TimeOfDay_SleepUntil();
  assert(TimeOfDay_IsNight()); // Night immediately, without touching actual playtime.
  assert(sav2.playTimeMinutes==17&&sav2.playTimeSeconds==24);
  u16 savedValue=VarGet(VAR_SLEEP_TIME_OFFSET);
  assert(savedValue<1440);
  // Simulate normal save/reload by copying only the saved variables.
  struct Save1 persisted=*gSaveBlock1Ptr;
  gSaveBlock1Ptr=&persisted;
  assert(VarGet(VAR_SLEEP_TIME_OFFSET)==savedValue&&TimeOfDay_IsNight());
  // The selected Night period ends after 30 more active minutes.
  sav2.playTimeMinutes=47;
  assert(!TimeOfDay_IsNight());
  // Recover accurate original 17:24 play time for another sleep.
  sav2.playTimeHours=0;sav2.playTimeMinutes=17;
  gSpecialVar_0x8004=0;TimeOfDay_SleepUntil();
  assert(!TimeOfDay_IsNight());
  savedValue=VarGet(VAR_SLEEP_TIME_OFFSET);
  gSpecialVar_0x8004=127;TimeOfDay_SleepUntil();assert(VarGet(VAR_SLEEP_TIME_OFFSET)==savedValue);
  // Day survives almost 30 minutes, then Night; 60 minutes returns Day.
  sav2.playTimeHours=0;sav2.playTimeMinutes=46;
  assert(!TimeOfDay_IsNight());
  sav2.playTimeMinutes=48;
  assert(TimeOfDay_IsNight());
  sav2.playTimeHours=1;sav2.playTimeMinutes=18;
  assert(!TimeOfDay_IsNight());
  return 0;
}
'''
with tempfile.TemporaryDirectory() as td:
 p=Path(td)/'bed.c'; exe=Path(td)/'bed';p.write_text(header+functions+body)
 subprocess.run(['gcc','-std=gnu11','-Wall','-Wextra','-Werror',str(p),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('PASS: actual bed map events, script fade/MUS_HEAL order, persistent phase and 30/30 automatic cycle')
