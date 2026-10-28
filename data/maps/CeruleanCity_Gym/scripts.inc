CeruleanCity_Gym_MapScripts:: @ 816AAA0
	.byte 0

CeruleanCity_Gym_EventScript_Misty:: @ 816AAA1
	goto_if_set FLAG_SYS_GAME_CLEAR, CeruleanCity_Gym_EventScript_AskRematch
	famechecker FAMECHECKER_MISTY, FCPICKSTATE_COLORED, UpdatePickStateFromSpecialVar8005
	trainerbattle_single TRAINER_LEADER_MISTY, CeruleanCity_Gym_Text_MistyIntro, CeruleanCity_Gym_Text_MistyDefeat, CeruleanCity_Gym_EventScript_MistyDefeated, NO_MUSIC
	goto_if_unset FLAG_GOT_TM03_FROM_MISTY, CeruleanCity_Gym_EventScript_GiveTM03
	msgbox CeruleanCity_Gym_Text_ExplainTM03
	release
	end

CeruleanCity_Gym_EventScript_MistyDefeated:: @ 816AAD3
	famechecker FAMECHECKER_MISTY, 1
	setflag FLAG_DEFEATED_MISTY
	setflag FLAG_BADGE02_GET
	setrespawn SPAWN_CERULEAN_CITY
	set_gym_trainers 2
	goto CeruleanCity_Gym_EventScript_GiveTM03
	end

CeruleanCity_Gym_EventScript_GiveTM03:: @ 816AAF9
	msgbox CeruleanCity_Gym_Text_ExplainCascadeBadge
	checkitemspace ITEM_TM03, 1
	compare VAR_RESULT, FALSE
	goto_if_eq CeruleanCity_Gym_EventScript_NoRoomForTM03
	giveitem_msg CeruleanCity_Gym_Text_ReceivedTM03FromMisty, ITEM_TM03
	setflag FLAG_GOT_TM03_FROM_MISTY
	msgbox CeruleanCity_Gym_Text_ExplainTM03
	release
	end

CeruleanCity_Gym_EventScript_NoRoomForTM03:: @ 816AB3A
	msgbox CeruleanCity_Gym_Text_BetterMakeRoomForThis
	release
	end

CeruleanCity_Gym_EventScript_Diana:: @ 816AB44
	trainerbattle_single TRAINER_PICNICKER_DIANA, CeruleanCity_Gym_Text_DianaIntro, CeruleanCity_Gym_Text_DianaDefeat
	msgbox CeruleanCity_Gym_Text_DianaPostBattle, MSGBOX_AUTOCLOSE
	end

CeruleanCity_Gym_EventScript_Luis:: @ 816AB5B
	trainerbattle_single TRAINER_SWIMMER_MALE_LUIS, CeruleanCity_Gym_Text_LuisIntro, CeruleanCity_Gym_Text_LuisDefeat
	famechecker FAMECHECKER_MISTY, 2
	msgbox CeruleanCity_Gym_Text_LuisPostBattle, MSGBOX_AUTOCLOSE
	end

CeruleanCity_Gym_EventScript_GymGuy:: @ 816AB7F
	lock
	faceplayer
	goto_if_set FLAG_DEFEATED_MISTY, CeruleanCity_Gym_EventScript_GymGuyPostVictory
	msgbox CeruleanCity_Gym_Text_GymGuyAdvice
	release
	end

CeruleanCity_Gym_EventScript_GymGuyPostVictory:: @ 816AB94
	msgbox CeruleanCity_Gym_Text_WeMakePrettyGoodTeam
	release
	end

CeruleanCity_Gym_EventScript_GymStatue:: @ 816AB9E
	lockall
	goto_if_set FLAG_BADGE02_GET, CeruleanCity_Gym_EventScript_GymStatuePostVictory
	msgbox CeruleanCity_Gym_Text_GymStatue
	releaseall
	end

CeruleanCity_Gym_EventScript_GymStatuePostVictory:: @ 816ABB2
	msgbox CeruleanCity_Gym_Text_GymStatuePlayerWon
	releaseall
	end


CeruleanCity_Gym_EventScript_AskRematch::
    lock
    faceplayer
    msgbox CeruleanCity_Gym_Text_GymRematchIntro, MSGBOX_YESNO
    compare VAR_RESULT, YES
    goto_if_ne CeruleanCity_Gym_EventScript_RematchDeclined
    cleartrainerflag TRAINER_GYM_REMATCH_MISTY
    trainerbattle_single TRAINER_GYM_REMATCH_MISTY, CeruleanCity_Gym_Text_GymRematchIntro, CeruleanCity_Gym_Text_GymRematchDefeat, CeruleanCity_Gym_EventScript_RematchVictory
    release
    end

CeruleanCity_Gym_EventScript_RematchVictory::
    compare VAR_GYM_REMATCH_MISTY, 3
    goto_if_ge CeruleanCity_Gym_EventScript_RematchDeclined
    addvar VAR_GYM_REMATCH_MISTY, 1
CeruleanCity_Gym_EventScript_RematchDeclined::
    release
    end

CeruleanCity_Gym_Text_GymRematchIntro::
    .string "Champion! Care for another battle?$"
CeruleanCity_Gym_Text_GymRematchDefeat::
    .string "Your strength is impressive!$"
