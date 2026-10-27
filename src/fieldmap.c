#include "global.h"
#include "gflib.h"
#include "overworld.h"
#include "script.h"
#include "new_menu_helpers.h"
#include "quest_log.h"
#include "fieldmap.h"
#include "field_effect.h"
#include "event_data.h"
#include "constants/vars.h"
#include "field_weather.h"
#include "constants/maps.h"

struct ConnectionFlags
{
    u8 south:1;
    u8 north:1;
    u8 west:1;
    u8 east:1;
};

static void InitMapLayoutData(struct MapHeader *);
static void InitBackupMapLayoutData(u16 *, u16, u16);
static void InitBackupMapLayoutConnections(struct MapHeader *);
static void FillSouthConnection(struct MapHeader const *, struct MapHeader const *, s32);
static void FillNorthConnection(struct MapHeader const *, struct MapHeader const *, s32);
static void FillWestConnection(struct MapHeader const *, struct MapHeader const *, s32);
static void FillEastConnection(struct MapHeader const *, struct MapHeader const *, s32);
static void LoadSavedMapView(void);
static struct MapConnection *GetIncomingConnection(u8, s32, s32);
static bool8 IsPosInIncomingConnectingMap(u8, s32, s32, struct MapConnection *);
static bool8 IsCoordInIncomingConnectingMap(s32, s32, s32, s32);
static u32 GetAttributeByMetatileIdAndMapLayout(const struct MapLayout *, u16, u8);

#define MapGridGetBorderTileAt(x, y) ({                                                            \
    u16 block;                                                                                     \
    s32 xprime;                                                                                    \
    s32 yprime;                                                                                    \
                                                                                                   \
    const struct MapLayout *mapLayout = gMapHeader.mapLayout;                                      \
                                                                                                   \
    xprime = x - 7;                                                                                \
    xprime += 8 * mapLayout->borderWidth;                                                          \
    xprime %= mapLayout->borderWidth;                                                              \
                                                                                                   \
    yprime = y - 7;                                                                                \
    yprime += 8 * mapLayout->borderHeight;                                                         \
    yprime %= mapLayout->borderHeight;                                                             \
                                                                                                   \
    block = mapLayout->border[xprime + yprime * mapLayout->borderWidth] | METATILE_COLLISION_MASK; \
})

#define AreCoordsWithinMapGridBounds(x, y) (x >= 0 && x < VMap.Xsize && y >= 0 && y < VMap.Ysize)

#define MapGridGetTileAt(x, y) (AreCoordsWithinMapGridBounds(x, y) ? VMap.map[x + VMap.Xsize * y] : MapGridGetBorderTileAt(x, y))

struct BackupMapLayout VMap;
EWRAM_DATA u16 gBackupMapData[VIRTUAL_MAP_SIZE] = {};
EWRAM_DATA struct MapHeader gMapHeader = {};
EWRAM_DATA struct Camera gCamera = {};
static EWRAM_DATA struct ConnectionFlags gMapConnectionFlags = {};
EWRAM_DATA u8 gGlobalFieldTintMode = QL_TINT_NONE;

static const struct ConnectionFlags sDummyConnectionFlags = {};

static const u32 sMetatileAttrMasks[METATILE_ATTRIBUTE_COUNT] = {
    [METATILE_ATTRIBUTE_BEHAVIOR]       = 0x000001ff,
    [METATILE_ATTRIBUTE_TERRAIN]        = 0x00003e00,
    [METATILE_ATTRIBUTE_2]              = 0x0003c000,
    [METATILE_ATTRIBUTE_3]              = 0x00fc0000,
    [METATILE_ATTRIBUTE_ENCOUNTER_TYPE] = 0x07000000,
    [METATILE_ATTRIBUTE_5]              = 0x18000000,
    [METATILE_ATTRIBUTE_LAYER_TYPE]     = 0x60000000,
    [METATILE_ATTRIBUTE_7]              = 0x80000000
};

static const u8 sMetatileAttrShifts[METATILE_ATTRIBUTE_COUNT] = {
    [METATILE_ATTRIBUTE_BEHAVIOR]       = 0,
    [METATILE_ATTRIBUTE_TERRAIN]        = 9,
    [METATILE_ATTRIBUTE_2]              = 14,
    [METATILE_ATTRIBUTE_3]              = 18,
    [METATILE_ATTRIBUTE_ENCOUNTER_TYPE] = 24,
    [METATILE_ATTRIBUTE_5]              = 27,
    [METATILE_ATTRIBUTE_LAYER_TYPE]     = 29,
    [METATILE_ATTRIBUTE_7]              = 31
};

const struct MapHeader * GetMapHeaderFromConnection(struct MapConnection * connection)
{
    return Overworld_GetMapHeaderByGroupAndId(connection->mapGroup, connection->mapNum);
}

void InitMap(void)
{
    InitMapLayoutData(&gMapHeader);
    RunOnLoadMapScript();
}

void InitMapFromSavedGame(void)
{
    InitMapLayoutData(&gMapHeader);
    LoadSavedMapView();
    RunOnLoadMapScript();
}

static void InitMapLayoutData(struct MapHeader * mapHeader)
{
    const struct MapLayout * mapLayout = mapHeader->mapLayout;
    CpuFastFill(METATILE_ID_UNDEFINED << 16 | METATILE_ID_UNDEFINED, gBackupMapData, sizeof(gBackupMapData));
    VMap.map = gBackupMapData;
    VMap.Xsize = mapLayout->width + 15;
    VMap.Ysize = mapLayout->height + 14;
    AGB_ASSERT_EX(VMap.Xsize * VMap.Ysize <= VIRTUAL_MAP_SIZE, ABSPATH("fieldmap.c"), 158);
    InitBackupMapLayoutData(mapLayout->map, mapLayout->width, mapLayout->height);
    InitBackupMapLayoutConnections(mapHeader);
}

static void InitBackupMapLayoutData(u16 *map, u16 width, u16 height)
{
    s32 y;
    u16 *dest = VMap.map;
    dest += VMap.Xsize * 7 + 7;

    for (y = 0; y < height; y++)
    {
        CpuCopy16(map, dest, width * sizeof(u16));
        dest += width + 15;
        map += width;
    }
}

