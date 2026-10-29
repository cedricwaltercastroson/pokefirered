"""v0.9.0.3.42: follower-only choreography, zero NPC gating.

Run: python3 tests/test_follower_only_choreography_v0900342.py
"""
from pathlib import Path
import re, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]
base_script=(ROOT/'src/script_movement.c').read_text()
trainer=(ROOT/'src/trainer_see.c').read_text()
follower=(ROOT/'src/following_pokemon.c').read_text()
header=(ROOT/'include/following_pokemon.h').read_text()
assert 'Follower_DeferNpcScriptStep' not in base_script
assert 'Follower_WaitTrainerAlertYield' not in trainer
assert 'Follower_ObserveTrainerAlert' in header
assert 'Follower_ObserveNpcScriptStep' in header
assert len(re.findall(r'Follower_ObserveTrainerAlert\(', trainer))==2
# The original v30/v31 NPC/Trainer control flow must remain byte-identical
# after removing observer notifications. Hashes are the locked source values,
# so this test remains portable in the user's own Cygwin repository.
import hashlib
vanilla_trainer=trainer.replace('#include "following_pokemon.h"\n','').replace('        Follower_ObserveTrainerAlert((u8)(trainerObj - gObjectEvents));\n','')
vanilla_script=base_script.replace('        if (!gObjectEvents[objEventId].isPlayer)\n            Follower_ObserveNpcScriptStep(objEventId, nextMoveActionId);\n','')
assert hashlib.sha256(vanilla_trainer.encode()).hexdigest()=='c191a529f260683456e055b69b7d6080a1369a37de0e733fa0528a804c9eb55e', 'Trainer source changed beyond notification'
assert hashlib.sha256(vanilla_script.encode()).hexdigest()=='0c4243bf6e5b4569ef0ca203bb337f1e57259405ed1d25b25415acc703d236a3', 'NPC movement control changed'
assert hashlib.sha256((ROOT/'data/maps/PokemonLeague_ChampionsRoom/scripts.inc').read_bytes()).hexdigest()=='08e6edcd5d27330733aed9de4d018bedf83264249529507744b7980db3992b5c', 'Original Oak route not restored'
# Actual helper is compiled and exercised against fake object-events.  The helper
# must not assign to or call movement APIs on NPC / Trainer object pointers.
start=follower.index('// Observer-only notification for NPC movement.')
end=follower.index('bool8 Follower_IsCollisionExempt(',start)
impl=follower[start:end]
assert 'ObjectEventSetHeldMovement(follower,' in impl
assert 'ObjectEventSetHeldMovement(npc,' not in impl
assert 'FreezeObjectEvent(npc)' not in impl
assert 'return TRUE' in impl
header_c=r'''
#include <assert.h>
#include <stdint.h>
typedef uint8_t u8; typedef int16_t s16; typedef uint8_t bool8;
#define TRUE 1
#define FALSE 0
#define OBJECT_EVENTS_COUNT 16
#define MAX_SPRITES 128
#define FOLLOWER_LOCAL_ID 254
#define MAP_GROUP(map) (MAP_##map >> 8)
#define MAP_NUM(map) (MAP_##map & 0xFF)
#define MAP_POKEMON_LEAGUE_CHAMPIONS_ROOM (79 | (1 << 8))
#define DIR_NONE 0
#define DIR_SOUTH 1
#define DIR_NORTH 2
#define DIR_WEST 3
#define DIR_EAST 4
#define MOVEMENT_ACTION_WALK_SLOW_DOWN 30
#define MOVEMENT_ACTION_WALK_SLOW_RIGHT 33
#define MOVEMENT_ACTION_WALK_NORMAL_DOWN 34
#define MOVEMENT_ACTION_WALK_NORMAL_RIGHT 37
#define MOVEMENT_ACTION_WALK_FAST_DOWN 38
#define MOVEMENT_ACTION_WALK_FAST_RIGHT 41
#define MOVEMENT_ACTION_WALK_FASTEST_DOWN 42
#define MOVEMENT_ACTION_WALK_FASTEST_RIGHT 45
#define FOLLOWER_HEAL_IDLE 0
struct ObjectEvent { int active,invisible,isPlayer,spriteId,facingDirection,localId; struct {s16 x,y;} currentCoords; };
struct { u8 objectEventId; } gPlayerAvatar = {0};
struct ObjectEvent gObjectEvents[OBJECT_EVENTS_COUNT];
struct {struct {u8 mapNum,mapGroup;} location;} saved;
#define gSaveBlock1Ptr (&saved)
static u8 QL_IS_PLAYBACK_STATE=FALSE;
static u8 sHallOfFameRecallActive=FALSE, sHealingRecallState=FOLLOWER_HEAL_IDLE;
static u8 sStairWarpRecall=FALSE, sFlyRecallActive=FALSE, sFieldTravelRecallActive=FALSE;
static u8 sWarpPadRecall=FALSE, sDoorWarpRecall=FALSE, sSpinTileRecall=FALSE;
static u8 sFollowerYieldHoldTrail=FALSE;
static s16 sFollowerYieldPlayerX,sFollowerYieldPlayerY;
static u8 last_action, follower_action_count, npc_action_count, cleared_trail;
static int blockedWest=0;
int TryGetObjectEventIdByLocalIdAndMap(u8 localId,u8 a,u8 b,u8 *id){(void)localId;(void)a;(void)b;*id=2;return 0;}
int ObjectEventIsHeldMovementActive(struct ObjectEvent *o){(void)o;return 0;}
int ObjectEventClearHeldMovementIfFinished(struct ObjectEvent *o){(void)o;return 1;}
int MapGridGetMetatileBehaviorAt(s16 x,s16 y){(void)x;(void)y;return 0;}
int MetatileBehavior_IsWarpPad(int a){(void)a;return 0;}
int Follower_IsSafeSpawnTile(s16 x,s16 y){
 if(x<0 || y<0 || x>20 || y>20)return 0;
 if(blockedWest && x==9 && y==12)return 0;
 return !(x==gObjectEvents[0].currentCoords.x && y==gObjectEvents[0].currentCoords.y)
  && !(x==gObjectEvents[1].currentCoords.x && y==gObjectEvents[1].currentCoords.y);
}
void MoveCoords(u8 dir,s16 *x,s16 *y){if(dir==DIR_SOUTH)++*y;else if(dir==DIR_NORTH)--*y;else if(dir==DIR_WEST)--*x;else if(dir==DIR_EAST)++*x;}
void UnfreezeObjectEvent(struct ObjectEvent *o){ assert(o==&gObjectEvents[2]); }
u8 GetWalkFastMovementAction(u8 direction){return 100+direction;}
int ObjectEventSetHeldMovement(struct ObjectEvent *o,u8 action){
 if(o==&gObjectEvents[2]) {follower_action_count++;last_action=action;MoveCoords(action-100,&o->currentCoords.x,&o->currentCoords.y);return 0;}
 npc_action_count++;return 0;
}
void Follower_ClearTrail(void){cleared_trail++;}
'''
main=r'''
int main(void){
 gObjectEvents[0].active=1;gObjectEvents[0].isPlayer=1;gObjectEvents[0].currentCoords.x=10;gObjectEvents[0].currentCoords.y=13;
 gObjectEvents[1].active=1;gObjectEvents[1].facingDirection=DIR_SOUTH;
 gObjectEvents[1].currentCoords.x=10;gObjectEvents[1].currentCoords.y=10;
 gObjectEvents[2].active=1;gObjectEvents[2].spriteId=3;
 gObjectEvents[2].currentCoords.x=10;gObjectEvents[2].currentCoords.y=12;
 Follower_ObserveTrainerAlert(1);
 assert(follower_action_count==1 && npc_action_count==0 && sFollowerYieldHoldTrail);
 assert(gObjectEvents[1].currentCoords.x==10 && gObjectEvents[1].currentCoords.y==10);
 assert(gObjectEvents[2].currentCoords.x==9 && gObjectEvents[2].currentCoords.y==12);
 assert(last_action==100+DIR_WEST);
 Follower_ObserveTrainerAlert(1);
 assert(follower_action_count==1);
 sFollowerYieldHoldTrail=0;gObjectEvents[2].currentCoords.x=10;gObjectEvents[2].currentCoords.y=12;
 blockedWest=1;
 Follower_ObserveTrainerAlert(1);
 assert(follower_action_count==2 && last_action==100+DIR_EAST);
 // Scripted NPC about to step into follower's tile: notify only; don't touch NPC.
 blockedWest=0;sFollowerYieldHoldTrail=0;
 gObjectEvents[2].currentCoords.x=10;gObjectEvents[2].currentCoords.y=12;
 gObjectEvents[1].currentCoords.x=10;gObjectEvents[1].currentCoords.y=11;
 Follower_ObserveNpcScriptStep(1,MOVEMENT_ACTION_WALK_NORMAL_DOWN);
 assert(follower_action_count==3 && npc_action_count==0);
 assert(gObjectEvents[1].currentCoords.x==10 && gObjectEvents[1].currentCoords.y==11);
 // Nonintersecting NPC step should do nothing.
 sFollowerYieldHoldTrail=0;
 gObjectEvents[2].currentCoords.x=9;gObjectEvents[2].currentCoords.y=12;
 Follower_ObserveNpcScriptStep(1,MOVEMENT_ACTION_WALK_NORMAL_DOWN);
 assert(follower_action_count==3);
 return 0;
}
'''
with tempfile.TemporaryDirectory() as t:
    src=Path(t)/'test.c';out=Path(t)/'test';src.write_text(header_c+impl+main)
    subprocess.run(['gcc','-std=gnu89','-Wall','-Wextra','-Werror',str(src),'-o',str(out)],check=True)
    subprocess.run([str(out)],check=True)
print('PASS: vanilla Trainer/NPC control flow and compiled follower-only one-tile alert/step responses')
