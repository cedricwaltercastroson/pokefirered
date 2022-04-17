#include "global.h"
#include "gflib.h"
#include "mail_data.h"
#include "pokemon_icon.h"
#include "graphics.h"

#define POKE_ICON_BASE_PAL_TAG 56000

struct MonIconSpriteTemplate
{
    const struct OamData * oam;
    const u8 *image;
    const union AnimCmd *const * anims;
    const union AffineAnimCmd *const * affineAnims;
    SpriteCallback callback;
    u16 paletteTag;
};

static u8 CreateMonIconSprite(const struct MonIconSpriteTemplate * template, s16 x, s16 y, u8 subpriority);
static void DestroyMonIconInternal(struct Sprite * sprite);

const u16 gMonIconPalettes[][16] = {
    INCBIN_U16("graphics/pokemon/icon_palettes/icon_palette_0.gbapal"),
    INCBIN_U16("graphics/pokemon/icon_palettes/icon_palette_1.gbapal"),
    INCBIN_U16("graphics/pokemon/icon_palettes/icon_palette_2.gbapal")
};

const u8 *const gMonIconTable[] = {
    [SPECIES_NONE]        = gMonIcon_QuestionMark,
    [SPECIES_BULBASAUR]   = gMonIcon_Bulbasaur,
    [SPECIES_IVYSAUR]     = gMonIcon_Ivysaur,
    [SPECIES_VENUSAUR]    = gMonIcon_Venusaur,
    [SPECIES_CHARMANDER]  = gMonIcon_Charmander,
    [SPECIES_CHARMELEON]  = gMonIcon_Charmeleon,
    [SPECIES_CHARIZARD]   = gMonIcon_Charizard,
    [SPECIES_SQUIRTLE]    = gMonIcon_Squirtle,
    [SPECIES_WARTORTLE]   = gMonIcon_Wartortle,
    [SPECIES_BLASTOISE]   = gMonIcon_Blastoise,
    [SPECIES_CATERPIE]    = gMonIcon_Caterpie,
    [SPECIES_METAPOD]     = gMonIcon_Metapod,
    [SPECIES_BUTTERFREE]  = gMonIcon_Butterfree,
    [SPECIES_WEEDLE]      = gMonIcon_Weedle,
    [SPECIES_KAKUNA]      = gMonIcon_Kakuna,
    [SPECIES_BEEDRILL]    = gMonIcon_Beedrill,
    [SPECIES_PIDGEY]      = gMonIcon_Pidgey,
    [SPECIES_PIDGEOTTO]   = gMonIcon_Pidgeotto,
    [SPECIES_PIDGEOT]     = gMonIcon_Pidgeot,
    [SPECIES_RATTATA]     = gMonIcon_Rattata,
    [SPECIES_RATICATE]    = gMonIcon_Raticate,
    [SPECIES_SPEAROW]     = gMonIcon_Spearow,
    [SPECIES_FEAROW]      = gMonIcon_Fearow,
    [SPECIES_EKANS]       = gMonIcon_Ekans,
    [SPECIES_ARBOK]       = gMonIcon_Arbok,
    [SPECIES_PIKACHU]     = gMonIcon_Pikachu,
    [SPECIES_RAICHU]      = gMonIcon_Raichu,
    [SPECIES_SANDSHREW]   = gMonIcon_Sandshrew,
    [SPECIES_SANDSLASH]   = gMonIcon_Sandslash,
    [SPECIES_NIDORAN_F]   = gMonIcon_NidoranF,
    [SPECIES_NIDORINA]    = gMonIcon_Nidorina,
    [SPECIES_NIDOQUEEN]   = gMonIcon_Nidoqueen,
    [SPECIES_NIDORAN_M]   = gMonIcon_NidoranM,
    [SPECIES_NIDORINO]    = gMonIcon_Nidorino,
    [SPECIES_NIDOKING]    = gMonIcon_Nidoking,
    [SPECIES_CLEFAIRY]    = gMonIcon_Clefairy,
    [SPECIES_CLEFABLE]    = gMonIcon_Clefable,
    [SPECIES_VULPIX]      = gMonIcon_Vulpix,
    [SPECIES_NINETALES]   = gMonIcon_Ninetales,
    [SPECIES_JIGGLYPUFF]  = gMonIcon_Jigglypuff,
    [SPECIES_WIGGLYTUFF]  = gMonIcon_Wigglytuff,
    [SPECIES_ZUBAT]       = gMonIcon_Zubat,
    [SPECIES_GOLBAT]      = gMonIcon_Golbat,
    [SPECIES_ODDISH]      = gMonIcon_Oddish,
    [SPECIES_GLOOM]       = gMonIcon_Gloom,
    [SPECIES_VILEPLUME]   = gMonIcon_Vileplume,
    [SPECIES_PARAS]       = gMonIcon_Paras,
    [SPECIES_PARASECT]    = gMonIcon_Parasect,
    [SPECIES_VENONAT]     = gMonIcon_Venonat,
    [SPECIES_VENOMOTH]    = gMonIcon_Venomoth,
    [SPECIES_DIGLETT]     = gMonIcon_Diglett,
    [SPECIES_DUGTRIO]     = gMonIcon_Dugtrio,
    [SPECIES_MEOWTH]      = gMonIcon_Meowth,
    [SPECIES_PERSIAN]     = gMonIcon_Persian,
    [SPECIES_PSYDUCK]     = gMonIcon_Psyduck,
    [SPECIES_GOLDUCK]     = gMonIcon_Golduck,
    [SPECIES_MANKEY]      = gMonIcon_Mankey,
    [SPECIES_PRIMEAPE]    = gMonIcon_Primeape,
    [SPECIES_GROWLITHE]   = gMonIcon_Growlithe,
    [SPECIES_ARCANINE]    = gMonIcon_Arcanine,
    [SPECIES_POLIWAG]     = gMonIcon_Poliwag,
    [SPECIES_POLIWHIRL]   = gMonIcon_Poliwhirl,
    [SPECIES_POLIWRATH]   = gMonIcon_Poliwrath,
    [SPECIES_ABRA]        = gMonIcon_Abra,
    [SPECIES_KADABRA]     = gMonIcon_Kadabra,
    [SPECIES_ALAKAZAM]    = gMonIcon_Alakazam,
    [SPECIES_MACHOP]      = gMonIcon_Machop,
    [SPECIES_MACHOKE]     = gMonIcon_Machoke,
    [SPECIES_MACHAMP]     = gMonIcon_Machamp,
    [SPECIES_BELLSPROUT]  = gMonIcon_Bellsprout,
    [SPECIES_WEEPINBELL]  = gMonIcon_Weepinbell,
    [SPECIES_VICTREEBEL]  = gMonIcon_Victreebel,
    [SPECIES_TENTACOOL]   = gMonIcon_Tentacool,
    [SPECIES_TENTACRUEL]  = gMonIcon_Tentacruel,
    [SPECIES_GEODUDE]     = gMonIcon_Geodude,
    [SPECIES_GRAVELER]    = gMonIcon_Graveler,
    [SPECIES_GOLEM]       = gMonIcon_Golem,
    [SPECIES_PONYTA]      = gMonIcon_Ponyta,
    [SPECIES_RAPIDASH]    = gMonIcon_Rapidash,
    [SPECIES_SLOWPOKE]    = gMonIcon_Slowpoke,
    [SPECIES_SLOWBRO]     = gMonIcon_Slowbro,
    [SPECIES_MAGNEMITE]   = gMonIcon_Magnemite,
    [SPECIES_MAGNETON]    = gMonIcon_Magneton,
    [SPECIES_FARFETCHD]   = gMonIcon_Farfetchd,
    [SPECIES_DODUO]       = gMonIcon_Doduo,
    [SPECIES_DODRIO]      = gMonIcon_Dodrio,
    [SPECIES_SEEL]        = gMonIcon_Seel,
    [SPECIES_DEWGONG]     = gMonIcon_Dewgong,
    [SPECIES_GRIMER]      = gMonIcon_Grimer,
    [SPECIES_MUK]         = gMonIcon_Muk,
    [SPECIES_SHELLDER]    = gMonIcon_Shellder,
    [SPECIES_CLOYSTER]    = gMonIcon_Cloyster,
    [SPECIES_GASTLY]      = gMonIcon_Gastly,
    [SPECIES_HAUNTER]     = gMonIcon_Haunter,
    [SPECIES_GENGAR]      = gMonIcon_Gengar,
    [SPECIES_ONIX]        = gMonIcon_Onix,
    [SPECIES_DROWZEE]     = gMonIcon_Drowzee,
    [SPECIES_HYPNO]       = gMonIcon_Hypno,
    [SPECIES_KRABBY]      = gMonIcon_Krabby,
    [SPECIES_KINGLER]     = gMonIcon_Kingler,
    [SPECIES_VOLTORB]     = gMonIcon_Voltorb,
    [SPECIES_ELECTRODE]   = gMonIcon_Electrode,
    [SPECIES_EXEGGCUTE]   = gMonIcon_Exeggcute,
    [SPECIES_EXEGGUTOR]   = gMonIcon_Exeggutor,
    [SPECIES_CUBONE]      = gMonIcon_Cubone,
    [SPECIES_MAROWAK]     = gMonIcon_Marowak,
    [SPECIES_HITMONLEE]   = gMonIcon_Hitmonlee,
    [SPECIES_HITMONCHAN]  = gMonIcon_Hitmonchan,
    [SPECIES_LICKITUNG]   = gMonIcon_Lickitung,
    [SPECIES_KOFFING]     = gMonIcon_Koffing,
    [SPECIES_WEEZING]     = gMonIcon_Weezing,
    [SPECIES_RHYHORN]     = gMonIcon_Rhyhorn,
    [SPECIES_RHYDON]      = gMonIcon_Rhydon,
    [SPECIES_CHANSEY]     = gMonIcon_Chansey,
    [SPECIES_TANGELA]     = gMonIcon_Tangela,
    [SPECIES_KANGASKHAN]  = gMonIcon_Kangaskhan,
    [SPECIES_HORSEA]      = gMonIcon_Horsea,
    [SPECIES_SEADRA]      = gMonIcon_Seadra,
    [SPECIES_GOLDEEN]     = gMonIcon_Goldeen,
    [SPECIES_SEAKING]     = gMonIcon_Seaking,
    [SPECIES_STARYU]      = gMonIcon_Staryu,
    [SPECIES_STARMIE]     = gMonIcon_Starmie,
    [SPECIES_MR_MIME]     = gMonIcon_Mrmime,
    [SPECIES_SCYTHER]     = gMonIcon_Scyther,
    [SPECIES_JYNX]        = gMonIcon_Jynx,
    [SPECIES_ELECTABUZZ]  = gMonIcon_Electabuzz,
    [SPECIES_MAGMAR]      = gMonIcon_Magmar,
    [SPECIES_PINSIR]      = gMonIcon_Pinsir,
    [SPECIES_TAUROS]      = gMonIcon_Tauros,
    [SPECIES_MAGIKARP]    = gMonIcon_Magikarp,
    [SPECIES_GYARADOS]    = gMonIcon_Gyarados,
    [SPECIES_LAPRAS]      = gMonIcon_Lapras,
    [SPECIES_DITTO]       = gMonIcon_Ditto,
    [SPECIES_EEVEE]       = gMonIcon_Eevee,
    [SPECIES_VAPOREON]    = gMonIcon_Vaporeon,
    [SPECIES_JOLTEON]     = gMonIcon_Jolteon,
    [SPECIES_FLAREON]     = gMonIcon_Flareon,
    [SPECIES_PORYGON]     = gMonIcon_Porygon,
    [SPECIES_OMANYTE]     = gMonIcon_Omanyte,
    [SPECIES_OMASTAR]     = gMonIcon_Omastar,
    [SPECIES_KABUTO]      = gMonIcon_Kabuto,
    [SPECIES_KABUTOPS]    = gMonIcon_Kabutops,
    [SPECIES_AERODACTYL]  = gMonIcon_Aerodactyl,
    [SPECIES_SNORLAX]     = gMonIcon_Snorlax,
    [SPECIES_ARTICUNO]    = gMonIcon_Articuno,
    [SPECIES_ZAPDOS]      = gMonIcon_Zapdos,
    [SPECIES_MOLTRES]     = gMonIcon_Moltres,
    [SPECIES_DRATINI]     = gMonIcon_Dratini,
    [SPECIES_DRAGONAIR]   = gMonIcon_Dragonair,
    [SPECIES_DRAGONITE]   = gMonIcon_Dragonite,
    [SPECIES_MEWTWO]      = gMonIcon_Mewtwo,
    [SPECIES_MEW]         = gMonIcon_Mew,
    [SPECIES_EGG]         = gMonIcon_Egg,
};