static void InitBackupMapLayoutConnections(struct MapHeader *mapHeader)
{
    s32 count;
    struct MapConnection *connection;
    s32 i;

    gMapConnectionFlags = sDummyConnectionFlags;

    /*
     * This null pointer check is new to FireRed.  It was kept in
     * Emerald, with the above struct assignment moved to after
     * this check.
     */
    if (mapHeader->connections)
    {
        count = mapHeader->connections->count;
        connection = mapHeader->connections->connections;
        for (i = 0; i < count; i++, connection++)
        {
            struct MapHeader const *cMap = GetMapHeaderFromConnection(connection);
            u32 offset = connection->offset;
            switch (connection->direction)
            {
            case CONNECTION_SOUTH:
                FillSouthConnection(mapHeader, cMap, offset);
                gMapConnectionFlags.south = TRUE;
                break;
            case CONNECTION_NORTH:
                FillNorthConnection(mapHeader, cMap, offset);
                gMapConnectionFlags.north = TRUE;
                break;
            case CONNECTION_WEST:
                FillWestConnection(mapHeader, cMap, offset);
                gMapConnectionFlags.west = TRUE;
                break;
            case CONNECTION_EAST:
                FillEastConnection(mapHeader, cMap, offset);
                gMapConnectionFlags.east = TRUE;
                break;
            }
        }
    }
}

static void FillConnection(s32 x, s32 y, const struct MapHeader *connectedMapHeader, s32 x2, s32 y2, s32 width, s32 height)
{
    s32 i;
    u16 *src;
    u16 *dest;
    s32 mapWidth;

    mapWidth = connectedMapHeader->mapLayout->width;
    src = &connectedMapHeader->mapLayout->map[mapWidth * y2 + x2];
    dest = &VMap.map[VMap.Xsize * y + x];

    for (i = 0; i < height; i++)
    {
        CpuCopy16(src, dest, width * 2);
        dest += VMap.Xsize;
        src += mapWidth;
    }
}

static void FillSouthConnection(struct MapHeader const *mapHeader, struct MapHeader const *connectedMapHeader, s32 offset)
{
    s32 x, y;
    s32 x2;
    s32 width;
    s32 cWidth;

    if (connectedMapHeader)
    {
        cWidth = connectedMapHeader->mapLayout->width;
        x = offset + 7;
        y = mapHeader->mapLayout->height + 7;
        if (x < 0)
        {
            x2 = -x;
            x += cWidth;
            if (x < VMap.Xsize)
                width = x;
            else
                width = VMap.Xsize;
            x = 0;
        }
        else
        {
            x2 = 0;
            if (x + cWidth < VMap.Xsize)
                width = cWidth;
            else
                width = VMap.Xsize - x;
        }

        FillConnection(
            x, y,
            connectedMapHeader,
            x2, /*y2*/ 0,
            width, /*height*/ 7);
    }
}

static void FillNorthConnection(struct MapHeader const *mapHeader, struct MapHeader const *connectedMapHeader, s32 offset)
{
    s32 x;
    s32 x2, y2;
    s32 width;
    s32 cWidth, cHeight;

    if (connectedMapHeader)
    {
        cWidth = connectedMapHeader->mapLayout->width;
        cHeight = connectedMapHeader->mapLayout->height;
        x = offset + 7;
        y2 = cHeight - 7;
        if (x < 0)
        {
            x2 = -x;
            x += cWidth;
            if (x < VMap.Xsize)
                width = x;
            else
                width = VMap.Xsize;
            x = 0;
        }
        else
        {
            x2 = 0;
            if (x + cWidth < VMap.Xsize)
                width = cWidth;
            else
                width = VMap.Xsize - x;
        }

        FillConnection(
            x, /*y*/ 0,
            connectedMapHeader,
            x2, y2,
            width, /*height*/ 7);

    }
}

static void FillWestConnection(struct MapHeader const *mapHeader, struct MapHeader const *connectedMapHeader, s32 offset)
{
    s32 y;
    s32 x2, y2;
    s32 height;
    s32 cWidth, cHeight;
    if (connectedMapHeader)
    {
        cWidth = connectedMapHeader->mapLayout->width;
        cHeight = connectedMapHeader->mapLayout->height;
        y = offset + 7;
        x2 = cWidth - 7;
        if (y < 0)
        {
            y2 = -y;
            if (y + cHeight < VMap.Ysize)
                height = y + cHeight;
            else
                height = VMap.Ysize;
            y = 0;
        }
        else
        {
            y2 = 0;
            if (y + cHeight < VMap.Ysize)
                height = cHeight;
            else
                height = VMap.Ysize - y;
        }

        FillConnection(
            /*x*/ 0, y,
            connectedMapHeader,
            x2, y2,
            /*width*/ 7, height);
    }
}

static void FillEastConnection(struct MapHeader const *mapHeader, struct MapHeader const *connectedMapHeader, s32 offset)
{
    s32 x, y;
    s32 y2;
    s32 height;
    s32 cHeight;
    if (connectedMapHeader)
    {
        cHeight = connectedMapHeader->mapLayout->height;
        x = mapHeader->mapLayout->width + 7;
        y = offset + 7;
        if (y < 0)
        {
            y2 = -y;
            if (y + cHeight < VMap.Ysize)
                height = y + cHeight;
            else
                height = VMap.Ysize;
            y = 0;
        }
        else
        {
            y2 = 0;
            if (y + cHeight < VMap.Ysize)
                height = cHeight;
            else
                height = VMap.Ysize - y;
        }

        FillConnection(
            x, y,
            connectedMapHeader,
            /*x2*/ 0, y2,
            /*width*/ 8, height);
    }
}

u8 MapGridGetZCoordAt(s32 x, s32 y)
{
    u16 block = MapGridGetTileAt(x, y);

    if (block == METATILE_ID_UNDEFINED)
        return 0;

    return block >> METATILE_ELEVATION_SHIFT;
}

u8 MapGridIsImpassableAt(s32 x, s32 y)
{
    u16 block = MapGridGetTileAt(x, y);

    if (block == METATILE_ID_UNDEFINED)
        return TRUE;

    return (block & METATILE_COLLISION_MASK) >> METATILE_COLLISION_SHIFT;
}

