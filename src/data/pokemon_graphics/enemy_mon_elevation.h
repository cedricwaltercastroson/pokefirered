// This determines how much higher above the usual position the enemy Pokémon
// is during battle. Species that float or fly have nonzero values.
const u8 gEnemyMonElevation[NUM_SPECIES] =
{
    [SPECIES_BUTTERFREE] = 8,
    [SPECIES_BEEDRILL] = 8,
    [SPECIES_PIDGEOT] = 4,
    [SPECIES_FEAROW] = 6,
    [SPECIES_ZUBAT] = 8,
    [SPECIES_GOLBAT] = 8,
    [SPECIES_VENOMOTH] = 8,
    [SPECIES_GEODUDE] = 16,
    [SPECIES_MAGNEMITE] = 16,
    [SPECIES_MAGNETON] = 8,
    [SPECIES_GASTLY] = 4,
    [SPECIES_HAUNTER] = 4,
    [SPECIES_VOLTORB] = 10,
    [SPECIES_ELECTRODE] = 12,
    [SPECIES_KOFFING] = 8,
    [SPECIES_WEEZING] = 6,
    [SPECIES_AERODACTYL] = 7,
    [SPECIES_ARTICUNO] = 6,
    [SPECIES_ZAPDOS] = 8,
    [SPECIES_MOLTRES] = 5,
    [SPECIES_DRAGONITE] = 6,
    [SPECIES_MEW] = 8,
};