const u8 gMonIconPaletteIndices[] = {
    [SPECIES_NONE]        = 0,
    [SPECIES_BULBASAUR]   = 1,
    [SPECIES_IVYSAUR]     = 1,
    [SPECIES_VENUSAUR]    = 1,
    [SPECIES_CHARMANDER]  = 0,
    [SPECIES_CHARMELEON]  = 0,
    [SPECIES_CHARIZARD]   = 0,
    [SPECIES_SQUIRTLE]    = 0,
    [SPECIES_WARTORTLE]   = 2,
    [SPECIES_BLASTOISE]   = 2,
    [SPECIES_CATERPIE]    = 1,
    [SPECIES_METAPOD]     = 1,
    [SPECIES_BUTTERFREE]  = 0,
    [SPECIES_WEEDLE]      = 1,
    [SPECIES_KAKUNA]      = 2,
    [SPECIES_BEEDRILL]    = 2,
    [SPECIES_PIDGEY]      = 0,
    [SPECIES_PIDGEOTTO]   = 0,
    [SPECIES_PIDGEOT]     = 0,
    [SPECIES_RATTATA]     = 2,
    [SPECIES_RATICATE]    = 1,
    [SPECIES_SPEAROW]     = 0,
    [SPECIES_FEAROW]      = 0,
    [SPECIES_EKANS]       = 2,
    [SPECIES_ARBOK]       = 2,
    [SPECIES_PIKACHU]     = 2,
    [SPECIES_RAICHU]      = 0,
    [SPECIES_SANDSHREW]   = 2,
    [SPECIES_SANDSLASH]   = 2,
    [SPECIES_NIDORAN_F]   = 2,
    [SPECIES_NIDORINA]    = 2,
    [SPECIES_NIDOQUEEN]   = 2,
    [SPECIES_NIDORAN_M]   = 2,
    [SPECIES_NIDORINO]    = 2,
    [SPECIES_NIDOKING]    = 2,
    [SPECIES_CLEFAIRY]    = 0,
    [SPECIES_CLEFABLE]    = 0,
    [SPECIES_VULPIX]      = 2,
    [SPECIES_NINETALES]   = 1,
    [SPECIES_JIGGLYPUFF]  = 0,
    [SPECIES_WIGGLYTUFF]  = 0,
    [SPECIES_ZUBAT]       = 2,
    [SPECIES_GOLBAT]      = 2,
    [SPECIES_ODDISH]      = 1,
    [SPECIES_GLOOM]       = 0,
    [SPECIES_VILEPLUME]   = 0,
    [SPECIES_PARAS]       = 0,
    [SPECIES_PARASECT]    = 0,
    [SPECIES_VENONAT]     = 0,
    [SPECIES_VENOMOTH]    = 2,
    [SPECIES_DIGLETT]     = 2,
    [SPECIES_DUGTRIO]     = 2,
    [SPECIES_MEOWTH]      = 1,
    [SPECIES_PERSIAN]     = 1,
    [SPECIES_PSYDUCK]     = 1,
    [SPECIES_GOLDUCK]     = 2,
    [SPECIES_MANKEY]      = 1,
    [SPECIES_PRIMEAPE]    = 2,
    [SPECIES_GROWLITHE]   = 0,
    [SPECIES_ARCANINE]    = 0,
    [SPECIES_POLIWAG]     = 0,
    [SPECIES_POLIWHIRL]   = 0,
    [SPECIES_POLIWRATH]   = 0,
    [SPECIES_ABRA]        = 2,
    [SPECIES_KADABRA]     = 2,
    [SPECIES_ALAKAZAM]    = 2,
    [SPECIES_MACHOP]      = 0,
    [SPECIES_MACHOKE]     = 2,
    [SPECIES_MACHAMP]     = 0,
    [SPECIES_BELLSPROUT]  = 1,
    [SPECIES_WEEPINBELL]  = 1,
    [SPECIES_VICTREEBEL]  = 1,
    [SPECIES_TENTACOOL]   = 2,
    [SPECIES_TENTACRUEL]  = 2,
    [SPECIES_GEODUDE]     = 1,
    [SPECIES_GRAVELER]    = 1,
    [SPECIES_GOLEM]       = 1,
    [SPECIES_PONYTA]      = 0,
    [SPECIES_RAPIDASH]    = 0,
    [SPECIES_SLOWPOKE]    = 0,
    [SPECIES_SLOWBRO]     = 0,
    [SPECIES_MAGNEMITE]   = 0,
    [SPECIES_MAGNETON]    = 0,
    [SPECIES_FARFETCHD]   = 1,
    [SPECIES_DODUO]       = 2,
    [SPECIES_DODRIO]      = 2,
    [SPECIES_SEEL]        = 2,
    [SPECIES_DEWGONG]     = 2,
    [SPECIES_GRIMER]      = 2,
    [SPECIES_MUK]         = 2,
    [SPECIES_SHELLDER]    = 2,
    [SPECIES_CLOYSTER]    = 2,
    [SPECIES_GASTLY]      = 2,
    [SPECIES_HAUNTER]     = 2,
    [SPECIES_GENGAR]      = 2,
    [SPECIES_ONIX]        = 2,
    [SPECIES_DROWZEE]     = 2,
    [SPECIES_HYPNO]       = 1,
    [SPECIES_KRABBY]      = 2,
    [SPECIES_KINGLER]     = 2,
    [SPECIES_VOLTORB]     = 0,
    [SPECIES_ELECTRODE]   = 0,
    [SPECIES_EXEGGCUTE]   = 0,
    [SPECIES_EXEGGUTOR]   = 1,
    [SPECIES_CUBONE]      = 1,
    [SPECIES_MAROWAK]     = 1,
    [SPECIES_HITMONLEE]   = 2,
    [SPECIES_HITMONCHAN]  = 2,
    [SPECIES_LICKITUNG]   = 1,
    [SPECIES_KOFFING]     = 2,
    [SPECIES_WEEZING]     = 2,
    [SPECIES_RHYHORN]     = 1,
    [SPECIES_RHYDON]      = 1,
    [SPECIES_CHANSEY]     = 0,
    [SPECIES_TANGELA]     = 0,
    [SPECIES_KANGASKHAN]  = 1,
    [SPECIES_HORSEA]      = 0,
    [SPECIES_SEADRA]      = 0,
    [SPECIES_GOLDEEN]     = 0,
    [SPECIES_SEAKING]     = 0,
    [SPECIES_STARYU]      = 2,
    [SPECIES_STARMIE]     = 2,
    [SPECIES_MR_MIME]     = 0,
    [SPECIES_SCYTHER]     = 1,
    [SPECIES_JYNX]        = 2,
    [SPECIES_ELECTABUZZ]  = 1,
    [SPECIES_MAGMAR]      = 0,
    [SPECIES_PINSIR]      = 2,
    [SPECIES_TAUROS]      = 2,
    [SPECIES_MAGIKARP]    = 0,
    [SPECIES_GYARADOS]    = 0,
    [SPECIES_LAPRAS]      = 2,
    [SPECIES_DITTO]       = 2,
    [SPECIES_EEVEE]       = 2,
    [SPECIES_VAPOREON]    = 0,
    [SPECIES_JOLTEON]     = 0,
    [SPECIES_FLAREON]     = 0,
    [SPECIES_PORYGON]     = 0,
    [SPECIES_OMANYTE]     = 0,
    [SPECIES_OMASTAR]     = 0,
    [SPECIES_KABUTO]      = 2,
    [SPECIES_KABUTOPS]    = 2,
    [SPECIES_AERODACTYL]  = 0,
    [SPECIES_SNORLAX]     = 1,
    [SPECIES_ARTICUNO]    = 0,
    [SPECIES_ZAPDOS]      = 0,
    [SPECIES_MOLTRES]     = 0,
    [SPECIES_DRATINI]     = 0,
    [SPECIES_DRAGONAIR]   = 0,
    [SPECIES_DRAGONITE]   = 2,
    [SPECIES_MEWTWO]      = 2,
    [SPECIES_MEW]         = 0,
    [SPECIES_EGG]         = 1
};

