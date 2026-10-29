"""Compiled scripted NPC and automatic trainer-sight yield checks, against real engine helper source.

Targeted host regression: no emulator or GBA ROM gameplay verification.
"""
import re
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = (ROOT / 'src/following_pokemon.c').read_text()
SCRIPT = (ROOT / 'src/script_movement.c').read_text()
TRAINER = (ROOT / 'src/trainer_see.c').read_text()
HEADER = (ROOT / 'include/following_pokemon.h').read_text()
CONSTANTS = (ROOT / 'include/constants/event_object_movement.h').read_text()


def source_function(name):
    pattern = re.compile(r'(?:static\s+)?(?:bool8|u8)\s+' + re.escape(name) + r'\s*\(')
    match = pattern.search(SOURCE)
    assert match, name
    start = match.start()
    brace = SOURCE.index('{', match.end())
    depth = 0
    for pos in range(brace, len(SOURCE)):
        if SOURCE[pos] == '{':
            depth += 1
        elif SOURCE[pos] == '}':
            depth -= 1
            if depth == 0:
                return SOURCE[start:pos + 1]
    raise AssertionError(name)

assert 'Follower_DeferNpcScriptStep(objEventId, nextMoveActionId)' in SCRIPT
assert 'Follower_BeginScriptedPlayerMovement();' in SCRIPT
assert 'Follower_DeferNpcScriptStep' in HEADER
assert '#include "following_pokemon.h"' in TRAINER
assert 'Follower_DeferNpcScriptStep((u8)(trainerObj - gObjectEvents), movementAction)' in TRAINER
begin = SOURCE.split('void Follower_BeginScriptedPlayerMovement(void)', 1)[1].split('void Follower_EndScriptedPlayerMovement(void)', 1)[0]
assert begin.index('sGeneralScriptFollowing = TRUE;') < begin.index('Follower_Remove();')
assert 'sNpcYieldHoldTrail' in SOURCE
assert 'sGeneralScriptFollowing || sNpcYieldInProgress' in SOURCE
assert 'sNpcYieldHoldTrail)' in SOURCE
assert 'sNpcYieldHoldTrail = FALSE;' in source_function('Follower_DeferNpcScriptStep')

symbols = dict(re.findall(r'^#define\s+(MOVEMENT_ACTION_\w+)\s+(0x[\dA-Fa-f]+|\d+)\s*$', CONSTANTS, re.M))
needed = re.findall(r'\bMOVEMENT_ACTION_[A-Z0-9_]+\b',
                    source_function('Follower_GetNpcScriptStepDirection')) + ['MOVEMENT_ACTION_FACE_PLAYER', 'MOVEMENT_ACTION_WALK_NORMAL_UP']
constant_decls = '\n'.join(f'#define {name} {symbols[name]}' for name in sorted(set(needed)))
helper = source_function('Follower_GetNpcScriptStepDirection')
yield_fn = source_function('Follower_DeferNpcScriptStep')

