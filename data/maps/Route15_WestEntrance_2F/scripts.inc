Route15_WestEntrance_2F_MapScripts:: @ 8170092
	.byte 0

Route15_WestEntrance_2F_EventScript_LeftBinoculars:: @ 8170093
	lockall
	msgbox Route15_WestEntrance_2F_Text_LargeShiningBird
	drawmonpic SPECIES_ARTICUNO, 10, 3
	delay 20
	waitbuttonpress
	erasemonpic
	setvar VAR_0x8004, SPECIES_ARTICUNO
	special SetSeenMon
	releaseall
	end

Route15_WestEntrance_2F_EventScript_RightBinoculars:: @ 81700B0
	msgbox Route15_WestEntrance_2F_Text_SmallIslandOnHorizon, MSGBOX_SIGN
	end
