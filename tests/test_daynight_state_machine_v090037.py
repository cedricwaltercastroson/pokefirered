#!/usr/bin/env python3
"""Compiled clock, safe fade, and immediate Quest Log restoration regressions."""
from pathlib import Path
import re,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
src=(root/'src/fieldmap.c').read_text()
ql=(root/'src/quest_log.c').read_text()

def extract(name):
    m=re.search(r'(?:static )?(?:bool8|u8|u32|void) '+name+r'\([^)]*\)\s*\{',src)
    assert m,name
    level=0
    for i in range(m.end()-1,len(src)):
        if src[i]=='{':level+=1
        if src[i]=='}':
            level-=1
            if level==0:return src[m.start():i+1]
    raise AssertionError(name)

assert 'TimeOfDay_RestoreAfterQuestLog();' in ql
assert 'TOD_FADE_DELAY 1' in src
assert 'Two virtual minutes per five seconds of active play' in src
header="""
#include <stdint.h>
#include <assert.h>
#include <stddef.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef u8 bool8;
#define TRUE 1
#define FALSE 0
#define TOD_UNINITIALIZED 0
#define TOD_DAY 1
#define TOD_NIGHT 2
#define QL_TINT_NONE 0
#define QL_TINT_BACKUP_GRAYSCALE 3
#define QL_STATE_PLAYBACK 2
#define QL_STATE_PLAYBACK_LAST 3
#define WEATHER_PAL_STATE_IDLE 3
#define NUM_PALS_TOTAL 13
#define RGB_BLACK 0
#define PALETTES_ALL 0xFFFFFFFFu
#define TOD_FADE_DELAY 1
struct Save2 {u16 playTimeHours; u8 playTimeMinutes, playTimeSeconds;};
struct Save1 {u8 dummy;};
struct MapLayout {u8 dummy;};
struct MapHeader {struct MapLayout *mapLayout;};
struct Weather {u8 palProcessingState, weatherChangeComplete, currWeather, nextWeather;};
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

static struct MapLayout mapLayout;
static struct MapHeader gMapHeader = {&mapLayout};
static struct Weather weather = {3,1,1,1};
static struct Weather *gWeatherPtr=&weather;
static struct {u8 active;} gPaletteFade;
static u8 gGlobalFieldTintMode,gQuestLogState,scriptBusy,scriptOneBusy,mapEligible=1;
static u8 sLastLoadedTimeOfDay=TOD_UNINITIALIZED;
static u8 sTimeOfDayTransition=0,sTimeOfDayPaletteRefresh=0;
static int reloadCount,gammaCount,fadeCount,lastFadeStart,lastFadeEnd,enableCount,disableCount,fieldEffectRefreshCount;
static bool8 TimeOfDay_IsEligibleMap(void) {return mapEligible;}
static bool8 ScriptContext1_IsScriptSetUp(void) {return scriptOneBusy;}
static bool8 ScriptContext2_IsEnabled(void) {return scriptBusy;}
static void ScriptContext2_Enable(void) {assert(!scriptBusy);scriptBusy=1;enableCount++;}
static void ScriptContext2_Disable(void) {assert(scriptBusy);scriptBusy=0;disableCount++;}
static bool8 BeginNormalPaletteFade(u32 mask,u8 delay,u8 start,u8 end,u16 color)
{
    assert(mask==PALETTES_ALL&&delay==1&&color==0&&!gPaletteFade.active);
    gPaletteFade.active=1;fadeCount++;lastFadeStart=start;lastFadeEnd=end;return TRUE;
}
static void FieldEffect_RefreshTimeOfDayPalette(void) {fieldEffectRefreshCount++;}
static void LoadMapTilesetPalettes(struct MapLayout *arg);
static void ApplyWeatherGammaShiftToPal(u8 index) {assert(index<13);gammaCount++;}
"""
loader="""
static void LoadMapTilesetPalettes(struct MapLayout *arg)
{
    assert(arg==&mapLayout);
    reloadCount++;
    if (!sTimeOfDayPaletteRefresh) sTimeOfDayTransition=0;
    sLastLoadedTimeOfDay=TimeOfDay_GetDesiredState();
}
"""
body="""
int main(void)
{
    assert(!TimeOfDay_IsNightAtSeconds(0));
    assert(!TimeOfDay_IsNightAtSeconds(1799));
    assert(TimeOfDay_IsNightAtSeconds(1800));
    assert(TimeOfDay_IsNightAtSeconds(3599));
    assert(!TimeOfDay_IsNightAtSeconds(3600));
    // Sleep until Night at zero playtime: immediate Night, persisted in saved var.
    gSpecialVar_0x8004=1; TimeOfDay_SleepUntil();
    assert(savedSleepOffset==720); assert(TimeOfDay_IsNight());
    // Cancel/invalid choice leaves the save phase unchanged.
    gSpecialVar_0x8004=3; TimeOfDay_SleepUntil(); assert(savedSleepOffset==720);
    gSpecialVar_0x8004=0; TimeOfDay_SleepUntil();
    assert(savedSleepOffset==0); assert(!TimeOfDay_IsNight());
    sLastLoadedTimeOfDay=TOD_DAY;
    TimeOfDay_TryUpdateOutdoorPalette(); assert(!fadeCount&&!reloadCount);
    save2.playTimeMinutes=30;
    weather.palProcessingState=1;
    TimeOfDay_TryUpdateOutdoorPalette();assert(!fadeCount);
    weather.palProcessingState=3;
    weather.weatherChangeComplete=0; // Stale flag must NOT lock the cycle.
    weather.nextWeather=2;
    TimeOfDay_TryUpdateOutdoorPalette();assert(!fadeCount); // Actual transition still blocks.
    weather.nextWeather=weather.currWeather;
    gQuestLogState=QL_STATE_PLAYBACK_LAST;
    TimeOfDay_TryUpdateOutdoorPalette();assert(!fadeCount);
    gQuestLogState=0;
    scriptOneBusy=1;
    TimeOfDay_TryUpdateOutdoorPalette();assert(!fadeCount);
    scriptOneBusy=0;
    TimeOfDay_TryUpdateOutdoorPalette();
    assert(fadeCount==1&&lastFadeStart==0&&lastFadeEnd==16&&scriptBusy);
    assert(!reloadCount&&sLastLoadedTimeOfDay==TOD_DAY);
    TimeOfDay_TryUpdateOutdoorPalette();assert(fadeCount==1);
    gPaletteFade.active=0;
    TimeOfDay_TryUpdateOutdoorPalette();
    assert(fadeCount==2&&lastFadeStart==16&&lastFadeEnd==0);
    assert(reloadCount==1&&gammaCount==13&&sLastLoadedTimeOfDay==TOD_NIGHT&&fieldEffectRefreshCount==1);
    gPaletteFade.active=0;
    TimeOfDay_TryUpdateOutdoorPalette();
    assert(!scriptBusy&&enableCount==1&&disableCount==1&&!sTimeOfDayTransition);
    TimeOfDay_TryUpdateOutdoorPalette();assert(fadeCount==2);
    sLastLoadedTimeOfDay=TOD_UNINITIALIZED;
    gGlobalFieldTintMode=1;gQuestLogState=QL_STATE_PLAYBACK_LAST;
    TimeOfDay_RestoreAfterQuestLog();assert(reloadCount==1);
    gGlobalFieldTintMode=0;gQuestLogState=0;
    TimeOfDay_RestoreAfterQuestLog();
    assert(reloadCount==2&&gammaCount==26&&sLastLoadedTimeOfDay==TOD_NIGHT&&fieldEffectRefreshCount==2);
    assert(fadeCount==2&&!scriptBusy);
    mapEligible=0;
    TimeOfDay_RestoreAfterQuestLog();assert(reloadCount==2);
    return 0;
}
"""
content=(header+'\n'.join(extract(n) for n in ['TimeOfDay_GetPlaySeconds','TimeOfDay_GetGameMinuteAtSeconds','TimeOfDay_IsNightAtSeconds','TimeOfDay_SleepUntil','TimeOfDay_IsNight'])+'\n'+extract('TimeOfDay_GetDesiredState')+'\n'+loader+
         extract('TimeOfDay_RefreshBackgroundPalettes')+'\n'+extract('TimeOfDay_RestoreAfterQuestLog')+'\n'+
         extract('TimeOfDay_TryUpdateOutdoorPalette')+body)
with tempfile.TemporaryDirectory() as td:
    file=Path(td)/'test.c'; file.write_text(content)
    subprocess.run(['gcc','-std=gnu11','-Wall','-Wextra','-Werror',str(file),'-o',str(Path(td)/'test')],check=True)
    subprocess.run([str(Path(td)/'test')],check=True)
print('PASS: compiled 30/30 clock, ~1s fade, Quest Log same-frame night, weather/script guards')