harness = r'''
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
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
#define FOLLOWER_LOCAL_ID 0xFE
#define FOLLOWER_HEAL_IDLE 0
#define QL_IS_PLAYBACK_STATE 0
#define MOVE_OBS 999
''' + constant_decls + r'''
struct Coords { s16 x, y; };
struct ObjectEvent { bool8 active, invisible, isPlayer, frozen; u8 spriteId; struct Coords currentCoords, previousCoords; bool8 heldMovementActive, heldMovementFinished; u8 movementActionId, facingDirection; };
struct SaveBlock { struct {u8 mapNum, mapGroup;} location; } saveBlock, *gSaveBlock1Ptr=&saveBlock;
struct {u8 objectEventId;} gPlayerAvatar;
struct ObjectEvent gObjectEvents[OBJECT_EVENTS_COUNT];
static bool8 sHallOfFameRecallActive, sStairWarpRecall, sFlyRecallActive, sFieldTravelRecallActive, sWarpPadRecall, sDoorWarpRecall, sSpinTileRecall;
static u8 sHealingRecallState;
static bool8 sNpcYieldInProgress, sNpcYieldHoldTrail;
static u8 sNpcYieldObjectId, sNpcYieldExtraDirection;
static bool8 sNpcYieldVisualClearance;
static s16 sNpcYieldPlayerX,sNpcYieldPlayerY;
static int forcedNoSafe, forcedNoTwoTile, trailsCleared, walks;
static u8 idFollower=2;
static int mapBehavior;
static bool8 TryGetObjectEventIdByLocalIdAndMap(u8 id, u8 m, u8 g, u8 *output) { if(id != FOLLOWER_LOCAL_ID) return TRUE; *output = idFollower; return FALSE; }
static bool8 ObjectEventIsHeldMovementActive(struct ObjectEvent *ob) {return ob->heldMovementActive;}
static u8 ObjectEventClearHeldMovementIfFinished(struct ObjectEvent *ob) {
 if (ob->heldMovementFinished) { ob->heldMovementActive=FALSE; return 1; }
 return 0;
}
static void UnfreezeObjectEvent(struct ObjectEvent *ob) {ob->frozen=FALSE;}
static void MoveCoords(u8 dir, s16 *x, s16 *y) {
 if(dir==DIR_SOUTH) (*y)++; if(dir==DIR_NORTH) (*y)--;
 if(dir==DIR_WEST) (*x)--; if(dir==DIR_EAST) (*x)++;
}
static u8 GetOppositeDirection(u8 dir) {if(dir==DIR_SOUTH)return DIR_NORTH; if(dir==DIR_NORTH)return DIR_SOUTH;if(dir==DIR_EAST)return DIR_WEST;return DIR_EAST;}
static bool8 Follower_IsSafeSpawnTile(s16 x, s16 y) {
 int i;
 if (forcedNoSafe || x<0 || x>15 || y<0 || y>15)return FALSE;
 if (forcedNoTwoTile && x==4 && y==5) return FALSE;
 for(i=0;i<OBJECT_EVENTS_COUNT;i++) if(gObjectEvents[i].active && gObjectEvents[i].currentCoords.x==x && gObjectEvents[i].currentCoords.y==y)return FALSE;
 return TRUE;
}
static u8 MapGridGetMetatileBehaviorAt(s16 x, s16 y) {(void)x;(void)y;return mapBehavior;}
static bool8 MetatileBehavior_IsWarpPad(u8 b){return b==1;}
static u8 GetWalkFastMovementAction(u8 dir){return dir;}
static bool8 ObjectEventSetHeldMovement(struct ObjectEvent *ob, u8 movement){
 if(ob->heldMovementActive)return TRUE;
 ob->movementActionId=movement;ob->heldMovementActive=TRUE;ob->heldMovementFinished=FALSE;walks++;return FALSE;
}
static void Follower_ClearTrail(void){trailsCleared++;}
''' + helper + '\n' + yield_fn + r'''
static void reset(void) {
 memset(gObjectEvents,0,sizeof(gObjectEvents));
 memset(&saveBlock,0,sizeof(saveBlock));
 sHallOfFameRecallActive=sStairWarpRecall=sFlyRecallActive=sFieldTravelRecallActive=0;
 sWarpPadRecall=sDoorWarpRecall=sSpinTileRecall=sHealingRecallState=0;
 sNpcYieldInProgress=sNpcYieldHoldTrail=sNpcYieldObjectId=sNpcYieldExtraDirection=0;
 forcedNoSafe=forcedNoTwoTile=trailsCleared=walks=mapBehavior=0;
 gPlayerAvatar.objectEventId=0;
 gObjectEvents[0].active=1;gObjectEvents[0].isPlayer=1;gObjectEvents[0].facingDirection=DIR_NORTH;
 gObjectEvents[0].currentCoords.x=4;gObjectEvents[0].currentCoords.y=2;
 gObjectEvents[1].active=1;gObjectEvents[1].currentCoords.x=3;gObjectEvents[1].currentCoords.y=3;
 gObjectEvents[2].active=1;gObjectEvents[2].spriteId=2;gObjectEvents[2].frozen=1;
 gObjectEvents[2].currentCoords.x=4;gObjectEvents[2].currentCoords.y=3;
 gObjectEvents[2].previousCoords=gObjectEvents[2].currentCoords;
}
static void test_yield(void) {
 reset();
 // NPC heading east directly onto the follower's rear tile.
 assert(Follower_DeferNpcScriptStep(1,MOVEMENT_ACTION_WALK_NORMAL_RIGHT)==TRUE);
 assert(walks==1 && trailsCleared==1 && sNpcYieldInProgress && sNpcYieldHoldTrail);
 assert(gObjectEvents[2].active && !gObjectEvents[2].invisible && !gObjectEvents[2].frozen);
 // The NPC script pointer cannot advance during the pending step.
 assert(Follower_DeferNpcScriptStep(1,MOVEMENT_ACTION_WALK_NORMAL_RIGHT)==TRUE);
 // Finish sidestep and permit the original NPC step, no new pathing.
 MoveCoords(gObjectEvents[2].movementActionId,&gObjectEvents[2].currentCoords.x,&gObjectEvents[2].currentCoords.y);
 gObjectEvents[2].heldMovementFinished=1;
 // Large sprites need a second tile of clearance when space permits.
 assert(Follower_DeferNpcScriptStep(1,MOVEMENT_ACTION_WALK_NORMAL_RIGHT)==TRUE);
 assert(walks==2 && sNpcYieldInProgress && sNpcYieldHoldTrail);
 MoveCoords(gObjectEvents[2].movementActionId,&gObjectEvents[2].currentCoords.x,&gObjectEvents[2].currentCoords.y);
 gObjectEvents[2].heldMovementFinished=1;
 assert(Follower_DeferNpcScriptStep(1,MOVEMENT_ACTION_WALK_NORMAL_RIGHT)==FALSE);
 assert(walks==2 && !sNpcYieldInProgress && !sNpcYieldHoldTrail);
 assert(gObjectEvents[2].currentCoords.x != 4 || gObjectEvents[2].currentCoords.y != 3);
 // Keep 2-tile clearance from the NPC's landing cell (4,3).
 assert(abs(gObjectEvents[2].currentCoords.x - 4)
      + abs(gObjectEvents[2].currentCoords.y - 3) >= 2);
}
static void test_single_step_fallback(void) {
 reset();
 forcedNoTwoTile=1;
 // A wide two-step sidestep is blocked, but a one-step escape is clear.
 assert(Follower_DeferNpcScriptStep(1,MOVEMENT_ACTION_WALK_NORMAL_RIGHT));
 assert(sNpcYieldExtraDirection==DIR_NONE);
 assert(walks==1 && sNpcYieldInProgress);
 MoveCoords(gObjectEvents[2].movementActionId,&gObjectEvents[2].currentCoords.x,&gObjectEvents[2].currentCoords.y);
 gObjectEvents[2].heldMovementFinished=1;
 assert(!Follower_DeferNpcScriptStep(1,MOVEMENT_ACTION_WALK_NORMAL_RIGHT));
 assert(walks==1 && !sNpcYieldInProgress);
}
static void test_facing_does_not_yield(void) {
 reset();
 assert(!Follower_DeferNpcScriptStep(1,MOVEMENT_ACTION_FACE_PLAYER));
 assert(walks==0);
}
static void test_nonintersecting_does_not_yield(void) {
 reset();
 assert(!Follower_DeferNpcScriptStep(1,MOVEMENT_ACTION_WALK_NORMAL_UP));
 assert(walks==0);
}
static void test_blocked_not_stalled(void) {
 reset();forcedNoSafe=1;
 assert(!Follower_DeferNpcScriptStep(1,MOVEMENT_ACTION_WALK_NORMAL_RIGHT));
 assert(walks==0 && gObjectEvents[2].active && !gObjectEvents[2].invisible);
}
static void test_hof_transition_preserved(void) {
 reset();sHallOfFameRecallActive=1;
 assert(!Follower_DeferNpcScriptStep(1,MOVEMENT_ACTION_WALK_NORMAL_RIGHT));
 assert(walks==0);
}
static void test_jump_two_intermediate(void) {
 reset(); gObjectEvents[1].currentCoords.x=2;
 // Jump from x=2 to x=4 crosses x=3, then x=4.
 assert(Follower_DeferNpcScriptStep(1,MOVEMENT_ACTION_JUMP_2_RIGHT));
}
static void test_follower_in_transit(void) {
 reset();
 // Current tile has moved, but the sprite is still crossing its previous tile.
 gObjectEvents[2].previousCoords=gObjectEvents[2].currentCoords;
 gObjectEvents[2].currentCoords.x=5;
 gObjectEvents[2].heldMovementActive=1;
 assert(Follower_DeferNpcScriptStep(1,MOVEMENT_ACTION_WALK_NORMAL_RIGHT));
 gObjectEvents[2].heldMovementFinished=1;
 assert(!Follower_DeferNpcScriptStep(1,MOVEMENT_ACTION_WALK_NORMAL_RIGHT));
}
static void test_no_follower(void) {
 reset();gObjectEvents[2].active=0;
 assert(!Follower_DeferNpcScriptStep(1,MOVEMENT_ACTION_WALK_NORMAL_RIGHT));
}
int main(void) {test_yield();test_facing_does_not_yield();test_nonintersecting_does_not_yield();test_blocked_not_stalled();test_hof_transition_preserved();test_jump_two_intermediate();test_follower_in_transit();test_no_follower();puts("PASS: compiled scripted NPC and trainer-sight follower yield regressions");}
'''