const struct SpritePalette gMonIconPaletteTable[] = {
    { gMonIconPalettes[0], POKE_ICON_BASE_PAL_TAG + 0 },
    { gMonIconPalettes[1], POKE_ICON_BASE_PAL_TAG + 1 },
    { gMonIconPalettes[2], POKE_ICON_BASE_PAL_TAG + 2 },
    // The following three point outside the gMonIconPalettes array
    // and are therefore invalid. Fortunately, they are never used.
    { gMonIconPalettes[3], POKE_ICON_BASE_PAL_TAG + 3 },
    { gMonIconPalettes[4], POKE_ICON_BASE_PAL_TAG + 4 },
    { gMonIconPalettes[5], POKE_ICON_BASE_PAL_TAG + 5 }
};

static const struct OamData sMonIconOamData = {
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x32),
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x32),
    .tileNum = 0x000,
    .priority = 1,
    .paletteNum = 0
};

static const union AnimCmd sMonIconAnim_Fast[] = {
    ANIMCMD_FRAME(0,  6),
    ANIMCMD_FRAME(1,  6),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd sMonIconAnim_MediumFast[] = {
    ANIMCMD_FRAME(0,  8),
    ANIMCMD_FRAME(1,  8),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd sMonIconAnim_MediumSlow[] = {
    ANIMCMD_FRAME(0, 14),
    ANIMCMD_FRAME(1, 14),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd sMonIconAnim_Slow[] = {
    ANIMCMD_FRAME(0, 22),
    ANIMCMD_FRAME(1, 22),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd sMonIconAnim_Still[] = {
    ANIMCMD_FRAME(0, 29),
    ANIMCMD_FRAME(0, 29),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd *const sMonIconAnims[] = {
    sMonIconAnim_Fast,
    sMonIconAnim_MediumFast,
    sMonIconAnim_MediumSlow,
    sMonIconAnim_Slow,
    sMonIconAnim_Still
};

static const union AffineAnimCmd sMonIconAffineAnim_0[] = {
    AFFINEANIMCMD_FRAME(0, 0, 0, 10),
    AFFINEANIMCMD_END
};

static const union AffineAnimCmd sMonIconAffineAnim_1[] = {
    AFFINEANIMCMD_FRAME(-2, -2, 0, 122),
    AFFINEANIMCMD_END
};

static const union AffineAnimCmd *const sMonIconAffineAnims[] = {
    sMonIconAffineAnim_0,
    sMonIconAffineAnim_1
};

static const u16 sSpriteImageSizes[][4] = {
    [ST_OAM_SQUARE] = {
        [ST_OAM_SIZE_0] = 0x020,
        [ST_OAM_SIZE_1] = 0x080,
        [ST_OAM_SIZE_2] = 0x200,
        [ST_OAM_SIZE_3] = 0x800,
    },
    [ST_OAM_H_RECTANGLE] = {
        [ST_OAM_SIZE_0] = 0x040,
        [ST_OAM_SIZE_1] = 0x080,
        [ST_OAM_SIZE_2] = 0x100,
        [ST_OAM_SIZE_3] = 0x400,
    },
    [ST_OAM_V_RECTANGLE] = {
        [ST_OAM_SIZE_0] = 0x040,
        [ST_OAM_SIZE_1] = 0x080,
        [ST_OAM_SIZE_2] = 0x100,
        [ST_OAM_SIZE_3] = 0x400,
    },
};

u8 CreateMonIcon(u16 species, SpriteCallback callback, s16 x, s16 y, u8 subpriority, u32 personality, bool32 extra)
{
    u8 spriteId;
    struct MonIconSpriteTemplate iconTemplate =
        {
            .oam = &sMonIconOamData,
            .image = GetMonIconPtr(species, personality, extra),
            .anims = sMonIconAnims,
            .affineAnims = sMonIconAffineAnims,
            .callback = callback,
            .paletteTag = POKE_ICON_BASE_PAL_TAG + gMonIconPaletteIndices[species],
        };

    if (species > NUM_SPECIES)
        iconTemplate.paletteTag = POKE_ICON_BASE_PAL_TAG;

    spriteId = CreateMonIconSprite(&iconTemplate, x, y, subpriority);

    UpdateMonIconFrame(&gSprites[spriteId]);

    return spriteId;
}

u8 CreateMonIcon_HandleDeoxys(u16 species, SpriteCallback callback, s16 x, s16 y, u8 subpriority, bool32 extra)
{
    u8 spriteId;
    struct MonIconSpriteTemplate iconTemplate =
        {
            .oam = &sMonIconOamData,
            .image = NULL,
            .anims = sMonIconAnims,
            .affineAnims = sMonIconAffineAnims,
            .callback = callback,
            .paletteTag = POKE_ICON_BASE_PAL_TAG + gMonIconPaletteIndices[species],
        };

    iconTemplate.image = GetMonIconTiles(species, extra);
    spriteId = CreateMonIconSprite(&iconTemplate, x, y, subpriority);

    UpdateMonIconFrame(&gSprites[spriteId]);

    return spriteId;
}

u16 GetIconSpecies(u16 species, u32 personality)
{
    u16 result;

    if (species > NUM_SPECIES)
            result = SPECIES_NONE;
    else
        result = species;

    return result;
}

u16 GetUnownLetterByPersonality(u32 personality)
{
    if (!personality)
        return 0;
    else
        return (((personality & 0x3000000) >> 18) | ((personality & 0x30000) >> 12) | ((personality & 0x300) >> 6) | (personality & 0x3)) % 0x1C;
}

u16 MailSpeciesToIconSpecies(u16 species)
{
    u16 value;

    if (species > (NUM_SPECIES - 1))
            species = SPECIES_NONE;
        return GetIconSpecies(species, 0);
}

const u8 *GetMonIconTiles(u16 species, bool32 extra)
{
    const u8 *iconSprite = gMonIconTable[species];
    
    return iconSprite;
}

const u8 *GetMonIconPtr(u16 species, u32 personality, bool32 extra)
{
    return GetMonIconTiles(GetIconSpecies(species, personality), extra);
}

void DestroyMonIcon(struct Sprite * sprite)
{
    DestroyMonIconInternal(sprite);
}

void LoadMonIconPalettes(void)
{
    u8 i;
    for (i = 0; i < NELEMS(gMonIconPaletteTable); i++)
        LoadSpritePalette(&gMonIconPaletteTable[i]);
}

void SafeLoadMonIconPalette(u16 species)
{
    u8 palIndex;
    if (species > NUM_SPECIES)
        species = SPECIES_NONE;
    palIndex = gMonIconPaletteIndices[species];
    if (IndexOfSpritePaletteTag(gMonIconPaletteTable[palIndex].tag) == 0xFF)
        LoadSpritePalette(&gMonIconPaletteTable[palIndex]);
}

void LoadMonIconPalette(u16 species)
{
    u8 palIndex;
    palIndex = gMonIconPaletteIndices[species];
    if (IndexOfSpritePaletteTag(gMonIconPaletteTable[palIndex].tag) == 0xFF)
        LoadSpritePalette(&gMonIconPaletteTable[palIndex]);
}

void FreeMonIconPalettes(void)
{
    u8 i;
    for (i = 0; i < 6; i++)
        FreeSpritePaletteByTag(gMonIconPaletteTable[i].tag);
}

void SafeFreeMonIconPalette(u16 species)
{
    u8 palIndex;
    if (species > NUM_SPECIES)
        species = SPECIES_NONE;
    palIndex = gMonIconPaletteIndices[species];
    FreeSpritePaletteByTag(gMonIconPaletteTable[palIndex].tag);
}

void FreeMonIconPalette(u16 species)
{
    u8 palIndex;
    palIndex = gMonIconPaletteIndices[species];
    FreeSpritePaletteByTag(gMonIconPaletteTable[palIndex].tag);
}

void SpriteCB_MonIcon(struct Sprite * sprite)
{
    UpdateMonIconFrame(sprite);
}

void LoadMonIconPalettesAt(u16 offset)
{
    int i;
    if (offset <= 0x100 - 0x60)
    {
        for (i = 0; i < (int)NELEMS(gMonIconPaletteTable); i++)
        {
            LoadPalette(gMonIconPaletteTable[i].data, offset, 0x20);
            offset += 0x10;
        }
    }
}

const u16 *GetValidMonIconPalettePtr(u16 species)
{
    if (species > NUM_SPECIES)
        species = SPECIES_NONE;
    return gMonIconPaletteTable[gMonIconPaletteIndices[species]].data;
}

u8 GetValidMonIconPalIndex(u16 species)
{
    if (species > NUM_SPECIES)
        species = SPECIES_NONE;
    return gMonIconPaletteIndices[species];
}

u8 GetMonIconPaletteIndexFromSpecies(u16 species)
{
    return gMonIconPaletteIndices[species];
}

u8 UpdateMonIconFrame(struct Sprite * sprite)
{
    u8 result = 0;

    if (sprite->animDelayCounter == 0)
    {
        s16 frame = sprite->anims[sprite->animNum][sprite->animCmdIndex].frame.imageValue;

        switch (frame)
        {
        case -1:
            break;
        case -2:
            sprite->animCmdIndex = 0;
            break;
        default:
            RequestSpriteCopy(
                // pointer arithmetic is needed to get the correct pointer to perform the sprite copy on.
                // because sprite->images is a struct def, it has to be casted to (u8 *) before any
                // arithmetic can be performed.
                (u8 *)sprite->images + (sSpriteImageSizes[sprite->oam.shape][sprite->oam.size] * frame),
                (u8 *)(OBJ_VRAM0 + sprite->oam.tileNum * TILE_SIZE_4BPP),
                sSpriteImageSizes[sprite->oam.shape][sprite->oam.size]);
            sprite->animDelayCounter = sprite->anims[sprite->animNum][sprite->animCmdIndex].frame.duration & 0xFF;
            sprite->animCmdIndex++;
            result = sprite->animCmdIndex;
            break;
        }
    }
    else
    {
        sprite->animDelayCounter--;
    }
    return result;
}

static u8 CreateMonIconSprite(const struct MonIconSpriteTemplate * iconTemplate, s16 x, s16 y, u8 subpriority)
{
    u8 spriteId;

    struct SpriteFrameImage image = { NULL, sSpriteImageSizes[iconTemplate->oam->shape][iconTemplate->oam->size] };

    struct SpriteTemplate spriteTemplate =
    {
        .tileTag = SPRITE_INVALID_TAG,
        .paletteTag = iconTemplate->paletteTag,
        .oam = iconTemplate->oam,
        .anims = iconTemplate->anims,
        .images = &image,
        .affineAnims = iconTemplate->affineAnims,
        .callback = iconTemplate->callback,
    };

    spriteId = CreateSprite(&spriteTemplate, x, y, subpriority);
    gSprites[spriteId].animPaused = TRUE;
    gSprites[spriteId].animBeginning = FALSE;
    gSprites[spriteId].images = (const struct SpriteFrameImage *)iconTemplate->image;
    return spriteId;
}

static void DestroyMonIconInternal(struct Sprite * sprite)
{
    struct SpriteFrameImage image = { NULL, sSpriteImageSizes[sprite->oam.shape][sprite->oam.size] };
    sprite->images = &image;
    DestroySprite(sprite);
}

void SetPartyHPBarSprite(struct Sprite * sprite, u8 animNum)
{
    sprite->animNum = animNum;
    sprite->animDelayCounter = 0;
    sprite->animCmdIndex = 0;
}
