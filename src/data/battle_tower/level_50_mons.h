const struct BattleTowerPokemonTemplate gBattleTowerLevel50Mons[] =
{
	{
		.species = SPECIES_PIKACHU,
		.heldItem = BATTLE_TOWER_ITEM_ORAN_BERRY,
		.teamFlags = 0x42,
		.moves = {
			MOVE_QUICK_ATTACK,
			MOVE_THUNDER_WAVE,
			MOVE_THUNDER_SHOCK,
			MOVE_GROWL
		},
		.evSpread = F_EV_SPREAD_SPEED | F_EV_SPREAD_DEFENSE,
		.nature = NATURE_HARDY,
	},
};
