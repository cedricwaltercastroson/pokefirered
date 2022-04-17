VictoryRoad_3F_MapScripts:: @ 81610AA
	map_script MAP_SCRIPT_ON_LOAD, VictoryRoad_3F_OnLoad
	map_script MAP_SCRIPT_ON_RESUME, VictoryRoad_3F_OnResume
	.byte 0

VictoryRoad_3F_OnResume:: @ 8163B07
	call_if_set FLAG_SYS_SPECIAL_WILD_BATTLE, VictoryRoad_3F_EventScript_TryRemoveMoltres
	end

VictoryRoad_3F_EventScript_TryRemoveMoltres:: @ 8163B11
	specialvar VAR_RESULT, GetBattleOutcome
	compare VAR_RESULT, B_OUTCOME_CAUGHT
	goto_if_ne EventScript_Return
	removeobject VAR_LAST_TALKED
	return

VictoryRoad_3F_OnLoad:: @ 81610B0
	call_if_unset FLAG_FOUGHT_MOLTRES, VictoryRoad_3F_EventScript_ShowMoltres
	compare VAR_MAP_SCENE_VICTORY_ROAD_3F, 100
	call_if_ne VictoryRoad_3F_EventScript_SetRockBarrier
	end

VictoryRoad_3F_EventScript_ShowMoltres:: @ 8163B2F
	clearflag FLAG_HIDE_MOLTRES
	return

VictoryRoad_3F_EventScript_Moltres:: @ 8163B33
	goto_if_questlog EventScript_ReleaseEnd
	special QuestLog_CutRecording
	lock
	faceplayer
	setwildbattle SPECIES_MOLTRES, 50, ITEM_NONE
	waitse
	playmoncry SPECIES_MOLTRES, 2
	message Text_Gyaoo
	waitmessage
	waitmoncry
	delay 10
	playbgm MUS_ENCOUNTER_GYM_LEADER, 0
	waitbuttonpress
	setflag FLAG_SYS_SPECIAL_WILD_BATTLE
	special StartLegendaryBattle
	waitstate
	clearflag FLAG_SYS_SPECIAL_WILD_BATTLE
	specialvar VAR_RESULT, GetBattleOutcome
	compare VAR_RESULT, B_OUTCOME_WON
	goto_if_eq VictoryRoad_3F_EventScript_DefeatedMoltres
	compare VAR_RESULT, B_OUTCOME_RAN
	goto_if_eq VictoryRoad_3F_EventScript_RanFromMoltres
	compare VAR_RESULT, B_OUTCOME_PLAYER_TELEPORTED
	goto_if_eq VictoryRoad_3F_EventScript_RanFromMoltres
	setflag FLAG_FOUGHT_MOLTRES
	release
	end

VictoryRoad_3F_EventScript_DefeatedMoltres:: @ 8163B96
	setflag FLAG_FOUGHT_MOLTRES
	goto EventScript_RemoveStaticMon
	end

VictoryRoad_3F_EventScript_RanFromMoltres:: @ 8163B9F
	setvar VAR_0x8004, SPECIES_MOLTRES
	goto EventScript_MonFlewAway
	end

VictoryRoad_3F_EventScript_SetRockBarrier:: @ 81610BC
	setmetatile 12, 12, METATILE_Cave_RockBarrier_Top, 1
	setmetatile 12, 13, METATILE_Cave_RockBarrier_Bottom, 1
	return

VictoryRoad_3F_EventScript_FloorSwitch:: @ 81610CF
	lockall
	compare VAR_MAP_SCENE_VICTORY_ROAD_3F, 100
	goto_if_eq VictoryRoad_3F_EventScript_FloorSwitchAlreadyPressed
	setmetatile 12, 12, METATILE_Cave_Floor_Ledge_Top, 0
	setmetatile 12, 13, METATILE_Cave_Floor_Ledge_Bottom, 0
	playse SE_ICE_BREAK
	special DrawWholeMapView
	waitse
	moveobjectoffscreen 7
	moveobjectoffscreen 10
	setvar VAR_MAP_SCENE_VICTORY_ROAD_3F, 100
	releaseall
	end

VictoryRoad_3F_EventScript_FloorSwitchAlreadyPressed:: @ 8161101
	releaseall
	end

VictoryRoad_3F_EventScript_George:: @ 8161103
	trainerbattle_single TRAINER_COOLTRAINER_GEORGE, VictoryRoad_3F_Text_GeorgeIntro, VictoryRoad_3F_Text_GeorgeDefeat
	msgbox VictoryRoad_3F_Text_GeorgePostBattle, MSGBOX_AUTOCLOSE
	end

VictoryRoad_3F_EventScript_Colby:: @ 816111A
	trainerbattle_single TRAINER_COOLTRAINER_COLBY, VictoryRoad_3F_Text_ColbyIntro, VictoryRoad_3F_Text_ColbyDefeat
	msgbox VictoryRoad_3F_Text_ColbyPostBattle, MSGBOX_AUTOCLOSE
	end

VictoryRoad_3F_EventScript_Caroline:: @ 8161131
	trainerbattle_single TRAINER_COOLTRAINER_CAROLINE, VictoryRoad_3F_Text_CarolineIntro, VictoryRoad_3F_Text_CarolineDefeat
	msgbox VictoryRoad_3F_Text_CarolinePostBattle, MSGBOX_AUTOCLOSE
	end

VictoryRoad_3F_EventScript_Alexa:: @ 8161148
	trainerbattle_single TRAINER_COOLTRAINER_ALEXA, VictoryRoad_3F_Text_AlexaIntro, VictoryRoad_3F_Text_AlexaDefeat
	msgbox VictoryRoad_3F_Text_AlexaPostBattle, MSGBOX_AUTOCLOSE
	end

VictoryRoad_3F_EventScript_Ray:: @ 816115F
	trainerbattle_double TRAINER_COOL_COUPLE_RAY_TYRA, VictoryRoad_3F_Text_RayIntro, VictoryRoad_3F_Text_RayDefeat, VictoryRoad_3F_Text_RayNotEnoughMons
	msgbox VictoryRoad_3F_Text_RayPostBattle, MSGBOX_AUTOCLOSE
	end

VictoryRoad_3F_EventScript_Tyra:: @ 816117A
	trainerbattle_double TRAINER_COOL_COUPLE_RAY_TYRA, VictoryRoad_3F_Text_TyraIntro, VictoryRoad_3F_Text_TyraDefeat, VictoryRoad_3F_Text_TyraNotEnoughMons
	msgbox VictoryRoad_3F_Text_TyraPostBattle, MSGBOX_AUTOCLOSE
	end
