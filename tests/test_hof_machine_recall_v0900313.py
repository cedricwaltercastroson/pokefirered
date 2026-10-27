"""Hall-of-Fame ceremony must recall the follower before the record effect."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
script = (root / 'data/maps/PokemonLeague_HallOfFame/scripts.inc').read_text()
specials = (root / 'data/specials.inc').read_text()
follower = (root / 'src/following_pokemon.c').read_text()
header = (root / 'include/following_pokemon.h').read_text()
room = script.split('PokemonLeague_HallOfFame_EventScript_EnterRoom::', 1)[1].split('PokemonLeague_HallOfFame_Movement_EnterRoom::')[0]
assert room.index('applymovement OBJ_EVENT_ID_PLAYER, PokemonLeague_HallOfFame_Movement_EnterRoom') < room.index('msgbox PokemonLeague_HallOfFame_Text_OakCongratulations')
assert room.index('msgbox PokemonLeague_HallOfFame_Text_OakCongratulations') < room.index('special Follower_BeginHallOfFameRecall') < room.index('dofieldeffect FLDEFF_HALL_OF_FAME_RECORD')
assert specials.index('def_special TimeOfDay_SleepUntil') < specials.index('def_special Follower_BeginHallOfFameRecall') < specials.index('gSpecialsEnd::')
assert 'u16 Follower_BeginHallOfFameRecall(void);' in header
recall = follower.split('u16 Follower_BeginHallOfFameRecall(void)',1)[1].split('// Called at Fly takeoff',1)[0]
assert 'sHallOfFameRecallActive = TRUE;' in recall
assert recall.index('Follower_Remove();') < recall.index('PlaySE(SE_BALL_TRADE);')
assert 'if (sHallOfFameRecallActive)' in follower.split('void Follower_Update(struct ObjectEvent *player)',1)[1]
assert 'sHallOfFameRecallActive = FALSE;' in follower.split('void Follower_ResetForMapLoad(void)',1)[1].split('static bool8 Follower_TryEnqueue',1)[0]
print('PASS: Hall of Fame follower recalled only before machine record; suppressed through ceremony')