u32 MapGridGetMetatileIdAt(s32 x, s32 y)
{
    u16 block = MapGridGetTileAt(x, y);

    if (block == METATILE_ID_UNDEFINED)
        return MapGridGetBorderTileAt(x, y) & METATILE_ID_MASK;

    return block & METATILE_ID_MASK;
}

u32 ExtractMetatileAttribute(u32 attributes, u8 attributeType)
{
    if (attributeType >= METATILE_ATTRIBUTE_COUNT)
        return attributes;

    return (attributes & sMetatileAttrMasks[attributeType]) >> sMetatileAttrShifts[attributeType];
}

u32 MapGridGetMetatileAttributeAt(s16 x, s16 y, u8 attributeType)
{
    u16 metatileId = MapGridGetMetatileIdAt(x, y);
    return GetAttributeByMetatileIdAndMapLayout(gMapHeader.mapLayout, metatileId, attributeType);
}

u32 MapGridGetMetatileBehaviorAt(s16 x, s16 y)
{
    return MapGridGetMetatileAttributeAt(x, y, METATILE_ATTRIBUTE_BEHAVIOR);
}

u8 MapGridGetMetatileLayerTypeAt(s16 x, s16 y)
{
    return MapGridGetMetatileAttributeAt(x, y, METATILE_ATTRIBUTE_LAYER_TYPE);
}

void MapGridSetMetatileIdAt(s32 x, s32 y, u16 metatile)
{
    s32 i;
    if (AreCoordsWithinMapGridBounds(x, y))
    {
        i = x + y * VMap.Xsize;
        VMap.map[i] = (VMap.map[i] & METATILE_ELEVATION_MASK) | (metatile & ~METATILE_ELEVATION_MASK);
    }
}

void MapGridSetMetatileEntryAt(s32 x, s32 y, u16 metatile)
{
    s32 i;
    if (AreCoordsWithinMapGridBounds(x, y))
    {
        i = x + VMap.Xsize * y;
        VMap.map[i] = metatile;
    }
}

void MapGridSetMetatileImpassabilityAt(s32 x, s32 y, bool32 impassable)
{
    if (AreCoordsWithinMapGridBounds(x, y))
    {
        if (impassable)
            VMap.map[x + VMap.Xsize * y] |= METATILE_COLLISION_MASK;
        else
            VMap.map[x + VMap.Xsize * y] &= ~METATILE_COLLISION_MASK;
    }
}

static u32 GetAttributeByMetatileIdAndMapLayout(const struct MapLayout *mapLayout, u16 metatile, u8 attributeType)
{
    u32 * attributes;

    if (metatile < NUM_METATILES_IN_PRIMARY)
    {
        attributes = mapLayout->primaryTileset->metatileAttributes;
        return ExtractMetatileAttribute(attributes[metatile], attributeType);
    }
    else if (metatile < NUM_METATILES_TOTAL)
    {
        attributes = mapLayout->secondaryTileset->metatileAttributes;
        return ExtractMetatileAttribute(attributes[metatile - NUM_METATILES_IN_PRIMARY], attributeType);
    }
    else
    {
        return 0xFF;
    }
}

void SaveMapView(void)
{
    s32 i, j;
    s32 x, y;
    u16 *mapView;
    s32 width;
    mapView = gSaveBlock2Ptr->mapView;
    width = VMap.Xsize;
    x = gSaveBlock1Ptr->pos.x;
    y = gSaveBlock1Ptr->pos.y;
    for (i = y; i < y + 14; i++)
    {
        for (j = x; j < x + 15; j++)
            *mapView++ = gBackupMapData[width * i + j];
    }
}

static bool32 SavedMapViewIsEmpty(void)
{
    u16 i;
    u32 marker = 0;

#ifndef UBFIX
    // BUG: This loop extends past the bounds of the mapView array. Its size is only 0x100.
    for (i = 0; i < 0x200; i++)
        marker |= gSaveBlock2Ptr->mapView[i];
#else
    for (i = 0; i < NELEMS(gSaveBlock2Ptr->mapView); i++)
        marker |= gSaveBlock2Ptr->mapView[i];
#endif

    if (marker == 0)
        return TRUE;
    else
        return FALSE;
}

static void ClearSavedMapView(void)
{
    CpuFill16(0, gSaveBlock2Ptr->mapView, sizeof(gSaveBlock2Ptr->mapView));
}

static void LoadSavedMapView(void)
{
    s32 i, j;
    s32 x, y;
    u16 *mapView;
    s32 width;
    mapView = gSaveBlock2Ptr->mapView;
    if (!SavedMapViewIsEmpty())
    {
        width = VMap.Xsize;
        x = gSaveBlock1Ptr->pos.x;
        y = gSaveBlock1Ptr->pos.y;
        for (i = y; i < y + 14; i++)
        {
            for (j = x; j < x + 15; j++)
            {
                gBackupMapData[j + width * i] = *mapView;
                mapView++;
            }
        }
        ClearSavedMapView();
    }
}

static void MoveMapViewToBackup(u8 direction)
{
    s32 width;
    u16 *mapView;
    s32 x0, y0;
    s32 x2, y2;
    u16 *src, *dest;
    s32 srci, desti;
    s32 r9, r8;
    s32 x, y;
    s32 i, j;
    mapView = gSaveBlock2Ptr->mapView;
    width = VMap.Xsize;
    r9 = 0;
    r8 = 0;
    x0 = gSaveBlock1Ptr->pos.x;
    y0 = gSaveBlock1Ptr->pos.y;
    x2 = 15;
    y2 = 14;
    switch (direction)
    {
    case CONNECTION_NORTH:
        y0 += 1;
        y2 = 13;
        break;
    case CONNECTION_SOUTH:
        r8 = 1;
        y2 = 13;
        break;
    case CONNECTION_WEST:
        x0 += 1;
        x2 = 14;
        break;
    case CONNECTION_EAST:
        r9 = 1;
        x2 = 14;
        break;
    }
    for (y = 0; y < y2; y++)
    {
        i = 0;
        j = 0;
        for (x = 0; x < x2; x++)
        {
            desti = width * (y + y0);
            srci = (y + r8) * 15 + r9;
            src = &mapView[srci + i];
            dest = &gBackupMapData[x0 + desti + j];
            *dest = *src;
            i++;
            j++;
        }
    }
    ClearSavedMapView();
}

