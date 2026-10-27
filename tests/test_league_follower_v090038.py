"""Updated League regression: cutscene trail and post-text visibility."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
follower = (root / 'src/following_pokemon.c').read_text()
league = (root / 'data/scripts/pokemon_league.inc').read_text()
assert 'applymovement OBJ_EVENT_ID_PLAYER, Movement_WalkUp5' in league
assert 'call PokemonLeague_EventScript_CloseEntry' in league
assert 'static bool8 Follower_IsLeagueScriptMap(void)' in follower
for room in ('LORELEIS', 'BRUNOS', 'AGATHAS', 'LANCES', 'CHAMPIONS'):
    assert f'MAP_POKEMON_LEAGUE_{room}_ROOM' in follower
assert 'MAP_POKEMON_LEAGUE_HALL_OF_FAME' in follower
begin = follower.split('void Follower_BeginScriptedPlayerMovement(void)',1)[1].split('void Follower_EndScriptedPlayerMovement(void)',1)[0]
assert begin.index('if (Follower_IsLeagueScriptMap()') < begin.index('Follower_Remove();')
assert 'sLeagueScriptFollowing = TRUE;' in begin
assert 'sSafariWarpAwaitStep = FALSE;' in begin
end = follower.split('void Follower_EndScriptedPlayerMovement(void)',1)[1].split('// Called at Fly takeoff',1)[0]
assert 'if (sLeagueScriptFollowing)' in end
assert 'sSafariWarpAwaitStep = TRUE;' not in end
update = follower.split('void Follower_Update(struct ObjectEvent *player)',1)[1]
assert 'UnfreezeObjectEvent(&gObjectEvents[objectId]);' in update
assert 'if (Follower_IsLeagueScriptMap()' in update
assert 'battleRecallFirstSpawn = TRUE;' in update
print('PASS: League uses live cutscene trail and returns after text without a new step')
