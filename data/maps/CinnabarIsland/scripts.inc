CinnabarIsland_MapScripts:: @ 8166ED6
	map_script MAP_SCRIPT_ON_TRANSITION, CinnabarIsland_OnTransition
	.byte 0

CinnabarIsland_OnTransition:: @ 8166EE1
	setworldmapflag FLAG_WORLD_MAP_CINNABAR_ISLAND
	call CinnabarIsland_EventScript_CheckUnlockGym
	end

CinnabarIsland_EventScript_CheckUnlockGym:: @ 8166F5E
	goto_if_set FLAG_HIDE_POKEMON_MANSION_B1F_SECRET_KEY, CinnabarIsland_EventScript_UnlockGym
	return

CinnabarIsland_EventScript_UnlockGym:: @ 8166F68
	setvar VAR_TEMP_1, 1
	return

@ Triggered when VAR_TEMP_1 is 0
CinnabarIsland_EventScript_GymDoorLocked:: @ 81671CA
	lockall
	applymovement OBJ_EVENT_ID_PLAYER, Movement_WalkInPlaceFastestUp
	waitmovement 0
	delay 20
	msgbox CinnabarIsland_Text_DoorIsLocked
	closemessage
	applymovement OBJ_EVENT_ID_PLAYER, CinnabarIsland_Movement_ForcePlayerFromDoor
	waitmovement 0
	releaseall
	end

CinnabarIsland_Movement_ForcePlayerFromDoor:: @ 81671ED
	walk_down
	step_end

CinnabarIsland_EventScript_Woman:: @ 81671EF
	lock
	faceplayer
	famechecker FAMECHECKER_BLAINE, 3
	msgbox CinnabarIsland_Text_BlaineLivedHereSinceBeforeLab
	release
	end

CinnabarIsland_EventScript_OldMan:: @ 8167208
	msgbox CinnabarIsland_Text_ScientistsExperimentInMansion, MSGBOX_NPC
	end

CinnabarIsland_EventScript_IslandSign:: @ 8167211
	msgbox CinnabarIsland_Text_IslandSign, MSGBOX_SIGN
	end

CinnabarIsland_EventScript_PokemonLabSign:: @ 816721A
	msgbox CinnabarIsland_Text_PokemonLab, MSGBOX_SIGN
	end

CinnabarIsland_EventScript_GymSign:: @ 8167223
	lockall
	famechecker FAMECHECKER_BLAINE, 0
	msgbox CinnabarIsland_Text_GymSign
	releaseall
	end
