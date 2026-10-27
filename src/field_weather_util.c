#include "global.h"
#include "field_weather.h"
#include "overworld.h"
#include "gflib.h"
#include "constants/maps.h"
#include "constants/weather.h"

// v0.9.0.3.8: Dynamic weather is opt-in for Kanto outdoor towns, routes,
// and the four open-air Safari Zone sections.
// The controller is session-scoped, so building entry/exit never rerolls weather.
// It intentionally does not alter the on-disk save block layout.
#define DYNAMIC_WEATHER_MIN_FRAMES (60 * 60 * 2)
#define DYNAMIC_WEATHER_VARIABLE_FRAMES (60 * 60 * 2)

struct DynamicOverworldWeather
{
    u32 rngState;
    u32 framesUntilChange;
    u8 weather;
    bool8 initialized;
};

static EWRAM_DATA struct DynamicOverworldWeather sDynamicWeather = {};

static bool8 IsDynamicWeatherMap(void);
static u32 NextDynamicWeatherRandom(void);
static void InitializeDynamicWeather(void);
static u8 ChooseNextDynamicWeather(void);
static u8 TranslateWeatherNum(u8 weather);
static void UpdateRainCounter(u8 newWeather, u8 oldWeather);

void SetSav1Weather(u32 weather)
{
    u8 oldWeather = gSaveBlock1Ptr->weather;
    gSaveBlock1Ptr->weather = TranslateWeatherNum(weather);
    UpdateRainCounter(gSaveBlock1Ptr->weather, oldWeather);
}

u8 GetSav1Weather(void)
{
    return gSaveBlock1Ptr->weather;
}

void SetSav1WeatherFromCurrMapHeader(void)
{
    u8 oldWeather = gSaveBlock1Ptr->weather;

    if (IsDynamicWeatherMap())
    {
        InitializeDynamicWeather();
        gSaveBlock1Ptr->weather = sDynamicWeather.weather;
    }
    else
    {
        // Dungeon and interior weather always comes from the original map header.
        gSaveBlock1Ptr->weather = TranslateWeatherNum(gMapHeader.weather);
    }

    UpdateRainCounter(gSaveBlock1Ptr->weather, oldWeather);
}

void SetWeather(u32 weather)
{
    SetSav1Weather(weather);
    SetNextWeather(GetSav1Weather());
}

void SetWeather_Unused(u32 weather)
{
    SetSav1Weather(weather);
    SetCurrentAndNextWeather(GetSav1Weather());
}

void DoCurrentWeather(void)
{
    u8 weather = GetSav1Weather();

    SetNextWeather(weather);
}

void ResumePausedWeather(void)
{
    u8 weather = GetSav1Weather();

    SetCurrentAndNextWeather(weather);
}

// Opt in only normal Kanto outdoors and the four open-air Safari Zone maps.
// Safari's header uses WEATHER_NONE while other ordinary Kanto maps use
// WEATHER_SUNNY. Never override special weather on indoor/dungeon maps.
static bool8 IsDynamicWeatherMap(void)
{
    u16 mapId = gSaveBlock1Ptr->location.mapNum | (gSaveBlock1Ptr->location.mapGroup << 8);

    switch (mapId)
    {
    case MAP_SAFARI_ZONE_CENTER:
    case MAP_SAFARI_ZONE_EAST:
    case MAP_SAFARI_ZONE_NORTH:
    case MAP_SAFARI_ZONE_WEST:
        return gMapHeader.weather == WEATHER_NONE || gMapHeader.weather == WEATHER_SUNNY;
    default:
        break;
    }

    if (gMapHeader.weather != WEATHER_SUNNY)
        return FALSE;

    switch (mapId)
    {
    case MAP_PALLET_TOWN:
    case MAP_VIRIDIAN_CITY:
    case MAP_PEWTER_CITY:
    case MAP_CERULEAN_CITY:
    case MAP_LAVENDER_TOWN:
    case MAP_VERMILION_CITY:
    case MAP_CELADON_CITY:
    case MAP_FUCHSIA_CITY:
    case MAP_CINNABAR_ISLAND:
    case MAP_INDIGO_PLATEAU_EXTERIOR:
    case MAP_SAFFRON_CITY:
    case MAP_SAFFRON_CITY_CONNECTION:
    case MAP_ROUTE1:
    case MAP_ROUTE2:
    case MAP_ROUTE3:
    case MAP_ROUTE4:
    case MAP_ROUTE5:
    case MAP_ROUTE6:
    case MAP_ROUTE7:
    case MAP_ROUTE8:
    case MAP_ROUTE9:
    case MAP_ROUTE10:
    case MAP_ROUTE11:
    case MAP_ROUTE12:
    case MAP_ROUTE13:
    case MAP_ROUTE14:
    case MAP_ROUTE15:
    case MAP_ROUTE16:
    case MAP_ROUTE17:
    case MAP_ROUTE18:
    case MAP_ROUTE19:
    case MAP_ROUTE20:
    case MAP_ROUTE21_NORTH:
    case MAP_ROUTE21_SOUTH:
    case MAP_ROUTE22:
    case MAP_ROUTE23:
    case MAP_ROUTE24:
    case MAP_ROUTE25:
        return TRUE;
    default:
        return FALSE;
    }
}

// Expose the exact same allowlist to the follower's outdoor rain palette
// correction. Special areas and every indoor/dungeon retain stock rendering.
bool8 DynamicOverworldWeather_IsEligibleMap(void)
{
    return IsDynamicWeatherMap();
}

