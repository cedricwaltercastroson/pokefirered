// Host regression for the actual v0.9.0 day/night implementation.
// gcc -std=gnu99 -DFIRERED -DREVISION=1 -DENGLISH -DMODERN=1 \
//   -iquote include tests/day_night_v090_runtime.c -o /tmp/day_night_v090 && /tmp/day_night_v090
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "global.h"
#include "gflib.h"
#include "field_weather.h"
#include "overworld.h"
#include "quest_log.h"

static struct SaveBlock2 saveTwo;
struct SaveBlock2 *gSaveBlock2Ptr = &saveTwo;
struct PaletteFadeControl gPaletteFade;
u16 gPlttBufferFaded[PLTT_BUFFER_SIZE];
static volatile u16 hardwarePalette[PLTT_BUFFER_SIZE];
static bool8 isOutdoor;
u8 gQuestLogState = 0;
u8 gGlobalFieldTintMode = QL_TINT_NONE;

bool8 DynamicOverworldWeather_IsEligibleMap(void)
{
    return isOutdoor;
}

// Redirect hardware palette writes into ordinary host memory.
#undef PLTT
#define PLTT ((uintptr_t)hardwarePalette)
#include "../src/day_night.c"

static void SetClock(u16 hours, u8 minutes, u8 seconds)
{
    saveTwo.playTimeHours = hours;
    saveTwo.playTimeMinutes = minutes;
    saveTwo.playTimeSeconds = seconds;
}

static void CheckNightColor(void)
{
    u16 dark = hardwarePalette[1];
    assert((dark & 31) <= 31);
    assert(((dark >> 5) & 31) <= 31);
    assert(((dark >> 10) & 31) >= (dark & 31));
    assert(gPlttBufferFaded[1] == RGB_WHITE);
    // Bright white stays nearly white; shadows deepen instead of global dimming.
    assert((dark & 31) >= 29 && (dark & 31) <= 31);
    assert(((dark >> 5) & 31) >= 30 && ((dark >> 5) & 31) <= 31);
    assert(((dark >> 10) & 31) >= 30 && ((dark >> 10) & 31) <= 31);
}

