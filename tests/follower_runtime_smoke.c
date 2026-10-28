// Host-side simulation of the real follower C module. Not a GBA/emulator test.
// Build: clang -std=gnu11 -O0 -ffunction-sections -fdata-sections ...
#include <assert.h>
#include <stdio.h>
#include "global.h"
#include "gflib.h"
#include "pokemon.h"
#include "following_pokemon.h"
#include "palette.h"
#include "field_weather.h"
#include "constants/weather.h"
#include "event_object_movement.h"
#include "constants/species.h"
#include "constants/event_object_movement.h"
#include "constants/quest_log.h"
#include "constants/maps.h"
#include "constants/songs.h"

struct SaveBlock1 mockSave;
struct SaveBlock1 *gSaveBlock1Ptr = &mockSave;
struct ObjectEvent gObjectEvents[OBJECT_EVENTS_COUNT];
struct PlayerAvatar gPlayerAvatar;
struct Sprite gSprites[64];
struct Pokemon gPlayerParty[PARTY_SIZE];
u8 gPlayerPartyCount = 1;
u8 gQuestLogState;
u8 gQuestLogPlaybackState;
u8 gSelectedObjectEvent;
u8 gGlobalFieldTintMode;
struct PaletteFadeControl gPaletteFade;
static struct Weather mockWeather;
struct Weather *const gWeatherPtr = &mockWeather;
u8 GetCurrentWeather(void) { return mockWeather.currWeather; }
u16 gPlttBufferUnfaded[PLTT_BUFFER_SIZE];
u16 gPlttBufferFaded[PLTT_BUFFER_SIZE];
static unsigned mockFaceTurns, mockFreezes, mockClears;

u8 gStringVar1[32], gStringVar4[1000];
static u16 mockSpecies = SPECIES_CHARMANDER;
static u16 mockHp = 20, mockMaxHp = 20, mockFriendship = 160;
static u32 mockStatus = 0;
static const char *mockNickname = "CHARMANDER";
static bool8 mockShiny = FALSE;
static bool8 mockWeatherFading = FALSE;
static bool8 mockScriptLocked = FALSE;
static bool8 mockScript1Active = FALSE;
static unsigned mockRecallSeCount;
static u16 mockLastSe;
static bool8 mockSpawnFailure;
static unsigned mockCryCount;
static u16 mockLastCrySpecies;
static u8 mockLastCryMode;
static s16 mockWaterX = -999, mockWaterY = -999;
static s16 mockSpinX = -999, mockSpinY = -999;
static u8 mockSpinBehavior;
static s16 mockDoorX = -999, mockDoorY = -999;
static s16 mockWarpX = -999, mockWarpY = -999;
static s16 mockLedgeX = -999, mockLedgeY = -999;
static u8 mockLedgeDir = DIR_NONE;

static const void *mockPaletteLoaded;
static unsigned mockPaletteLoads;
static unsigned mockWeatherPaletteCalls;
static bool8 mockWeatherFadeScreen;

bool8 IsMonShiny(struct Pokemon *mon)
{
    (void)mon;
    return mockShiny;
}

u8 *StringCopy(u8 *dest, const u8 *src)
{
    size_t n = strlen((const char *)src);
    memcpy(dest, src, n + 1);
    return dest + n;
}

u8 *StringExpandPlaceholders(u8 *dest, const u8 *src)
{
    // The host compiler keeps the literal placeholder rather than using
    // FireRed's original charmap converter. Resolve it here for smoke tests.
    const char *p = (const char *)src;
    const char *token = "{STR_VAR_1}";
    size_t n = strlen(token);
    char *out = (char *)dest;
    while (*p)
    {
        if (strncmp(p, token, n) == 0)
        {
            const char *name = (const char *)gStringVar1;
            while (*name) *out++ = *name++;
            p += n;
        }
        else
            *out++ = *p++;
    }
    *out = 0;
    return (u8 *)out;
}
static s16 mockBlockedX = -999, mockBlockedY = -999;
static unsigned mockMoves, mockRemoves, mockTeleports;

u32 GetMonData(struct Pokemon *mon, s32 field, u8 *data)
{
    if (field == MON_DATA_SPECIES)
        return mockSpecies;
    if (field == MON_DATA_IS_EGG)
        return FALSE;
    if (field == MON_DATA_NICKNAME)
    {
        if (data)
            memcpy(data, mockNickname, strlen(mockNickname) + 1);
        return 0;
    }
    if (field == MON_DATA_HP)
        return mockHp;
    if (field == MON_DATA_MAX_HP)
        return mockMaxHp;
    if (field == MON_DATA_FRIENDSHIP)
        return mockFriendship;
    if (field == MON_DATA_STATUS)
        return mockStatus;
    return 0;
}

bool8 TryGetObjectEventIdByLocalIdAndMap(u8 localId, u8 mapNum, u8 mapGroup, u8 *id)
{
    u8 i;
    // FireRed's special local-ID lookup ignores map group/number.
    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        if (gObjectEvents[i].active && gObjectEvents[i].localId == localId)
        {
            *id = i;
            return FALSE;
        }
    }
    *id = OBJECT_EVENTS_COUNT;
    return TRUE;
}

void RemoveFollowerObjectEventById(u8 id)
{
    if (id < OBJECT_EVENTS_COUNT && gObjectEvents[id].active
     && !gObjectEvents[id].isPlayer
     && gObjectEvents[id].localId == FOLLOWER_LOCAL_ID
     && gObjectEvents[id].graphicsId == OBJ_EVENT_GFX_FOLLOWER)
    {
        gObjectEvents[id].active = FALSE;
        mockRemoves++;
    }
}

void RemoveFollowerObjectEvent(void)
{
    u8 i;
    // Stub of the engine-side global sweep, including stale-map duplicates.
    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        struct ObjectEvent *e = &gObjectEvents[i];
        if (e->active && !e->isPlayer && e->localId == FOLLOWER_LOCAL_ID
         && e->graphicsId == OBJ_EVENT_GFX_FOLLOWER)
        {
            e->active = FALSE;
            mockRemoves++;
        }
    }
}

int SpawnSpecialObjectEventParameterized(u8 gfx, u8 behavior, u8 localId, s16 x, s16 y, u8 elevation)
{
    assert(localId == FOLLOWER_LOCAL_ID);
    if (mockSpawnFailure)
        return OBJECT_EVENTS_COUNT;
    gObjectEvents[1].active = TRUE;
    gObjectEvents[1].localId = localId;
    gObjectEvents[1].graphicsId = gfx;
    gObjectEvents[1].currentCoords.x = x;
    gObjectEvents[1].currentCoords.y = y;
    gObjectEvents[1].currentElevation = elevation;
    gObjectEvents[1].spriteId = 1;
    // FireRed initializes a freshly spawned object; don't inherit a stale
    // held movement from a previously removed follower in the test stub.
    gObjectEvents[1].heldMovementActive = FALSE;
    gObjectEvents[1].heldMovementFinished = FALSE;
    gObjectEvents[1].movementActionId = 0;
    return 1;
}

u8 GetObjectEventIdByXY(s16 x, s16 y)
{
    u8 i;
    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        if (gObjectEvents[i].active && gObjectEvents[i].currentCoords.x == x
         && gObjectEvents[i].currentCoords.y == y)
            return i;
    }
    return OBJECT_EVENTS_COUNT;
}

u8 GetOppositeDirection(u8 d)
{
    if (d == DIR_EAST) return DIR_WEST;
    if (d == DIR_WEST) return DIR_EAST;
    if (d == DIR_SOUTH) return DIR_NORTH;
    if (d == DIR_NORTH) return DIR_SOUTH;
    return DIR_NONE;
}

void MoveCoords(u8 d, s16 *x, s16 *y)
{
    if (d == DIR_EAST) ++*x;
    if (d == DIR_WEST) --*x;
    if (d == DIR_SOUTH) ++*y;
    if (d == DIR_NORTH) --*y;
}

bool8 MapGridIsImpassableAt(s32 x, s32 y) { return x == mockBlockedX && y == mockBlockedY; }
s32 GetMapBorderIdAt(s32 x, s32 y) { return 0; }
u8 MapGridGetMetatileBehaviorAt(s32 x, s32 y)
{
    if (x == mockSpinX && y == mockSpinY)
        return mockSpinBehavior;
    if (x == mockDoorX && y == mockDoorY)
        return 0x69; // MB_WARP_DOOR
    if (x == mockWarpX && y == mockWarpY)
        return 0x6F; // MB_WARP_PAD
    return (x == mockWaterX && y == mockWaterY) ? 0x10 : 0;
}
bool8 MetatileBehavior_IsSurfable(u8 b) { return b == 0x10; }
bool8 MetatileBehavior_IsWarpDoor(u8 b) { return b == 0x69; }
bool8 MetatileBehavior_IsWarpDoor_2(u8 b) { return b == 0x69; }
bool8 MetatileBehavior_IsNonAnimDoor(u8 b) { return b == 0x6A; }
bool8 MetatileBehavior_IsSpinRight(u8 b) { return b == 0x54; }
bool8 MetatileBehavior_IsSpinLeft(u8 b) { return b == 0x55; }
bool8 MetatileBehavior_IsSpinUp(u8 b) { return b == 0x56; }
bool8 MetatileBehavior_IsSpinDown(u8 b) { return b == 0x57; }
bool8 MetatileBehavior_IsSpinTile(u8 b) { return b >= 0x54 && b <= 0x57; }
bool8 MetatileBehavior_IsStopSpinning(u8 b) { return b == 0x58; }
bool8 MetatileBehavior_IsWarpPad(u8 b) { return b == 0x6F; }
bool8 ScriptContext2_IsEnabled(void) { return mockScriptLocked; }
bool8 ScriptContext1_IsScriptSetUp(void) { return mockScript1Active; }
void PlaySE(u16 songNum) { mockLastSe = songNum; mockRecallSeCount++; }
void PlayCry7(u16 species, u8 mode)
{
    mockCryCount++;
    mockLastCrySpecies = species;
    mockLastCryMode = mode;
}
u8 GetLedgeJumpDirection(s16 x, s16 y, u8 direction)
{
    return (x == mockLedgeX && y == mockLedgeY && direction == mockLedgeDir) ? direction : DIR_NONE;
}

void LoadPalette(const void *src, u16 offset, u16 size)
{
    assert(offset == 0x100 + 11 * 16);
    assert(size == 32);
    assert(src != NULL);
    mockPaletteLoaded = src;
    memcpy(&gPlttBufferUnfaded[offset], src, size);
    memcpy(&gPlttBufferFaded[offset], src, size);
    mockPaletteLoads++;
}
bool8 IsWeatherNotFadingIn(void) { return !mockWeatherFading; }
void UpdateSpritePaletteWithWeather(u8 palIdx)
{
    unsigned i;
    assert(palIdx == 11);
    mockWeatherPaletteCalls++;
    // The engine initialises newly loaded OBJ palettes at the fade colour
    // during a weather-controlled fade-in, even if the underlying selected
    // palette is already correct. Approximate that visible-buffer contract.
    if (mockWeatherFadeScreen)
        for (i = 0; i < 16; i++)
            gPlttBufferFaded[0x100 + palIdx * 16 + i] = 0;
    else
        memcpy(&gPlttBufferFaded[0x100 + palIdx * 16],
               &gPlttBufferUnfaded[0x100 + palIdx * 16], 32);
}
// Native BlendPalette uses GBA RGB555 arithmetic; model its normal fade.
static unsigned mockBlends;
void BlendPalette(u16 palOffset, u16 numEntries, u8 coeff, u16 blendColor)
{
    u16 i;
    mockBlends++;
    for (i = 0; i < numEntries; i++)
    {
        u16 src = gPlttBufferUnfaded[palOffset + i];
        int r = src & 31, g = (src >> 5) & 31, b = (src >> 10) & 31;
        int rr = blendColor & 31, gg = (blendColor >> 5) & 31, bb = (blendColor >> 10) & 31;
        r += ((rr - r) * coeff) >> 4;
        g += ((gg - g) * coeff) >> 4;
        b += ((bb - b) * coeff) >> 4;
        gPlttBufferFaded[palOffset + i] = r | (g << 5) | (b << 10);
    }
}
void ApplyGlobalFieldPaletteTint(u8 paletteIdx)
{
    u8 i;
    u16 *palette = &gPlttBufferUnfaded[0x100 + paletteIdx * 16];
    if (gGlobalFieldTintMode != QL_TINT_NONE)
    {
        // Simulate the Quest Log grayscale transformation. Subsequent map
        // loads will correct it by restoring the selected follower palette.
        for (i = 0; i < 16; i++)
        {
            u16 c = palette[i];
            u16 r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
            u16 gray = (r + g + b) / 3;
            palette[i] = gray | (gray << 5) | (gray << 10);
        }
        memcpy(&gPlttBufferFaded[0x100 + paletteIdx * 16], palette, 32);
    }
}
void ObjectEventTurn(struct ObjectEvent *e, u8 dir) { e->facingDirection = dir; mockFaceTurns++; }
u8 GetPlayerFacingDirection(void) { return gObjectEvents[gPlayerAvatar.objectEventId].facingDirection; }
bool8 FreezeObjectEvent(struct ObjectEvent *e) { e->frozen = TRUE; mockFreezes++; return FALSE; }

bool8 ObjectEventIsHeldMovementActive(struct ObjectEvent *e) { return e->heldMovementActive; }
u8 ObjectEventClearHeldMovementIfFinished(struct ObjectEvent *e)
{
    if (e->heldMovementFinished)
    {
        e->heldMovementActive = FALSE;
        e->heldMovementFinished = FALSE;
        return TRUE;
    }
    return FALSE;
}
bool8 ObjectEventIsMovementOverridden(struct ObjectEvent *e) { return e->heldMovementActive || e->singleMovementActive; }
void ObjectEventClearHeldMovementIfActive(struct ObjectEvent *e) { e->heldMovementActive = FALSE; mockClears++; }
// FireRed lockall freezes NPCs, including the dynamically spawned follower.
// The League cutscene explicitly unfreezes ONLY its follower for path tracking.
void UnfreezeObjectEvent(struct ObjectEvent *e) { e->frozen = FALSE; }

void MoveObjectEventToMapCoords(struct ObjectEvent *e, s16 x, s16 y)
{
    e->currentCoords.x = x;
    e->currentCoords.y = y;
    mockTeleports++;
}

u8 GetWalkNormalMovementAction(u32 d) { return d; }
u8 GetWalkFastMovementAction(u32 d) { return d + 20; }
u8 GetWalkFastestMovementAction(u32 d) { return d + 40; }
u8 GetJump2MovementAction(u32 d) { return d + 60; }
u8 GetSpinMovementAction(u32 d) { return d + 80; }
bool8 ObjectEventSetHeldMovement(struct ObjectEvent *e, u8 action)
{
    assert(!e->heldMovementActive);
    e->heldMovementActive = TRUE;
    e->movementActionId = action;
    mockMoves++;
    return FALSE;
}

static void CompleteFollowerMovement(void)
{
    struct ObjectEvent *e = &gObjectEvents[1];
    u8 action = e->movementActionId;
    u8 dir = action % 20;
    s16 x = e->currentCoords.x, y = e->currentCoords.y;
    MoveCoords(dir, &x, &y);
    if (action >= 60 && action < 80) MoveCoords(dir, &x, &y);
    e->currentCoords.x = x;
    e->currentCoords.y = y;
    e->heldMovementFinished = TRUE;
}