// Independent PRNG: dynamic weather must not alter the game's encounter/battle
// RNG. Use the trainer ID and play clock for session variation.
static u32 NextDynamicWeatherRandom(void)
{
    sDynamicWeather.rngState = sDynamicWeather.rngState * 1664525 + 1013904223;
    return sDynamicWeather.rngState >> 16;
}

// Each independent weather draw has an 80% chance of clear skies and a
// COMBINED 20% chance of precipitation: 17% rain, 3% thunderstorm. Unlike the
// previous state machine, sunshine is not forced to become rain at the next
// refresh. Keep using our private PRNG, never the encounter RNG.
static u8 ChooseNextDynamicWeather(void)
{
    u32 roll = NextDynamicWeatherRandom() % 100;
    if (roll < 80)
        return WEATHER_SUNNY;
    if (roll < 97)
        return WEATHER_RAIN;
    return WEATHER_RAIN_THUNDERSTORM;
}

static void InitializeDynamicWeather(void)
{
    if (sDynamicWeather.initialized)
        return;

    sDynamicWeather.rngState = 0x8A5CD789
        ^ ((u32)gSaveBlock2Ptr->playerTrainerId[0] << 24)
        ^ ((u32)gSaveBlock2Ptr->playerTrainerId[1] << 16)
        ^ ((u32)gSaveBlock2Ptr->playerTrainerId[2] << 8)
        ^ gSaveBlock2Ptr->playerTrainerId[3]
        ^ ((u32)gSaveBlock2Ptr->playTimeHours << 10)
        ^ (gSaveBlock2Ptr->playTimeMinutes << 4)
        ^ gSaveBlock2Ptr->playTimeSeconds;
    sDynamicWeather.weather = ChooseNextDynamicWeather();
    sDynamicWeather.framesUntilChange = DYNAMIC_WEATHER_MIN_FRAMES
        + NextDynamicWeatherRandom() % DYNAMIC_WEATHER_VARIABLE_FRAMES;
    sDynamicWeather.initialized = TRUE;
}

void DynamicOverworldWeather_Update(void)
{
    u8 nextWeather;

    if (!sDynamicWeather.initialized || !IsDynamicWeatherMap())
        return;

    // Do not interfere with scripted weather overrides or field transitions.
    if (gSaveBlock1Ptr->weather != sDynamicWeather.weather
        || gWeatherPtr->currWeather != gWeatherPtr->nextWeather
        || gPaletteFade.active)
        return;

    if (--sDynamicWeather.framesUntilChange != 0)
        return;

    nextWeather = ChooseNextDynamicWeather();
    sDynamicWeather.weather = nextWeather;
    sDynamicWeather.framesUntilChange = DYNAMIC_WEATHER_MIN_FRAMES
        + NextDynamicWeatherRandom() % DYNAMIC_WEATHER_VARIABLE_FRAMES;
    // A second clear-weather roll is intentional: 20% precipitation is a
    // true per-selection probability, not a forced change of state.
    if (gSaveBlock1Ptr->weather != nextWeather)
        SetWeather(nextWeather); // Native gradual weather transition.
}

static const u8 sWeatherCycleRoute119[] = {
    WEATHER_SUNNY,
    WEATHER_RAIN,
    WEATHER_RAIN_THUNDERSTORM,
    WEATHER_RAIN,
};

static const u8 sWeatherCycleRoute123[] = {
    WEATHER_SUNNY,
    WEATHER_SUNNY,
    WEATHER_RAIN,
    WEATHER_SUNNY,
};

static u8 TranslateWeatherNum(u8 weather)
{
    switch (weather)
    {
    case WEATHER_NONE:               return WEATHER_NONE;
    case WEATHER_SUNNY_CLOUDS:       return WEATHER_SUNNY_CLOUDS;
    case WEATHER_SUNNY:              return WEATHER_SUNNY;
    case WEATHER_RAIN:               return WEATHER_RAIN;
    case WEATHER_SNOW:               return WEATHER_SNOW;
    case WEATHER_RAIN_THUNDERSTORM:  return WEATHER_RAIN_THUNDERSTORM;
    case WEATHER_FOG_HORIZONTAL:     return WEATHER_FOG_HORIZONTAL;
    case WEATHER_VOLCANIC_ASH:       return WEATHER_VOLCANIC_ASH;
    case WEATHER_SANDSTORM:          return WEATHER_SANDSTORM;
    case WEATHER_FOG_DIAGONAL:       return WEATHER_FOG_DIAGONAL;
    case WEATHER_UNDERWATER:         return WEATHER_UNDERWATER;
    case WEATHER_SHADE:              return WEATHER_SHADE;
    case WEATHER_DROUGHT:            return WEATHER_DROUGHT;
    case WEATHER_DOWNPOUR:           return WEATHER_DOWNPOUR;
    case WEATHER_UNDERWATER_BUBBLES: return WEATHER_UNDERWATER_BUBBLES;
    case WEATHER_ROUTE119_CYCLE:     return sWeatherCycleRoute119[gSaveBlock1Ptr->weatherCycleStage];
    case WEATHER_ROUTE123_CYCLE:     return sWeatherCycleRoute123[gSaveBlock1Ptr->weatherCycleStage];
    default:                         return WEATHER_NONE;
    }
}

void UpdateWeatherPerDay(u16 increment)
{
    u16 weatherStage = gSaveBlock1Ptr->weatherCycleStage + increment;
    weatherStage %= 4;
    gSaveBlock1Ptr->weatherCycleStage = weatherStage;
}

static void UpdateRainCounter(u8 newWeather, u8 oldWeather)
{
    if (newWeather != oldWeather
        && (newWeather == WEATHER_RAIN || newWeather == WEATHER_RAIN_THUNDERSTORM))
        IncrementGameStat(GAME_STAT_GOT_RAINED_ON);
}
