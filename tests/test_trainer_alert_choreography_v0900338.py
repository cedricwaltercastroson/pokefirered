"""Host C simulation of the v38 Trainer exclamation choreography.

Extracts real follower and trainer-see functions; this does not run a GBA ROM.
"""
from pathlib import Path
import re
import subprocess
import tempfile
from zipfile import ZipFile

ROOT = Path(__file__).resolve().parents[1]
FOLLOWER = (ROOT / 'src/following_pokemon.c').read_text()
TRAINER = (ROOT / 'src/trainer_see.c').read_text()
HEADER = (ROOT / 'include/following_pokemon.h').read_text()
OAK = (ROOT / 'data/maps/PokemonLeague_ChampionsRoom/scripts.inc').read_text()
BASE = ZipFile('/mnt/data/FireRed_Followers_v0_9_0_3_17_Faster_Transition_Full_Source_Test(2).zip').read('data/maps/PokemonLeague_ChampionsRoom/scripts.inc').decode()


def extract(source, name):
    matches = list(re.finditer(r'(?:static\s+)?(?:bool8|u8|void|s16)\s+' + re.escape(name) + r'\s*\(', source))
    assert matches, name
    start = matches[-1].start()
    brace = source.index('{', matches[-1].end())
    depth = 0
    for i in range(brace, len(source)):
        if source[i] == '{':
            depth += 1
        elif source[i] == '}':
            depth -= 1
            if depth == 0:
                return source[start:i + 1]
    raise AssertionError(name)


assert 'Follower_NotifyTrainerBattleReturn' in (ROOT / 'src/battle_setup.c').read_text()
assert 'Follower_NotifyTrainerBattleReturn(void)' in FOLLOWER
assert 'Follower_NotifyTrainerBattleReturn(void)' in HEADER
assert 'Follower_BeginTrainerAlertYield' in HEADER
assert 'Follower_WaitTrainerAlertYield' in HEADER
assert TRAINER.count('Follower_BeginTrainerAlertYield((u8)(trainerObj - gObjectEvents))') == 1
assert TRAINER.count('Follower_WaitTrainerAlertYield((u8)(trainerObj - gObjectEvents))') == 1
assert 'Follower_DeferNpcScriptStep' not in TRAINER  # retired per-step approach experiment
assert 'Follower_BeginAdjacentTrainerYield' not in TRAINER
assert 'sTrainerAlertYieldExtraDirection' not in FOLLOWER  # v40: only one map tile
assert 'sNpcYieldHoldTrail' in FOLLOWER
assert 'Follower_DeferNpcScriptStep' in (ROOT / 'src/script_movement.c').read_text()

start = 'PokemonLeague_ChampionsRoom_Movement_OakEnter::'
end = 'PokemonLeague_ChampionsRoom_Movement_OakExit::'
assert OAK[OAK.index(start):OAK.index(end)] == BASE[BASE.index(start):BASE.index(end)]