int main(void)
{
    u16 i;
    u16 muted, source;
    u16 oldChroma, newChroma;
    for (i = 0; i < PLTT_BUFFER_SIZE; ++i)
        gPlttBufferFaded[i] = RGB(20, 20, 20);
    gPlttBufferFaded[0] = RGB_BLACK;
    gPlttBufferFaded[1] = RGB_WHITE;
    gPlttBufferFaded[16 * (16 + 11) + 1] = RGB(24, 12, 30); // reserved follower sprite palette

    // New game begins in daylight; 120 gameplay minutes complete one full day.
    SetClock(0, 0, 0);
    assert(DayNight_GetMinuteOfDay() == 480);
    assert(DayNight_GetMinuteOfDay() == (8 * 60));
    SetClock(0, 45, 0);
    assert(DayNight_GetMinuteOfDay() == 17 * 60);
    SetClock(1, 0, 0);
    assert(DayNight_GetMinuteOfDay() == 20 * 60);
    SetClock(1, 45, 0);
    assert(DayNight_GetMinuteOfDay() == 5 * 60);
    SetClock(2, 0, 0);
    assert(DayNight_GetMinuteOfDay() == 8 * 60);
    SetClock(123, 31, 10);
    assert(DayNight_GetMinuteOfDay() == ((8 * 60 + ((123 * 3600 + 31 * 60 + 10) % 7200) / 5) % 1440));

    // Strict two-state switch, with no gradual in-between light states.
    assert(DayNight_IsNight(7 * 60 + 59));
    assert(!DayNight_IsNight(8 * 60));
    assert(!DayNight_IsNight(12 * 60));
    assert(!DayNight_IsNight(19 * 60 + 59));
    assert(DayNight_IsNight(20 * 60));
    assert(DayNight_IsNight(23 * 60 + 59));
    assert(DayNight_IsNight(5 * 60));

    // Daytime explicitly restores the weather-processed source palette,
    // even if display memory still contains the previous night's tint.
    SetClock(0, 0, 0);
    isOutdoor = TRUE;
    hardwarePalette[1] = 0x4321;
    DayNight_ApplyOverworldPalette();
    assert(hardwarePalette[1] == RGB_WHITE);

    // Night colour grade applies to all palettes, including the follower.
    // Dark environmental colours stay readable; pure black remains black.
    gPlttBufferFaded[3] = RGB(5, 5, 5);
    gPlttBufferFaded[4] = RGB(20, 8, 5);
    SetClock(1, 0, 0);
    DayNight_ApplyOverworldPalette();
    assert(hardwarePalette[0] == RGB_BLACK);
    CheckNightColor();
    assert((hardwarePalette[3] & 31) >= 2);
    assert(((hardwarePalette[3] >> 10) & 31) >= ((hardwarePalette[3] >> 5) & 31));
    assert(((hardwarePalette[4] & 31) > ((hardwarePalette[4] >> 5) & 31)));
    assert(hardwarePalette[16 * (16 + 11) + 1] != gPlttBufferFaded[16 * (16 + 11) + 1]);
    // Rich colour must survive: red-brown stays deep red, not pale grey.
    source = gPlttBufferFaded[4];
    muted = hardwarePalette[4];
    oldChroma = (source & 31) - ((source >> 10) & 31);
    newChroma = (muted & 31) - ((muted >> 10) & 31);
    assert(newChroma >= 10); // v0.9.0.3.2 washed this down to ~4
    assert(newChroma <= oldChroma);
    assert((muted & 31) >= 16); // dark red, not near-black
    assert(((muted >> 5) & 31) < ((source >> 5) & 31)); // deepen midtones
    // Chroma not lost on the follower's reserved palette slot.
    assert(hardwarePalette[16 * (16 + 11) + 1] != gPlttBufferFaded[16 * (16 + 11) + 1]);
    assert(hardwarePalette[0] == RGB_BLACK);
    // Repeated updates do not accumulate brightness loss in source buffers.
    DayNight_ApplyOverworldPalette();
    CheckNightColor();

    // Night -> day at exactly 08:00: exact palette restoration without leaving the
    // map, including the dedicated follower palette slot.
    SetClock(2, 0, 0);
    DayNight_ApplyOverworldPalette();
    assert(hardwarePalette[1] == RGB_WHITE);
    assert(hardwarePalette[16 * (16 + 11) + 1] == gPlttBufferFaded[16 * (16 + 11) + 1]);
    // A rain-processed palette should return to its weather colour, not the
    // raw unweathered tileset colour. Subsequent weather updates remain safe.
    gPlttBufferFaded[2] = RGB(18, 23, 27);
    hardwarePalette[2] = RGB(5, 8, 10);
    DayNight_ApplyOverworldPalette();
    assert(hardwarePalette[2] == RGB(18, 23, 27));
    // 60 active minutes later = 20:00 and night must immediately apply.
    SetClock(3, 0, 0);
    DayNight_ApplyOverworldPalette();
    assert(hardwarePalette[2] != gPlttBufferFaded[2]);
    SetClock(3, 59, 55);
    DayNight_ApplyOverworldPalette();
    assert(hardwarePalette[2] != gPlttBufferFaded[2]);
    SetClock(4, 0, 0);
    DayNight_ApplyOverworldPalette();
    assert(hardwarePalette[2] == gPlttBufferFaded[2]);

    // Quest Log playback must control its own grayscale/sepia fade and saved
    // game transition palettes. Both playback and final return use the field
    // VBlank callback, even on otherwise eligible outdoor maps such as Route 1.
    SetClock(1, 0, 0); // 20:00 — night: expose a missing guard.
    isOutdoor = TRUE;
    gPlttBufferFaded[1] = RGB_WHITE;
    hardwarePalette[1] = 0x4321;
    gQuestLogState = QL_STATE_PLAYBACK;
    DayNight_ApplyOverworldPalette();
    assert(hardwarePalette[1] == 0x4321);
    gQuestLogState = QL_STATE_PLAYBACK_LAST;
    hardwarePalette[1] = 0x1234;
    DayNight_ApplyOverworldPalette();
    assert(hardwarePalette[1] == 0x1234);
    gQuestLogState = 0;
    gGlobalFieldTintMode = QL_TINT_GRAYSCALE;
    hardwarePalette[1] = 0x2345;
    DayNight_ApplyOverworldPalette();
    assert(hardwarePalette[1] == 0x2345);
    gGlobalFieldTintMode = QL_TINT_BACKUP_GRAYSCALE;
    DayNight_ApplyOverworldPalette();
    assert(hardwarePalette[1] == 0x2345);
    gGlobalFieldTintMode = QL_TINT_NONE;
    DayNight_ApplyOverworldPalette();
    assert(hardwarePalette[1] != 0x2345);
    SetClock(2, 0, 0); // Restored normal day afterwards.
    DayNight_ApplyOverworldPalette();
    assert(hardwarePalette[1] == RGB_WHITE);

    // Indoor/dungeon weather (Pokémon Tower's fog included) is never tinted.
    isOutdoor = FALSE;
    hardwarePalette[1] = 0x1234;
    DayNight_ApplyOverworldPalette();
    assert(hardwarePalette[1] == 0x1234);
    isOutdoor = TRUE;
    gPaletteFade.bufferTransferDisabled = TRUE;
    DayNight_ApplyOverworldPalette();
    assert(hardwarePalette[1] == 0x1234);

    puts("PASS: two-state 2-hour clock, richer deep-tone night, intact daylight, weather/follower protection");
    return 0;
}