int main(void)
{
    struct ObjectEvent *player = &gObjectEvents[0];
    unsigned before;
    player->active = TRUE;
    player->isPlayer = TRUE;
    player->facingDirection = DIR_EAST;
    player->currentCoords.x = 10;
    player->currentCoords.y = 10;
    mockSave.location.mapNum = 1;
    mockSave.location.mapGroup = 1;

    // v0.9.0.3.22: a B-skipped Quest Log can reach the field with no
    // follower sprite yet. The repair must wait until one actually exists;
    // v0.9.0.3.21 incorrectly cancelled it immediately in this state.
    before = mockPaletteLoads;
    Follower_ArmQuestLogSkipPaletteRepair();
    Follower_RepairQuestLogSkipSpriteSlot();
    Follower_RepairQuestLogSkipPalette();
    assert(mockPaletteLoads == before);
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    assert(gObjectEvents[1].currentCoords.x == 9);
    {
        unsigned i;
        // Palette colours in slot 11 alone cannot repair a sprite which is
        // drawing from another OBJ slot. Correct its actual OAM paletteNum
        // before BuildOamBuffer, then repair the displayed colours.
        for (i = 0; i < 4; i++)
        {
            u8 spriteId = gObjectEvents[1].spriteId;
            unsigned prior = mockPaletteLoads;
            gSprites[spriteId].oam.paletteNum = 3;
            gPlttBufferFaded[0x1B1] = 0;
            Follower_RepairQuestLogSkipSpriteSlot();
            assert(gSprites[spriteId].oam.paletteNum == 11);
            Follower_RepairQuestLogSkipPalette();
            assert(mockPaletteLoads == prior + 1);
            assert(gPlttBufferFaded[0x1B1] == gPlttBufferUnfaded[0x1B1]);
        }
        // No permanent palette work once the four *ready* frames complete.
        before = mockPaletteLoads;
        for (i = 0; i < 90; i++)
            Follower_RepairQuestLogSkipPalette();
        assert(mockPaletteLoads == before);
    }
    // The next unfaded overworld update is the post-load palette repair.
    Follower_Update(player);
    // Changing shiny status for the same species swaps only the OBJ palette,
    // without respawning or disturbing the following movement trail.
    {
        const void *normalPalette = mockPaletteLoaded;
        unsigned beforeRemoves = mockRemoves;
        unsigned beforeLoads = mockPaletteLoads;
        mockShiny = TRUE;
        Follower_Update(player);
        assert(mockPaletteLoads == beforeLoads + 1);
        assert(mockPaletteLoaded != normalPalette);
        assert(mockRemoves == beforeRemoves && gObjectEvents[1].active);
        Follower_RefreshPalette();
        assert(mockPaletteLoaded != normalPalette);
        mockShiny = FALSE;
        Follower_Update(player);
        assert(mockPaletteLoaded == normalPalette);
        assert(mockRemoves == beforeRemoves);
    }
    // Emerald's horizontal spacing: visual sprite offset only, one tile
    // behind in the object-event grid. East places sprite 8px to the west.
    assert(gSprites[1].x2 == -8);
    assert(gSprites[1].y2 == 0);
    assert(gObjectEvents[1].currentCoords.x == 9);
    ObjectEventTurn(&gObjectEvents[1], DIR_WEST);
    Follower_Update(player);
    assert(gSprites[1].x2 == 8);
    ObjectEventTurn(&gObjectEvents[1], DIR_NORTH);
    Follower_Update(player);
    assert(gSprites[1].x2 == 0);
    ObjectEventTurn(&gObjectEvents[1], DIR_SOUTH);
    Follower_Update(player);
    assert(gSprites[1].x2 == 0);
    ObjectEventTurn(&gObjectEvents[1], DIR_EAST);
    Follower_Update(player);
    assert(gSprites[1].x2 == -8);

    player->currentCoords.x = 11;
    Follower_Update(player);
    assert(mockMoves == 1);
    before = mockTeleports;
    player->currentCoords.y = 11;
    Follower_Update(player);
    assert(mockMoves == 1 && mockTeleports == before); // no corner snap

    CompleteFollowerMovement();
    Follower_Update(player);
    assert(mockMoves == 2);
    CompleteFollowerMovement();
    Follower_Update(player);
    assert(gObjectEvents[1].currentCoords.x == 11);
    CompleteFollowerMovement();
    Follower_Update(player);
    assert(gObjectEvents[1].currentCoords.y == 10);
    assert(mockTeleports == before);

    // A fresh map can load with an impassable tile behind the player.
    Follower_ResetForMapLoad();
    gObjectEvents[1].active = FALSE;
    mockSave.location.mapNum = 2;
    player->currentCoords.x = 20;
    player->currentCoords.y = 20;
    player->facingDirection = DIR_EAST;
    mockBlockedX = 19; mockBlockedY = 20;
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    player->currentCoords.x = 21;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    assert(gObjectEvents[1].currentCoords.x == 20);

    // v0.8.23: Bicycle recall plays once on actual removal; dismount
    // plays the Center-style ball-open sound once on successful respawn.
    before = mockRemoves;
    {
        unsigned seBefore = mockRecallSeCount;
        gPlayerAvatar.flags |= PLAYER_AVATAR_FLAG_MACH_BIKE;
        Follower_Update(player);
        assert(!gObjectEvents[1].active && mockRemoves == before + 1);
        assert(mockRecallSeCount == seBefore + 1 && mockLastSe == SE_BALL_TRADE);
        Follower_Update(player); // Still cycling: no duplicate recall sound.
        assert(mockRecallSeCount == seBefore + 1);
        gPlayerAvatar.flags = 0;
        Follower_Update(player);
        assert(gObjectEvents[1].active);
        assert(mockRecallSeCount == seBefore + 2 && mockLastSe == SE_BALL_OPEN);
        Follower_Update(player);
        assert(mockRecallSeCount == seBefore + 2); // No repeated release.
    }

    // A failed follower allocation must NOT play the bicycle release yet.
    // Once the sprite can be created, the pending sound fires exactly once.
    {
        unsigned seBefore = mockRecallSeCount;
        gPlayerAvatar.flags = PLAYER_AVATAR_FLAG_ACRO_BIKE;
        Follower_Update(player);
        assert(!gObjectEvents[1].active);
        assert(mockRecallSeCount == seBefore + 1 && mockLastSe == SE_BALL_TRADE);
        gPlayerAvatar.flags = PLAYER_AVATAR_FLAG_ON_FOOT;
        mockSpawnFailure = TRUE;
        Follower_Update(player);
        assert(!gObjectEvents[1].active && mockRecallSeCount == seBefore + 1);
        mockSpawnFailure = FALSE;
        Follower_Update(player);
        assert(gObjectEvents[1].active);
        assert(mockRecallSeCount == seBefore + 2 && mockLastSe == SE_BALL_OPEN);
        Follower_Update(player);
        assert(mockRecallSeCount == seBefore + 2);
    }

    // v0.8.1: Quest Log replay should display the current party leader.
    // Playback is a separate overworld mode, not a reason to recall it.
    before = mockRemoves;
    gQuestLogState = QL_STATE_PLAYBACK;
    Follower_Update(player);
    assert(gObjectEvents[1].active && mockRemoves == before);
    gQuestLogState = 0;
    Follower_Update(player);
    assert(gObjectEvents[1].active);

    // v0.8.1: The bicycle item callback signals its completed on-foot
    // transition. The pending latch is released without requiring a warp.
    before = mockRemoves;
    {
        unsigned seBefore = mockRecallSeCount;
        gPlayerAvatar.flags = PLAYER_AVATAR_FLAG_MACH_BIKE;
        Follower_Update(player);
        assert(!gObjectEvents[1].active && mockRemoves == before + 1);
        assert(mockRecallSeCount == seBefore + 1 && mockLastSe == SE_BALL_TRADE);
        // A stale transition hold used to prevent any return until changing maps.
        gPlayerAvatar.flags = PLAYER_AVATAR_FLAG_ON_FOOT;
        gPlayerAvatar.preventStep = TRUE;
        Follower_Update(player);
        assert(!gObjectEvents[1].active);
        assert(mockRecallSeCount == seBefore + 1); // Not during dismount.
        Follower_AfterBicycleDismount();
        Follower_Update(player);
        assert(gObjectEvents[1].active);
        assert(mockRecallSeCount == seBefore + 2 && mockLastSe == SE_BALL_OPEN);
        gPlayerAvatar.preventStep = FALSE;
    }

    // When the field loader completes, it primes a follower before rendering
    // the visible first frame (e.g., returning to the map after a battle).
    Follower_ResetForMapLoad();
    gObjectEvents[1].active = FALSE;
    gPlayerAvatar.objectEventId = 0;
    Follower_PrimeForOverworldDisplay();
    assert(gObjectEvents[1].active);

    // v0.8.4: Quest Log startup MUST NOT flash the follower in front of
    // the replay player, even for one frame. If the player's recorded
    // direction is unknown, wait for a real step and use the vacated tile.
    Follower_ResetForMapLoad();
    gObjectEvents[1].active = FALSE;
    mockBlockedX = -999; mockBlockedY = -999;
    gQuestLogState = QL_STATE_PLAYBACK;
    player->currentCoords.x = 30;
    player->currentCoords.y = 30;
    player->facingDirection = DIR_NONE;
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    Follower_Update(player); // Stationary replay frames must not spawn ahead.
    assert(!gObjectEvents[1].active);
    player->facingDirection = DIR_EAST;
    player->currentCoords.x = 31;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    assert(gObjectEvents[1].currentCoords.x == 30 && gObjectEvents[1].currentCoords.y == 30);

    // If an old Quest Log sprite has overlapped the player, restore only
    // to the verified REAR position; do not choose an arbitrary neighbor.
    MoveObjectEventToMapCoords(&gObjectEvents[1], 31, 30);
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    assert(gObjectEvents[1].currentCoords.x == 30 && gObjectEvents[1].currentCoords.y == 30);

    // Valid facing with a clear rear tile must still create the follower
    // before the first recorded step; the fix is not a blanket delay.
    Follower_ResetForMapLoad();
    gObjectEvents[1].active = FALSE;
    player->currentCoords.x = 30;
    player->currentCoords.y = 30;
    player->facingDirection = DIR_NORTH;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    assert(gObjectEvents[1].currentCoords.x == 30 && gObjectEvents[1].currentCoords.y == 31);

    // Repro of v0.8.3 visual bug: replay opens outside a building with
    // the tile immediately behind the player blocked. The old search tried
    // the SOUTH neighbor (front of a south-facing player) and jumped back
    // after the first recorded movement. v0.8.4 defers until the first step.
    Follower_ResetForMapLoad();
    gObjectEvents[1].active = FALSE;
    player->currentCoords.x = 30;
    player->currentCoords.y = 30;
    player->facingDirection = DIR_SOUTH;
    mockBlockedX = 30; mockBlockedY = 29; // Rear = north, e.g. door/wall.
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    player->currentCoords.y = 31;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    assert(gObjectEvents[1].currentCoords.x == 30 && gObjectEvents[1].currentCoords.y == 30);

    // An already-overlapping replay sprite with an obstructed rear should
    // be recalled rather than teleported in front of the player.
    MoveObjectEventToMapCoords(&gObjectEvents[1], 30, 31);
    mockBlockedX = 30; mockBlockedY = 30;
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    mockBlockedX = -999; mockBlockedY = -999;
    gQuestLogState = 0;

    // v0.8.6 regression: when a trainer blocks the player's rear tile after
    // battle, the follower must NOT appear on a left, right, or front tile.
    // If the real rear tile is blocked, wait for a real movement step.
    Follower_ResetForMapLoad();
    gObjectEvents[1].active = FALSE;
    player->currentCoords.x = 30;
    player->currentCoords.y = 30;
    player->facingDirection = DIR_EAST;
    gObjectEvents[2].active = TRUE;
    gObjectEvents[2].localId = 22;
    gObjectEvents[2].currentCoords.x = 29; // Rear of east-facing player.
    gObjectEvents[2].currentCoords.y = 30;
    Follower_NotifyBattleReturn();
    Follower_PrimeForOverworldDisplay();
    assert(!gObjectEvents[1].active); // No sideways post-battle snap.
    Follower_Update(player);           // Still stationary during trainer text.
    assert(!gObjectEvents[1].active);
    gObjectEvents[2].active = FALSE;
    player->currentCoords.x = 31;     // First walking step vacates (30,30).
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    assert(gObjectEvents[1].currentCoords.x == 30 && gObjectEvents[1].currentCoords.y == 30);
    assert(gObjectEvents[1].facingDirection == DIR_EAST);

    // v0.8.15: an unobstructed post-battle return is also recalled. It
    // NEVER appears during stationary trainer dialogue or the field fade;
    // the first player-controlled step triggers a fresh follower.
    Follower_ResetForMapLoad();
    gObjectEvents[1].active = FALSE;
    player->currentCoords.x = 30;
    player->currentCoords.y = 30;
    player->facingDirection = DIR_NORTH;
    Follower_NotifyBattleReturn();
    Follower_PrimeForOverworldDisplay();
    assert(!gObjectEvents[1].active);
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    mockScriptLocked = TRUE;
    player->currentCoords.y = 29; // A cutscene moved the player, not input.
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    mockScriptLocked = FALSE;
    Follower_Update(player); // Remain hidden at the cutscene's final tile.
    assert(!gObjectEvents[1].active);
    player->currentCoords.y = 28;
    gPlayerAvatar.tileTransitionState = T_TILE_TRANSITION;
    Follower_Update(player); // Spawn as the first player-controlled step starts.
    assert(gObjectEvents[1].active);
    gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    assert(gObjectEvents[1].currentCoords.x == 30 && gObjectEvents[1].currentCoords.y == 29);
    assert(gObjectEvents[1].facingDirection == DIR_NORTH);

    // Repeated battles must never reuse a stale follower OBJ or teleport it.
    before = mockRemoves;
    Follower_NotifyBattleReturn();
    Follower_PrimeForOverworldDisplay();
    assert(mockRemoves == before + 1 && !gObjectEvents[1].active);
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    player->currentCoords.y = 27;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    assert(gObjectEvents[1].currentCoords.y == 28);

    // Return fade while palette buffers are stale: do not reload a palette
    // and do not draw a follower on ANY post-battle fading frame. Once a
    // normal/shiny follower spawns after the first step, its fresh OBJ slot
    // must already have the correct selected palette.
    {
        u16 *unfaded = &gPlttBufferUnfaded[0x100 + 11 * 16];
        u16 *faded = &gPlttBufferFaded[0x100 + 11 * 16];
        unsigned i, prevLoads;
        const void *normalPalette = mockPaletteLoaded;
        gPaletteFade.active = TRUE;
        mockWeatherFading = TRUE;
        mockWeatherFadeScreen = TRUE;
        Follower_NotifyBattleReturn();
        prevLoads = mockPaletteLoads;
        Follower_PrimeForOverworldDisplay();
        assert(!gObjectEvents[1].active && mockPaletteLoads == prevLoads);
        for (i = 0; i < 16; i++)
            unfaded[i] = faded[i] = 0x4210; // Actual reported grey frame.
        Follower_StabilizeBattleReturnPalette();
        Follower_StabilizeBattleReturnSpriteSlot();
        Follower_Update(player);
        assert(!gObjectEvents[1].active);
        assert(mockPaletteLoads == prevLoads);
        gPaletteFade.active = FALSE;
        mockWeatherFading = FALSE;
        mockWeatherFadeScreen = FALSE;
        Follower_Update(player);
        assert(!gObjectEvents[1].active);
        player->currentCoords.y = 26;
        Follower_Update(player);
        assert(gObjectEvents[1].active && gObjectEvents[1].currentCoords.y == 27);
        assert(mockPaletteLoads == prevLoads + 1);
        assert(mockPaletteLoaded == normalPalette);
        assert(memcmp(unfaded, normalPalette, 32) == 0);
        prevLoads = mockPaletteLoads;
        Follower_Update(player);
        assert(mockPaletteLoads == prevLoads); // No deferred second repaint.

        mockShiny = TRUE;
        Follower_NotifyBattleReturn();
        Follower_PrimeForOverworldDisplay();
        assert(!gObjectEvents[1].active);
        Follower_Update(player);
        assert(!gObjectEvents[1].active);
        player->currentCoords.y = 25;
        Follower_Update(player);
        assert(gObjectEvents[1].active && gObjectEvents[1].currentCoords.y == 26);
        assert(mockPaletteLoaded != normalPalette);
        assert(memcmp(unfaded, mockPaletteLoaded, 32) == 0);
        mockShiny = FALSE;
        Follower_Update(player);
    }

    player->currentCoords.x = 45;
    player->currentCoords.y = 45;
    player->facingDirection = DIR_EAST;
    // v0.8.15 regression: an NPC can occupy the very tile vacated by the
    // player. Do not spawn on top of it or pick a side/front alternative.
    Follower_ResetForMapLoad();
    gObjectEvents[1].active = FALSE;
    player->currentCoords.x = 80;
    player->currentCoords.y = 80;
    player->facingDirection = DIR_EAST;
    Follower_NotifyBattleReturn();
    Follower_PrimeForOverworldDisplay();
    gObjectEvents[2].active = TRUE;
    gObjectEvents[2].currentCoords.x = 80;
    gObjectEvents[2].currentCoords.y = 80;
    player->currentCoords.x = 81;
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    gObjectEvents[2].active = FALSE;
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    player->currentCoords.x = 82;
    Follower_Update(player);
    assert(gObjectEvents[1].active && gObjectEvents[1].currentCoords.x == 81);
    assert(gObjectEvents[1].currentCoords.y == 80);

    // The camera rebases player tiles even while the follower is absent.
    // The stored vacated tile must follow the exact same coordinate shift.
    Follower_ResetForMapLoad();
    gObjectEvents[1].active = FALSE;
    player->currentCoords.x = 60;
    player->currentCoords.y = 60;
    Follower_NotifyBattleReturn();
    Follower_PrimeForOverworldDisplay();
    Follower_RebaseForCameraUpdate(10, 0);
    player->currentCoords.x = 50;
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    player->currentCoords.x = 51;
    Follower_Update(player);
    assert(gObjectEvents[1].active && gObjectEvents[1].currentCoords.x == 50);
    assert(gObjectEvents[1].currentCoords.y == 60);

    // A separate full map load cancels battle recall; normal map spawning
    // must not be suppressed permanently by a previous interrupted battle.
    Follower_NotifyBattleReturn();
    Follower_PrimeForOverworldDisplay();
    assert(!gObjectEvents[1].active);
    Follower_ResetForMapLoad();
    Follower_Update(player);
    assert(gObjectEvents[1].active);

    // A spinner reached immediately after battle takes ownership of recall.
    // Only STOP_SPINNING may release the follower, not its delayed battle
    // anchor or the intermediate forced movement coordinates.
    Follower_ResetForMapLoad();
    gObjectEvents[1].active = FALSE;
    player->currentCoords.x = 90;
    player->currentCoords.y = 90;
    player->facingDirection = DIR_EAST;
    player->currentMetatileBehavior = 0;
    Follower_NotifyBattleReturn();
    Follower_PrimeForOverworldDisplay();
    mockSpinX = 90; mockSpinY = 90; mockSpinBehavior = 0x54;
    player->currentMetatileBehavior = 0x54; // SPIN_RIGHT
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    player->currentCoords.x = 93;
    mockSpinX = 93; mockSpinY = 90; mockSpinBehavior = 0x58;
    player->currentMetatileBehavior = 0x58; // STOP_SPINNING
    gPlayerAvatar.flags = PLAYER_AVATAR_FLAG_FORCED;
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    gPlayerAvatar.flags = 0;
    Follower_Update(player);
    assert(gObjectEvents[1].active && gObjectEvents[1].currentCoords.x == 92);
    mockSpinX = -999;
    player->currentMetatileBehavior = 0;

    player->currentCoords.x = 45;
    player->currentCoords.y = 45;
    player->facingDirection = DIR_EAST;
    // Stairs temporarily move the player sprite by sub-tile pixel offsets.
    // Recall the follower during the entrance/exit animation, including the
    // ResetObjectEvents stage, then restore at the rear after exit completion.
    before = mockRemoves;
    Follower_BeginStairWarp();
    assert(mockRemoves == before + 1 && !gObjectEvents[1].active);
    Follower_ResetForMapLoad();
    Follower_PrimeForOverworldDisplay();
    assert(!gObjectEvents[1].active); // Held across map reload and fade.
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    Follower_EndStairWarp();
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    assert(gObjectEvents[1].currentCoords.x == 44 && gObjectEvents[1].currentCoords.y == 45);

    // Trainer still occupies the apparent rear position: the post-battle
    // follower must stay hidden and use a safe VACATED tile, never a side.
    gObjectEvents[2].active = TRUE;
    gObjectEvents[2].currentCoords.x = 44;
    gObjectEvents[2].currentCoords.y = 45;
    before = mockRemoves;
    Follower_NotifyBattleReturn();
    Follower_PrimeForOverworldDisplay();
    assert(mockRemoves == before + 1);
    assert(!gObjectEvents[1].active);
    gObjectEvents[2].active = FALSE;
    Follower_Update(player); // No stationary respawn even after NPC leaves.
    assert(!gObjectEvents[1].active);
    player->currentCoords.x = 46;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    assert(gObjectEvents[1].currentCoords.x == 45);

    // Trainer return dialog should not trigger a release while locked.
    Follower_BeginScriptedPlayerMovement();
    Follower_EndScriptedPlayerMovement();
    mockScriptLocked = TRUE;
    Follower_NotifyBattleReturn();
    Follower_PrimeForOverworldDisplay();
    assert(!gObjectEvents[1].active);
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    mockScriptLocked = FALSE;
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    player->currentCoords.x = 47;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    assert(gObjectEvents[1].currentCoords.x == 46);

    // A genuinely active scripted movement stays recalled and cannot make
    // the follower appear even after a battle. Script release restores the
    // established scripted-movement behaviour (not battle fade behaviour).
    Follower_BeginScriptedPlayerMovement();
    assert(!gObjectEvents[1].active);
    Follower_NotifyBattleReturn();
    Follower_PrimeForOverworldDisplay();
    assert(!gObjectEvents[1].active);
    Follower_EndScriptedPlayerMovement();
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    player->currentCoords.x = 48;
    Follower_Update(player);
    assert(gObjectEvents[1].active);

    // v0.7.1: FireRed's Jump2 updates currentCoords TWICE. The follower must
    // not treat the intermediate ledge tile as a normal walking destination.
    // Like Emerald's delayed jump action, wait until the player takes one
    // step AFTER landing before the follower performs a native Jump2.
    Follower_ResetForMapLoad();
    gObjectEvents[1].active = FALSE;
    mockBlockedX = -999;
    mockSave.location.mapNum = 3;
    player->currentCoords.x = 40;
    player->currentCoords.y = 40;
    player->facingDirection = DIR_EAST;
    player->movementActionId = GetWalkNormalMovementAction(DIR_EAST);
    Follower_Update(player);
    assert(gObjectEvents[1].currentCoords.x == 39);
    player->currentCoords.x = 41;
    Follower_Update(player);
    CompleteFollowerMovement();
    Follower_Update(player);
    assert(gObjectEvents[1].currentCoords.x == 40);
    mockLedgeX = 42; mockLedgeY = 40; mockLedgeDir = DIR_EAST;
    player->movementActionId = GetJump2MovementAction(DIR_EAST);
    player->currentCoords.x = 42; // Jump2 starts: position is the LEDGE tile
    Follower_Update(player);
    assert(gObjectEvents[1].movementActionId < 60); // walk onto takeoff
    CompleteFollowerMovement();
    Follower_Update(player);
    assert(gObjectEvents[1].currentCoords.x == 41);
    before = mockMoves;
    player->currentCoords.x = 43; // Jump2 midpoint: arrives at landing
    Follower_Update(player);
    assert(mockMoves == before);  // never walk onto the ledge tile (x42)
    assert(gObjectEvents[1].currentCoords.x == 41);
    Follower_Update(player);      // stop on landing; follower must not overlap
    assert(mockMoves == before);
    player->movementActionId = GetWalkNormalMovementAction(DIR_EAST);
    player->currentCoords.x = 44;
    {
        unsigned audioBefore = mockRecallSeCount;
        Follower_Update(player);
        assert(mockRecallSeCount == audioBefore + 1 && mockLastSe == SE_LEDGE);
        // The queued jump must sound exactly once, even if held midair.
        Follower_Update(player);
        assert(mockRecallSeCount == audioBefore + 1);
    }
    assert(mockMoves == before + 1);
    assert(gObjectEvents[1].movementActionId == GetJump2MovementAction(DIR_EAST));
    CompleteFollowerMovement();
    Follower_Update(player);
    assert(gObjectEvents[1].currentCoords.x == 43);
    assert(gObjectEvents[1].currentCoords.x == player->currentCoords.x - 1);

    // If the callback samples only takeoff and landing (delta of two), the
    // path still must queue a delayed jump rather than walk through the ledge.
    Follower_ResetForMapLoad();
    gObjectEvents[1].active = FALSE;
    mockSave.location.mapNum = 4;
    player->currentCoords.x = 40;
    player->currentCoords.y = 40;
    player->movementActionId = GetWalkNormalMovementAction(DIR_EAST);
    Follower_Update(player);
    player->currentCoords.x = 41;
    Follower_Update(player);
    CompleteFollowerMovement();
    Follower_Update(player);
    assert(gObjectEvents[1].currentCoords.x == 40);
    player->movementActionId = GetJump2MovementAction(DIR_EAST);
    player->currentCoords.x = 43; // callback missed intermediate x42
    Follower_Update(player);
    CompleteFollowerMovement();
    Follower_Update(player);
    assert(gObjectEvents[1].currentCoords.x == 41);
    player->movementActionId = GetWalkNormalMovementAction(DIR_EAST);
    player->currentCoords.x = 44;
    {
        unsigned audioBefore = mockRecallSeCount;
        Follower_Update(player);
        assert(mockRecallSeCount == audioBefore + 1 && mockLastSe == SE_LEDGE);
    }
    assert(gObjectEvents[1].movementActionId == GetJump2MovementAction(DIR_EAST));
    CompleteFollowerMovement();
    Follower_Update(player);
    assert(gObjectEvents[1].currentCoords.x == 43);
    mockLedgeX = -999; mockLedgeY = -999; mockLedgeDir = DIR_NONE;

    // Validate the two-stage jump in all four directions, independently of
    // the east-only case above. The ledge tile is never enqueued as a walk.
    {
        const u8 dirs[] = {DIR_SOUTH, DIR_NORTH, DIR_WEST, DIR_EAST};
        u8 k;
        for (k = 0; k < 4; k++)
        {
            s16 x = 110, y = 110;
            u8 dir = dirs[k];
            Follower_ResetForMapLoad();
            gObjectEvents[1].active = FALSE;
            mockSave.location.mapNum = 12 + k;
            player->facingDirection = dir;
            player->currentCoords.x = x;
            player->currentCoords.y = y;
            player->movementActionId = GetWalkNormalMovementAction(dir);
            Follower_Update(player);
            MoveCoords(dir, &x, &y);
            player->currentCoords.x = x;
            player->currentCoords.y = y;
            Follower_Update(player);
            CompleteFollowerMovement();
            Follower_Update(player);
            mockLedgeX = x;
            mockLedgeY = y;
            MoveCoords(dir, &mockLedgeX, &mockLedgeY);
            mockLedgeDir = dir;
            player->movementActionId = GetJump2MovementAction(dir);
            MoveCoords(dir, &x, &y); // first half of the player's jump
            player->currentCoords.x = x;
            player->currentCoords.y = y;
            Follower_Update(player);
            CompleteFollowerMovement();
            Follower_Update(player);
            before = mockMoves;
            MoveCoords(dir, &x, &y); // landing
            player->currentCoords.x = x;
            player->currentCoords.y = y;
            Follower_Update(player);
            assert(mockMoves == before);
            player->movementActionId = GetWalkNormalMovementAction(dir);
            MoveCoords(dir, &x, &y); // leave landing tile
            player->currentCoords.x = x;
            player->currentCoords.y = y;
            {
                unsigned audioBefore = mockRecallSeCount;
                Follower_Update(player);
                assert(mockRecallSeCount == audioBefore + 1 && mockLastSe == SE_LEDGE);
            }
            assert(gObjectEvents[1].movementActionId == GetJump2MovementAction(dir));
            CompleteFollowerMovement();
            Follower_Update(player);
            assert(gObjectEvents[1].currentCoords.x == x - (dir == DIR_EAST) + (dir == DIR_WEST));
            assert(gObjectEvents[1].currentCoords.y == y - (dir == DIR_SOUTH) + (dir == DIR_NORTH));
        }
    }
    mockLedgeX = -999; mockLedgeY = -999; mockLedgeDir = DIR_NONE;

    // Camera rebase during the *first half* of a jump must adjust the
    // remembered landing tile and all queued positions consistently.
    Follower_ResetForMapLoad();
    gObjectEvents[1].active = FALSE;
    mockSave.location.mapNum = 20;
    player->facingDirection = DIR_EAST;
    player->currentCoords.x = 80;
    player->currentCoords.y = 80;
    player->movementActionId = GetWalkNormalMovementAction(DIR_EAST);
    Follower_Update(player);
    player->currentCoords.x = 81;
    Follower_Update(player);
    CompleteFollowerMovement();
    Follower_Update(player);
    mockLedgeX = 82; mockLedgeY = 80; mockLedgeDir = DIR_EAST;
    player->movementActionId = GetJump2MovementAction(DIR_EAST);
    player->currentCoords.x = 82;
    Follower_Update(player);
    CompleteFollowerMovement();
    Follower_Update(player);
    // The new map's coordinates all shift by +10 on seamless connection.
    Follower_BeginMapConnectionTransition();
    mockSave.location.mapNum = 21;
    Follower_FinishMapConnectionTransition();
    player->currentCoords.x += 10;
    gObjectEvents[1].currentCoords.x += 10;
    mockLedgeX += 10;
    Follower_RebaseForCameraUpdate(-10, 0);
    player->currentCoords.x = 93;
    Follower_Update(player);
    assert(gObjectEvents[1].currentCoords.x == 91);
    player->movementActionId = GetWalkNormalMovementAction(DIR_EAST);
    player->currentCoords.x = 94;
    Follower_Update(player);
    assert(gObjectEvents[1].movementActionId == GetJump2MovementAction(DIR_EAST));
    CompleteFollowerMovement();
    Follower_Update(player);
    assert(gObjectEvents[1].currentCoords.x == 93);
    mockLedgeX = -999; mockLedgeY = -999; mockLedgeDir = DIR_NONE;

    // Script's lock has finished moving the follower. Force the correct
    // facing immediately, even if a previously completed held step lingers.
    gPlayerAvatar.objectEventId = 0;
    gSelectedObjectEvent = 1;
    gObjectEvents[1].singleMovementActive = FALSE;
    gObjectEvents[1].heldMovementActive = TRUE;
    gObjectEvents[1].frozen = FALSE;
    player->facingDirection = DIR_EAST;
    before = mockFaceTurns;
    Follower_PrepareInteraction();
    assert(mockFaceTurns == before + 1);
    assert(gObjectEvents[1].facingDirection == DIR_WEST);
    assert(gObjectEvents[1].heldMovementActive == FALSE);
    assert(gObjectEvents[1].frozen);
    assert(gSprites[1].x2 == 8);
    // v0.8.21: one cry per explicit A-button follower interaction,
    // using the actual lead species regardless of normal/shiny palette.
    before = mockCryCount;
    assert(Follower_PlayInteractionCry() == TRUE);
    assert(mockCryCount == before + 1);
    assert(mockLastCrySpecies == SPECIES_CHARMANDER && mockLastCryMode == 0);
    assert(Follower_PlayInteractionCry() == TRUE); // A second deliberate interaction.
    assert(mockCryCount == before + 2);
    Follower_EndInteraction();
    assert(Follower_PlayInteractionCry() == FALSE); // Movement/spawn: no cry.
    assert(mockCryCount == before + 2);

    // Guard stale selections or mismatched party while the interaction is
    // active. Never play an incorrect species cry for an orphan object.
    mockShiny = TRUE;
    Follower_PrepareInteraction();
    assert(Follower_PlayInteractionCry() == TRUE);
    assert(mockLastCrySpecies == SPECIES_CHARMANDER);
    before = mockCryCount;
    gSelectedObjectEvent = OBJECT_EVENTS_COUNT;
    assert(Follower_PlayInteractionCry() == FALSE);
    gSelectedObjectEvent = 1;
    gObjectEvents[1].localId = 1;
    assert(Follower_PlayInteractionCry() == FALSE);
    gObjectEvents[1].localId = FOLLOWER_LOCAL_ID;
    mockSpecies = SPECIES_BULBASAUR; // Existing follower still Charmander.
    assert(Follower_PlayInteractionCry() == FALSE);
    mockSpecies = SPECIES_CHARMANDER;
    gPlayerPartyCount = 0;
    assert(Follower_PlayInteractionCry() == FALSE);
    gPlayerPartyCount = 1;
    assert(mockCryCount == before);
    Follower_EndInteraction();
    mockShiny = FALSE;

    // All four directions, including the opposite horizontal offset.
    player->facingDirection = DIR_WEST;
    Follower_PrepareInteraction();
    assert(gObjectEvents[1].facingDirection == DIR_EAST);
    assert(gSprites[1].x2 == -8);
    Follower_EndInteraction();
    player->facingDirection = DIR_NORTH;
    Follower_PrepareInteraction();
    assert(gObjectEvents[1].facingDirection == DIR_SOUTH);
    assert(gSprites[1].x2 == 0);
    Follower_EndInteraction();
    player->facingDirection = DIR_SOUTH;
    Follower_PrepareInteraction();
    assert(gObjectEvents[1].facingDirection == DIR_NORTH);
    assert(gSprites[1].x2 == 0);
    Follower_EndInteraction();
    assert(mockFreezes >= 4 && mockClears >= 4);

    // Verify nickname insertion and the health / happiness dialogue tiers.
    mockHp = 20; mockMaxHp = 20; mockFriendship = 245;
    Follower_PrepareInteraction();
    assert(strstr((const char *)gStringVar4, "CHARMANDER is thrilled") != NULL);
    Follower_EndInteraction();

    mockHp = 3;
    Follower_PrepareInteraction();
    assert(strstr((const char *)gStringVar4, "seems tired") != NULL);
    Follower_EndInteraction();

    mockHp = 20; mockStatus = 1;
    Follower_PrepareInteraction();
    assert(strstr((const char *)gStringVar4, "does not look") != NULL);
    Follower_EndInteraction();
    mockStatus = 0; mockHp = 0;
    Follower_PrepareInteraction();
    assert(strstr((const char *)gStringVar4, "is exhausted") != NULL);
    Follower_EndInteraction();

    mockHp = 20; mockFriendship = 30; mockNickname = "TESTMON";
    Follower_PrepareInteraction();
    assert(strstr((const char *)gStringVar4, "TESTMON seems a little") != NULL);
    Follower_EndInteraction();

    // Pause queued follower dispatch while interacting, then resume.
    Follower_ResetForMapLoad();
    gObjectEvents[1].active = FALSE;
    player->currentCoords.x = 60;
    player->currentCoords.y = 60;
    player->facingDirection = DIR_EAST;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    before = mockMoves;
    Follower_PrepareInteraction();
    player->currentCoords.x = 61;
    Follower_Update(player);
    assert(mockMoves == before);
    Follower_EndInteraction();
    Follower_Update(player);
    assert(mockMoves == before + 1);

    // Seamless route connection: preserve the sprite and rebased trail, not
    // a destroy/respawn with a two-frame gap. A pre-existing duplicate is
    // removed without harming the primary visible follower.
    Follower_ResetForMapLoad();
    gObjectEvents[1].active = FALSE;
    gObjectEvents[2].active = FALSE;
    mockSave.location.mapNum = 1;
    mockSave.location.mapGroup = 1;
    player->currentCoords.x = 75;
    player->currentCoords.y = 55;
    player->facingDirection = DIR_EAST;
    Follower_Update(player);
    assert(gObjectEvents[1].active && gObjectEvents[1].currentCoords.x == 74);
    gObjectEvents[2] = gObjectEvents[1];
    gObjectEvents[2].spriteId = 2;
    gObjectEvents[2].currentCoords.x = 10; // Orphan from previous map.
    before = mockRemoves;
    Follower_BeginMapConnectionTransition();
    assert(gObjectEvents[1].active && gObjectEvents[2].active);
    mockSave.location.mapNum = 2;
    Follower_FinishMapConnectionTransition();
    assert(gObjectEvents[1].active && !gObjectEvents[2].active);
    assert(gObjectEvents[1].mapNum == 2);
    assert(mockRemoves == before + 1);
    // FireRed translates every active object's position by the camera delta.
    player->currentCoords.x -= 50;
    player->currentCoords.y -= 10;
    gObjectEvents[1].currentCoords.x -= 50;
    gObjectEvents[1].currentCoords.y -= 10;
    Follower_RebaseForCameraUpdate(50, 10);
    before = mockRemoves;
    Follower_Update(player);
    assert(gObjectEvents[1].active && mockRemoves == before);
    assert(gObjectEvents[1].currentCoords.x == 24);
    // Seamless rebasing must not drop the visual lateral gap.
    assert(gSprites[1].x2 == -8);
    player->currentCoords.x++;
    Follower_Update(player);
    assert(mockMoves > 0);
    CompleteFollowerMovement();
    Follower_Update(player);
    assert(gObjectEvents[1].currentCoords.x == 25);
    assert(mockRemoves == before);
    // Connected map palette refresh must leave follower object untouched.
    Follower_RefreshPalette();
    assert(gObjectEvents[1].active);

    // Repeat connection with a queued corner turn still in flight.
    player->currentCoords.y++;
    Follower_Update(player);
    player->currentCoords.x++;
    Follower_Update(player);
    Follower_BeginMapConnectionTransition();
    mockSave.location.mapNum = 3;
    Follower_FinishMapConnectionTransition();
    player->currentCoords.x += 18;
    gObjectEvents[1].currentCoords.x += 18;
    Follower_RebaseForCameraUpdate(-18, 0);
    before = mockTeleports;
    CompleteFollowerMovement();
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    assert(mockTeleports == before);  // No snapping while trail was rebased.

    // Regression: a follower spawned while the Quest Log is grayscale must
    // not retain its grayscale palette after gameplay resumes from a save.
    Follower_ResetForMapLoad();
    gObjectEvents[1].active = FALSE;
    mockSave.location.mapNum = 8;
    player->currentCoords.x = 24;
    player->currentCoords.y = 24;
    player->facingDirection = DIR_EAST;
    gGlobalFieldTintMode = QL_TINT_GRAYSCALE;
    mockShiny = FALSE;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    // INCBIN art is stubbed to zeros on host; imitate FireRed replacing a
    // palette with grey after the follower first loaded its source colours.
    gPlttBufferUnfaded[0x1B0 + 2] = 0x4210;
    gPlttBufferFaded[0x1B0 + 2] = 0x4210;
    assert(memcmp(&gPlttBufferUnfaded[0x1B0], mockPaletteLoaded, 32) != 0);
    before = mockPaletteLoads;
    Follower_Update(player);
    assert(mockPaletteLoads == before); // Still in Quest Log grayscale.
    gGlobalFieldTintMode = QL_TINT_NONE;
    gPaletteFade.active = TRUE;
    Follower_Update(player);
    assert(mockPaletteLoads == before); // Do not interrupt field fade.
    gPaletteFade.active = FALSE;
    Follower_Update(player);
    assert(mockPaletteLoads == before + 1);
    assert(memcmp(&gPlttBufferUnfaded[0x1B0], mockPaletteLoaded, 32) == 0);
    assert(memcmp(&gPlttBufferFaded[0x1B0], mockPaletteLoaded, 32) == 0);
    // A late overwrite of the same palette slot should self-heal, while an
    // unmodified palette should not be reloaded every gameplay frame.
    before = mockPaletteLoads;
    Follower_Update(player);
    assert(mockPaletteLoads == before);
    gPlttBufferUnfaded[0x1B0 + 2] = 0x4210;
    Follower_Update(player);
    assert(mockPaletteLoads == before + 1);
    assert(memcmp(&gPlttBufferUnfaded[0x1B0], mockPaletteLoaded, 32) == 0);

    // Shiny case: during a save-load fade, the correct UNFADED palette may
    // survive while only the displayed FADED buffer is wrong. The scheduled
    // post-fade repaint restores both copies without respawning the sprite.
    Follower_ResetForMapLoad();
    gObjectEvents[1].active = FALSE;
    mockSave.location.mapNum = 9;
    mockShiny = TRUE;
    gPaletteFade.active = TRUE;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    gPlttBufferFaded[0x1B0 + 7] = 0x4210;
    before = mockPaletteLoads;
    mockWeatherFading = TRUE;
    gPaletteFade.active = FALSE;
    Follower_Update(player);
    assert(mockPaletteLoads == before); // Field weather fade is still active.
    mockWeatherFading = FALSE;
    Follower_Update(player);
    assert(mockPaletteLoads == before + 1);
    assert(memcmp(&gPlttBufferFaded[0x1B0], mockPaletteLoaded, 32) == 0);
    assert(memcmp(&gPlttBufferUnfaded[0x1B0], mockPaletteLoaded, 32) == 0);

    // v0.7: Surf recall must persist through the dismount script and water
    // boundary even after FireRed has cleared the SURFING flag.
    Follower_ResetForMapLoad();
    gObjectEvents[1].active = FALSE;
    player->currentCoords.x = 50;
    player->currentCoords.y = 50;
    player->facingDirection = DIR_EAST;
    mockSave.location.mapNum = 10;
    gPlayerAvatar.flags = 0;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    before = mockRemoves;
    {
        unsigned seBefore = mockRecallSeCount;
        gPlayerAvatar.flags = PLAYER_AVATAR_FLAG_SURFING;
        Follower_Update(player);
        assert(!gObjectEvents[1].active && mockRemoves == before + 1);
        assert(mockRecallSeCount == seBefore + 1 && mockLastSe == SE_BALL_TRADE);
        Follower_Update(player);
        assert(mockRecallSeCount == seBefore + 1); // No repeated Surf recall.
    }
    gPlayerAvatar.flags = PLAYER_AVATAR_FLAG_ON_FOOT;
    gPlayerAvatar.preventStep = TRUE;
    mockScriptLocked = TRUE;
    Follower_Update(player);
    assert(!gObjectEvents[1].active); // dismount task still running
    gPlayerAvatar.preventStep = FALSE;
    mockScriptLocked = FALSE;
    mockWaterX = 50; mockWaterY = 50;
    Follower_Update(player);
    assert(!gObjectEvents[1].active); // on water: do not show follower
    mockWaterX = -999; mockWaterY = -999;
    gPlayerAvatar.tileTransitionState = T_TILE_TRANSITION;
    Follower_Update(player);
    assert(!gObjectEvents[1].active); // still moving onto land
    gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
    player->heldMovementActive = TRUE;
    Follower_Update(player);
    assert(!gObjectEvents[1].active); // held movement unfinished
    player->heldMovementActive = FALSE;
    // FireRed's shore-jump callback is the authoritative completion point.
    // It happens after the script unlocks, but before the next manual step.
    Follower_AfterSurfDismount();
    Follower_Update(player);
    assert(!gObjectEvents[1].active); // Do not pop during landing/while idle.
    assert(mockLastSe == SE_BALL_TRADE); // No release before actual spawn.
    // Continuous directional input may immediately move one tile; don't
    // require an idle or key-up frame before the deferred follower release.
    player->currentCoords.x = 51;
    mockSpawnFailure = TRUE;
    Follower_Update(player);
    assert(!gObjectEvents[1].active); // Failed allocation keeps sound queued.
    assert(mockLastSe == SE_BALL_TRADE);
    mockSpawnFailure = FALSE;
    player->currentCoords.x = 52;
    Follower_Update(player);
    assert(gObjectEvents[1].active); // First subsequent safe vacated tile.
    assert(gObjectEvents[1].currentCoords.x == 51);
    assert(gObjectEvents[1].currentCoords.y == 50);
    assert(mockLastSe == SE_BALL_OPEN);
    {
        unsigned seBefore = mockRecallSeCount;
        Follower_Update(player);
        assert(mockRecallSeCount == seBefore); // No repeated release.
    }

    // Blocked land tiles never force a side/front release; keep the return
    // queued until the player vacates another valid tile.
    {
        unsigned seBefore;
        Follower_ResetForMapLoad();
        gObjectEvents[1].active = FALSE;
        mockSave.location.mapNum = 11;
        player->currentCoords.x = 60;
        player->currentCoords.y = 60;
        player->facingDirection = DIR_EAST;
        gPlayerAvatar.flags = PLAYER_AVATAR_FLAG_ON_FOOT;
        Follower_Update(player);
        assert(gObjectEvents[1].active);
        seBefore = mockRecallSeCount;
        gPlayerAvatar.flags = PLAYER_AVATAR_FLAG_SURFING;
        Follower_Update(player);
        assert(!gObjectEvents[1].active);
        assert(mockRecallSeCount == seBefore + 1);
        gPlayerAvatar.flags = PLAYER_AVATAR_FLAG_ON_FOOT;
        gPlayerAvatar.preventStep = FALSE;
        Follower_AfterSurfDismount();
        mockBlockedX = 60; mockBlockedY = 60;
        player->currentCoords.x = 61;
        Follower_Update(player);
        assert(!gObjectEvents[1].active);
        assert(mockRecallSeCount == seBefore + 1);
        mockBlockedX = -999; mockBlockedY = -999;
        player->currentCoords.x = 62;
        Follower_Update(player);
        assert(gObjectEvents[1].active);
        assert(gObjectEvents[1].currentCoords.x == 61);
        assert(mockRecallSeCount == seBefore + 2 && mockLastSe == SE_BALL_OPEN);
    }

    // v0.8.24: Fly recalls at takeoff, but a destination map reload must
    // NEVER recreate the follower in the arrival fade or landing cutscene.
    // Queue the same one-time SE_BALL_OPEN release used by bicycle dismount
    // until the first controllable movement vacates a safe landing tile.
    {
        unsigned seBefore = mockRecallSeCount;
        Follower_BeginFlyRecall();
        assert(!gObjectEvents[1].active);
        assert(mockRecallSeCount == seBefore + 1 && mockLastSe == SE_BALL_TRADE);
        Follower_Update(player);
        Follower_Update(player);
        assert(!gObjectEvents[1].active && mockRecallSeCount == seBefore + 1);
        Follower_BeginFlyRecall(); // Repeated takeoff callback: no duplicate.
        assert(mockRecallSeCount == seBefore + 1);
        Follower_ResetForMapLoad(); // Engine reconstructs destination map.
        gObjectEvents[1].active = FALSE;
        mockSave.location.mapNum = 34;
        player->currentCoords.x = 80;
        player->currentCoords.y = 80;
        player->facingDirection = DIR_EAST;
        mockScriptLocked = TRUE;
        gPaletteFade.active = TRUE;
        Follower_PrimeForOverworldDisplay();
        assert(!gObjectEvents[1].active);
        Follower_Update(player);
        assert(!gObjectEvents[1].active && mockRecallSeCount == seBefore + 1);
        gPaletteFade.active = FALSE;
        mockScriptLocked = FALSE;
        Follower_Update(player); // Landing tile, now controllable, no step.
        assert(!gObjectEvents[1].active && mockRecallSeCount == seBefore + 1);
        mockScript1Active = TRUE;
        player->currentCoords.x = 81; // A script moves the player one tile.
        Follower_Update(player);
        assert(!gObjectEvents[1].active); // Scripted step is not a release.
        mockScript1Active = FALSE;
        Follower_Update(player); // New idle anchor; still no spawn.
        assert(!gObjectEvents[1].active);
        player->currentCoords.x = 82; // First real player-controlled step.
        mockSpawnFailure = TRUE;
        Follower_Update(player);
        assert(!gObjectEvents[1].active); // OAM full, keep queued.
        assert(mockRecallSeCount == seBefore + 1);
        mockSpawnFailure = FALSE;
        player->currentCoords.x = 83; // Following step, slot now available.
        Follower_Update(player);
        assert(gObjectEvents[1].active);
        assert(gObjectEvents[1].currentCoords.x == 82);
        assert(gObjectEvents[1].currentCoords.y == 80);
        assert(mockRecallSeCount == seBefore + 2 && mockLastSe == SE_BALL_OPEN);
        Follower_Update(player);
        assert(mockRecallSeCount == seBefore + 2); // Release plays only once.
    }

    // v0.7: no spawning during underwater traversal, nor while transition
    // flags have already requested bike/Surf rendering.
    gPlayerAvatar.flags = PLAYER_AVATAR_FLAG_UNDERWATER;
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    gPlayerAvatar.flags = 0;
    gPlayerAvatar.preventStep = FALSE;
    Follower_Update(player);
    assert(gObjectEvents[1].active);

    // No recall audio if the follower was already absent before mounting.
    {
        unsigned seBefore = mockRecallSeCount;
        Follower_ResetForMapLoad();
        RemoveFollowerObjectEvent();
        gPlayerAvatar.flags = PLAYER_AVATAR_FLAG_MACH_BIKE;
        Follower_Update(player);
        assert(mockRecallSeCount == seBefore);
        gPlayerAvatar.flags = PLAYER_AVATAR_FLAG_ON_FOOT;
        Follower_Update(player);
        assert(gObjectEvents[1].active);
        assert(mockRecallSeCount == seBefore); // No phantom release either.
    }

    // A queued transition flag must also suppress respawning, even before
    // FireRed updates its currently active player avatar flags.
    gObjectEvents[1].active = FALSE;
    Follower_ResetForMapLoad();
    gPlayerAvatar.transitionFlags = PLAYER_AVATAR_FLAG_MACH_BIKE;
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    gPlayerAvatar.transitionFlags = 0;
    mockScriptLocked = TRUE;
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    mockScriptLocked = FALSE;
    Follower_Update(player);
    assert(gObjectEvents[1].active);

    // v0.7: a two-tile script teleport is NOT a ledge jump. Only the exact
    // intermediate directional ledge tile should allow Jump2 movement.
    Follower_ResetForMapLoad();
    gObjectEvents[1].active = FALSE;
    mockSave.location.mapNum = 11;
    player->currentCoords.x = 70;
    player->currentCoords.y = 70;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    player->currentCoords.x = 72;
    before = mockMoves;
    {
        unsigned audioBefore = mockRecallSeCount;
        Follower_Update(player);
        assert(mockRecallSeCount == audioBefore);
    }
    assert(mockMoves == before); // resynchronise instead of jumping a wall

    // v0.7.2: Nurse Joy says yes; follower is safely recalled. No respawn
    // is permitted during the effect or the remaining scripted dialogue.
    Follower_ResetForMapLoad();
    gObjectEvents[1].active = FALSE;
    mockSave.location.mapNum = 22;
    player->currentCoords.x = 50;
    player->currentCoords.y = 50;
    player->facingDirection = DIR_EAST;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    mockScript1Active = TRUE;
    mockScriptLocked = TRUE;
    before = mockRecallSeCount;
    assert(Follower_BeginHealingRecall() == TRUE);
    assert(mockRecallSeCount == before + 1);
    assert(mockLastSe == SE_BALL_TRADE);
    assert(!gObjectEvents[1].active);
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    Follower_EndHealingRecall();
    Follower_Update(player); // nurse closing dialogue is still active
    assert(!gObjectEvents[1].active);
    mockScript1Active = FALSE; // script context has stopped, but context 2 is held
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    mockScriptLocked = FALSE; // script finishes and player control resumes
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    assert(gObjectEvents[1].currentCoords.x == 49);
    // A second visit works without requiring a map reload.
    assert(Follower_BeginHealingRecall() == TRUE);
    assert(!gObjectEvents[1].active);
    Follower_EndHealingRecall();
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    // No visible follower: do not play a recall sound or require a pause.
    gObjectEvents[1].active = FALSE;
    before = mockRecallSeCount;
    assert(Follower_BeginHealingRecall() == FALSE);
    assert(mockRecallSeCount == before);
    Follower_EndHealingRecall();
    mockScript1Active = TRUE;
    mockScriptLocked = TRUE;
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    // Map load must reset recall suppression if a warp interrupts healing.
    Follower_ResetForMapLoad();
    mockScript1Active = FALSE;
    mockScriptLocked = FALSE;
    Follower_Update(player);
    assert(gObjectEvents[1].active);

    // v0.8: applymovement on the PLAYER initiates a cinematic scene. The
    // follower must not trace those forced positions, nor return just because
    // the movement action completes while the cutscene is still active.
    Follower_ResetForMapLoad();
    gObjectEvents[1].active = FALSE;
    mockSave.location.mapNum = 23;
    player->currentCoords.x = 65;
    player->currentCoords.y = 65;
    player->facingDirection = DIR_EAST;
    gPlayerAvatar.preventStep = FALSE;
    gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    before = mockRemoves;
    mockScript1Active = TRUE;
    mockScriptLocked = TRUE;
    Follower_BeginScriptedPlayerMovement();
    assert(mockRemoves == before + 1 && !gObjectEvents[1].active);
    player->currentCoords.x += 4; // scripted relocation, not player walking
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    Follower_EndScriptedPlayerMovement();
    Follower_Update(player);
    assert(!gObjectEvents[1].active); // task complete; dialogue still active
    mockScript1Active = FALSE;
    Follower_Update(player);
    assert(!gObjectEvents[1].active); // script 2 still holds the player
    mockScriptLocked = FALSE;
    gPlayerAvatar.preventStep = TRUE;
    Follower_Update(player);
    assert(!gObjectEvents[1].active); // controls still disabled
    gPlayerAvatar.preventStep = FALSE;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    assert(gObjectEvents[1].currentCoords.x == player->currentCoords.x - 1);
    // No additional disappearance on a normal conversation: only an actual
    // scripted player movement calls Begin.
    before = mockRemoves;
    mockScriptLocked = TRUE;
    mockScript1Active = TRUE;
    Follower_Update(player);
    assert(gObjectEvents[1].active && mockRemoves == before);
    mockScriptLocked = FALSE;
    mockScript1Active = FALSE;
    // Even if release is called again, the follower remains unaffected.
    Follower_EndScriptedPlayerMovement();
    Follower_Update(player);
    assert(gObjectEvents[1].active && mockRemoves == before);
    // Warping out of a cutscene resets suppression and cannot strand it.
    Follower_BeginScriptedPlayerMovement();
    assert(!gObjectEvents[1].active);
    Follower_ResetForMapLoad();
    mockSave.location.mapNum = 24;
    Follower_Update(player);
    assert(gObjectEvents[1].active);

    // v0.9.0.3.12: League entry is a walking cutscene, not a recall.
    // The follower must animate along Red's actual tiles and still be present
    // after lockall/releaseall and subsequent text. Other script types above
    // must continue to recall as before.
    Follower_ResetForMapLoad();
    mockSave.location.mapGroup = 1;
    mockSave.location.mapNum = 75; // MAP_POKEMON_LEAGUE_LORELEIS_ROOM
    player->currentCoords.x = 65;
    player->currentCoords.y = 65;
    player->facingDirection = DIR_NORTH;
    gPlayerAvatar.preventStep = FALSE;
    mockScript1Active = FALSE;
    mockScriptLocked = FALSE;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    before = mockRemoves;
    mockScript1Active = TRUE;
    mockScriptLocked = TRUE;
    gObjectEvents[1].frozen = TRUE; // lockall
    Follower_BeginScriptedPlayerMovement();
    assert(gObjectEvents[1].active && mockRemoves == before);
    Follower_Update(player);
    assert(!gObjectEvents[1].frozen);
    {
        int i;
        unsigned movesStart = mockMoves;
        for (i = 0; i < 5; i++)
        {
            player->currentCoords.y--;
            Follower_Update(player);
            if (gObjectEvents[1].heldMovementActive)
                CompleteFollowerMovement();
        }
        assert(gObjectEvents[1].active && mockRemoves == before);
        assert(mockMoves > movesStart); // Cutscene trail actually animates.
    }
    Follower_EndScriptedPlayerMovement();
    Follower_Update(player); // text still has script context locked
    assert(gObjectEvents[1].active && mockRemoves == before);
    mockScript1Active = FALSE;
    mockScriptLocked = FALSE;
    Follower_Update(player); // post-text, stationary player
    assert(gObjectEvents[1].active && mockRemoves == before);

    // Hall of Fame: follow the entrance walk, then recall only immediately
    // before the machine recording effect. The script remains locked and
    // subsequent updates must not spawn a second follower during the scene.
    Follower_ResetForMapLoad();
    mockSave.location.mapNum = MAP_POKEMON_LEAGUE_HALL_OF_FAME & 0xff;
    mockSave.location.mapGroup = MAP_POKEMON_LEAGUE_HALL_OF_FAME >> 8;
    gObjectEvents[1].active = FALSE;
    player->currentCoords.x = 65;
    player->currentCoords.y = 65;
    player->facingDirection = DIR_NORTH;
    mockScript1Active = TRUE;
    mockScriptLocked = TRUE;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    Follower_BeginScriptedPlayerMovement();
    player->currentCoords.y--;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    Follower_EndScriptedPlayerMovement();
    before = mockRecallSeCount;
    assert(Follower_BeginHallOfFameRecall() == TRUE);
    assert(!gObjectEvents[1].active);
    assert(mockRecallSeCount == before + 1 && mockLastSe == SE_BALL_TRADE);
    assert(Follower_BeginHallOfFameRecall() == FALSE); // idempotent
    for (int hofFrame = 0; hofFrame < 12; hofFrame++)
        Follower_Update(player);
    assert(!gObjectEvents[1].active && mockRecallSeCount == before + 1);
    Follower_ResetForMapLoad(); // scene ends; new map reload restores follower
    mockSave.location.mapNum = 75;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    mockScript1Active = FALSE;
    mockScriptLocked = FALSE;

    // Arrival from a rear-only warp begins with no follower. The script
    // itself should restore the follower after the first valid vacated step,
    // instead of stranding it until an unrelated manual step after the text.
    Follower_ResetForMapLoad();
    gObjectEvents[1].active = FALSE;
    player->currentCoords.x = 70;
    player->currentCoords.y = 70;
    Follower_BeginRearOnlyWarp();
    mockScript1Active = TRUE;
    mockScriptLocked = TRUE;
    Follower_BeginScriptedPlayerMovement();
    player->currentCoords.y--;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    Follower_EndScriptedPlayerMovement();
    assert(gObjectEvents[1].active);
    mockScript1Active = FALSE;
    mockScriptLocked = FALSE;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    // League post-battle text must restore the follower without requiring
    // an extra manual step, but never during trainer dialogue or fades.
    Follower_ResetForMapLoad();
    gObjectEvents[1].active = FALSE;
    player->currentCoords.x = 75;
    player->currentCoords.y = 75;
    player->facingDirection = DIR_NORTH;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    Follower_NotifyBattleReturn();
    Follower_PrimeForOverworldDisplay();
    assert(!gObjectEvents[1].active);
    mockScript1Active = TRUE;
    mockScriptLocked = TRUE;
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    mockScript1Active = FALSE;
    mockScriptLocked = FALSE;
    gPaletteFade.active = TRUE;
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    gPaletteFade.active = FALSE;
    Follower_Update(player); // text and fade finished; no player step
    assert(gObjectEvents[1].active);
    assert(gObjectEvents[1].currentCoords.x == 75);
    assert(gObjectEvents[1].currentCoords.y == 76);

    // v0.9.0.3.24: direct Gym Leader rematches must behave like the League:
    // only after the trainer's script and fades release the player, respawn
    // behind Red without demanding an otherwise pointless manual step.
    Follower_ResetForMapLoad();
    mockSave.location.mapNum = MAP_NUM(PEWTER_CITY_GYM);
    mockSave.location.mapGroup = MAP_GROUP(PEWTER_CITY_GYM);
    gObjectEvents[1].active = FALSE;
    player->currentCoords.x = 62;
    player->currentCoords.y = 62;
    player->facingDirection = DIR_NORTH;
    gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
    player->heldMovementActive = FALSE;
    mockWeatherFading = FALSE;
    mockScript1Active = FALSE;
    mockScriptLocked = FALSE;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    Follower_NotifyGymRematchReturn();
    Follower_PrimeForOverworldDisplay();
    assert(!gObjectEvents[1].active);
    mockScript1Active = TRUE;
    mockScriptLocked = TRUE;
    Follower_Update(player);
    assert(!gObjectEvents[1].active); // Gym victory dialogue still open.
    mockScript1Active = FALSE;
    mockScriptLocked = FALSE;
    gPaletteFade.active = TRUE;
    Follower_Update(player);
    assert(!gObjectEvents[1].active); // Fade still in progress.
    gPaletteFade.active = FALSE;
    mockWeatherFading = TRUE;
    Follower_Update(player);
    assert(!gObjectEvents[1].active); // Weather fade still in progress.
    mockWeatherFading = FALSE;
    Follower_Update(player); // No player step required, like the League.
    assert(gObjectEvents[1].active);
    assert(gObjectEvents[1].currentCoords.x == 62);
    assert(gObjectEvents[1].currentCoords.y == 63);

    // Gym rematch: obstructed rear waits rather than jumping beside trainer.
    Follower_NotifyGymRematchReturn();
    Follower_PrimeForOverworldDisplay();
    assert(!gObjectEvents[1].active);
    mockBlockedX = 62;
    mockBlockedY = 63;
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    player->currentCoords.y = 61;
    Follower_Update(player); // Vacated (62,62) is safe.
    assert(gObjectEvents[1].active);
    assert(gObjectEvents[1].currentCoords.x == 62);
    assert(gObjectEvents[1].currentCoords.y == 62);
    mockBlockedX = -999;
    mockBlockedY = -999;

    // v0.9.0.3.25: after an ordinary wild encounter the follower returns
    // behind Red as soon as field control and palette/weather fades settle.
    // This must NOT rely on any player movement, including in tall grass.
    Follower_ResetForMapLoad();
    mockSave.location.mapNum = MAP_NUM(ROUTE1);
    mockSave.location.mapGroup = MAP_GROUP(ROUTE1);
    gObjectEvents[1].active = FALSE;
    player->currentCoords.x = 56;
    player->currentCoords.y = 56;
    player->facingDirection = DIR_EAST;
    gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
    player->heldMovementActive = FALSE;
    mockScript1Active = FALSE;
    mockScriptLocked = FALSE;
    mockWeatherFading = FALSE;
    gPaletteFade.active = FALSE;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    Follower_NotifyWildBattleReturn();
    Follower_PrimeForOverworldDisplay();
    assert(!gObjectEvents[1].active);
    mockScript1Active = TRUE;
    mockScriptLocked = TRUE;
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    mockScript1Active = FALSE;
    mockScriptLocked = FALSE;
    gPaletteFade.active = TRUE;
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    gPaletteFade.active = FALSE;
    mockWeatherFading = TRUE;
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    mockWeatherFading = FALSE;
    Follower_Update(player); // Exactly the same player tile; no D-pad input.
    assert(gObjectEvents[1].active);
    assert(gObjectEvents[1].currentCoords.x == 55);
    assert(gObjectEvents[1].currentCoords.y == 56);

    // If a rear tile is obstructed, the wild return must wait for the first
    // player-vacated tile instead of spawning atop another NPC or to the side.
    Follower_NotifyWildBattleReturn();
    Follower_PrimeForOverworldDisplay();
    assert(!gObjectEvents[1].active);
    mockBlockedX = 55;
    mockBlockedY = 56;
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    player->currentCoords.x = 57;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    assert(gObjectEvents[1].currentCoords.x == 56);
    assert(gObjectEvents[1].currentCoords.y == 56);
    mockBlockedX = -999;
    mockBlockedY = -999;

    // A following ordinary trainer battle must reset the wild-only latch.
    Follower_NotifyBattleReturn();
    Follower_PrimeForOverworldDisplay();
    assert(!gObjectEvents[1].active);
    Follower_Update(player); // Stationary: not the wild return any more.
    assert(!gObjectEvents[1].active);
    player->currentCoords.x = 58;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    assert(gObjectEvents[1].currentCoords.x == 57);

    // A normal trainer in the same Gym must keep the original step-only return.
    Follower_ResetForMapLoad();
    mockSave.location.mapNum = MAP_NUM(PEWTER_CITY_GYM);
    mockSave.location.mapGroup = MAP_GROUP(PEWTER_CITY_GYM);
    gObjectEvents[1].active = FALSE;
    player->currentCoords.x = 62;
    player->currentCoords.y = 61;
    player->facingDirection = DIR_NORTH;
    Follower_Update(player);
    Follower_NotifyBattleReturn();
    Follower_PrimeForOverworldDisplay();
    assert(!gObjectEvents[1].active);
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    player->currentCoords.y = 60;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    assert(gObjectEvents[1].currentCoords.y == 61);

    Follower_ResetForMapLoad();
    mockSave.location.mapNum = 23;
    mockSave.location.mapGroup = 0;
    gObjectEvents[1].active = FALSE;
    player->heldMovementActive = FALSE;
    mockScript1Active = FALSE;
    mockScriptLocked = FALSE;

    // v0.8.9: suppress the follower during every Rocket spin movement,
    // including the stop-tile animation, then restore behind the player.
    {
        const u8 dirs[4] = {DIR_EAST, DIR_WEST, DIR_NORTH, DIR_SOUTH};
        const u8 tiles[4] = {0x54, 0x55, 0x56, 0x57};
        u8 j;
        for (j = 0; j < 4; j++)
        {
            s16 sx = 40, sy = 40, stopX, stopY, rearX, rearY;
            unsigned seBefore = mockRecallSeCount;
            unsigned movesBefore = mockMoves;
            Follower_ResetForMapLoad();
            RemoveFollowerObjectEvent();
            mockSpinX = -999;
            mockSpinY = -999;
            mockBlockedX = -999;
            mockBlockedY = -999;
            gPlayerAvatar.flags &= ~PLAYER_AVATAR_FLAG_FORCED;
            gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
            player->heldMovementActive = FALSE;
            player->heldMovementFinished = FALSE;
            player->currentCoords.x = sx;
            player->currentCoords.y = sy;
            player->currentMetatileBehavior = 0;
            player->facingDirection = dirs[j];
            Follower_Update(player);
            assert(gObjectEvents[1].active);

            // The first spinner tile recalls the follower immediately.
            mockSpinX = sx;
            mockSpinY = sy;
            mockSpinBehavior = tiles[j];
            player->currentMetatileBehavior = tiles[j];
            Follower_Update(player);
            assert(!gObjectEvents[1].active);
            assert(mockRecallSeCount == seBefore); // No repeated recall noise

            // The forced movement and intermediate directional pads must
            // never recreate or animate the follower.
            gPlayerAvatar.flags |= PLAYER_AVATAR_FLAG_FORCED;
            gPlayerAvatar.lastSpinTile = tiles[j];
            MoveCoords(dirs[j], &player->currentCoords.x, &player->currentCoords.y);
            mockSpinX = player->currentCoords.x;
            mockSpinY = player->currentCoords.y;
            player->currentMetatileBehavior = tiles[j];
            Follower_Update(player);
            assert(!gObjectEvents[1].active);
            assert(mockMoves == movesBefore);

            stopX = player->currentCoords.x;
            stopY = player->currentCoords.y;
            MoveCoords(dirs[j], &stopX, &stopY);
            rearX = player->currentCoords.x;
            rearY = player->currentCoords.y;
            player->currentCoords.x = stopX;
            player->currentCoords.y = stopY;
            mockSpinX = stopX;
            mockSpinY = stopY;
            mockSpinBehavior = 0x58;
            player->currentMetatileBehavior = 0x58;
            player->heldMovementActive = TRUE;
            player->heldMovementFinished = FALSE;
            gPlayerAvatar.tileTransitionState = T_TILE_TRANSITION;
            Follower_Update(player);
            assert(!gObjectEvents[1].active); // Still mid-stop animation
            gPlayerAvatar.flags &= ~PLAYER_AVATAR_FLAG_FORCED;
            Follower_Update(player);
            assert(!gObjectEvents[1].active); // Tile movement not finished
            gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
            Follower_Update(player);
            assert(!gObjectEvents[1].active); // Held animation unfinished
            player->heldMovementFinished = TRUE;
            Follower_Update(player);
            assert(gObjectEvents[1].active);
            assert(gObjectEvents[1].currentCoords.x == rearX);
            assert(gObjectEvents[1].currentCoords.y == rearY);
            assert(mockRecallSeCount == seBefore);
            // If the object-event cache still reports an arrow for a frame
            // while the map grid is already STOP, do not recall it again.
            before = mockRemoves;
            player->currentMetatileBehavior = tiles[j];
            Follower_Update(player);
            assert(gObjectEvents[1].active && mockRemoves == before);
            player->currentMetatileBehavior = 0x58;

            // Once visible, ordinary walking must never re-trigger the
            // old native SPIN action even if the rear tile is an arrow.
            player->heldMovementActive = FALSE;
            player->heldMovementFinished = FALSE;
            mockSpinX = rearX; mockSpinY = rearY;
            mockSpinBehavior = tiles[j];
            MoveCoords(dirs[j], &player->currentCoords.x, &player->currentCoords.y);
            player->currentMetatileBehavior = 0;
            Follower_Update(player);
            assert(!gObjectEvents[1].active ||
                   gObjectEvents[1].movementActionId != GetSpinMovementAction(dirs[j]));
        }
    }
    // v0.8.20.1: uninterrupted directional input must release a recalled
    // follower ON the first ordinary step even though the player is still
    // transitioning and has a new unfinished held movement action.
    {
        const u8 dirs[4] = {DIR_EAST, DIR_WEST, DIR_NORTH, DIR_SOUTH};
        const u8 tiles[4] = {0x54, 0x55, 0x56, 0x57};
        u8 j;
        for (j = 0; j < 4; j++)
        {
            s16 stopX, stopY;
            unsigned seBefore, removalsBefore;
            Follower_ResetForMapLoad();
            RemoveFollowerObjectEvent();
            gPlayerAvatar.flags &= ~PLAYER_AVATAR_FLAG_FORCED;
            gPlayerAvatar.preventStep = FALSE;
            gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
            mockScript1Active = FALSE;
            mockScriptLocked = FALSE;
            mockBlockedX = -999;
            mockSpinX = -999;
            player->heldMovementActive = FALSE;
            player->heldMovementFinished = FALSE;
            player->currentCoords.x = 80;
            player->currentCoords.y = 80;
            player->facingDirection = dirs[j];
            player->currentMetatileBehavior = 0;
            Follower_Update(player);
            assert(gObjectEvents[1].active);
            seBefore = mockRecallSeCount;

            // Enter the spin arrow; recall immediately.
            mockSpinX = player->currentCoords.x;
            mockSpinY = player->currentCoords.y;
            mockSpinBehavior = tiles[j];
            player->currentMetatileBehavior = tiles[j];
            Follower_Update(player);
            assert(!gObjectEvents[1].active);

            // Stop tile: the forced movement has *not* yet settled.
            gPlayerAvatar.flags |= PLAYER_AVATAR_FLAG_FORCED;
            MoveCoords(dirs[j], &player->currentCoords.x, &player->currentCoords.y);
            stopX = player->currentCoords.x;
            stopY = player->currentCoords.y;
            mockSpinX = stopX;
            mockSpinY = stopY;
            mockSpinBehavior = 0x58;
            player->currentMetatileBehavior = 0x58;
            player->heldMovementActive = TRUE;
            player->heldMovementFinished = FALSE;
            gPlayerAvatar.tileTransitionState = T_TILE_TRANSITION;
            Follower_Update(player);
            assert(!gObjectEvents[1].active);
            gPlayerAvatar.flags &= ~PLAYER_AVATAR_FLAG_FORCED;
            Follower_Update(player);
            assert(!gObjectEvents[1].active); // not in stop animation

            // The user never releases D-pad: immediately begin a new step.
            // Deliberately leave currentMetatileBehavior stale at SPIN_ARROW;
            // the map grid, currentCoords and action are authoritative.
            player->previousCoords.x = stopX;
            player->previousCoords.y = stopY;
            MoveCoords(dirs[j], &player->currentCoords.x, &player->currentCoords.y);
            player->currentMetatileBehavior = tiles[j];
            player->heldMovementActive = TRUE;
            player->heldMovementFinished = FALSE;
            gPlayerAvatar.tileTransitionState = T_TILE_TRANSITION;
            Follower_Update(player);
            assert(gObjectEvents[1].active);
            assert(gObjectEvents[1].currentCoords.x == stopX);
            assert(gObjectEvents[1].currentCoords.y == stopY);
            assert(mockRecallSeCount == seBefore);
            removalsBefore = mockRemoves;
            Follower_Update(player); // stale cached arrow must not re-recall
            assert(gObjectEvents[1].active && mockRemoves == removalsBefore);

            // Keep holding through another step, with NO idle update.
            player->currentMetatileBehavior = 0;
            player->previousCoords = player->currentCoords;
            MoveCoords(dirs[j], &player->currentCoords.x, &player->currentCoords.y);
            Follower_Update(player);
            assert(gObjectEvents[1].active && mockRemoves == removalsBefore);
        }
    }

    // v0.8.20.1: when the STOP tile becomes occupied by an NPC immediately
    // after being vacated, keep hidden and retry at the next vacated tile
    // while movement continues uninterrupted (no side-spawn or idle wait).
    Follower_ResetForMapLoad();
    RemoveFollowerObjectEvent();
    mockBlockedX = -999;
    mockSpinX = 120; mockSpinY = 120; mockSpinBehavior = 0x54;
    player->heldMovementActive = FALSE;
    player->heldMovementFinished = FALSE;
    gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
    gPlayerAvatar.flags &= ~PLAYER_AVATAR_FLAG_FORCED;
    player->currentCoords.x = 120;
    player->currentCoords.y = 120;
    player->currentMetatileBehavior = 0x54;
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    player->previousCoords = player->currentCoords;
    player->currentCoords.x = 121;
    mockSpinX = 121; mockSpinBehavior = 0x58;
    player->currentMetatileBehavior = 0x58;
    gPlayerAvatar.flags |= PLAYER_AVATAR_FLAG_FORCED;
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    gPlayerAvatar.flags &= ~PLAYER_AVATAR_FLAG_FORCED;
    player->previousCoords = player->currentCoords;
    player->currentCoords.x = 122;
    player->currentMetatileBehavior = 0;
    mockBlockedX = 121; mockBlockedY = 120;
    player->heldMovementActive = TRUE;
    player->heldMovementFinished = FALSE;
    gPlayerAvatar.tileTransitionState = T_TILE_TRANSITION;
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    player->previousCoords = player->currentCoords;
    player->currentCoords.x = 123;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    assert(gObjectEvents[1].currentCoords.x == 122);
    assert(gObjectEvents[1].currentCoords.y == 120);
    mockBlockedX = -999;
    mockSpinX = -999;
    gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
    player->heldMovementActive = FALSE;
    player->heldMovementFinished = FALSE;

    // v0.8.20.1: a delayed follower callback may first observe the
    // already-started controlled step. Recover STOP from previousCoords.
    Follower_ResetForMapLoad();
    RemoveFollowerObjectEvent();
    mockSpinX = 140; mockSpinY = 140; mockSpinBehavior = 0x54;
    gPlayerAvatar.flags &= ~PLAYER_AVATAR_FLAG_FORCED;
    gPlayerAvatar.preventStep = FALSE;
    gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
    player->heldMovementActive = FALSE;
    player->currentCoords.x = 140; player->currentCoords.y = 140;
    player->currentMetatileBehavior = 0x54;
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    mockSpinX = 141; mockSpinBehavior = 0x58;
    player->previousCoords.x = 141; player->previousCoords.y = 140;
    player->currentCoords.x = 142;
    player->currentMetatileBehavior = 0x54; // stale cached arrow
    player->heldMovementActive = TRUE;
    player->heldMovementFinished = FALSE;
    gPlayerAvatar.tileTransitionState = T_TILE_TRANSITION;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    assert(gObjectEvents[1].currentCoords.x == 141);
    assert(gObjectEvents[1].currentCoords.y == 140);

    // Scripted movement following a spin must not itself release the follower,
    // and the later first controlled step must not depend on an idle frame.
    Follower_ResetForMapLoad();
    RemoveFollowerObjectEvent();
    mockSpinX = 150; mockSpinY = 150; mockSpinBehavior = 0x54;
    player->currentCoords.x = 150; player->currentCoords.y = 150;
    player->currentMetatileBehavior = 0x54;
    player->heldMovementActive = FALSE;
    gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    mockSpinX = 151; mockSpinBehavior = 0x58;
    player->currentCoords.x = 151;
    player->currentMetatileBehavior = 0x58;
    gPlayerAvatar.flags |= PLAYER_AVATAR_FLAG_FORCED;
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    gPlayerAvatar.flags &= ~PLAYER_AVATAR_FLAG_FORCED;
    mockScript1Active = TRUE;
    player->currentCoords.x = 152;
    gPlayerAvatar.tileTransitionState = T_TILE_TRANSITION;
    player->heldMovementActive = TRUE;
    player->heldMovementFinished = FALSE;
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    player->currentCoords.x = 153;
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    mockScript1Active = FALSE;
    player->currentCoords.x = 154;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    assert(gObjectEvents[1].currentCoords.x == 153);
    assert(gObjectEvents[1].currentCoords.y == 150);
    mockSpinX = -999;
    player->currentMetatileBehavior = 0;
    gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
    player->heldMovementActive = FALSE;
    player->heldMovementFinished = FALSE;

    // A stop tile blocked behind the player must not cause a side spawn.
    // Wait for the player to vacate a real tile and use that tile instead.
    Follower_ResetForMapLoad();
    // v0.8.15: previously tested palette guards still exist for compatibility
    // but must be DISABLED during a post-battle recall with no visible object.
    // A bad palette during the battle fade must never be painted on screen by
    // a prematurely created follower; a genuine player step is the release.
    {
        u16 *unfaded = &gPlttBufferUnfaded[0x1B0];
        unsigned beforeLoads;
        unsigned i;
        Follower_ResetForMapLoad();
        gObjectEvents[1].active = FALSE;
        gGlobalFieldTintMode = QL_TINT_NONE;
        gPaletteFade.active = TRUE;
        mockWeatherFading = TRUE;
        player->currentCoords.x = 50;
        player->currentCoords.y = 50;
        player->facingDirection = DIR_EAST;
        Follower_NotifyBattleReturn();
        Follower_PrimeForOverworldDisplay();
        assert(!gObjectEvents[1].active);
        for (i = 0; i < 16; i++) unfaded[i] = 0x4210;
        beforeLoads = mockPaletteLoads;
        Follower_StabilizeBattleReturnPalette();
        Follower_StabilizeBattleReturnSpriteSlot();
        assert(mockPaletteLoads == beforeLoads);
        Follower_Update(player);
        assert(!gObjectEvents[1].active);
        gPaletteFade.active = FALSE;
        mockWeatherFading = FALSE;
        Follower_Update(player);
        assert(!gObjectEvents[1].active);
        player->currentCoords.x = 51;
        Follower_Update(player);
        assert(gObjectEvents[1].active);
        assert(gObjectEvents[1].currentCoords.x == 50);
    }

    RemoveFollowerObjectEvent();
    gPlayerAvatar.flags &= ~PLAYER_AVATAR_FLAG_FORCED;
    player->heldMovementActive = FALSE;
    player->heldMovementFinished = FALSE;
    gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
    mockBlockedX = -999;
    mockSpinX = -999;
    player->currentCoords.x = 50;
    player->currentCoords.y = 50;
    player->currentMetatileBehavior = 0;
    player->facingDirection = DIR_EAST;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    mockSpinX = 50; mockSpinY = 50; mockSpinBehavior = 0x54;
    player->currentMetatileBehavior = 0x54;
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    player->currentCoords.x = 53;
    mockSpinX = 53; mockSpinY = 50; mockSpinBehavior = 0x58;
    player->currentMetatileBehavior = 0x58;
    mockBlockedX = 52;
    mockBlockedY = 50;
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    player->currentCoords.x = 54;
    player->currentMetatileBehavior = 0;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    assert(gObjectEvents[1].currentCoords.x == 53);
    assert(gObjectEvents[1].currentCoords.y == 50);

    // A warp during a spin clears the suppression latch; no permanent loss.
    Follower_ResetForMapLoad();
    RemoveFollowerObjectEvent();
    mockBlockedX = -999;
    mockSpinX = -999;
    mockSave.location.mapNum++;
    player->currentCoords.x = 60;
    player->currentCoords.y = 60;
    player->currentMetatileBehavior = 0;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    // v0.8.17: Emerald-style ordinary doorway: the *scripted* exit step
    // counts, so the follower does not wait for an additional manual step.
    mockBlockedX = -999;
    mockBlockedY = -999;
    mockSpinX = -999;
    mockDoorX = -999;
    mockDoorY = -999;
    gPlayerAvatar.flags = PLAYER_AVATAR_FLAG_ON_FOOT;
    gPlayerAvatar.preventStep = FALSE;
    gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
    player->heldMovementActive = FALSE;
    player->heldMovementFinished = FALSE;
    player->facingDirection = DIR_NORTH;
    mockDoorX = 70; mockDoorY = 70; // Animated door runs an exit step.
    Follower_BeginDoorWarp();
    assert(!gObjectEvents[1].active);
    Follower_ResetForMapLoad();
    mockSave.location.mapNum++;
    player->currentCoords.x = 70;
    player->currentCoords.y = 70;
    gPaletteFade.active = TRUE;
    mockWeatherFading = TRUE;
    mockScriptLocked = TRUE;
    Follower_PrimeForOverworldDisplay();
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    // Entrance: the game automatically walks NORTH onto the destination map.
    player->currentCoords.y = 69;
    player->heldMovementActive = TRUE;
    player->heldMovementFinished = FALSE;
    gPlayerAvatar.tileTransitionState = T_TILE_TRANSITION;
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    // Release as soon as fade completes, even though the automatic
    // door-exit step is still animating and the script still owns control.
    gPaletteFade.active = FALSE;
    mockWeatherFading = FALSE;
    Follower_Update(player);
    assert(gObjectEvents[1].active); // NO end-of-step / stop-to-idle delay!
    assert(gPlayerAvatar.tileTransitionState == T_TILE_TRANSITION);
    assert(player->heldMovementActive && !player->heldMovementFinished);
    mockScriptLocked = FALSE;
    gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
    player->heldMovementFinished = TRUE;
    assert(gObjectEvents[1].currentCoords.x == 70);
    assert(gObjectEvents[1].currentCoords.y == 70); // vacated doorway tile
    // Keep a direction held: follower walking starts on the NEXT tile action
    // while the player's step animation is still active (no idle requirement).
    before = mockMoves;
    player->currentCoords.y = 68;
    player->heldMovementActive = TRUE;
    player->heldMovementFinished = FALSE;
    gPlayerAvatar.tileTransitionState = T_TILE_TRANSITION;
    Follower_Update(player);
    assert(gObjectEvents[1].active && mockMoves == before + 1);
    gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
    player->heldMovementActive = FALSE;
    before = mockRemoves;
    Follower_Update(player);
    assert(gObjectEvents[1].active && mockRemoves == before);
    mockDoorX = -999; mockDoorY = -999;

    // Nonanimated Pokémon Center/building EXIT. Old tile is marked as an
    // otherwise impassable warp threshold: Emerald still lets its follower
    // emerge through the actual door, never to the side/front.
    Follower_BeginDoorWarp();
    Follower_ResetForMapLoad();
    mockSave.location.mapNum++;
    player->currentCoords.x = 80;
    player->currentCoords.y = 80;
    player->facingDirection = DIR_SOUTH;
    mockBlockedX = 80;
    mockBlockedY = 80;
    mockDoorX = 80;
    mockDoorY = 80;
    mockScriptLocked = TRUE;
    gPlayerAvatar.preventStep = TRUE;
    player->heldMovementActive = FALSE;
    player->heldMovementFinished = FALSE;
    Follower_PrimeForOverworldDisplay();
    assert(!gObjectEvents[1].active);
    player->currentCoords.y = 81;
    gPlayerAvatar.tileTransitionState = T_TILE_TRANSITION;
    player->heldMovementActive = TRUE;
    player->heldMovementFinished = FALSE;
    Follower_Update(player);
    assert(gObjectEvents[1].active); // Starts during scripted exit movement.
    assert(gPlayerAvatar.preventStep && mockScriptLocked);
    mockScriptLocked = FALSE;
    gPlayerAvatar.preventStep = FALSE;
    gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
    player->heldMovementFinished = TRUE;
    assert(gObjectEvents[1].currentCoords.x == 80);
    assert(gObjectEvents[1].currentCoords.y == 80);
    mockDoorX = -999; mockDoorY = -999;
    mockBlockedX = -999; mockBlockedY = -999;
    player->heldMovementActive = FALSE;

    // A fade-only interior destination has NO automatic door-exit step.
    // Emerald's generic warp logic must not wait for a SECOND manual step;
    // when the scene is unlocked the safe rear tile can be used immediately.
    Follower_BeginDoorWarp();
    Follower_ResetForMapLoad();
    mockSave.location.mapNum++;
    player->currentCoords.x = 100;
    player->currentCoords.y = 100;
    player->facingDirection = DIR_SOUTH;
    mockScriptLocked = TRUE;
    gPaletteFade.active = TRUE;
    Follower_PrimeForOverworldDisplay();
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    mockScriptLocked = FALSE;
    gPaletteFade.active = FALSE;
    Follower_Update(player); // No movement has occurred.
    assert(gObjectEvents[1].active);
    assert(gObjectEvents[1].currentCoords.x == 100);
    assert(gObjectEvents[1].currentCoords.y == 99);

    // Fade-only rear obstruction cannot justify a front/side spawn.
    Follower_BeginDoorWarp();
    Follower_ResetForMapLoad();
    mockSave.location.mapNum++;
    player->currentCoords.x = 110;
    player->currentCoords.y = 110;
    player->facingDirection = DIR_SOUTH;
    mockBlockedX = 110;
    mockBlockedY = 109;
    Follower_PrimeForOverworldDisplay();
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    mockBlockedX = -999;
    player->currentCoords.y = 111;
    gPlayerAvatar.tileTransitionState = T_TILE_TRANSITION;
    player->heldMovementActive = TRUE;
    player->heldMovementFinished = FALSE;
    Follower_Update(player);
    assert(gObjectEvents[1].active); // During continuous walking, not at rest.
    gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
    player->heldMovementActive = FALSE;
    assert(gObjectEvents[1].currentCoords.x == 110);
    assert(gObjectEvents[1].currentCoords.y == 110);

    // Ordinary rear wall or NPC is NOT a doorway. No unsafe front/side
    // relocation; following waits until the player leaves a clear tile.
    Follower_BeginDoorWarp();
    Follower_ResetForMapLoad();
    mockSave.location.mapNum++;
    player->currentCoords.x = 90;
    player->currentCoords.y = 90;
    player->facingDirection = DIR_EAST;
    mockBlockedX = 90;
    mockBlockedY = 90;
    mockShiny = TRUE;
    Follower_PrimeForOverworldDisplay();
    player->currentCoords.x = 91;
    Follower_Update(player);
    assert(!gObjectEvents[1].active);
    mockBlockedX = -999;
    player->currentCoords.x = 92;
    Follower_Update(player);
    assert(gObjectEvents[1].active);
    assert(gObjectEvents[1].currentCoords.x == 91);
    assert(gObjectEvents[1].currentCoords.y == 90);
    assert(mockPaletteLoaded != NULL);
    mockShiny = FALSE;
    Follower_Update(player);
    assert(gObjectEvents[1].active);

    // v0.8.22: Pokemon Tower's horizontal fog must not whiten the follower.
    // A dynamic follower is a normal OBJ, not a fog weather particle.
    {
        u16 clean[16];
        unsigned calls;
        memcpy(clean, &gPlttBufferUnfaded[0x100 + 11 * 16], sizeof(clean));
        calls = mockWeatherPaletteCalls;
        mockWeather.currWeather = WEATHER_FOG_HORIZONTAL;
        mockWeather.palProcessingState = WEATHER_PAL_STATE_IDLE;
        Follower_RefreshPalette();
        assert(mockWeatherPaletteCalls == calls);
        assert(memcmp(clean, &gPlttBufferFaded[0x100 + 11 * 16], sizeof(clean)) == 0);
        mockShiny = TRUE;
        Follower_Update(player); // Shiny palette also stays full-strength.
        memcpy(clean, &gPlttBufferUnfaded[0x100 + 11 * 16], sizeof(clean));
        assert(mockWeatherPaletteCalls == calls);
        assert(memcmp(clean, &gPlttBufferFaded[0x100 + 11 * 16], sizeof(clean)) == 0);
        mockShiny = FALSE;
        Follower_Update(player);
        memcpy(clean, &gPlttBufferUnfaded[0x100 + 11 * 16], sizeof(clean));

        // Fog fade-in: honour the actual weather blend coefficient, without
        // using UpdateSpritePaletteWithWeather (which registers fog whitening).
        mockWeather.palProcessingState = WEATHER_PAL_STATE_SCREEN_FADING_IN;
        mockWeather.fadeScreenCounter = 0;
        mockWeather.fadeDestColor = 0; // fade from black
        Follower_RefreshPalette();
        assert(mockWeatherPaletteCalls == calls);
        assert(gPlttBufferFaded[0x100 + 11 * 16 + 1] == 0);
        mockWeather.fadeScreenCounter = 8;
        Follower_RefreshPalette();
        assert(mockWeatherPaletteCalls == calls);
        assert(gPlttBufferFaded[0x100 + 11 * 16 + 1] != clean[1]);
        assert(gPlttBufferFaded[0x100 + 11 * 16 + 1] != 0);
        mockWeather.fadeScreenCounter = 16;
        mockWeather.palProcessingState = WEATHER_PAL_STATE_IDLE;
        Follower_RefreshPalette();
        assert(memcmp(clean, &gPlttBufferFaded[0x100 + 11 * 16], sizeof(clean)) == 0);

        // Fog fade-out must still use global GBA palette-fade coefficients.
        mockWeather.palProcessingState = WEATHER_PAL_STATE_SCREEN_FADING_OUT;
        gPaletteFade.y = 16;
        gPaletteFade.blendColor = 0;
        Follower_RefreshPalette();
        assert(gPlttBufferFaded[0x100 + 11 * 16 + 1] == 0);
        gPaletteFade.y = 0;
        mockWeather.currWeather = WEATHER_NONE;
        mockWeather.palProcessingState = WEATHER_PAL_STATE_IDLE;
        Follower_RefreshPalette();
        assert(mockWeatherPaletteCalls == calls + 1); // non-fog unaffected
    }
    // v0.8.24: Sabrina's Gym warp pads recall before teleport-out and keep
    // the follower hidden across a SAME-MAP ResetObjectEvents. Do not spawn
    // into an occupied or second warp pad, or before teleport-in finishes.
    {
        unsigned seBefore;
        Follower_ResetForMapLoad();
        gObjectEvents[1].active = FALSE;
        mockSave.location.mapNum = 61;
        player->currentCoords.x = 45;
        player->currentCoords.y = 45;
        player->facingDirection = DIR_EAST;
        gPlayerAvatar.flags = PLAYER_AVATAR_FLAG_ON_FOOT;
        gPlayerAvatar.transitionFlags = 0;
        mockWarpX = 100; mockWarpY = 100;
        Follower_Update(player);
        assert(gObjectEvents[1].active);
        seBefore = mockRecallSeCount;
        Follower_BeginWarpPadRecall();
        assert(!gObjectEvents[1].active && mockRecallSeCount == seBefore + 1);
        assert(mockLastSe == SE_BALL_TRADE);
        Follower_Update(player); // Departure and fade still running.
        assert(!gObjectEvents[1].active);
        Follower_ResetForMapLoad(); // SAME map group + number.
        gObjectEvents[1].active = FALSE;
        player->currentCoords.x = 100;
        player->currentCoords.y = 100;
        mockScriptLocked = TRUE;
        gPaletteFade.active = TRUE;
        Follower_PrimeForOverworldDisplay();
        Follower_Update(player);
        assert(!gObjectEvents[1].active && mockRecallSeCount == seBefore + 1);
        Follower_EndWarpPadRecall(); // Only after teleport-in animation ends.
        Follower_Update(player);
        assert(!gObjectEvents[1].active); // Still locked/fading.
        gPaletteFade.active = FALSE;
        mockScriptLocked = FALSE;
        mockBlockedX = 99; mockBlockedY = 100;
        Follower_Update(player);
        assert(!gObjectEvents[1].active); // Blocked rear tile.
        mockBlockedX = -999; mockBlockedY = -999;
        player->currentCoords.x = 101; // Vacates destination warp tile.
        gPlayerAvatar.tileTransitionState = T_TILE_TRANSITION;
        player->heldMovementActive = TRUE;
        Follower_Update(player);
        assert(!gObjectEvents[1].active); // Never spawn on warp pad.
        mockWarpX = -999; mockWarpY = -999;
        mockSpawnFailure = TRUE;
        player->currentCoords.x = 102;
        Follower_Update(player);
        assert(!gObjectEvents[1].active && mockRecallSeCount == seBefore + 1);
        mockSpawnFailure = FALSE;
        player->currentCoords.x = 103; // Held key, no idle frame.
        Follower_Update(player);
        assert(gObjectEvents[1].active);
        assert(gObjectEvents[1].currentCoords.x == 102);
        assert(mockRecallSeCount == seBefore + 2 && mockLastSe == SE_BALL_OPEN);
        player->heldMovementActive = FALSE;
        gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
        Follower_Update(player);
        assert(mockRecallSeCount == seBefore + 2);

        // A chain warp can begin before the first follower respawn; no
        // phantom recall sound when an object was already hidden.
        seBefore = mockRecallSeCount;
        Follower_BeginWarpPadRecall();
        assert(!gObjectEvents[1].active && mockRecallSeCount == seBefore + 1);
        Follower_BeginWarpPadRecall();
        assert(mockRecallSeCount == seBefore + 1);
        Follower_ResetForMapLoad();
        gObjectEvents[1].active = FALSE;
        player->currentCoords.x = 110;
        player->currentCoords.y = 110;
        player->facingDirection = DIR_EAST;
        Follower_PrimeForOverworldDisplay();
        Follower_EndWarpPadRecall();
        Follower_Update(player);
        assert(gObjectEvents[1].active); // Safe rear tile, landing complete.
        assert(gObjectEvents[1].currentCoords.x == 109);
        assert(mockRecallSeCount == seBefore + 2 && mockLastSe == SE_BALL_OPEN);
    }
    // Fly started while another recall had already hidden the follower:
    // neither a new recall nor its eventual return may emit phantom audio.
    {
        unsigned seBefore;
        Follower_ResetForMapLoad();
        gObjectEvents[1].active = FALSE;
        mockSave.location.mapNum = 62;
        player->currentCoords.x = 120;
        player->currentCoords.y = 120;
        player->facingDirection = DIR_EAST;
        seBefore = mockRecallSeCount;
        Follower_BeginFlyRecall();
        assert(mockRecallSeCount == seBefore);
        Follower_ResetForMapLoad();
        mockSave.location.mapNum = 63;
        player->currentCoords.x = 130;
        player->currentCoords.y = 130;
        Follower_PrimeForOverworldDisplay();
        Follower_Update(player);
        assert(!gObjectEvents[1].active && mockRecallSeCount == seBefore);
        player->currentCoords.x = 131;
        Follower_Update(player);
        assert(gObjectEvents[1].active);
        assert(gObjectEvents[1].currentCoords.x == 130);
        assert(mockRecallSeCount == seBefore);
    }
    // v0.8.25.3: Safari Zone open-air warp must not use the generic
    // pre-display side/front-tile fallback. With continuous held input,
    // reveal only on the FIRST safe player-vacated tile.
    {
        unsigned seBefore = mockRecallSeCount;
        mockScriptLocked = FALSE;
        mockScript1Active = FALSE;
        gPaletteFade.active = FALSE;
        mockSpawnFailure = FALSE;
        mockBlockedX = -999; mockBlockedY = -999;
        gPlayerAvatar.preventStep = FALSE;
        gPlayerAvatar.flags = PLAYER_AVATAR_FLAG_ON_FOOT;
        gPlayerAvatar.transitionFlags = 0;
        gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
        player->heldMovementActive = FALSE;
        Follower_ResetForMapLoad();
        gObjectEvents[1].active = FALSE;
        mockSave.location.mapNum = 70;
        player->currentCoords.x = 70;
        player->currentCoords.y = 70;
        player->facingDirection = DIR_WEST;
        Follower_Update(player);
        assert(gObjectEvents[1].active);
        Follower_BeginSafariZoneWarp();
        assert(!gObjectEvents[1].active);
        Follower_ResetForMapLoad();
        mockSave.location.mapNum = 71;
        player->currentCoords.x = 80;
        player->currentCoords.y = 80;
        player->facingDirection = DIR_WEST;
        gPaletteFade.active = TRUE;
        mockScriptLocked = TRUE;
        Follower_PrimeForOverworldDisplay();
        Follower_Update(player);
        assert(!gObjectEvents[1].active);
        gPaletteFade.active = FALSE;
        mockScriptLocked = FALSE;
        Follower_Update(player);
        assert(!gObjectEvents[1].active); // No premature front/rear spawn while still.
        player->currentCoords.x = 79;     // Held left without an idle frame.
        gPlayerAvatar.tileTransitionState = T_TILE_TRANSITION;
        player->heldMovementActive = TRUE;
        Follower_Update(player);
        assert(gObjectEvents[1].active);
        assert(gObjectEvents[1].currentCoords.x == 80);
        assert(gObjectEvents[1].currentCoords.y == 80);
        assert(mockRecallSeCount == seBefore); // Safari fix adds no phantom audio.
        player->heldMovementActive = FALSE;
        gPlayerAvatar.tileTransitionState = T_NOT_MOVING;

        // Warp while moving, then try release against a blocked rear tile;
        // it must NEVER substitute a side or forward sprite location.
        Follower_BeginSafariZoneWarp();
        Follower_ResetForMapLoad();
        mockSave.location.mapNum = 72;
        player->currentCoords.x = 90;
        player->currentCoords.y = 90;
        player->facingDirection = DIR_WEST;
        Follower_PrimeForOverworldDisplay();
        assert(!gObjectEvents[1].active);
        Follower_Update(player);
        mockBlockedX = 90; mockBlockedY = 90;
        player->currentCoords.x = 89;
        Follower_Update(player);
        assert(!gObjectEvents[1].active);
        mockBlockedX = -999; mockBlockedY = -999;
        mockSpawnFailure = TRUE;
        player->currentCoords.x = 88;
        Follower_Update(player);
        assert(!gObjectEvents[1].active); // No free object-event capacity.
        mockSpawnFailure = FALSE;
        player->currentCoords.x = 87;
        Follower_Update(player);
        assert(gObjectEvents[1].active);
        assert(gObjectEvents[1].currentCoords.x == 88);
        assert(gObjectEvents[1].currentCoords.y == 90);

        // A chain of Safari warps must carry the suppression latch across
        // another same-session map reset, never release on an old anchor.
        Follower_BeginSafariZoneWarp();
        Follower_ResetForMapLoad();
        mockSave.location.mapNum = 73;
        player->currentCoords.x = 95;
        player->currentCoords.y = 95;
        Follower_PrimeForOverworldDisplay();
        Follower_BeginSafariZoneWarp();
        Follower_ResetForMapLoad();
        mockSave.location.mapNum = 74;
        player->currentCoords.x = 100;
        player->currentCoords.y = 100;
        Follower_PrimeForOverworldDisplay();
        Follower_Update(player);
        assert(!gObjectEvents[1].active);
        player->currentCoords.x = 99;
        Follower_Update(player);
        assert(gObjectEvents[1].active);
        assert(gObjectEvents[1].currentCoords.x == 100);
        assert(gObjectEvents[1].currentCoords.y == 100);
    }
    // v0.8.26: Dig, Escape Rope and field Teleport share a recall and
    // deferred return. The arrival animation may turn the player, and a held
    // D-pad can start a new step without an idle frame.
    {
        int fieldMove;
        for (fieldMove = 0; fieldMove < 3; fieldMove++)
        {
            unsigned before;
            Follower_ResetForMapLoad();
            mockScriptLocked = FALSE;
            mockScript1Active = FALSE;
            gPaletteFade.active = FALSE;
            mockWeatherFading = FALSE;
            mockBlockedX = -999; mockBlockedY = -999;
            mockSpawnFailure = FALSE;
            gPlayerAvatar.flags = PLAYER_AVATAR_FLAG_ON_FOOT;
            gPlayerAvatar.transitionFlags = 0;
            gPlayerAvatar.preventStep = FALSE;
            gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
            player->heldMovementActive = FALSE;
            player->invisible = FALSE;
            mockSave.location.mapNum = 100 + fieldMove * 2;
            mockSave.location.mapGroup = 1;
            player->currentCoords.x = 40;
            player->currentCoords.y = 40;
            player->facingDirection = DIR_EAST;
            gObjectEvents[1].active = FALSE;
            Follower_Update(player);
            assert(gObjectEvents[1].active);
            before = mockRecallSeCount;
            Follower_BeginFieldTravelRecall();
            assert(!gObjectEvents[1].active);
            assert(mockRecallSeCount == before + 1 && mockLastSe == SE_BALL_TRADE);
            Follower_Update(player); // departure animation: no spawn.
            Follower_BeginFieldTravelRecall(); // duplicate callback: no audio.
            assert(mockRecallSeCount == before + 1);
            Follower_ResetForMapLoad(); // map-load reset must preserve latch.
            gObjectEvents[1].active = FALSE;
            mockSave.location.mapNum++;
            player->currentCoords.x = 60;
            player->currentCoords.y = 60;
            player->facingDirection = DIR_WEST;
            mockScriptLocked = TRUE;
            gPaletteFade.active = TRUE;
            Follower_PrimeForOverworldDisplay();
            Follower_Update(player);
            assert(!gObjectEvents[1].active);
            gPaletteFade.active = FALSE;
            mockScriptLocked = FALSE;
            Follower_Update(player); // still landing, even though fade done.
            assert(!gObjectEvents[1].active);
            Follower_EndFieldTravelRecall();
            Follower_Update(player); // idle landing tile, no spawn.
            assert(!gObjectEvents[1].active);
            mockScript1Active = TRUE;
            player->currentCoords.x = 61;
            Follower_Update(player); // script movement cannot release.
            assert(!gObjectEvents[1].active);
            mockScript1Active = FALSE;
            Follower_Update(player); // settle new anchor.
            assert(!gObjectEvents[1].active);
            mockBlockedX = 61; mockBlockedY = 60;
            player->currentCoords.x = 62; // held movement starts first step.
            player->heldMovementActive = TRUE;
            gPlayerAvatar.tileTransitionState = T_TILE_TRANSITION;
            Follower_Update(player);
            assert(!gObjectEvents[1].active); // blocked rear: do not use front.
            mockBlockedX = -999; mockBlockedY = -999;
            mockSpawnFailure = TRUE;
            player->currentCoords.x = 63;
            Follower_Update(player);
            assert(!gObjectEvents[1].active);
            assert(mockRecallSeCount == before + 1);
            mockSpawnFailure = FALSE;
            player->currentCoords.x = 64;
            Follower_Update(player);
            assert(gObjectEvents[1].active);
            assert(gObjectEvents[1].currentCoords.x == 63);
            assert(gObjectEvents[1].currentCoords.y == 60);
            assert(mockRecallSeCount == before + 2 && mockLastSe == SE_BALL_OPEN);
            player->heldMovementActive = FALSE;
            gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
            Follower_Update(player);
            assert(mockRecallSeCount == before + 2);
        }
    }
    // v0.8.27: leaving a building uses an animated SOUTH exit step. That
    // scripted step must not materialise the follower. The first voluntary
    // held-D-pad step reveals it above the newly vacated outside tile.
    {
        unsigned seBefore = mockRecallSeCount;
        mockScriptLocked = FALSE;
        mockScript1Active = FALSE;
        gPaletteFade.active = FALSE;
        mockWeatherFading = FALSE;
        gPlayerAvatar.preventStep = FALSE;
        gPlayerAvatar.flags = PLAYER_AVATAR_FLAG_ON_FOOT;
        gPlayerAvatar.transitionFlags = 0;
        gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
        player->heldMovementActive = FALSE;
        player->heldMovementFinished = FALSE;
        mockSpawnFailure = FALSE;
        mockBlockedX = -999; mockBlockedY = -999;
        mockDoorX = -999; mockDoorY = -999;
        mockWarpX = -999; mockWarpY = -999;
        Follower_ResetForMapLoad();
        gObjectEvents[1].active = FALSE;
        mockSave.location.mapNum = 80;
        player->currentCoords.x = 50;
        player->currentCoords.y = 50;
        player->facingDirection = DIR_SOUTH;
        Follower_Update(player);
        assert(gObjectEvents[1].active);
        Follower_BeginBuildingExitWarp();
        assert(!gObjectEvents[1].active);
        Follower_ResetForMapLoad();
        mockSave.location.mapNum = 81;
        player->currentCoords.x = 50;
        player->currentCoords.y = 60;
        gPaletteFade.active = TRUE;
        mockScriptLocked = TRUE;
        Follower_PrimeForOverworldDisplay();
        Follower_Update(player);
        assert(!gObjectEvents[1].active);
        // FireRed automatically steps down while the destination door task
        // still owns ScriptContext2, sometimes updating tile coords early.
        player->currentCoords.y = 61;
        player->heldMovementActive = TRUE;
        gPlayerAvatar.tileTransitionState = T_TILE_TRANSITION;
        Follower_Update(player);
        assert(!gObjectEvents[1].active);
        gPaletteFade.active = FALSE;
        mockScriptLocked = FALSE;
        gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
        player->heldMovementActive = FALSE;
        Follower_Update(player);  // No premature spawn before task callback.
        assert(!gObjectEvents[1].active);
        Follower_EndBuildingExitWarp();  // Destination exit task has ended.
        Follower_Update(player);
        assert(!gObjectEvents[1].active); // No stationary pop-in.
        player->currentCoords.y = 62;
        player->heldMovementActive = TRUE;
        gPlayerAvatar.tileTransitionState = T_TILE_TRANSITION;
        Follower_Update(player);
        assert(gObjectEvents[1].active);
        assert(gObjectEvents[1].currentCoords.x == 50);
        assert(gObjectEvents[1].currentCoords.y == 61);
        assert(mockRecallSeCount == seBefore); // No unintended release audio.
        player->heldMovementActive = FALSE;
        gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
    }
    // Cave/indoor/Route23-to-VictoryRoad warp: exactly the established
    // Safari rear-only release. Never appear during fade or stationary idle.
    {
        mockScriptLocked = FALSE;
        gPaletteFade.active = FALSE;
        Follower_BeginRearOnlyWarp();
        Follower_ResetForMapLoad();
        mockSave.location.mapNum = 82;
        player->currentCoords.x = 100;
        player->currentCoords.y = 100;
        player->facingDirection = DIR_NORTH;
        mockScriptLocked = TRUE;
        gPaletteFade.active = TRUE;
        Follower_PrimeForOverworldDisplay();
        Follower_Update(player);
        assert(!gObjectEvents[1].active);
        mockScriptLocked = FALSE;
        gPaletteFade.active = FALSE;
        Follower_Update(player);
        assert(!gObjectEvents[1].active);
        mockBlockedX = 100; mockBlockedY = 100;
        player->currentCoords.y = 99;
        player->heldMovementActive = TRUE;
        gPlayerAvatar.tileTransitionState = T_TILE_TRANSITION;
        Follower_Update(player);
        assert(!gObjectEvents[1].active); // Rear occupied, never front.
        mockBlockedX = -999; mockBlockedY = -999;
        player->currentCoords.y = 98;
        Follower_Update(player);
        assert(gObjectEvents[1].active);
        assert(gObjectEvents[1].currentCoords.x == 100);
        assert(gObjectEvents[1].currentCoords.y == 99);
        player->heldMovementActive = FALSE;
        gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
    }
    // v0.9.0.3.19: Fly/field Teleport -> Pokemon Center -> immediate exit.
    // The first arrival's deferred return MUST be superseded when the player
    // starts entering the Center, even if not one outdoor tile was walked.
    // Releasing BOTH travel and door handoffs would strand the follower or
    // spawn it on the wrong map's tile. No hiding or forced extra steps.
    {
        int travel;
        for (travel = 0; travel < 2; travel++)
        {
            Follower_ResetForMapLoad();
            mockScriptLocked = FALSE;
            mockScript1Active = FALSE;
            gPaletteFade.active = FALSE;
            mockWeatherFading = FALSE;
            mockSpawnFailure = FALSE;
            mockBlockedX = -999; mockBlockedY = -999;
            mockWarpX = -999; mockWarpY = -999;
            mockDoorX = -999; mockDoorY = -999;
            gPlayerAvatar.flags = PLAYER_AVATAR_FLAG_ON_FOOT;
            gPlayerAvatar.transitionFlags = 0;
            gPlayerAvatar.preventStep = FALSE;
            gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
            player->heldMovementActive = FALSE;
            player->heldMovementFinished = FALSE;
            player->invisible = FALSE;
            mockSave.location.mapNum = 120 + travel * 3;
            mockSave.location.mapGroup = 2;
            player->currentCoords.x = 40;
            player->currentCoords.y = 40;
            player->facingDirection = DIR_SOUTH;
            gObjectEvents[1].active = FALSE;
            Follower_Update(player);
            assert(gObjectEvents[1].active);

            if (travel == 0)
                Follower_BeginFlyRecall();
            else
                Follower_BeginFieldTravelRecall();
            assert(!gObjectEvents[1].active);
            Follower_ResetForMapLoad();
            mockSave.location.mapNum++;
            player->currentCoords.x = 60;
            player->currentCoords.y = 60; // Flying/teleporting Center landing.
            Follower_PrimeForOverworldDisplay();
            if (travel == 1)
                Follower_EndFieldTravelRecall();
            Follower_Update(player);
            assert(!gObjectEvents[1].active); // First outdoor step not taken.

            // Center entrance is itself a new warp, not a delayed Fly step.
            Follower_BeginRearOnlyWarp();
            Follower_ResetForMapLoad();
            mockSave.location.mapNum++;
            player->currentCoords.x = 12;
            player->currentCoords.y = 12;
            Follower_PrimeForOverworldDisplay();
            Follower_Update(player);
            assert(!gObjectEvents[1].active);

            // Leave immediately without moving one controllable indoor tile.
            Follower_BeginBuildingExitWarp();
            Follower_ResetForMapLoad();
            mockSave.location.mapNum++;
            player->currentCoords.x = 60;
            player->currentCoords.y = 60;
            mockScriptLocked = TRUE;
            gPaletteFade.active = TRUE;
            Follower_PrimeForOverworldDisplay();
            Follower_Update(player);
            assert(!gObjectEvents[1].active);
            player->currentCoords.y = 61; // Engine's automatic door step.
            player->heldMovementActive = TRUE;
            gPlayerAvatar.tileTransitionState = T_TILE_TRANSITION;
            Follower_Update(player);
            assert(!gObjectEvents[1].active);
            gPaletteFade.active = FALSE;
            mockScriptLocked = FALSE;
            player->heldMovementActive = FALSE;
            gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
            Follower_EndBuildingExitWarp();
            mockDoorX = 60; mockDoorY = 60; // Building door behind player.
            Follower_Update(player);
            // Finished exit animation is NOT a safe spawn point: the rear
            // tile belongs to the Pokemon Center's door graphic.
            assert(!gObjectEvents[1].active);
            player->currentCoords.y = 62; // First manual tile step outdoors.
            player->heldMovementActive = TRUE;
            gPlayerAvatar.tileTransitionState = T_TILE_TRANSITION;
            Follower_Update(player);
            assert(gObjectEvents[1].active);
            assert(gObjectEvents[1].currentCoords.x == 60);
            assert(gObjectEvents[1].currentCoords.y == 61); // Vacated outdoor tile.
            mockDoorX = -999; mockDoorY = -999;
            player->heldMovementActive = FALSE;
            gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
            Follower_Update(player);
            assert(gObjectEvents[1].active);

            // If even the first vacated tile is another door threshold,
            // delay one more step; never use door art or a side/front tile.
            // Reproduce a second interrupted field Teleport first so this
            // also runs with the interrupted-travel handoff flag set.
            Follower_BeginFieldTravelRecall();
            Follower_ResetForMapLoad();
            mockSave.location.mapNum++;
            player->currentCoords.x = 60;
            player->currentCoords.y = 60;
            Follower_PrimeForOverworldDisplay();
            Follower_EndFieldTravelRecall();
            Follower_BeginRearOnlyWarp();
            Follower_ResetForMapLoad();
            mockSave.location.mapNum++;
            player->currentCoords.x = 12;
            player->currentCoords.y = 12;
            Follower_PrimeForOverworldDisplay();
            Follower_BeginBuildingExitWarp();
            Follower_ResetForMapLoad();
            mockSave.location.mapNum++;
            player->currentCoords.x = 60;
            player->currentCoords.y = 60;
            player->facingDirection = DIR_SOUTH;
            mockScriptLocked = TRUE;
            gPaletteFade.active = TRUE;
            Follower_PrimeForOverworldDisplay();
            player->currentCoords.y = 61;
            gPaletteFade.active = FALSE;
            mockScriptLocked = FALSE;
            Follower_EndBuildingExitWarp();
            mockDoorX = 60; mockDoorY = 61; // Door graphic on exit tile.
            Follower_Update(player);
            assert(!gObjectEvents[1].active);
            player->currentCoords.y = 62;
            Follower_Update(player);
            assert(!gObjectEvents[1].active); // Do not overlap door.
            player->currentCoords.y = 63;
            Follower_Update(player);
            assert(gObjectEvents[1].active);
            assert(gObjectEvents[1].currentCoords.y == 62);
            mockDoorX = -999; mockDoorY = -999;
        }
    }
    // Quest Log final scene: normal gameplay resumes without a map warp.
    // A stale door handoff may still own the dynamic follower, leaving it
    // absent forever. A one-time final-scene callback must clear that latch
    // and rebuild directly behind the saved player on a safe tile.
    {
        Follower_ResetForMapLoad();
        mockScriptLocked = FALSE;
        mockScript1Active = FALSE;
        gPaletteFade.active = FALSE;
        mockWeatherFading = FALSE;
        mockDoorX = -999; mockDoorY = -999;
        mockBlockedX = -999; mockBlockedY = -999;
        mockWarpX = -999; mockWarpY = -999;
        gPlayerAvatar.flags = PLAYER_AVATAR_FLAG_ON_FOOT;
        gPlayerAvatar.transitionFlags = 0;
        gPlayerAvatar.preventStep = FALSE;
        gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
        player->heldMovementActive = FALSE;
        player->heldMovementFinished = FALSE;
        player->invisible = FALSE;
        player->facingDirection = DIR_EAST;
        player->currentCoords.x = 80;
        player->currentCoords.y = 80;
        mockSave.location.mapNum = 90;
        mockSave.location.mapGroup = 2;
        gObjectEvents[1].active = FALSE;
        Follower_Update(player);
        assert(gObjectEvents[1].active);
        Follower_BeginDoorWarp();
        assert(!gObjectEvents[1].active);
        gQuestLogState = QL_STATE_PLAYBACK_LAST;
        Follower_ResumeAfterQuestLog(); // Never during active playback.
        assert(!gObjectEvents[1].active);
        gQuestLogState = 0;
        Follower_ResumeAfterQuestLog(); // EndQuestLog after releasing scripts.
        assert(gObjectEvents[1].active);
        assert(gObjectEvents[1].currentCoords.x == 79);
        assert(gObjectEvents[1].currentCoords.y == 80);
        // If the rear is obstructed, do not create a side/front overlap.
        Follower_BeginDoorWarp();
        mockBlockedX = 79; mockBlockedY = 80;
        Follower_ResumeAfterQuestLog();
        assert(!gObjectEvents[1].active);
        player->currentCoords.x = 81;
        Follower_Update(player);
        assert(gObjectEvents[1].active);
        assert(gObjectEvents[1].currentCoords.x == 80);
        assert(gObjectEvents[1].currentCoords.y == 80);
        mockBlockedX = -999; mockBlockedY = -999;
    }
    // v0.9.0.3.20 Viridian Gym: fade-only exit opens onto a narrow porch.
    // Actual city warp is (36,10), with porch (36,11), grid coords +7.
    // The player must move sideways because (36,12) is impassable.
    // Never spawn the follower on either entrance or porch tile.
    {
        mockScriptLocked = FALSE;
        mockScript1Active = FALSE;
        gPaletteFade.active = FALSE;
        mockWeatherFading = FALSE;
        mockBlockedX = -999; mockBlockedY = -999;
        mockDoorX = -999; mockDoorY = -999;
        gPlayerAvatar.flags = PLAYER_AVATAR_FLAG_ON_FOOT;
        gPlayerAvatar.preventStep = FALSE;
        gPlayerAvatar.tileTransitionState = T_NOT_MOVING;
        player->heldMovementActive = FALSE;
        player->heldMovementFinished = FALSE;
        player->facingDirection = DIR_SOUTH;
        mockSave.location.mapNum = MAP_NUM(VIRIDIAN_CITY_GYM);
        mockSave.location.mapGroup = MAP_GROUP(VIRIDIAN_CITY_GYM);
        player->currentCoords.x = 24;
        player->currentCoords.y = 29;
        Follower_ResetForMapLoad();
        Follower_Update(player);
        assert(gObjectEvents[1].active);
        Follower_BeginBuildingExitWarp();
        assert(!gObjectEvents[1].active);
        Follower_ResetForMapLoad();
        mockSave.location.mapNum = MAP_NUM(VIRIDIAN_CITY);
        mockSave.location.mapGroup = MAP_GROUP(VIRIDIAN_CITY);
        player->currentCoords.x = 43;
        player->currentCoords.y = 17; // Gym door warp (36,10).
        gPaletteFade.active = TRUE;
        mockScriptLocked = TRUE;
        mockBlockedX = 43; mockBlockedY = 17;
        mockDoorX = 43; mockDoorY = 17;
        Follower_PrimeForOverworldDisplay();
        Follower_Update(player);
        assert(!gObjectEvents[1].active);
        mockScriptLocked = FALSE;
        gPaletteFade.active = FALSE;
        Follower_EndBuildingExitWarp(); // Generic fade-only exit, no auto walk.
        Follower_Update(player);
        assert(!gObjectEvents[1].active);
        player->currentCoords.y = 18; // First controllable step onto porch.
        Follower_Update(player);
        assert(!gObjectEvents[1].active); // Never on the Gym doorway.
        player->currentCoords.x = 42; // Can't go south: step sideways.
        player->facingDirection = DIR_WEST;
        Follower_Update(player);
        assert(!gObjectEvents[1].active); // Never on the narrow porch.
        player->currentCoords.x = 41; // Now vacated walkway is clear.
        Follower_Update(player);
        assert(gObjectEvents[1].active);
        assert(gObjectEvents[1].currentCoords.x == 42);
        assert(gObjectEvents[1].currentCoords.y == 18);
        // No permanent suppression when the player continues walking.
        player->currentCoords.x = 40;
        Follower_Update(player);
        assert(gObjectEvents[1].active);
        mockBlockedX = -999; mockBlockedY = -999;
        mockDoorX = -999; mockDoorY = -999;
    }
    puts("PASS: runtime follower suite including v0.8.27 non-overworld deferred warps");
    return 0;
}