s32 GetMapBorderIdAt(s32 x, s32 y)
{
    if (MapGridGetTileAt(x, y) == METATILE_ID_UNDEFINED)
        return CONNECTION_INVALID;

    if (x >= VMap.Xsize - 8)
    {
        if (!gMapConnectionFlags.east)
            return CONNECTION_INVALID;

        return CONNECTION_EAST;
    }

    if (x < 7)
    {
        if (!gMapConnectionFlags.west)
            return CONNECTION_INVALID;

        return CONNECTION_WEST;
    }

    if (y >= VMap.Ysize - 7)
    {
        if (!gMapConnectionFlags.south)
            return CONNECTION_INVALID;

        return CONNECTION_SOUTH;
    }

    if (y < 7)
    {
        if (!gMapConnectionFlags.north)
            return CONNECTION_INVALID;

        return CONNECTION_NORTH;
    }

    return CONNECTION_NONE;
}

static s32 GetPostCameraMoveMapBorderId(s32 x, s32 y)
{
    return GetMapBorderIdAt(gSaveBlock1Ptr->pos.x + 7 + x, gSaveBlock1Ptr->pos.y + 7 + y);
}

bool32 CanCameraMoveInDirection(s32 direction)
{
    s32 x, y;
    x = gSaveBlock1Ptr->pos.x + 7 + gDirectionToVectors[direction].x;
    y = gSaveBlock1Ptr->pos.y + 7 + gDirectionToVectors[direction].y;

    if (GetMapBorderIdAt(x, y) == CONNECTION_INVALID)
        return FALSE;

    return TRUE;
}

static void SetPositionFromConnection(struct MapConnection *connection, int direction, s32 x, s32 y)
{
    struct MapHeader const *mapHeader;
    mapHeader = GetMapHeaderFromConnection(connection);
    switch (direction)
    {
    case CONNECTION_EAST:
        gSaveBlock1Ptr->pos.x = -x;
        gSaveBlock1Ptr->pos.y -= connection->offset;
        break;
    case CONNECTION_WEST:
        gSaveBlock1Ptr->pos.x = mapHeader->mapLayout->width;
        gSaveBlock1Ptr->pos.y -= connection->offset;
        break;
    case CONNECTION_SOUTH:
        gSaveBlock1Ptr->pos.x -= connection->offset;
        gSaveBlock1Ptr->pos.y = -y;
        break;
    case CONNECTION_NORTH:
        gSaveBlock1Ptr->pos.x -= connection->offset;
        gSaveBlock1Ptr->pos.y = mapHeader->mapLayout->height;
        break;
    }
}

bool8 CameraMove(s32 x, s32 y)
{
    s32 direction;
    struct MapConnection *connection;
    s32 old_x, old_y;
    gCamera.active = FALSE;
    direction = GetPostCameraMoveMapBorderId(x, y);
    if (direction == CONNECTION_NONE || direction == CONNECTION_INVALID)
    {
        gSaveBlock1Ptr->pos.x += x;
        gSaveBlock1Ptr->pos.y += y;
    }
    else
    {
        SaveMapView();
        old_x = gSaveBlock1Ptr->pos.x;
        old_y = gSaveBlock1Ptr->pos.y;
        connection = GetIncomingConnection(direction, gSaveBlock1Ptr->pos.x, gSaveBlock1Ptr->pos.y);
        SetPositionFromConnection(connection, direction, x, y);
        LoadMapFromCameraTransition(connection->mapGroup, connection->mapNum);
        gCamera.active = TRUE;
        gCamera.x = old_x - gSaveBlock1Ptr->pos.x;
        gCamera.y = old_y - gSaveBlock1Ptr->pos.y;
        gSaveBlock1Ptr->pos.x += x;
        gSaveBlock1Ptr->pos.y += y;
        MoveMapViewToBackup(direction);
    }
    return gCamera.active;
}

struct MapConnection *GetIncomingConnection(u8 direction, s32 x, s32 y)
{
    s32 count;
    struct MapConnection *connection;
    const struct MapConnections *connections = gMapHeader.connections;
    s32 i;

#ifdef UBFIX // UB: Multiple possible null dereferences
    if (connections == NULL || connections->connections == NULL)
        return NULL;
#endif
    count = connections->count;
    connection = connections->connections;
    for (i = 0; i < count; i++, connection++)
    {
        if (connection->direction == direction && IsPosInIncomingConnectingMap(direction, x, y, connection) == TRUE)
            return connection;
    }
    return NULL;

}

static bool8 IsPosInIncomingConnectingMap(u8 direction, s32 x, s32 y, struct MapConnection *connection)
{
    struct MapHeader const *mapHeader;
    mapHeader = GetMapHeaderFromConnection(connection);
    switch (direction)
    {
    case CONNECTION_SOUTH:
    case CONNECTION_NORTH:
        return IsCoordInIncomingConnectingMap(x, gMapHeader.mapLayout->width, mapHeader->mapLayout->width, connection->offset);
    case CONNECTION_WEST:
    case CONNECTION_EAST:
        return IsCoordInIncomingConnectingMap(y, gMapHeader.mapLayout->height, mapHeader->mapLayout->height, connection->offset);
    }
    return FALSE;
}

static bool8 IsCoordInIncomingConnectingMap(s32 coord, s32 srcMax, s32 destMax, s32 offset)
{
    s32 offset2 = max(offset, 0);

    if (destMax + offset < srcMax)
        srcMax = destMax + offset;

    if (offset2 <= coord && coord <= srcMax)
        return TRUE;

    return FALSE;
}

static bool32 IsCoordInConnectingMap(s32 coord, s32 max)
{
    if (coord >= 0 && coord < max)
        return TRUE;

    return FALSE;
}

