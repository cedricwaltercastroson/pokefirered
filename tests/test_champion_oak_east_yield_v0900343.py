"""v0.9.0.3.43: Oak's original left turn must never meet a west-yielded follower."""
from pathlib import Path
import hashlib
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
follower = (ROOT / 'src/following_pokemon.c').read_text()
script = (ROOT / 'data/maps/PokemonLeague_ChampionsRoom/scripts.inc').read_bytes()
assert hashlib.sha256(script).hexdigest() == '08e6edcd5d27330733aed9de4d018bedf83264249529507744b7980db3992b5c', 'Oak route altered'
assert b'walk_up\n\twalk_up\n\twalk_up\n\twalk_up\n\twalk_up\n\twalk_left' in script
assert 'oakEnteringChampionAisle' in follower

# Use the same real extracted C helper that v42 regression coverage exercises.
namespace = {"__file__": str(ROOT / "tests/test_follower_only_choreography_v0900342.py")}
fixture = (ROOT / 'tests/test_follower_only_choreography_v0900342.py').read_text()
exec(compile(fixture.split('with tempfile.TemporaryDirectory() as t:')[0], '<v42 harness>', 'exec'), namespace)
header = namespace['header_c'].replace('#define MOVEMENT_ACTION_WALK_NORMAL_DOWN 34', '#define MOVEMENT_ACTION_WALK_NORMAL_DOWN 34\n#define MOVEMENT_ACTION_WALK_NORMAL_UP 35\n#define MOVEMENT_ACTION_WALK_NORMAL_LEFT 36').replace('static int blockedWest=0;', 'static int blockedWest=0, blockedEast=0;').replace(
    'if(blockedWest && x==9 && y==12)return 0;',
    'if(blockedWest && x==9 && y==12)return 0;\n if(blockedEast && x==14 && y==17)return 0;')
impl = namespace['impl']
body = r'''
static void reset(void)
{
 int i;
 for(i=0;i<OBJECT_EVENTS_COUNT;i++) {
  gObjectEvents[i].active=0;
  gObjectEvents[i].invisible=0;
  gObjectEvents[i].localId=0;
  gObjectEvents[i].isPlayer=0;
 }
 sFollowerYieldHoldTrail=0;
 follower_action_count=0;
 npc_action_count=0;
 blockedWest=0;
 blockedEast=0;
 saved.location.mapGroup=MAP_GROUP(POKEMON_LEAGUE_CHAMPIONS_ROOM);
 saved.location.mapNum=MAP_NUM(POKEMON_LEAGUE_CHAMPIONS_ROOM);
 gObjectEvents[0].active=1;
 gObjectEvents[0].isPlayer=1;
 gObjectEvents[0].currentCoords.x=13;
 gObjectEvents[0].currentCoords.y=16;
 gObjectEvents[1].active=1;
 gObjectEvents[1].localId=2; /* Oak's original local ID */
 gObjectEvents[1].currentCoords.x=13;
 gObjectEvents[1].currentCoords.y=18; /* (6,11) + map border */
 gObjectEvents[2].active=1;
 gObjectEvents[2].spriteId=3;
 gObjectEvents[2].currentCoords.x=13;
 gObjectEvents[2].currentCoords.y=17; /* (6,10) + border */
}
int main(void)
{
 /* Oak's final north step is into the follower's tile; yield east. */
 reset();
 Follower_ObserveNpcScriptStep(1,MOVEMENT_ACTION_WALK_NORMAL_UP);
 assert(follower_action_count==1 && npc_action_count==0);
 assert(last_action==100+DIR_EAST);
 assert(gObjectEvents[2].currentCoords.x==14 && gObjectEvents[2].currentCoords.y==17);
 assert(gObjectEvents[1].currentCoords.x==13 && gObjectEvents[1].currentCoords.y==18);
 /* Oak continues up, west, up, up. The follower remains out of the route. */
 MoveCoords(DIR_NORTH,&gObjectEvents[1].currentCoords.x,&gObjectEvents[1].currentCoords.y);
 assert(gObjectEvents[1].currentCoords.x!=gObjectEvents[2].currentCoords.x);
 Follower_ObserveNpcScriptStep(1,MOVEMENT_ACTION_WALK_NORMAL_LEFT);
 MoveCoords(DIR_WEST,&gObjectEvents[1].currentCoords.x,&gObjectEvents[1].currentCoords.y);
 assert(gObjectEvents[1].currentCoords.x==12 && gObjectEvents[1].currentCoords.y==17);
 assert(gObjectEvents[1].currentCoords.x!=gObjectEvents[2].currentCoords.x);
 assert(follower_action_count==1 && npc_action_count==0);
 /* This exception must not change the default west side for another NPC. */
 reset();gObjectEvents[1].localId=3;
 Follower_ObserveNpcScriptStep(1,MOVEMENT_ACTION_WALK_NORMAL_UP);
 assert(follower_action_count==1 && last_action==100+DIR_WEST);
 /* Nor change the normal rule for Oak on any other map. */
 reset();saved.location.mapNum=0;
 Follower_ObserveNpcScriptStep(1,MOVEMENT_ACTION_WALK_NORMAL_UP);
 assert(follower_action_count==1 && last_action==100+DIR_WEST);
 /* If east is blocked, never yield onto Oak's future west-turn tile. */
 reset();blockedEast=1;
 Follower_ObserveNpcScriptStep(1,MOVEMENT_ACTION_WALK_NORMAL_UP);
 assert(follower_action_count==0 && npc_action_count==0);
 assert(gObjectEvents[1].currentCoords.x==13 && gObjectEvents[1].currentCoords.y==18);
 assert(!sFollowerYieldHoldTrail);
 return 0;
}
'''
with tempfile.TemporaryDirectory() as tmp:
    c_file = Path(tmp) / 'oak_east.c'
    exe = Path(tmp) / 'oak_east'
    c_file.write_text(header + impl + body)
    subprocess.run(['gcc','-std=gnu89','-Wall','-Wextra','-Werror',str(c_file),'-o',str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('PASS: original Oak route, one-tile east, unrelated NPC/map unchanged, safe blocked fallback')
