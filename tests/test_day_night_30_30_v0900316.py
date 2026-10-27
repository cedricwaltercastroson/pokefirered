#!/usr/bin/env python3
"""Compile production clock methods; verify 30/30 timing, saved sleep phase and no overflow."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
field = (root/'src/fieldmap.c').read_text()
assert 'gWeatherPtr->currWeather != gWeatherPtr->nextWeather' in field
assert 'gWeatherPtr->weatherChangeComplete' not in field[field.index('void TimeOfDay_TryUpdateOutdoorPalette(void)'):field.index('static void LoadTilesetPalette')]

def extract(name):
    m = re.search(r'(?:static )?(?:bool8|u8|u32|void) '+name+r'\([^)]*\)\s*\{', field)
    assert m, name
    level = 0
    for i in range(m.end()-1, len(field)):
        if field[i] == '{': level += 1
        elif field[i] == '}':
            level -= 1
            if level == 0:
                return field[m.start():i+1]
    raise AssertionError(name)

header = r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef u8 bool8;
#define TOD_MINUTES_PER_CYCLE 1440
#define TOD_DAY_START_MINUTES 480
#define TOD_NIGHT_START_MINUTES 1200
#define VAR_SLEEP_TIME_OFFSET 0x40FF
struct Save2 {u16 playTimeHours; u8 playTimeMinutes, playTimeSeconds;};
struct Save1 {u16 offset;};
static struct Save2 save2,*gSaveBlock2Ptr=&save2;
static struct Save1 save1,*gSaveBlock1Ptr=&save1;
static u16 gSpecialVar_0x8004;
static u16 VarGet(u16 id){assert(id==VAR_SLEEP_TIME_OFFSET);return gSaveBlock1Ptr->offset;}
static void VarSet(u16 id,u16 value){assert(id==VAR_SLEEP_TIME_OFFSET);gSaveBlock1Ptr->offset=value;}
'''
functions = '\n'.join(extract(n) for n in (
    'TimeOfDay_GetPlaySeconds', 'TimeOfDay_GetGameMinuteAtSeconds',
    'TimeOfDay_IsNightAtSeconds', 'TimeOfDay_SleepUntil', 'TimeOfDay_IsNight'))
body = r'''
int main(void) {
    assert(!TimeOfDay_IsNightAtSeconds(0));
    assert(!TimeOfDay_IsNightAtSeconds(1799));
    assert(TimeOfDay_IsNightAtSeconds(1800));
    assert(TimeOfDay_IsNightAtSeconds(3599));
    assert(!TimeOfDay_IsNightAtSeconds(3600));
    assert(!TimeOfDay_IsNightAtSeconds(5399));
    assert(TimeOfDay_IsNightAtSeconds(5400));
    assert(TimeOfDay_GetGameMinuteAtSeconds(0xffffffffu)
       == (480u + ((0xffffffffu % 3600u) * 2u / 5u)) % 1440u);
    // Bed begins a NIGHT period at an arbitrary save playtime; virtual-minute
    // precision permits a 1-2-second boundary quantization.
    save2.playTimeMinutes=14; save2.playTimeSeconds=7;
    gSpecialVar_0x8004=1;
    TimeOfDay_SleepUntil();
    assert(TimeOfDay_IsNight());
    assert(save2.playTimeMinutes==14 && save2.playTimeSeconds==7);
    struct Save1 persisted=*gSaveBlock1Ptr;
    gSaveBlock1Ptr=&persisted; // Reload exactly the saved clock variable.
    assert(TimeOfDay_IsNight());
    save2.playTimeMinutes=44; save2.playTimeSeconds=4;
    assert(TimeOfDay_IsNight());
    save2.playTimeSeconds=7;
    assert(!TimeOfDay_IsNight());
    // Day choice similarly resets to a full 30-minute daytime window.
    gSpecialVar_0x8004=0;
    TimeOfDay_SleepUntil();
    assert(!TimeOfDay_IsNight());
    save2.playTimeHours=1;save2.playTimeMinutes=14;save2.playTimeSeconds=4;
    assert(!TimeOfDay_IsNight());
    save2.playTimeSeconds=7;
    assert(TimeOfDay_IsNight());
    // Bad menu choice must not alter existing saved phase.
    u16 oldOffset=VarGet(VAR_SLEEP_TIME_OFFSET);
    gSpecialVar_0x8004=2;
    TimeOfDay_SleepUntil();
    assert(VarGet(VAR_SLEEP_TIME_OFFSET)==oldOffset);
    return 0;
}
'''
with tempfile.TemporaryDirectory() as td:
    file = Path(td)/'clock.c'; exe=Path(td)/'clock'
    file.write_text(header+functions+body)
    subprocess.run(['gcc','-std=gnu11','-Wall','-Wextra','-Werror', str(file),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS: production 30/30 cycle, exact sleep restart, save/reload, long-playtime wrap, stale-weather guard')
