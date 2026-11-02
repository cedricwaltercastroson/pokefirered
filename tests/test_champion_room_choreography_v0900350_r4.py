#!/usr/bin/env python3
"""Source-level Champion Room contracts; gameplay behaviour still needs QA."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
src = (root / 'src/following_pokemon.c').read_text()
scr = (root / 'data/maps/PokemonLeague_ChampionsRoom/scripts.inc').read_text()
hdr = (root / 'include/following_pokemon.h').read_text()
spc = (root / 'data/specials.inc').read_text()
assert src.count('u16 Follower_ChampionRoomRestore(void)') == 1
assert 'u16 Follower_ChampionRoomRestore(void);' in hdr
assert spc.count('def_special Follower_ChampionRoomRestore') == 1
scene = scr.split('PokemonLeague_ChampionsRoom_EventScript_EnterRoom::', 1)[1].split('PokemonLeague_ChampionsRoom_EventScript_QuestLogEnd::', 1)[0]
assert scene.index('PokemonLeague_ChampionsRoom_Text_PostBattle') < scene.index('special Follower_ChampionRoomRestore') < scene.index('addobject LOCALID_PROF_OAK')
assert scene.index('special Follower_ChampionRoomRestore') < scene.index('PokemonLeague_ChampionsRoom_Movement_PlayerExit')
assert scene.count('special Follower_ChampionRoomRestore') == 1
moves = scr.split('PokemonLeague_ChampionsRoom_Movement_OakEnter::',1)[1].split('PokemonLeague_ChampionsRoom_Movement_OakExit::',1)[0]
steps = [l.strip() for l in moves.splitlines() if l.strip() in ('walk_up','walk_down','walk_left','walk_right')]
assert steps == ['walk_up'] * 4 + ['walk_left'] + ['walk_up'] * 3, steps
x,y = 6,15
path = []
for m in steps:
    x += (m == 'walk_right') - (m == 'walk_left')
    y += (m == 'walk_down') - (m == 'walk_up')
    path.append((x,y))
assert (x,y) == (5,8) and (6,10) not in path
assert '(6,10)' in moves
part = src.split('static bool8 Follower_YieldForNpc(',1)[1].split('void Follower_ObserveTrainerAlert(',1)[0]
assert 'gObjectEvents[npcId].localId == 2' in part
assert 'return FALSE;' in part.split('gObjectEvents[npcId].localId == 2',1)[1].split('if (direction <',1)[0]
special = src.split('u16 Follower_ChampionRoomRestore(void)',1)[1].split('u16 Follower_BeginHallOfFameRecall',1)[0]
for token in ('sLeagueScriptFollowing = TRUE;', 'sBattleRecallAwaitStep = FALSE;', 'sPreDisplaySpawn = TRUE;', 'Follower_Update(player);', 'sPreDisplaySpawn = FALSE;', 'Follower_IsSafeSpawnTile(x, y)'):
    assert token in special, token
assert 'Follower_FindAdjacentTile' not in special
assert 'Follower_TakeBattleEntrySlide' not in special
print('PASS: post-battle follower stays on rear tile; Oak reroutes around it; scripted following preserved')
