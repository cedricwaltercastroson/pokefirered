#include "global.h"
#include "event_data.h"
#include "random.h"
#include "constants/maps.h"

struct RenewableHiddenItemData
{
    s8 mapGroup;
    s8 mapNum;
    u8 filler[2];
    u8 rare[8];     // 10%
    u8 uncommon[8]; // 30%
    u8 common[8];   // 60%
};

static void SampleRenewableItemFlags(void);

static const struct RenewableHiddenItemData sRenewableHiddenItems[] = {
    {
        .mapGroup = MAP_GROUP(ROUTE20),
        .mapNum = MAP_NUM(ROUTE20),
        .rare = {
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF
        },
        .uncommon = {
            HIDDEN_ITEM_ROUTE20_STARDUST,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF
        },
        .common = {
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF
        }
    },
    {
        .mapGroup = MAP_GROUP(ROUTE21_NORTH),
        .mapNum = MAP_NUM(ROUTE21_NORTH),
        .rare = {
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF
        },
        .uncommon = {
            HIDDEN_ITEM_ROUTE21_NORTH_PEARL,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF
        },
        .common = {
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF
        }
    },
    {
        .mapGroup = MAP_GROUP(UNDERGROUND_PATH_NORTH_SOUTH_TUNNEL),
        .mapNum = MAP_NUM(UNDERGROUND_PATH_NORTH_SOUTH_TUNNEL),
        .rare = {
            HIDDEN_ITEM_UNDERGROUND_PATH_NORTH_SOUTH_TUNNEL_ETHER,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF
        },
        .uncommon = {
            HIDDEN_ITEM_UNDERGROUND_PATH_NORTH_SOUTH_TUNNEL_POTION,
            HIDDEN_ITEM_UNDERGROUND_PATH_NORTH_SOUTH_TUNNEL_ANTIDOTE,
            HIDDEN_ITEM_UNDERGROUND_PATH_NORTH_SOUTH_TUNNEL_PARALYZE_HEAL,
            HIDDEN_ITEM_UNDERGROUND_PATH_NORTH_SOUTH_TUNNEL_AWAKENING,
            HIDDEN_ITEM_UNDERGROUND_PATH_NORTH_SOUTH_TUNNEL_BURN_HEAL,
            HIDDEN_ITEM_UNDERGROUND_PATH_NORTH_SOUTH_TUNNEL_ICE_HEAL,
            0xFF,
            0xFF
        },
        .common = {
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF
        }
    },
    {
        .mapGroup = MAP_GROUP(UNDERGROUND_PATH_EAST_WEST_TUNNEL),
        .mapNum = MAP_NUM(UNDERGROUND_PATH_EAST_WEST_TUNNEL),
        .rare = {
            HIDDEN_ITEM_UNDERGROUND_PATH_EAST_WEST_TUNNEL_ETHER,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF
        },
        .uncommon = {
            HIDDEN_ITEM_UNDERGROUND_PATH_EAST_WEST_TUNNEL_POTION,
            HIDDEN_ITEM_UNDERGROUND_PATH_EAST_WEST_TUNNEL_ANTIDOTE,
            HIDDEN_ITEM_UNDERGROUND_PATH_EAST_WEST_TUNNEL_PARALYZE_HEAL,
            HIDDEN_ITEM_UNDERGROUND_PATH_EAST_WEST_TUNNEL_AWAKENING,
            HIDDEN_ITEM_UNDERGROUND_PATH_EAST_WEST_TUNNEL_BURN_HEAL,
            HIDDEN_ITEM_UNDERGROUND_PATH_EAST_WEST_TUNNEL_ICE_HEAL,
            0xFF,
            0xFF
        },
        .common = {
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF
        }
    },
    {
        .mapGroup = MAP_GROUP(MT_MOON_B1F),
        .mapNum = MAP_NUM(MT_MOON_B1F),
        .rare = {
            HIDDEN_ITEM_MT_MOON_B1F_TINY_MUSHROOM,
            HIDDEN_ITEM_MT_MOON_B1F_TINY_MUSHROOM_2,
            HIDDEN_ITEM_MT_MOON_B1F_TINY_MUSHROOM_3,
            HIDDEN_ITEM_MT_MOON_B1F_BIG_MUSHROOM,
            HIDDEN_ITEM_MT_MOON_B1F_BIG_MUSHROOM_2,
            HIDDEN_ITEM_MT_MOON_B1F_BIG_MUSHROOM_3,
            0xFF,
            0xFF
        },
        .uncommon = {
            HIDDEN_ITEM_MT_MOON_B1F_TINY_MUSHROOM,
            HIDDEN_ITEM_MT_MOON_B1F_TINY_MUSHROOM_2,
            HIDDEN_ITEM_MT_MOON_B1F_TINY_MUSHROOM_3,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF
        },
        .common = {
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF
        }
    },
};

void SetAllRenewableItemFlags(void)
{
    u8 i, j;

    for (i = 0; i < 15; i++)
    {
        const u8 * rare = sRenewableHiddenItems[i].rare;
        const u8 * uncommon = sRenewableHiddenItems[i].uncommon;
        const u8 * common = sRenewableHiddenItems[i].common;
        for (j = 0; j < 8; j++)
        {
            if (rare[j] != 0xFF)
                FlagSet(FLAG_HIDDEN_ITEMS_START + rare[j]);
            if (uncommon[j] != 0xFF)
                FlagSet(FLAG_HIDDEN_ITEMS_START + uncommon[j]);
            if (common[j] != 0xFF)
                FlagSet(FLAG_HIDDEN_ITEMS_START + common[j]);
        }
    }
}

void IncrementRenewableHiddenItemStepCounter(void)
{
    u16 var = VarGet(VAR_RENEWABLE_ITEM_STEP_COUNTER);
    if (var < 1500)
    {
        VarSet(VAR_RENEWABLE_ITEM_STEP_COUNTER, var + 1);
    }
}

void TryRegenerateRenewableHiddenItems(void)
{
    u8 i;
    u8 found_map = 0xFF;
    for (i = 0; i < 15; i++)
    {
        if (sRenewableHiddenItems[i].mapGroup == gSaveBlock1Ptr->location.mapGroup && sRenewableHiddenItems[i].mapNum == gSaveBlock1Ptr->location.mapNum)
            found_map = i;
    }

    if (found_map == 0xFF)
        return;
    if (VarGet(VAR_RENEWABLE_ITEM_STEP_COUNTER) >= 1500)
    {
        VarSet(VAR_RENEWABLE_ITEM_STEP_COUNTER, 0);
        SetAllRenewableItemFlags();
        SampleRenewableItemFlags();
    }
}

static void SampleRenewableItemFlags(void)
{
    u8 i, j;
    const u8 * flags;
    u16 rval;

    for (i = 0; i < 15; i++)
    {
        rval = Random() % 100;
        if (rval >= 90)
            flags = sRenewableHiddenItems[i].rare;
        else if (rval >= 60)
            flags = sRenewableHiddenItems[i].uncommon;
        else
            flags = sRenewableHiddenItems[i].common;
        for (j = 0; j < 8; j++)
        {
            if (flags[j] != 0xFF)
                FlagClear(FLAG_HIDDEN_ITEMS_START + flags[j]);
        }
    }
}
