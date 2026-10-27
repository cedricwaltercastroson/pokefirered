#!/usr/bin/env python3
"""Compile the production clock boundary controller against minimal weather/fade mocks."""
from pathlib import Path
import subprocess
import tempfile
import os

ROOT = Path(__file__).resolve().parents[1]
SOURCE = (ROOT / 'src/fieldmap.c').read_text()


def get_fn(header):
    start = SOURCE.index(header)
    brace = SOURCE.index('{', start)
    nesting = 0
    for i in range(brace, len(SOURCE)):
        if SOURCE[i] == '{':
            nesting += 1
        elif SOURCE[i] == '}':
            nesting -= 1
            if nesting == 0:
                return SOURCE[start:i + 1]
    raise AssertionError(header)


minute_fn = get_fn('static u32 TimeOfDay_GetGameMinuteAtSeconds(u32 seconds)')
clock_fn = get_fn('static bool8 TimeOfDay_IsNightAtSeconds(u32 seconds)')
update_fn = get_fn('void TimeOfDay_TryUpdateOutdoorPalette(void)')

harness = r'''
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
typedef unsigned char u8;
typedef unsigned int u32;
typedef bool bool8;
#define TRUE true
#define FALSE false
#define QL_TINT_NONE 0
#define QL_STATE_PLAYBACK 1
#define QL_STATE_PLAYBACK_LAST 2
#define WEATHER_PAL_STATE_IDLE 0
#define PALETTES_ALL 0xFFFFFFFFU
#define TOD_FADE_DELAY 1
#define RGB_BLACK 0
#define TOD_DAY 1
#define TOD_NIGHT 2
static u8 sTimeOfDayTransition=0;
static u8 sLastLoadedTimeOfDay=TOD_DAY;
static int script1=0,script2=0;
static int mapIsEligible=1;
static int fadeBegins=0, refreshes=0, scriptedDisable=0;
static u32 clockSeconds=1799;
struct { int active; } gPaletteFade;
struct { int palProcessingState, weatherChangeComplete, currWeather, nextWeather; } weather;
static typeof(weather) *gWeatherPtr=&weather;
static char save1,save2,layout;
static void *gSaveBlock1Ptr=&save1,*gSaveBlock2Ptr=&save2;
static struct { void *mapLayout; } gMapHeader={&layout};
static int gGlobalFieldTintMode=QL_TINT_NONE, gQuestLogState=0;
static bool8 TimeOfDay_IsEligibleMap(void) { return mapIsEligible; }
static bool8 ScriptContext1_IsScriptSetUp(void) { return script1; }
static bool8 ScriptContext2_IsEnabled(void) { return script2; }
static void ScriptContext2_Enable(void) { script2=1; }
static void ScriptContext2_Disable(void) { script2=0; scriptedDisable++; }
static bool8 BeginNormalPaletteFade(u32 selected, int delay, int start, int target, int color) {
 (void)selected;(void)delay;(void)start;(void)target;(void)color;
 if (gPaletteFade.active) return FALSE;
 gPaletteFade.active=1; fadeBegins++;
 return TRUE;
}
static bool8 TimeOfDay_IsNightAtSeconds(u32 seconds);
static void TimeOfDay_RefreshBackgroundPalettes(void) {
 refreshes++;
 sLastLoadedTimeOfDay = TimeOfDay_IsNightAtSeconds(clockSeconds) ? TOD_NIGHT : TOD_DAY;
}
'''
harness += r'''
#define TOD_MINUTES_PER_CYCLE 1440
#define TOD_DAY_START_MINUTES 480
#define TOD_NIGHT_START_MINUTES 1200
'''
harness += minute_fn + '\n' + clock_fn + '\n' 
harness += r'''
static u8 TimeOfDay_GetDesiredState(void) {
  return TimeOfDay_IsNightAtSeconds(clockSeconds) ? TOD_NIGHT : TOD_DAY;
}
'''
harness += update_fn + '\n'
harness += r'''
#define CHECK(condition, label) do { if (!(condition)) { fprintf(stderr, "FAIL: %s\n",label);return 1; } } while(0)
static void reset(void) {
 weather.palProcessingState = 0; weather.weatherChangeComplete=0;
 weather.currWeather=1; weather.nextWeather=1;
 sTimeOfDayTransition=0; sLastLoadedTimeOfDay=TOD_DAY;
 gPaletteFade.active=0; gQuestLogState=0; gGlobalFieldTintMode=0;
 fadeBegins=0; refreshes=0; scriptedDisable=0; script1=0; script2=0;
 mapIsEligible=1;
}
int main(void) {
 CHECK(!TimeOfDay_IsNightAtSeconds(1799),"before 30min = day");
 CHECK(TimeOfDay_IsNightAtSeconds(1800),"at 30min = night");
 CHECK(TimeOfDay_IsNightAtSeconds(3599),"before 60min = night");
 CHECK(!TimeOfDay_IsNightAtSeconds(3600),"at 60min = day");
 reset(); clockSeconds=1799;
 TimeOfDay_TryUpdateOutdoorPalette();
 CHECK(fadeBegins==0,"before threshold: no fade");
 clockSeconds=1800;
 TimeOfDay_TryUpdateOutdoorPalette();
 CHECK(fadeBegins==1 && sTimeOfDayTransition==1,
  "stale FALSE weather completion must not prevent 30min transition");
 gPaletteFade.active=0;
 TimeOfDay_TryUpdateOutdoorPalette();
 CHECK(fadeBegins==2 && refreshes==1 && sTimeOfDayTransition==2,
  "black screen swaps correct palettes and fades in");
 gPaletteFade.active=0;
 TimeOfDay_TryUpdateOutdoorPalette();
 CHECK(sTimeOfDayTransition==0 && scriptedDisable==1 && !script2,
  "transition returns player control");
 reset(); clockSeconds=1800; weather.nextWeather=2;
 TimeOfDay_TryUpdateOutdoorPalette();
 CHECK(fadeBegins==0,"genuine ongoing weather transition deferred");
 weather.nextWeather=weather.currWeather;
 TimeOfDay_TryUpdateOutdoorPalette();
 CHECK(fadeBegins==1,"transition resumes after actual weather settles");
 reset(); weather.palProcessingState=2;
 TimeOfDay_TryUpdateOutdoorPalette();
 CHECK(fadeBegins==0,"weather gamma processing still owns palette");
 reset(); script1=1;
 TimeOfDay_TryUpdateOutdoorPalette();
 CHECK(fadeBegins==0,"script guards retained");
 reset(); gQuestLogState=QL_STATE_PLAYBACK_LAST;
 TimeOfDay_TryUpdateOutdoorPalette();
 CHECK(fadeBegins==0,"quest log guard retained");
 reset(); mapIsEligible=0;
 TimeOfDay_TryUpdateOutdoorPalette();
 CHECK(fadeBegins==0,"interiors unaffected");
 puts("PASS: production time-boundary state machine + stale weather completion regression");
 return 0;
}
'''
with tempfile.TemporaryDirectory() as d:
    src = Path(d, 'test.c')
    exe = Path(d, 'test')
    src.write_text(harness)
    subprocess.run(['gcc','-std=gnu99','-Wno-unused-function','-O2','-Wall',str(src),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
