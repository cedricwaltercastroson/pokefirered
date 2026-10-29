#!/usr/bin/env python3
"""All eight rematches avoid repeating the invitation and retain battle callbacks."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
gyms={
 'PewterCity_Gym':'BROCK','CeruleanCity_Gym':'MISTY',
 'VermilionCity_Gym':'LT_SURGE','CeladonCity_Gym':'ERIKA',
 'FuchsiaCity_Gym':'KOGA','SaffronCity_Gym':'SABRINA',
 'CinnabarIsland_Gym':'BLAINE','ViridianCity_Gym':'GIOVANNI',
}
for gym,leader in gyms.items():
    source=(root/'data/maps'/gym/'scripts.inc').read_text()
    prefix=f'{gym}_Text_GymRematch'
    script=source.split(f'{gym}_EventScript_AskRematch::',1)[1].split(f'{gym}_EventScript_RematchVictory::',1)[0]
    assert re.search(r'\bmsgbox '+prefix+r'Intro, MSGBOX_YESNO',script),gym
    assert f'compare VAR_RESULT, YES' in script,gym
    assert f'goto_if_ne {gym}_EventScript_RematchDeclined' in script,gym
    assert script.count(f'cleartrainerflag TRAINER_GYM_REMATCH_{leader}')==1,gym
    assert script.count(f'trainerbattle_single TRAINER_GYM_REMATCH_{leader}, {prefix}Begin, {prefix}Defeat, {gym}_EventScript_RematchVictory')==1,gym
    assert script.count(prefix+'Intro')==1,gym
    assert source.count(f'{prefix}Begin::')==1,gym
    assert source.count(f'{prefix}Intro::')==1,gym
    assert source.count(f'{prefix}Defeat::')==1,gym
    def get_text(label):
        return re.search(re.escape(label)+r'::\s*\.string "([^"]+)"',source).group(1)
    assert get_text(prefix+'Intro')!=get_text(prefix+'Begin'),gym
    assert get_text(prefix+'Begin').endswith('$')
    callback=source.split(f'{gym}_EventScript_RematchVictory::',1)[1].split(f'{gym}_EventScript_RematchDeclined::',1)[0]
    assert f'compare VAR_GYM_REMATCH_{leader}, 3' in callback
    assert f'addvar VAR_GYM_REMATCH_{leader}, 1' in callback
    assert f'trainerbattle_single TRAINER_LEADER_{leader}' in source or leader=='GIOVANNI',gym
print('PASS: eight Gym Leaders, unique rematch invitation/battle intro, preserved battle callbacks and tier progress')