def trainer_function(name):
    pattern = re.compile(r'static\s+bool8\s+' + re.escape(name) + r'\s*\(')
    match = pattern.search(TRAINER)
    assert match, name
    # The declaration appears above its implementation.
    matches = list(pattern.finditer(TRAINER))
    match = matches[-1]
    start = match.start()
    brace = TRAINER.index('{', match.end())
    depth = 0
    for pos in range(brace, len(TRAINER)):
        if TRAINER[pos] == '{':
            depth += 1
        elif TRAINER[pos] == '}':
            depth -= 1
            if depth == 0:
                return TRAINER[start:pos + 1]
    raise AssertionError(name)

trainer_fn = trainer_function('TrainerSeeFunc_TrainerApproach')
trainer_harness = r'''
#define tTrainerRange data[3]
#define tFuncId data[0]
struct Task {s16 data[16];};
static bool8 ObjectEventIsMovementOverridden(struct ObjectEvent *o) {return o->heldMovementActive;}
static u8 GetWalkNormalMovementAction(u8 direction) {return MOVEMENT_ACTION_WALK_NORMAL_DOWN + direction - DIR_SOUTH;}
''' + trainer_fn + r'''
static void reset_trainer_behind(void) {
 reset();
 gObjectEvents[1].currentCoords.x=4;
 gObjectEvents[1].currentCoords.y=5;
 gObjectEvents[1].facingDirection=DIR_NORTH;
}
static void test_trainer_from_behind(void) {
 struct Task t={0};
 struct ObjectEvent *tr;
 reset_trainer_behind();
 tr=&gObjectEvents[1];
 t.tTrainerRange=2;
 // From y=5 to y=4: no follower overlap.
 assert(!TrainerSeeFunc_TrainerApproach(1,&t,tr));
 assert(t.tTrainerRange==1 && walks==1 && !sNpcYieldInProgress);
 tr->currentCoords.y=4;
 tr->heldMovementFinished=1;
 // From y=4 to y=3: occupied by follower. Yield before advancing.
 assert(!TrainerSeeFunc_TrainerApproach(1,&t,tr));
 assert(t.tTrainerRange==1 && walks==2);
 assert(sNpcYieldInProgress && sNpcYieldHoldTrail);
 assert(gObjectEvents[2].active && !gObjectEvents[2].invisible);
 // While follower is moving, trainer range and held movement are unchanged.
 assert(!TrainerSeeFunc_TrainerApproach(1,&t,tr));
 assert(t.tTrainerRange==1 && walks==2);
 MoveCoords(gObjectEvents[2].movementActionId,
            &gObjectEvents[2].currentCoords.x, &gObjectEvents[2].currentCoords.y);
 gObjectEvents[2].heldMovementFinished=1;
 assert(!TrainerSeeFunc_TrainerApproach(1,&t,tr));
 // The trainer still waits while the follower takes a SECOND safe tile.
 assert(t.tTrainerRange==1 && walks==3 && sNpcYieldInProgress);
 MoveCoords(gObjectEvents[2].movementActionId,
            &gObjectEvents[2].currentCoords.x, &gObjectEvents[2].currentCoords.y);
 gObjectEvents[2].heldMovementFinished=1;
 assert(!TrainerSeeFunc_TrainerApproach(1,&t,tr));
 assert(t.tTrainerRange==0 && walks==4);
 assert(!sNpcYieldInProgress && !sNpcYieldHoldTrail);
 assert(gObjectEvents[2].active && !gObjectEvents[2].invisible);
 assert(abs(gObjectEvents[2].currentCoords.x-4) + abs(gObjectEvents[2].currentCoords.y-3)>=2);
}
static void test_trainer_other_directions(void) {
 struct Task t={0};
 reset();
 // Approaching from north stops in front of the player, no yield required.
 gObjectEvents[1].currentCoords.x=4;
 gObjectEvents[1].currentCoords.y=0;
 gObjectEvents[1].facingDirection=DIR_SOUTH;
 t.tTrainerRange=1;
 assert(!TrainerSeeFunc_TrainerApproach(1,&t,&gObjectEvents[1]));
 assert(t.tTrainerRange==0 && !sNpcYieldInProgress);
 reset();
 // Approaching from the left also avoids the rear follower.
 gObjectEvents[1].currentCoords.x=2;
 gObjectEvents[1].currentCoords.y=2;
 gObjectEvents[1].facingDirection=DIR_EAST;
 t.tTrainerRange=1;
 assert(!TrainerSeeFunc_TrainerApproach(1,&t,&gObjectEvents[1]));
 assert(t.tTrainerRange==0 && !sNpcYieldInProgress);
}
static void test_trainer_no_follower_or_space(void) {
 struct Task t={0};
 reset_trainer_behind();
 gObjectEvents[1].currentCoords.y=4;
 gObjectEvents[2].active=0;
 t.tTrainerRange=1;
 assert(!TrainerSeeFunc_TrainerApproach(1,&t,&gObjectEvents[1]));
 assert(t.tTrainerRange==0 && walks==1);
 reset_trainer_behind();
 gObjectEvents[1].currentCoords.y=4;
 forcedNoSafe=1;
 t.tTrainerRange=1;
 // Respect original no-softlock fallback in cramped geometry.
 assert(!TrainerSeeFunc_TrainerApproach(1,&t,&gObjectEvents[1]));
 assert(t.tTrainerRange==0 && walks==1);
}
'''
harness = harness.replace('int main(void) {test_yield();', trainer_harness + '\nint main(void) {test_single_step_fallback();test_trainer_from_behind();test_trainer_other_directions();test_trainer_no_follower_or_space();test_yield();')

with tempfile.TemporaryDirectory() as tmp:
    src = Path(tmp) / 'choreography.c'
    out = Path(tmp) / 'choreography'
    src.write_text(harness)
    subprocess.run(['gcc', '-std=gnu11', '-O0', '-Wall', '-Wextra', '-Werror',
                    '-Wno-unused-parameter','-Wno-misleading-indentation',str(src),'-o',str(out)],check=True)
    subprocess.run([str(out)], check=True)
print('PASS: source contract and trainer-sight movement simulation')
