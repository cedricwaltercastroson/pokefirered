PewterCity_Gym_MapScripts:: @ 816A592
	.byte 0

PewterCity_Gym_EventScript_Brock:: @ 816A593
	goto_if_set FLAG_SYS_GAME_CLEAR, PewterCity_Gym_EventScript_AskRematch
	famechecker FAMECHECKER_BROCK, FCPICKSTATE_COLORED, UpdatePickStateFromSpecialVar8005
	trainerbattle_single TRAINER_LEADER_BROCK, PewterCity_Gym_Text_BrockIntro, PewterCity_Gym_Text_BrockDefeat, PewterCity_Gym_EventScript_DefeatedBrock, NO_MUSIC
	goto_if_unset FLAG_GOT_TM39_FROM_BROCK, PewterCity_Gym_EventScript_GiveTM39
	msgbox PewterCity_Gym_Text_BrockPostBattle
	release
	end

PewterCity_Gym_EventScript_DefeatedBrock:: @ 816A5C5
	famechecker FAMECHECKER_BROCK, 1
	setflag FLAG_DEFEATED_BROCK
	setflag FLAG_BADGE01_GET
	setvar VAR_MAP_SCENE_PEWTER_CITY, 1
	setflag FLAG_HIDE_PEWTER_CITY_GYM_GUIDE
	clearflag FLAG_HIDE_PEWTER_CITY_RUNNING_SHOES_GUY
	set_gym_trainers 1
	goto PewterCity_Gym_EventScript_GiveTM39
	end

PewterCity_Gym_EventScript_GiveTM39:: @ 816A5F3
	msgbox PewterCity_Gym_Text_TakeThisWithYou
	checkitemspace ITEM_TM39, 1
	compare VAR_RESULT, FALSE
	goto_if_eq PewterCity_Gym_EventScript_NoRoomForTM39
	giveitem_msg PewterCity_Gym_Text_ReceivedTM39FromBrock, ITEM_TM39
	setflag FLAG_GOT_TM39_FROM_BROCK
	msgbox PewterCity_Gym_Text_ExplainTM39
	release
	end

PewterCity_Gym_EventScript_NoRoomForTM39:: @ 816A634
	msgbox PewterCity_Gym_Text_DontHaveRoomForThis
	release
	end

PewterCity_Gym_EventScript_Liam:: @ 816A63E
	trainerbattle_single TRAINER_CAMPER_LIAM, PewterCity_Gym_Text_LiamIntro, PewterCity_Gym_Text_LiamDefeat
	msgbox PewterCity_Gym_Text_LiamPostBattle, MSGBOX_AUTOCLOSE
	end

PewterCity_Gym_EventScript_GymGuy:: @ 816A655
	lock
	faceplayer
	goto_if_set FLAG_DEFEATED_BROCK, PewterCity_Gym_EventScript_GymGuyPostVictory
	msgbox PewterCity_Gym_Text_LetMeTakeYouToTheTop, MSGBOX_YESNO
	compare VAR_RESULT, YES
	goto_if_eq PewterCity_Gym_EventScript_GymGuyTakeMeToTop
	compare VAR_RESULT, NO
	goto_if_eq PewterCity_Gym_EventScript_GymGuyDontTakeMeToTop
	end

PewterCity_Gym_EventScript_GymGuyPostVictory:: @ 816A67F
	msgbox PewterCity_Gym_Text_YoureChampMaterial
	release
	end

PewterCity_Gym_EventScript_GymGuyTakeMeToTop:: @ 816A689
	msgbox PewterCity_Gym_Text_LetsGetHappening
	goto PewterCity_Gym_EventScript_GymGuyAdvice
	end

PewterCity_Gym_EventScript_GymGuyDontTakeMeToTop:: @ 816A697
	msgbox PewterCity_Gym_Text_ItsFreeLetsGetHappening
	goto PewterCity_Gym_EventScript_GymGuyAdvice
	end

PewterCity_Gym_EventScript_GymGuyAdvice:: @ 816A6A5
	msgbox PewterCity_Gym_Text_TryDifferentPartyOrders
	release
	end

PewterCity_Gym_EventScript_GymStatue:: @ 816A6AF
	lockall
	goto_if_set FLAG_BADGE01_GET, PewterCity_Gym_EventScript_GymStatuePostVictory
	msgbox PewterCity_Gym_Text_GymStatue
	releaseall
	end

PewterCity_Gym_EventScript_GymStatuePostVictory:: @ 816A6C3
	msgbox PewterCity_Gym_Text_GymStatuePlayerWon
	releaseall
	end


PewterCity_Gym_EventScript_AskRematch::
    lock
    faceplayer
    msgbox PewterCity_Gym_Text_GymRematchIntro, MSGBOX_YESNO
    compare VAR_RESULT, YES
    goto_if_ne PewterCity_Gym_EventScript_RematchDeclined
    cleartrainerflag TRAINER_GYM_REMATCH_BROCK
    trainerbattle_single TRAINER_GYM_REMATCH_BROCK, PewterCity_Gym_Text_GymRematchBegin, PewterCity_Gym_Text_GymRematchDefeat, PewterCity_Gym_EventScript_RematchVictory
    release
    end

PewterCity_Gym_EventScript_RematchVictory::
    compare VAR_GYM_REMATCH_BROCK, 3
    goto_if_ge PewterCity_Gym_EventScript_RematchDeclined
    addvar VAR_GYM_REMATCH_BROCK, 1
PewterCity_Gym_EventScript_RematchDeclined::
    release
    end

PewterCity_Gym_Text_GymRematchIntro::
    .string "Champion! Care for another battle?$"
PewterCity_Gym_Text_GymRematchBegin::
    .string "Ready for a challenge!$"
PewterCity_Gym_Text_GymRematchDefeat::
    .string "Your strength is impressive!$"
