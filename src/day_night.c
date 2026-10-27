#include "global.h"
#include "gflib.h"
#include "day_night.h"
#include "field_weather.h"
#include "overworld.h"
#include "quest_log.h"

// 120 real gameplay minutes = 24 in-game hours. FireRed's existing saved
// playtime is our clock; we do not need an RTC or a save-format migration.
// New games begin at 08:00, in full daylight.
#define DAY_NIGHT_CYCLE_SECONDS (120 * 60)
#define DAY_NIGHT_START_MINUTE  (8 * 60)
#define DAY_NIGHT_MINUTES       (24 * 60)

// Only two states. Unlike the v0.9.0.2 prototype, there is no warm
// morning, evening, or colour interpolation.
// 08:00-19:59 = native FireRed daylight; 20:00-07:59 = dark night.
#define DAY_NIGHT_DAY_START_MINUTE (8 * 60)
#define DAY_NIGHT_NIGHT_START_MINUTE (20 * 60)

// v0.9.0.3.3: richer night colour grading, with no global brightness scale.
// Retain 224/256 of native chroma (vs 120/256 in the pale v0.9.0.3.2),
// then apply a gentle pivoted contrast curve: darker midtone/shadow colours,
// intact bright scenery, and a very small cool shift. RGB555 integer-only.
#define DAY_NIGHT_CHROMA_WEIGHT 224
#define DAY_NIGHT_NEUTRAL_WEIGHT (256 - DAY_NIGHT_CHROMA_WEIGHT)
#define DAY_NIGHT_CONTRAST_STRENGTH 40
#define DAY_NIGHT_CONTRAST_PIVOT 16
#define DAY_NIGHT_NIGHT_TONE_OFFSET 1

static u16 DayNight_GetMinuteOfDay(void)
{
    u32 seconds = (u32)gSaveBlock2Ptr->playTimeHours * 3600
                + (u32)gSaveBlock2Ptr->playTimeMinutes * 60
                + gSaveBlock2Ptr->playTimeSeconds;

    return (DAY_NIGHT_START_MINUTE + (seconds % DAY_NIGHT_CYCLE_SECONDS) / 5)
        % DAY_NIGHT_MINUTES;
}

static bool8 DayNight_IsNight(u16 minute)
{
    // Two states only: 08:00-19:59 native day; 20:00-07:59 night.
    return minute < DAY_NIGHT_DAY_START_MINUTE
        || minute >= DAY_NIGHT_NIGHT_START_MINUTE;
}

static u16 DayNight_ClampColourChannel(s16 tone, u16 original)
{
    // Retain texture in low RGB555 values without lifting actual black.
    if (tone < 1 && original != 0)
        return 1;
    if (tone < 0)
        return 0;
    if (tone > 31)
        return 31;
    return (u16)tone;
}

static s16 DayNight_GradeChannel(u16 channel, u16 luma)
{
    s16 tone = (channel * DAY_NIGHT_CHROMA_WEIGHT
              + luma * DAY_NIGHT_NEUTRAL_WEIGHT + 128) >> 8;

    // Increase tonal separation around a neutral pivot instead of reducing
    // every pixel's brightness. Highlights stay luminous; midtones and
    // shadows become deeper and less flat.
    tone += ((tone - DAY_NIGHT_CONTRAST_PIVOT)
             * DAY_NIGHT_CONTRAST_STRENGTH) / 256;
    return tone - DAY_NIGHT_NIGHT_TONE_OFFSET;
}

static void DayNight_TintPalette(volatile u16 *dest, const u16 *src,
                                 u16 count, bool8 night)
{
    u16 i;

    // Restore the exact WEATHER-PROCESSED source palette during daytime.
    // Never read back from previously tinted hardware palette memory.
    if (!night)
    {
        for (i = 0; i < count; i++)
            dest[i] = src[i];
        return;
    }

    for (i = 0; i < count; i++)
    {
        u16 color = src[i];
        u16 r, g, b, luma;
        s16 gradedR, gradedG, gradedB;

        // Preserve transparent and outline black, plus RGB555 upper bit.
        if ((color & 0x7FFF) == 0)
        {
            dest[i] = color;
            continue;
        }

        r = color & 31;
        g = (color >> 5) & 31;
        b = (color >> 10) & 31;
        luma = (r * 77 + g * 150 + b * 29 + 128) >> 8;

        gradedR = DayNight_GradeChannel(r, luma) - 1; // quiet blue cast
        gradedG = DayNight_GradeChannel(g, luma);
        gradedB = DayNight_GradeChannel(b, luma);

        dest[i] = (color & 0x8000)
                | (DayNight_ClampColourChannel(gradedB, b) << 10)
                | (DayNight_ClampColourChannel(gradedG, g) << 5)
                | DayNight_ClampColourChannel(gradedR, r);
    }
}

void DayNight_ApplyOverworldPalette(void)
{
    bool8 night;

    // Quest Log replay owns its own grayscale/sepia palette processing.
    // Never re-grade those palettes in the field VBlank callback: the Quest
    // Log uses the same callback during playback AND the final saved-game
    // scene. Resume the day/night effect only on return to normal play.
    if (gQuestLogState == QL_STATE_PLAYBACK
        || gQuestLogState == QL_STATE_PLAYBACK_LAST
        || gGlobalFieldTintMode != QL_TINT_NONE)
        return;

    // Share the exact outdoor-map allowlist with dynamic weather. This keeps
    // all buildings, caves, Safari Zone, Viridian Forest and Pokémon Tower's
    // original fog, shade and palette behaviour intact.
    if (!DynamicOverworldWeather_IsEligibleMap()
        || gPaletteFade.bufferTransferDisabled)
        return;

    night = DayNight_IsNight(DayNight_GetMinuteOfDay());

    // Modify ONLY the hardware palette, never the source buffers. This also
    // explicitly restores neutral daytime colours on the first day frame.
    // Weather gamma, shiny follower palettes and field fades remain intact.
    // Called after the native palette transfer in the overworld VBlank.
    DayNight_TintPalette((volatile u16 *)PLTT, gPlttBufferFaded,
                         PLTT_BUFFER_SIZE, night);
}
