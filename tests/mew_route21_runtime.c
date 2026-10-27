// Compile from repo root:
// gcc -std=gnu99 -DFIRERED -DREVISION=1 -DENGLISH -DMODERN=1 -DBUGFIX \
//   -iquote include tests/mew_route21_runtime.c -o /tmp/mew_route21_test && /tmp/mew_route21_test
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "global.h"
#include "pokemon.h"
#include "overworld.h"
#include "field_player_avatar.h"
#include "fieldmap.h"
#include "constants/maps.h"
#include "constants/region_map_sections.h"

struct SaveBlock1 sSaveOne;
struct SaveBlock1 *gSaveBlock1Ptr = &sSaveOne;
struct Pokemon gEnemyParty[PARTY_SIZE];
static struct MapHeader sMockMapHeader;
static u32 sRandomCalls;
static u16 sRandomResult;
static u32 sSpawnCount;
static u32 sTileType = TILE_ENCOUNTER_LAND;
static u16 sLastSpecies;
static u32 sLastStatus;
static u32 sLastHp = 25;
static u32 sLastIvs = 42;
static u32 sLastPersonality = 1234;
static u32 sLastMonInitCount;

u16 Random(void)
{
    sRandomCalls++;
    return sRandomResult;
}
u16 GetStarterSpecies(void) { return SPECIES_BULBASAUR; }
void PlayerGetDestCoords(s16 *x, s16 *y) { *x = 17; *y = 28; }
u32 MapGridGetMetatileAttributeAt(s16 x, s16 y, u8 attributeType)
{
    assert(attributeType == METATILE_ATTRIBUTE_ENCOUNTER_TYPE);
    return sTileType;
}
const struct MapHeader *const Overworld_GetMapHeaderByGroupAndId(u16 group, u16 num)
{
    assert(group == MAP_GROUP(ROUTE21_NORTH));
    assert(num == MAP_NUM(ROUTE21_NORTH));
    sMockMapHeader.regionMapSectionId = MAPSEC_ROUTE_21;
    return &sMockMapHeader;
}
void ZeroEnemyPartyMons(void) { memset(gEnemyParty, 0, sizeof(gEnemyParty)); }
void CreateMon(struct Pokemon *mon, u16 species, u8 level, u8 fixedIV, u8 hasFixedPersonality, u32 fixedPersonality, u8 otIdType, u32 fixedOtId)
{
    sLastMonInitCount++;
    sLastSpecies = species;
}
void CreateMonWithIVsPersonality(struct Pokemon *mon, u16 species, u8 level, u32 ivs, u32 personality)
{
    sSpawnCount++;
    assert(species == SPECIES_MEW);
    assert(ivs == sLastIvs);
    assert(personality == sLastPersonality);
}
u32 GetMonData(struct Pokemon *mon, s32 field)
{
    switch (field)
    {
    case MON_DATA_MAX_HP: return sLastHp;
    case MON_DATA_HP: return sLastHp;
    case MON_DATA_STATUS: return sLastStatus;
    case MON_DATA_IVS: return sLastIvs;
    case MON_DATA_PERSONALITY: return sLastPersonality;
    default: return 0;
    }
}
void SetMonData(struct Pokemon *mon, s32 field, const void *data)
{
    if (field == MON_DATA_HP)
        assert(*(const u16 *)data == sLastHp);
    else if (field == MON_DATA_STATUS)
        assert(*(const u32 *)data == sLastStatus);
}

#include "../src/roamer.c"

static void VerifyRoute21Only(void)
{
    u8 group, num;
    GetRoamerLocation(&group, &num);
    assert(group == MAP_GROUP(ROUTE21_NORTH));
    assert(num == MAP_NUM(ROUTE21_NORTH));
    assert(IsRoamerAt(MAP_GROUP(ROUTE21_NORTH), MAP_NUM(ROUTE21_NORTH)));
    assert(!IsRoamerAt(MAP_GROUP(ROUTE1), MAP_NUM(ROUTE1)));
    assert(!IsRoamerAt(MAP_GROUP(PALLET_TOWN), MAP_NUM(PALLET_TOWN)));
    assert(!IsRoamerAt(MAP_GROUP(ROUTE21_SOUTH), MAP_NUM(ROUTE21_SOUTH)));
    assert(GetRoamerLocationMapSectionId() == MAPSEC_ROUTE_21);
}

int main(void)
{
    u32 before, i;
    struct Pokemon dummy = {0};
    InitRoamer();
    assert(sLastSpecies == SPECIES_MEW);
    assert(ROAMER->species == SPECIES_MEW && ROAMER->active);
    assert(ROAMER->level == 5);
    VerifyRoute21Only();

    // Map transitions and save loads must not rotate Mew or loop forever.
    before = sRandomCalls;
    for (i = 0; i < 20000; i++)
    {
        RoamerMove();
        RoamerMoveToOtherLocationSet();
        UpdateLocationHistoryForRoamer();
    }
    assert(sRandomCalls == before);
    VerifyRoute21Only();

    // Existing saves whose runtime location was Route 1 automatically repair.
    sRoamerLocation[MAP_GRP] = MAP_GROUP(ROUTE1);
    sRoamerLocation[MAP_NUM] = MAP_NUM(ROUTE1);
    assert(IsRoamerAt(MAP_GROUP(ROUTE21_NORTH), MAP_NUM(ROUTE21_NORTH)));
    VerifyRoute21Only();

    // Non-Route 21 and water/Sweet Scent encounters never produce Mew.
    sSaveOne.location.mapGroup = MAP_GROUP(ROUTE1);
    sSaveOne.location.mapNum = MAP_NUM(ROUTE1);
    assert(!TryStartRoamerEncounter());
    sSaveOne.location.mapGroup = MAP_GROUP(ROUTE21_NORTH);
    sSaveOne.location.mapNum = MAP_NUM(ROUTE21_NORTH);
    sTileType = TILE_ENCOUNTER_WATER;
    assert(!TryStartRoamerEncounter());
    sTileType = TILE_ENCOUNTER_LAND;
    sRandomResult = 1;
    assert(!TryStartRoamerEncounter());
    sRandomResult = 0;
    assert(TryStartRoamerEncounter());
    assert(sSpawnCount == 1);

    // Battle update still persists the same roamer; no map rotation on flee.
    sLastHp = 7;
    sLastStatus = 8;
    UpdateRoamerHPStatus(&dummy);
    assert(ROAMER->hp == 7 && ROAMER->status == 8);
    VerifyRoute21Only();

    // Defeat/capture permanently marks this single roamer inactive.
    SetRoamerInactive();
    assert(!TryStartRoamerEncounter());
    assert(!IsRoamerAt(MAP_GROUP(ROUTE21_NORTH), MAP_NUM(ROUTE21_NORTH)));
    assert(GetRoamerLocationMapSectionId() == MAPSEC_NONE);
    printf("PASS: Mew fixed to Route 21 grass; 20k transitions, save repair, encounter gating, uniqueness\n");
    return 0;
}
