// Host regression for the actual v0.8.25 weather-controller source.
// gcc -std=gnu99 -DFIRERED -DREVISION=1 -DENGLISH -DMODERN=1 \
//   -iquote include tests/dynamic_weather_runtime.c -o /tmp/weather_test && /tmp/weather_test
#include <assert.h>
#include <stdio.h>
#include "global.h"
#include "gflib.h"
#include "field_weather.h"
#include "constants/maps.h"
#include "constants/weather.h"

struct SaveBlock1 saveOne;
struct SaveBlock2 saveTwo;
struct SaveBlock1 *gSaveBlock1Ptr = &saveOne;
struct SaveBlock2 *gSaveBlock2Ptr = &saveTwo;
struct MapHeader gMapHeader;
struct PaletteFadeControl gPaletteFade;
static struct Weather mockWeather;
struct Weather *const gWeatherPtr = &mockWeather;
static unsigned changes;
static unsigned rainCounters;

void SetNextWeather(u8 weather)
{
    mockWeather.nextWeather = weather;
    changes++;
}
void SetCurrentAndNextWeather(u8 weather)
{
    mockWeather.currWeather = weather;
    mockWeather.nextWeather = weather;
}
void IncrementGameStat(u8 statId)
{
    rainCounters++;
}

// Bring in actual implementation, including the internal state for fast tests.
#include "../src/field_weather_util.c"

static void EnterMap(u16 map, u8 headerWeather)
{
    gSaveBlock1Ptr->location.mapGroup = map >> 8;
    gSaveBlock1Ptr->location.mapNum = map & 0xFF;
    gMapHeader.weather = headerWeather;
    SetSav1WeatherFromCurrMapHeader();
    SetCurrentAndNextWeather(GetSav1Weather());
}

int main(void)
{
    u8 originalWeather, newerWeather;
    u32 remaining;
    unsigned initialChanges;
    u16 i;
    const u16 outdoors[] = { MAP_PALLET_TOWN, MAP_VIRIDIAN_CITY, MAP_SAFFRON_CITY, MAP_ROUTE1, MAP_ROUTE17, MAP_ROUTE25 };
    const u16 safari[] = { MAP_SAFARI_ZONE_CENTER, MAP_SAFARI_ZONE_EAST, MAP_SAFARI_ZONE_NORTH, MAP_SAFARI_ZONE_WEST };
    const u16 excluded[] = { MAP_POKEMON_TOWER_3F, MAP_POKEMON_TOWER_7F, MAP_VIRIDIAN_FOREST, MAP_PALLET_TOWN_PLAYERS_HOUSE_1F };

    saveTwo.playerTrainerId[0] = 27;
    saveTwo.playerTrainerId[1] = 95;
    EnterMap(MAP_PALLET_TOWN, WEATHER_SUNNY);
    assert(sDynamicWeather.initialized);
    assert(GetSav1Weather() == sDynamicWeather.weather);
    assert(GetSav1Weather() == WEATHER_SUNNY || GetSav1Weather() == WEATHER_RAIN || GetSav1Weather() == WEATHER_RAIN_THUNDERSTORM);
    originalWeather = sDynamicWeather.weather;
    remaining = sDynamicWeather.framesUntilChange;
    for (i=0;i<sizeof(outdoors)/sizeof(outdoors[0]);i++)
    {
        EnterMap(outdoors[i], WEATHER_SUNNY);
        assert(GetSav1Weather() == originalWeather);
        assert(sDynamicWeather.framesUntilChange == remaining);
    }

    for (i=0;i<sizeof(safari)/sizeof(safari[0]);i++)
    {
        EnterMap(safari[i], WEATHER_NONE);
        assert(IsDynamicWeatherMap());
        assert(GetSav1Weather() == originalWeather);
        assert(sDynamicWeather.framesUntilChange == remaining);
    }
    for (i=0;i<sizeof(excluded)/sizeof(excluded[0]);i++)
    {
        EnterMap(excluded[i], WEATHER_FOG_HORIZONTAL);
        assert(GetSav1Weather() == WEATHER_FOG_HORIZONTAL);
        assert(!IsDynamicWeatherMap());
        DynamicOverworldWeather_Update();
        assert(sDynamicWeather.framesUntilChange == remaining);
    }
    EnterMap(MAP_PALLET_TOWN, WEATHER_SHADE); // A future custom override on a normally eligible map
    assert(GetSav1Weather() == WEATHER_SHADE && !IsDynamicWeatherMap());
    EnterMap(MAP_ROUTE1, WEATHER_SUNNY);
    assert(GetSav1Weather() == originalWeather);

    // A fade and pending weather transition must not advance timer.
    sDynamicWeather.framesUntilChange = 1;
    gPaletteFade.active = TRUE;
    DynamicOverworldWeather_Update();
    assert(sDynamicWeather.framesUntilChange == 1);
    gPaletteFade.active = FALSE;
    mockWeather.nextWeather = (mockWeather.currWeather == WEATHER_RAIN ? WEATHER_SUNNY : WEATHER_RAIN);
    DynamicOverworldWeather_Update();
    assert(sDynamicWeather.framesUntilChange == 1);
    mockWeather.nextWeather = mockWeather.currWeather;

    // Scripted weather overrides must not be overwritten by the dynamic system.
    saveOne.weather = WEATHER_FOG_DIAGONAL;
    DynamicOverworldWeather_Update();
    assert(sDynamicWeather.framesUntilChange == 1);
    assert(GetSav1Weather() == WEATHER_FOG_DIAGONAL);
    saveOne.weather = originalWeather;

    initialChanges = changes;
    DynamicOverworldWeather_Update();
    newerWeather = sDynamicWeather.weather;
    assert(changes == initialChanges + (newerWeather != originalWeather));
    assert(GetSav1Weather() == newerWeather);
    assert(mockWeather.nextWeather == newerWeather);
    assert(sDynamicWeather.framesUntilChange >= DYNAMIC_WEATHER_MIN_FRAMES);
    assert(sDynamicWeather.framesUntilChange < DYNAMIC_WEATHER_MIN_FRAMES + DYNAMIC_WEATHER_VARIABLE_FRAMES);
    // A building visit does not re-roll weather or its countdown.
    remaining = sDynamicWeather.framesUntilChange;
    EnterMap(MAP_PALLET_TOWN_PLAYERS_HOUSE_1F, WEATHER_NONE);
    DynamicOverworldWeather_Update();
    EnterMap(MAP_PALLET_TOWN, WEATHER_SUNNY);
    assert(GetSav1Weather() == newerWeather);
    assert(sDynamicWeather.framesUntilChange == remaining);

    // A fixed independent PRNG sample must produce approximately 20% rain
    // or thunderstorms, without the old guaranteed rain-after-sun behavior.
    {
        u32 sunny = 0, rain = 0, thunder = 0;
        sDynamicWeather.rngState = 0x12345678;
        for (i = 0; i < 20000; i++)
        {
            u8 w = ChooseNextDynamicWeather();
            if (w == WEATHER_SUNNY) sunny++;
            else if (w == WEATHER_RAIN) rain++;
            else if (w == WEATHER_RAIN_THUNDERSTORM) thunder++;
            else assert(0);
        }
        assert(sunny > 15500 && sunny < 16500);
        assert(rain > 3200 && rain < 3600);
        assert(thunder > 450 && thunder < 750);
        printf("20,000 draws: clear %u, rain %u, thunderstorm %u\n", sunny, rain, thunder);
    }
    puts("PASS: Kanto and Safari allowlist, 20% rain selection, persistence, transitions and script overrides");
    return 0;
}