static s32 IsPosInConnectingMap(struct MapConnection *connection, s32 x, s32 y)
{
    struct MapHeader const *mapHeader;
    mapHeader = GetMapHeaderFromConnection(connection);
    switch (connection->direction)
    {
    case CONNECTION_SOUTH:
    case CONNECTION_NORTH:
        return IsCoordInConnectingMap(x - connection->offset, mapHeader->mapLayout->width);
    case CONNECTION_WEST:
    case CONNECTION_EAST:
        return IsCoordInConnectingMap(y - connection->offset, mapHeader->mapLayout->height);
    }
    return FALSE;
}

struct MapConnection *GetMapConnectionAtPos(s16 x, s16 y)
{
    s32 count;
    struct MapConnection *connection;
    s32 i;
    u8 direction;
    if (!gMapHeader.connections)
    {
        return NULL;
    }
    else
    {
        count = gMapHeader.connections->count;
        connection = gMapHeader.connections->connections;
        for (i = 0; i < count; i++, connection++)
        {
            direction = connection->direction;
            if ((direction == CONNECTION_DIVE || direction == CONNECTION_EMERGE)
                || (direction == CONNECTION_NORTH && y > 6)
                || (direction == CONNECTION_SOUTH && y < gMapHeader.mapLayout->height + 7)
                || (direction == CONNECTION_WEST && x > 6)
                || (direction == CONNECTION_EAST && x < gMapHeader.mapLayout->width + 7))
            {
                continue;
            }

            if (IsPosInConnectingMap(connection, x - 7, y - 7) == TRUE)
                return connection;
        }
    }
    return NULL;
}

void SetCameraFocusCoords(u16 x, u16 y)
{
    gSaveBlock1Ptr->pos.x = x - 7;
    gSaveBlock1Ptr->pos.y = y - 7;
}

void GetCameraFocusCoords(u16 *x, u16 *y)
{
    *x = gSaveBlock1Ptr->pos.x + 7;
    *y = gSaveBlock1Ptr->pos.y + 7;
}

// Unused
static void SetCameraCoords(u16 x, u16 y)
{
    gSaveBlock1Ptr->pos.x = x;
    gSaveBlock1Ptr->pos.y = y;
}

void GetCameraCoords(u16 *x, u16 *y)
{
    *x = gSaveBlock1Ptr->pos.x;
    *y = gSaveBlock1Ptr->pos.y;
}

static void CopyTilesetToVram(struct Tileset const *tileset, u16 numTiles, u16 offset)
{
    if (tileset)
    {
        if (!tileset->isCompressed)
            LoadBgTiles(2, tileset->tiles, numTiles * 32, offset);
        else
            DecompressAndCopyTileDataToVram2(2, tileset->tiles, numTiles * 32, offset, 0);
    }
}

static void CopyTilesetToVramUsingHeap(struct Tileset const *tileset, u16 numTiles, u16 offset)
{
    if (tileset)
    {
        if (!tileset->isCompressed)
            LoadBgTiles(2, tileset->tiles, numTiles * 32, offset);
        else
            DecompressAndLoadBgGfxUsingHeap2(2, tileset->tiles, numTiles * 32, offset, 0);
    }
}

static void Fieldmap_ApplyGlobalTintToPaletteEntries(u16 offset, u16 size)
{
    switch (gGlobalFieldTintMode)
    {
    case QL_TINT_NONE:
        return;
    case QL_TINT_GRAYSCALE:
        TintPalette_GrayScale(gPlttBufferUnfaded + offset, size);
        break;
    case QL_TINT_SEPIA:
        TintPalette_SepiaTone(gPlttBufferUnfaded + offset, size);
        break;
    case QL_TINT_BACKUP_GRAYSCALE:
        QuestLog_BackUpPalette(offset, size);
        TintPalette_GrayScale(gPlttBufferUnfaded + offset, size);
        break;
    default:
        return;
    }
    CpuCopy16(gPlttBufferUnfaded + offset, gPlttBufferFaded + offset, size * sizeof(u16));
}

void Fieldmap_ApplyGlobalTintToPaletteSlot(u8 slot, u8 count)
{
    switch (gGlobalFieldTintMode)
    {
    case QL_TINT_NONE:
        return;
    case QL_TINT_GRAYSCALE:
        TintPalette_GrayScale(gPlttBufferUnfaded + slot * 16, count * 16);
        break;
    case QL_TINT_SEPIA:
        TintPalette_SepiaTone(gPlttBufferUnfaded + slot * 16, count * 16);
        break;
    case QL_TINT_BACKUP_GRAYSCALE:
        QuestLog_BackUpPalette(slot * 16, count * 16);
        TintPalette_GrayScale(gPlttBufferUnfaded + slot * 16, count * 16);
        break;
    default:
        return;
    }
    CpuFastCopy(gPlttBufferUnfaded + slot * 16, gPlttBufferFaded + slot * 16, count * 16 * sizeof(u16));
}

// v0.9.0.3.14: Crystal-inspired day/night selection at palette load time.
// The approved Pallet Town/General night banks are preserved byte-for-byte.
// Safari Zone reuses General and Fuchsia City night banks; no recolouring.
// Only active-play-time map loads or one safe 08:00/20:00 boundary refresh
// update BG palettes. Never write to hardware palettes from VBlank.
// Using playtime avoids save migration: 60 active minutes = 24 game hours.
// Day: 08:00 to 19:59; Night: 20:00 to 07:59.
// Extra caution: Quest Log, special weather and interiors always own their palettes.
// Optional QA override: 0 = automatic; 1 = force DAY; 2 = force NIGHT.
// Leave as 0 in normal builds. Restriction to eligible outdoor maps still applies.
#define TOD_DEBUG_FORCE_PERIOD 0
#define TOD_UNINITIALIZED 0
#define TOD_DAY           1
#define TOD_NIGHT         2

static EWRAM_DATA u8 sLastLoadedTimeOfDay = TOD_UNINITIALIZED;
// 0 = idle, 1 = fade to black, 2 = fade back in. These are transient
// overworld-only states and are NEVER written to the save file.
static EWRAM_DATA u8 sTimeOfDayTransition = 0;
static EWRAM_DATA bool8 sTimeOfDayPaletteRefresh = FALSE;
#define TOD_FADE_DELAY 1 // Fast native BG/OBJ fade; roughly 1 second for both stages at 60 FPS.


