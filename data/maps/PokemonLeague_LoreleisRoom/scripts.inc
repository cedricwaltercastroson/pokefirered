PokemonLeague_LoreleisRoom_MapScripts:: @ 816256C
	map_script MAP_SCRIPT_ON_RESUME, PokemonLeague_LoreleisRoom_OnResume
	map_script MAP_SCRIPT_ON_LOAD, PokemonLeague_LoreleisRoom_OnLoad
	map_script MAP_SCRIPT_ON_TRANSITION, PokemonLeague_LoreleisRoom_OnTransition
	map_script MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE, PokemonLeague_LoreleisRoom_OnWarp
	map_script MAP_SCRIPT_ON_FRAME_TABLE, PokemonLeague_LoreleisRoom_OnFrame
	.byte 0

PokemonLeague_LoreleisRoom_OnResume:: @ 8162586
	setvar VAR_0x8004, 0
	call PokemonLeague_EventScript_DoLightingEffect
	end

PokemonLeague_LoreleisRoom_OnLoad:: @ 8162591
	clearflag FLAG_DEFEATED_LORELEI
	end

PokemonLeague_LoreleisRoom_EventScript_CloseEntry:: @ 81625A6
	call PokemonLeague_EventScript_CloseEntry
	return

PokemonLeague_LoreleisRoom_EventScript_SetDoorOpen:: @ 81625AC
	call PokemonLeague_EventScript_SetDoorOpen
	return

PokemonLeague_LoreleisRoom_OnTransition:: @ 81625B2
	setworldmapflag FLAG_WORLD_MAP_POKEMON_LEAGUE_LORELEIS_ROOM
	end

PokemonLeague_LoreleisRoom_OnWarp:: @ 81625B6
	map_script_2 VAR_TEMP_1, 0, PokemonLeague_LoreleisRoom_EventScript_TurnPlayerNorth
	.2byte 0

PokemonLeague_LoreleisRoom_EventScript_TurnPlayerNorth:: @ 81625C0
	turnobject OBJ_EVENT_ID_PLAYER, DIR_NORTH
	end

PokemonLeague_LoreleisRoom_OnFrame:: @ 81625C5
	map_script_2 VAR_MAP_SCENE_POKEMON_LEAGUE, 0, PokemonLeague_LoreleisRoom_EventScript_EnterRoom
	.2byte 0

PokemonLeague_LoreleisRoom_EventScript_EnterRoom:: @ 81625CF
	lockall
	call PokemonLeague_EventScript_EnterRoom
	setvar VAR_MAP_SCENE_POKEMON_LEAGUE, 1
	releaseall
	end

PokemonLeague_LoreleisRoom_EventScript_Lorelei:: @ 81625DC
	lock
	faceplayer
	famechecker FAMECHECKER_LORELEI, FCPICKSTATE_COLORED, UpdatePickStateFromSpecialVar8005
	goto_if_set FLAG_DEFEATED_LORELEI, PokemonLeague_LoreleisRoom_EventScript_PostBattle
	famechecker FAMECHECKER_LORELEI, 0
	famechecker FAMECHECKER_LORELEI, 1
	call_if_unset FLAG_DEFEATED_LORELEI, PokemonLeague_LoreleisRoom_EventScript_Intro
	setflag FLAG_TEMP_3
	setflag FLAG_TEMP_5
	call_if_unset FLAG_DEFEATED_LORELEI, PokemonLeague_LoreleisRoom_EventScript_Battle
	clearflag FLAG_TEMP_5
	goto PokemonLeague_LoreleisRoom_EventScript_DefeatedLorelei
	end

PokemonLeague_LoreleisRoom_EventScript_Intro:: @ 8162641
	msgbox PokemonLeague_LoreleisRoom_Text_Intro
	return

PokemonLeague_LoreleisRoom_EventScript_RematchIntro:: @ 816264A
	msgbox PokemonLeague_LoreleisRoom_Text_RematchIntro
	return

PokemonLeague_LoreleisRoom_EventScript_Battle:: @ 8162653
	trainerbattle_no_intro TRAINER_ELITE_FOUR_LORELEI, PokemonLeague_LoreleisRoom_Text_Defeat
	return

PokemonLeague_LoreleisRoom_EventScript_Rematch:: @ 816265E
	trainerbattle_no_intro TRAINER_ELITE_FOUR_LORELEI_2, PokemonLeague_LoreleisRoom_Text_Defeat
	return

PokemonLeague_LoreleisRoom_EventScript_PostBattle:: @ 8162669
	msgbox PokemonLeague_LoreleisRoom_Text_PostBattle
	release
	end

PokemonLeague_LoreleisRoom_EventScript_DefeatedLorelei:: @ 8162673
	setflag FLAG_DEFEATED_LORELEI
	call PokemonLeague_EventScript_OpenDoor
	msgbox PokemonLeague_LoreleisRoom_Text_PostBattle
	release
	end