code = r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char u8;
typedef short s16;
typedef u8 bool8;
#define TRUE 1
#define FALSE 0
#define DIR_NONE 0
#define DIR_SOUTH 1
#define DIR_NORTH 2
#define DIR_WEST 3
#define DIR_EAST 4
#define OBJECT_EVENTS_COUNT 16
#define MAX_SPRITES 64
#define FOLLOWER_LOCAL_ID 254
#define FOLLOWER_HEAL_IDLE 0
#define QL_IS_PLAYBACK_STATE 0
#define FLDEFF_EXCLAMATION_MARK_ICON 2
#define MOVEMENT_TYPE_TREE_DISGUISE 30
#define MOVEMENT_TYPE_MOUNTAIN_DISGUISE 31
#define MOVEMENT_TYPE_BURIED 32
#define tFuncId data[0]
#define tTrainerRange data[3]
struct Coords {s16 x,y;};
struct ObjectEvent {
 bool8 active,invisible,isPlayer,frozen;
 u8 spriteId,facingDirection,movementType;
 struct Coords currentCoords;
 bool8 heldMovementActive,heldMovementFinished;
 u8 movementActionId;
};
struct Task {s16 data[16];};
struct Save {struct {u8 mapNum,mapGroup;} location;} save,*gSaveBlock1Ptr=&save;
struct {u8 objectEventId;} gPlayerAvatar;
struct ObjectEvent gObjectEvents[OBJECT_EVENTS_COUNT];
static bool8 sNpcYieldInProgress,sNpcYieldHoldTrail,sTrainerAlertYieldActive,sTrainerAlertVisualHold;
static u8 sNpcYieldObjectId;
static s16 sTrainerAlertPlayerX,sTrainerAlertPlayerY;
static bool8 sHallOfFameRecallActive,sStairWarpRecall,sFlyRecallActive,sFieldTravelRecallActive,sWarpPadRecall,sDoorWarpRecall,sSpinTileRecall;
static u8 sHealingRecallState;
static int walks,clearTrail,forceOne,forceNone,iconActive;
static u8 gFieldEffectArguments[3];
static bool8 TryGetObjectEventIdByLocalIdAndMap(u8 id,u8 map,u8 group,u8 *o) {(void)map;(void)group;if(id!=FOLLOWER_LOCAL_ID)return TRUE;*o=2;return FALSE;}
static bool8 ObjectEventIsHeldMovementActive(struct ObjectEvent *o) {return o->heldMovementActive;}
static bool8 ObjectEventClearHeldMovementIfFinished(struct ObjectEvent *o) {if(o->heldMovementFinished){o->heldMovementActive=FALSE;return TRUE;}return FALSE;}
static void UnfreezeObjectEvent(struct ObjectEvent *o) {o->frozen=FALSE;}
static void MoveCoords(u8 dir,s16 *x,s16 *y) {if(dir==DIR_SOUTH)(*y)++;if(dir==DIR_NORTH)(*y)--;if(dir==DIR_EAST)(*x)++;if(dir==DIR_WEST)(*x)--;}
static u8 GetOppositeDirection(u8 dir) {if(dir==DIR_SOUTH)return DIR_NORTH;if(dir==DIR_NORTH)return DIR_SOUTH;if(dir==DIR_EAST)return DIR_WEST;return DIR_EAST;}
static bool8 Follower_IsSafeSpawnTile(s16 x,s16 y) {
 int i;
 if(forceNone || x<0||x>=15||y<0||y>=15)return FALSE;
 if(forceOne && (x==2 || x==6)) return FALSE;
 for(i=0;i<OBJECT_EVENTS_COUNT;i++)if(gObjectEvents[i].active && gObjectEvents[i].currentCoords.x==x && gObjectEvents[i].currentCoords.y==y)return FALSE;
 return TRUE;
}
static u8 MapGridGetMetatileBehaviorAt(s16 x,s16 y){(void)x;(void)y;return 0;}
static bool8 MetatileBehavior_IsWarpPad(u8 b){(void)b;return FALSE;}
static u8 GetWalkFastMovementAction(u8 dir){return dir;}
static bool8 ObjectEventSetHeldMovement(struct ObjectEvent *o,u8 action){if(o->heldMovementActive)return TRUE;o->heldMovementActive=TRUE;o->heldMovementFinished=FALSE;o->movementActionId=action;walks++;return FALSE;}
static void Follower_ClearTrail(void){clearTrail++;}
static void ObjectEventGetLocalIdAndMap(struct ObjectEvent *o,u8 *id,u8 *map,u8 *group){(void)o;*id=1;*map=1;*group=1;}
static void FieldEffectStart(int effect){(void)effect;iconActive=1;}
static bool8 FieldEffectActiveListContains(int effect){(void)effect;return iconActive;}
static u8 GetFaceDirectionMovementAction(u8 dir){return dir;}
''' + '\n\n'.join(extract(FOLLOWER, n) for n in ('Follower_AbsCoordDelta', 'Follower_BeginTrainerAlertYield', 'Follower_WaitTrainerAlertYield')) + '\n\n' + '\n\n'.join(extract(TRAINER, n) for n in ('TrainerSeeFunc_StartExclMark', 'TrainerSeeFunc_WaitExclMark')) + r'''
static void reset(void) {
 memset(gObjectEvents,0,sizeof(gObjectEvents));
 sNpcYieldInProgress=sNpcYieldHoldTrail=sTrainerAlertYieldActive=sTrainerAlertVisualHold=0;
 sNpcYieldObjectId=0;
 sHallOfFameRecallActive=sStairWarpRecall=sFlyRecallActive=sFieldTravelRecallActive=0;
 sWarpPadRecall=sDoorWarpRecall=sSpinTileRecall=sHealingRecallState=0;
 walks=clearTrail=forceOne=forceNone=iconActive=0;
 gPlayerAvatar.objectEventId=0;
 gObjectEvents[0].active=TRUE;gObjectEvents[0].isPlayer=TRUE;
 gObjectEvents[0].currentCoords.x=4;gObjectEvents[0].currentCoords.y=2;
 gObjectEvents[1].active=TRUE;gObjectEvents[1].facingDirection=DIR_NORTH;
 gObjectEvents[1].currentCoords.x=4;gObjectEvents[1].currentCoords.y=5;
 gObjectEvents[2].active=TRUE;gObjectEvents[2].spriteId=2;gObjectEvents[2].frozen=TRUE;
 gObjectEvents[2].currentCoords.x=4;gObjectEvents[2].currentCoords.y=3;
}
static void finishStep(void){struct ObjectEvent *f=&gObjectEvents[2]; assert(f->heldMovementActive);MoveCoords(f->movementActionId,&f->currentCoords.x,&f->currentCoords.y);f->heldMovementFinished=TRUE;}
static void testRearAlert(void){
 struct Task task={0};reset(); task.tFuncId=1;task.tTrainerRange=0; // zero approach steps, like Route 11 Dave
 assert(TrainerSeeFunc_StartExclMark(0,&task,&gObjectEvents[1]));
 assert(iconActive && task.tFuncId==2);
 assert(walks==2); // follower and trainer's facing action
 assert(sTrainerAlertYieldActive && sNpcYieldHoldTrail && !gObjectEvents[2].invisible);
 assert(!TrainerSeeFunc_WaitExclMark(0,&task,&gObjectEvents[1]));
 assert(task.tFuncId==2);
 finishStep();
 assert(!TrainerSeeFunc_WaitExclMark(0,&task,&gObjectEvents[1]));
 assert(walks==2); // no second sidestep is allowed
 iconActive=0;
 assert(TrainerSeeFunc_WaitExclMark(0,&task,&gObjectEvents[1]));
 assert(task.tFuncId==3 && !sTrainerAlertYieldActive && !sNpcYieldInProgress);
 assert(sNpcYieldHoldTrail && !gObjectEvents[2].invisible);
 assert(abs(gObjectEvents[2].currentCoords.x-4)==1);
}
static void testFrontNoChange(void){reset();gObjectEvents[1].currentCoords.y=0;gObjectEvents[1].facingDirection=DIR_SOUTH; assert(!Follower_BeginTrainerAlertYield(1));assert(walks==0);}
static void testFallback(void){reset();forceOne=1;assert(Follower_BeginTrainerAlertYield(1));assert(walks==1);finishStep();assert(!Follower_WaitTrainerAlertYield(1));assert(!sTrainerAlertYieldActive);}
static void testNoSpace(void){reset();forceNone=1;assert(!Follower_BeginTrainerAlertYield(1));assert(walks==0);}
static void testNoFollower(void){reset();gObjectEvents[2].invisible=TRUE;assert(!Follower_BeginTrainerAlertYield(1));assert(walks==0);}
static void testSideEncounter(void){reset();gObjectEvents[1].currentCoords.x=5;gObjectEvents[1].currentCoords.y=2;gObjectEvents[1].facingDirection=DIR_WEST;assert(Follower_BeginTrainerAlertYield(1));assert(walks==1);}
int main(void){testRearAlert();testFrontNoChange();testFallback();testNoSpace();testNoFollower();testSideEncounter();puts("PASS: actual Trainer alert source choreography, zero-step, one-tile clearance and no-space fallback");return 0;}
'''

with tempfile.TemporaryDirectory() as d:
    c = Path(d) / 'test.c'
    exe = Path(d) / 'test'
    c.write_text(code)
    subprocess.run(['gcc', '-std=gnu89', '-O0', '-Wall', '-Wextra', '-Werror',
                    '-Wno-unused-parameter', '-Wno-unused-function', str(c), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('PASS: v38 isolated C event / movement regression, original Oak path and v31 trainer return')