// These two are the EXACT user-approved v0.9.0.3.6.5 palettes.
static const u16 sNightGeneral[] = INCBIN_U16("data/tilesets/night/pallet_town/general.gbapal");
static const u16 sNightPalletTown[] = INCBIN_U16("data/tilesets/night/pallet_town/pallet_town.gbapal");
static const u16 sNightViridianCity[] = INCBIN_U16("data/tilesets/night/outdoor/viridian_city/viridian_city.gbapal");
static const u16 sNightPewterCity[] = INCBIN_U16("data/tilesets/night/outdoor/pewter_city/pewter_city.gbapal");
static const u16 sNightCeruleanCity[] = INCBIN_U16("data/tilesets/night/outdoor/cerulean_city/cerulean_city.gbapal");
static const u16 sNightLavenderTown[] = INCBIN_U16("data/tilesets/night/outdoor/lavender_town/lavender_town.gbapal");
static const u16 sNightVermilionCity[] = INCBIN_U16("data/tilesets/night/outdoor/vermilion_city/vermilion_city.gbapal");
static const u16 sNightCeladonCity[] = INCBIN_U16("data/tilesets/night/outdoor/celadon_city/celadon_city.gbapal");
static const u16 sNightFuchsiaCity[] = INCBIN_U16("data/tilesets/night/outdoor/fuchsia_city/fuchsia_city.gbapal");
static const u16 sNightCinnabarIsland[] = INCBIN_U16("data/tilesets/night/outdoor/cinnabar_island/cinnabar_island.gbapal");
static const u16 sNightIndigoPlateau[] = INCBIN_U16("data/tilesets/night/outdoor/indigo_plateau/indigo_plateau.gbapal");
static const u16 sNightSaffronCity[] = INCBIN_U16("data/tilesets/night/outdoor/saffron_city/saffron_city.gbapal");

extern const struct Tileset gTileset_General;
extern const struct Tileset gTileset_PalletTown;
extern const struct Tileset gTileset_ViridianCity;
extern const struct Tileset gTileset_PewterCity;
extern const struct Tileset gTileset_CeruleanCity;
extern const struct Tileset gTileset_LavenderTown;
extern const struct Tileset gTileset_VermilionCity;
extern const struct Tileset gTileset_CeladonCity;
extern const struct Tileset gTileset_FuchsiaCity;
extern const struct Tileset gTileset_CinnabarIsland;
extern const struct Tileset gTileset_IndigoPlateau;
extern const struct Tileset gTileset_SaffronCity;

// Uses 30 active minutes DAY / 30 active minutes NIGHT (60 per full cycle).
// The saved bed adjustment is in virtual minutes (0..1439); the faster clock
// must never reinterpret this stored value as a real-time seconds offset.
// SaveBlock layout and actual accumulated playtime remain unchanged.
#define TOD_MINUTES_PER_CYCLE (24 * 60)
#define TOD_DAY_START_MINUTES (8 * 60)
#define TOD_NIGHT_START_MINUTES (20 * 60)
static u32 TimeOfDay_GetPlaySeconds(void)
{
    return (u32)gSaveBlock2Ptr->playTimeHours * 3600
         + (u32)gSaveBlock2Ptr->playTimeMinutes * 60
         + gSaveBlock2Ptr->playTimeSeconds;
}

// One full 24-hour cycle runs in 3600 active gameplay seconds. Reducing
// seconds modulo the real-time cycle before multiplication avoids overflow
// on long-standing saves with large accumulated playtime.
static u32 TimeOfDay_GetGameMinuteAtSeconds(u32 seconds)
{
    // Two virtual minutes per five seconds of active play.
    return (TOD_DAY_START_MINUTES + (seconds % 3600) * 2 / 5)
        % TOD_MINUTES_PER_CYCLE;
}

static bool8 TimeOfDay_IsNightAtSeconds(u32 seconds)
{
    u32 gameMinute = TimeOfDay_GetGameMinuteAtSeconds(seconds);
    return gameMinute >= TOD_NIGHT_START_MINUTES
        || gameMinute < TOD_DAY_START_MINUTES;
}

// 0 = choose 08:00, 1 = choose 20:00. Sleep starts a fresh 30-minute
// period without modifying recorded playtime or the existing save layout.
void TimeOfDay_SleepUntil(void)
{
    u32 targetMinute;
    u32 unshiftedMinute;
    if (gSaveBlock1Ptr == NULL || gSaveBlock2Ptr == NULL)
        return;
    if (gSpecialVar_0x8004 == 0)
        targetMinute = TOD_DAY_START_MINUTES;
    else if (gSpecialVar_0x8004 == 1)
        targetMinute = TOD_NIGHT_START_MINUTES;
    else
        return;
    unshiftedMinute = TimeOfDay_GetGameMinuteAtSeconds(TimeOfDay_GetPlaySeconds());
    VarSet(VAR_SLEEP_TIME_OFFSET,
           (targetMinute + TOD_MINUTES_PER_CYCLE - unshiftedMinute)
           % TOD_MINUTES_PER_CYCLE);
}

static bool8 TimeOfDay_IsNight(void)
{
    u32 gameMinute;
    // Keep the original unshifted path available for old saves.
    if (VarGet(VAR_SLEEP_TIME_OFFSET) == 0)
        return TimeOfDay_IsNightAtSeconds(TimeOfDay_GetPlaySeconds());

    gameMinute = (TimeOfDay_GetGameMinuteAtSeconds(TimeOfDay_GetPlaySeconds())
        + (u32)(VarGet(VAR_SLEEP_TIME_OFFSET) % TOD_MINUTES_PER_CYCLE))
        % TOD_MINUTES_PER_CYCLE;
    return gameMinute >= TOD_NIGHT_START_MINUTES
        || gameMinute < TOD_DAY_START_MINUTES;
}

// All four outdoor Safari Zone sections use the General primary tileset and
// the Fuchsia City secondary tileset. Both already have approved night banks.
// Reuse those exact colours and the existing map-load/clock-boundary pipeline.
// Outdoor weather eligibility continues to guard ordinary Kanto maps; special
// Safari interiors and rest houses do not enter this switch.
static bool8 TimeOfDay_IsEligibleMap(void)
{
    u16 mapId;

    if (gSaveBlock1Ptr == NULL || gMapHeader.mapLayout == NULL)
        return FALSE;

    mapId = gSaveBlock1Ptr->location.mapNum | (gSaveBlock1Ptr->location.mapGroup << 8);
    switch (mapId)
    {
    case MAP_SAFARI_ZONE_CENTER:
    case MAP_SAFARI_ZONE_EAST:
    case MAP_SAFARI_ZONE_NORTH:
    case MAP_SAFARI_ZONE_WEST:
        // Only the open-air Safari Zone layout is eligible. Should a map
        // header be customized later, fail safely instead of applying the
        // wrong palette to an unrelated layout.
        return gMapHeader.mapLayout->primaryTileset == &gTileset_General
            && gMapHeader.mapLayout->secondaryTileset == &gTileset_FuchsiaCity
            && DynamicOverworldWeather_IsEligibleMap();
    default:
        return DynamicOverworldWeather_IsEligibleMap();
    }
}

static u8 TimeOfDay_GetDesiredState(void)
{

    // Use the 38-map lighting allowlist, distinct from the new Safari weather
    // eligibility. Special/indoor map palettes must remain unchanged.
    if (!TimeOfDay_IsEligibleMap())
        return TOD_DAY;

    // The Quest Log's final Saved Game scene reloads the ACTUAL saved map
    // under a grayscale effect that also backs up its colour palettes.
    // Select the real day/night BG source now, while the scene is still
    // grayscale, so that backup already contains the correct night colours.
    // Otherwise a day-coloured backup can become visible for one frame when
    // the Quest Log releases the overworld, before our later correction.
    // All other playback/tint modes still use the original Quest Log rules.
    if (!(gQuestLogState == QL_STATE_PLAYBACK_LAST
          && gGlobalFieldTintMode == QL_TINT_BACKUP_GRAYSCALE)
        && (gGlobalFieldTintMode != QL_TINT_NONE
            || gQuestLogState == QL_STATE_PLAYBACK
            || gQuestLogState == QL_STATE_PLAYBACK_LAST))
        return TOD_DAY;

#if TOD_DEBUG_FORCE_PERIOD == 1
    return TOD_DAY;
#elif TOD_DEBUG_FORCE_PERIOD == 2
    return TOD_NIGHT;
#endif

    return TimeOfDay_IsNight() ? TOD_NIGHT : TOD_DAY;
}

// Field-effect grass uses its own OBJ palette; the background palette banks
// alone cannot grade that foreground artwork. Only query the same safe state.
bool8 TimeOfDay_IsNightForFieldEffects(void)
{
    // Keep the Quest Log's sprite grayscale/sepia palettes in full control.
    // The special BACKUP_GRAYSCALE exception above is for map BG sources only.
    if (gSaveBlock1Ptr == NULL || gSaveBlock2Ptr == NULL || gMapHeader.mapLayout == NULL
        || gGlobalFieldTintMode != QL_TINT_NONE
        || gQuestLogState == QL_STATE_PLAYBACK
        || gQuestLogState == QL_STATE_PLAYBACK_LAST)
        return FALSE;
    return TimeOfDay_GetDesiredState() == TOD_NIGHT;
}

static const u16 *TimeOfDay_GetNightPalette(const struct Tileset *tileset)
{
    if (tileset == gMapHeader.mapLayout->primaryTileset)
        return sNightGeneral;
    if (tileset == gMapHeader.mapLayout->secondaryTileset)
    {
        if (tileset == &gTileset_PalletTown)
            return sNightPalletTown;
        if (tileset == &gTileset_ViridianCity)
            return sNightViridianCity;
        if (tileset == &gTileset_PewterCity)
            return sNightPewterCity;
        if (tileset == &gTileset_CeruleanCity)
            return sNightCeruleanCity;
        if (tileset == &gTileset_LavenderTown)
            return sNightLavenderTown;
        if (tileset == &gTileset_VermilionCity)
            return sNightVermilionCity;
        if (tileset == &gTileset_CeladonCity)
            return sNightCeladonCity;
        if (tileset == &gTileset_FuchsiaCity)
            return sNightFuchsiaCity;
        if (tileset == &gTileset_CinnabarIsland)
            return sNightCinnabarIsland;
        if (tileset == &gTileset_IndigoPlateau)
            return sNightIndigoPlateau;
        if (tileset == &gTileset_SaffronCity)
            return sNightSaffronCity;
    }
    return NULL; // Never use a mismatched palette for an unknown tileset.
}

// Refresh only map BG sources, then let FireRed's weather system process them.
// Used both under the black screen of our transition and when the Quest Log
// releases palette ownership; never touch player/follower OBJ source palettes.
static void TimeOfDay_RefreshBackgroundPalettes(void)
{
    u8 i;
    sTimeOfDayPaletteRefresh = TRUE;
    LoadMapTilesetPalettes(gMapHeader.mapLayout);
    sTimeOfDayPaletteRefresh = FALSE;
    // Restore the grass field-effect OBJ palette alongside BG sources.
    // The shared grass palette must not remain bright green at night.
    FieldEffect_RefreshTimeOfDayPalette();
    for (i = 0; i < NUM_PALS_TOTAL; i++)
        ApplyWeatherGammaShiftToPal(i);
}

// The final Saved Game Quest Log scene restores a backup of the palette before
// resuming normal gameplay. Correct the BG source banks in that SAME frame,
// before VBlank displays the overworld; do not start a second visible fade.
void TimeOfDay_RestoreAfterQuestLog(void)
{
    if (gSaveBlock1Ptr == NULL || gSaveBlock2Ptr == NULL
        || gMapHeader.mapLayout == NULL || !TimeOfDay_IsEligibleMap()
        || gGlobalFieldTintMode != QL_TINT_NONE
        || gQuestLogState == QL_STATE_PLAYBACK
        || gQuestLogState == QL_STATE_PLAYBACK_LAST)
        return;
    sTimeOfDayTransition = 0;
    TimeOfDay_RefreshBackgroundPalettes();
}

// Use the normal FireRed palette fade engine, never a per-VBlank tint.
// At the 08:00/20:00 boundary, quickly fade all palettes to black, swap
// BG source banks invisibly, and immediately fade back (about 1 second total).
// Animation keeps running; only player input is briefly guarded so warps and
// scripts cannot take palette ownership mid-swap. No extra black-screen wait.
// This is not used during map load or Quest Log.
void TimeOfDay_TryUpdateOutdoorPalette(void)
{
    u8 newState;
    if (sTimeOfDayTransition == 1)
    {
        if (gPaletteFade.active)
            return;
        // We own the black frame; switch palette source banks here.
        TimeOfDay_RefreshBackgroundPalettes();
        if (BeginNormalPaletteFade(PALETTES_ALL, TOD_FADE_DELAY, 16, 0, RGB_BLACK))
            sTimeOfDayTransition = 2;
        return;
    }
    if (sTimeOfDayTransition == 2)
    {
        if (!gPaletteFade.active)
        {
            sTimeOfDayTransition = 0;
            ScriptContext2_Disable();
        }
        return;
    }
    if (gSaveBlock1Ptr == NULL || gSaveBlock2Ptr == NULL
        || gMapHeader.mapLayout == NULL || !TimeOfDay_IsEligibleMap()
        || gGlobalFieldTintMode != QL_TINT_NONE
        || gQuestLogState == QL_STATE_PLAYBACK
        || gQuestLogState == QL_STATE_PLAYBACK_LAST
        || gPaletteFade.active || ScriptContext1_IsScriptSetUp()
        || ScriptContext2_IsEnabled()
        || gWeatherPtr->palProcessingState != WEATHER_PAL_STATE_IDLE
        // SetNextWeather() can leave weatherChangeComplete FALSE even when
        // currWeather == nextWeather and weather palettes are already idle.
        // Check actual weather activity instead, so a 30-minute clock boundary
        // cannot remain blocked by a stale completion flag.
        || gWeatherPtr->currWeather != gWeatherPtr->nextWeather)
        return;

    newState = TimeOfDay_GetDesiredState();
    if (newState == sLastLoadedTimeOfDay)
        return;
    if (BeginNormalPaletteFade(PALETTES_ALL, TOD_FADE_DELAY, 0, 16, RGB_BLACK))
    {
        // Do not start another script or allow movement until fade-in ends.
        ScriptContext2_Enable();
        sTimeOfDayTransition = 1;
    }
}

static void LoadTilesetPalette(struct Tileset const *tileset, u16 destOffset, u16 size)
{
    u16 black = RGB_BLACK;
    const u16 *paletteData;

    if (tileset)
    {
        paletteData = tileset->palettes;
        // Select existing night source palettes only on eligible outdoor map loads.
        if (TimeOfDay_GetDesiredState() == TOD_NIGHT)
        {
            const u16 *nightPalette = TimeOfDay_GetNightPalette(tileset);
            if (nightPalette != NULL)
                paletteData = nightPalette;
        }

        if (tileset->isSecondary == FALSE)
        {
            LoadPalette(&black, destOffset, 2);
            LoadPalette(paletteData + 1, destOffset + 1, size - 2);
            Fieldmap_ApplyGlobalTintToPaletteEntries(destOffset + 1, (size - 2) >> 1);
        }
        else if (tileset->isSecondary == TRUE)
        {
            LoadPalette(paletteData + (NUM_PALS_IN_PRIMARY * 16), destOffset, size);
            Fieldmap_ApplyGlobalTintToPaletteEntries(destOffset, size >> 1);
        }
        else
        {
            LoadCompressedPalette((u32*)tileset->palettes, destOffset, size);
            Fieldmap_ApplyGlobalTintToPaletteEntries(destOffset, size >> 1);
        }
    }
}

void CopyPrimaryTilesetToVram(const struct MapLayout *mapLayout)
{
    CopyTilesetToVram(mapLayout->primaryTileset, NUM_TILES_IN_PRIMARY, 0);
}

void CopySecondaryTilesetToVram(const struct MapLayout *mapLayout)
{
    CopyTilesetToVram(mapLayout->secondaryTileset, NUM_TILES_TOTAL - NUM_TILES_IN_PRIMARY, NUM_TILES_IN_PRIMARY);
}

void CopySecondaryTilesetToVramUsingHeap(const struct MapLayout *mapLayout)
{
    CopyTilesetToVramUsingHeap(mapLayout->secondaryTileset, NUM_TILES_TOTAL - NUM_TILES_IN_PRIMARY, NUM_TILES_IN_PRIMARY);
}

static void LoadPrimaryTilesetPalette(const struct MapLayout *mapLayout)
{
    LoadTilesetPalette(mapLayout->primaryTileset, 0, NUM_PALS_IN_PRIMARY * 16 * 2);
}

void LoadSecondaryTilesetPalette(const struct MapLayout *mapLayout)
{
    LoadTilesetPalette(mapLayout->secondaryTileset, NUM_PALS_IN_PRIMARY * 16, (NUM_PALS_TOTAL - NUM_PALS_IN_PRIMARY) * 16 * 2);
}

void CopyMapTilesetsToVram(struct MapLayout const *mapLayout)
{
    if (mapLayout)
    {
        CopyTilesetToVramUsingHeap(mapLayout->primaryTileset, NUM_TILES_IN_PRIMARY, 0);
        CopyTilesetToVramUsingHeap(mapLayout->secondaryTileset, NUM_TILES_TOTAL - NUM_TILES_IN_PRIMARY, NUM_TILES_IN_PRIMARY);
    }
}

void LoadMapTilesetPalettes(struct MapLayout const *mapLayout)
{
    if (mapLayout)
    {
        // Unknown during Quest Log grayscale/sepia playback. Restore once
        // overworld gameplay regains palette ownership.
        // An ordinary map load cancels a pending boundary fade. The explicit
        // under-black palette refresh above is the only exception.
        if (!sTimeOfDayPaletteRefresh)
            sTimeOfDayTransition = 0;
        sLastLoadedTimeOfDay = (gGlobalFieldTintMode != QL_TINT_NONE
            || gQuestLogState == QL_STATE_PLAYBACK
            || gQuestLogState == QL_STATE_PLAYBACK_LAST)
            ? TOD_UNINITIALIZED : TimeOfDay_GetDesiredState();
        LoadPrimaryTilesetPalette(mapLayout);
        LoadSecondaryTilesetPalette(mapLayout);
    }
}
