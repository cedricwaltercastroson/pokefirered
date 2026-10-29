"""Regression for Route 11 Youngster Dave's zero-step sight-range-one encounter.

Compiles follower yield helpers taken from actual source against an isolated
movement harness. Does not claim to run the GBA engine or emulator.
"""
from pathlib import Path
import re, subprocess, tempfile, runpy, json
root=Path(__file__).resolve().parents[1]
mapdata=json.loads((root/'data/maps/Route11/map.json').read_text())
trainer=next(x for x in mapdata['object_events'] if x['script']=='Route11_EventScript_Dave')
assert trainer['trainer_sight_or_berry_tree_id']=='1'
assert trainer['movement_type']=='MOVEMENT_TYPE_WALK_SEQUENCE_DOWN_RIGHT_LEFT_UP'
assert 'I raised my POKéMON carefully.' in (root/'data/maps/Route11/text.inc').read_text()
src=(root/'src/following_pokemon.c').read_text()
trainer_src=(root/'src/trainer_see.c').read_text()
assert 'Follower_BeginAdjacentTrainerYield' in (root/'include/following_pokemon.h').read_text()
assert 'Follower_BeginAdjacentTrainerYield((u8)(trainerObj - gObjectEvents))' in trainer_src
assert re.search(r'TrainerSeeFunc_PrepareToEngage\(.+?\)\s*\{.*?Follower_BeginAdjacentTrainerYield',trainer_src,re.S)
assert 'TrainerApproachPlayer(&gObjectEvents[trainerObjId], approachDistance - 1);' in trainer_src

# Reuse the exact movement harness exercised by the earlier 2026 QA patch.
legacy=runpy.run_path(str(root/'tests/test_trainer_sight_follower_yield_v0900334.py'))
harness=legacy['harness']
oldmain=harness.index('int main(void) {test_single_step_fallback();')
harness=harness[:oldmain]
start=src.index('bool8 Follower_BeginAdjacentTrainerYield(u8 trainerId)')
brace=src.index('{',start)
depth=0
for i in range(brace,len(src)):
    if src[i]=='{': depth+=1
    if src[i]=='}':
        depth-=1
        if depth==0:
            adjacent_fn=src[start:i+1]
            break
# Also compile the real TrainerSeeFunc_PrepareToEngage task routine.
match = list(re.finditer(r'static bool8 TrainerSeeFunc_PrepareToEngage\(u8 taskId, struct Task \* task, struct ObjectEvent \* trainerObj\)', trainer_src))[-1]
start2 = match.start()
brace2 = trainer_src.index('{',match.end())
depth2=0
for j in range(brace2,len(trainer_src)):
    if trainer_src[j]=='{': depth2+=1
    if trainer_src[j]=='}':
        depth2-=1
        if depth2==0:
            trainer_engage_fn = trainer_src[start2:j+1]
            break
harness+='\n'+adjacent_fn+r'''
#define tData5 data[5]
static int engaged;
static u8 GetTrainerFacingDirectionMovementType(u8 dir){return dir;}
static void SetTrainerMovementType(struct ObjectEvent *ob,u8 type){(void)ob;(void)type;}
static void OverrideMovementTypeForObjectEvent(struct ObjectEvent *ob,u8 type){(void)ob;(void)type;}
static void OverrideTemplateCoordsForObjectEvent(struct ObjectEvent *ob){(void)ob;engaged++;}
static void CancelPlayerForcedMovement(void){}
static void finish_follower_step(void) {
 struct ObjectEvent *f=&gObjectEvents[2];
 MoveCoords(f->movementActionId,&f->currentCoords.x,&f->currentCoords.y);
 f->heldMovementFinished=1;
}
static void test_route11_adjacent(void) {
 reset();
 // Dave is one tile behind a north-facing player; no trainer walking step.
 gObjectEvents[1].currentCoords.x=4;
 gObjectEvents[1].currentCoords.y=3;
 gObjectEvents[1].facingDirection=DIR_NORTH;
 gObjectEvents[2].currentCoords.x=4;
 gObjectEvents[2].currentCoords.y=3;
 assert(Follower_BeginAdjacentTrainerYield(1));
 assert(sNpcYieldInProgress && sNpcYieldHoldTrail && walks==1);
 assert(!gObjectEvents[2].invisible);
 assert(gObjectEvents[1].currentCoords.x==4 && gObjectEvents[1].currentCoords.y==3);
 assert(Follower_DeferNpcScriptStep(1,MOVEMENT_ACTION_FACE_PLAYER));
 finish_follower_step();
 assert(Follower_DeferNpcScriptStep(1,MOVEMENT_ACTION_FACE_PLAYER));
 assert(walks==2);
 finish_follower_step();
 assert(!Follower_DeferNpcScriptStep(1,MOVEMENT_ACTION_FACE_PLAYER));
 assert(!sNpcYieldInProgress && !sNpcYieldHoldTrail);
 assert(!gObjectEvents[2].invisible);
 assert(abs(gObjectEvents[2].currentCoords.x-4)+abs(gObjectEvents[2].currentCoords.y-3)==2);
 assert(gObjectEvents[1].currentCoords.x==4 && gObjectEvents[1].currentCoords.y==3);
}
static void test_trainer_task_zero_step(void) {
 struct Task t={0};
 reset();engaged=0;
 gObjectEvents[1].currentCoords.x=4;gObjectEvents[1].currentCoords.y=3;
 gObjectEvents[1].facingDirection=DIR_NORTH;
 gObjectEvents[2].currentCoords.x=4;gObjectEvents[2].currentCoords.y=3;
 t.tTrainerRange=0;t.tFuncId=4;
 assert(!TrainerSeeFunc_PrepareToEngage(0,&t,&gObjectEvents[1]));
 assert(t.tData5==1 && engaged==0 && walks==1);
 assert(!TrainerSeeFunc_PrepareToEngage(0,&t,&gObjectEvents[1]));
 assert(t.tData5==1 && walks==1 && engaged==0);
 finish_follower_step();
 assert(!TrainerSeeFunc_PrepareToEngage(0,&t,&gObjectEvents[1]));
 assert(walks==2 && engaged==0);
 finish_follower_step();
 assert(!TrainerSeeFunc_PrepareToEngage(0,&t,&gObjectEvents[1]));
 assert(t.tData5==2 && engaged==1 && t.tFuncId==5);
}
static void test_no_contact(void) {
 reset();
 gObjectEvents[1].currentCoords.x=4;
 gObjectEvents[1].currentCoords.y=0;
 assert(!Follower_BeginAdjacentTrainerYield(1));
 assert(walks==0);
}
static void test_blocked_does_not_stall(void) {
 reset();forcedNoSafe=1;
 gObjectEvents[1].currentCoords.x=4;
 gObjectEvents[1].currentCoords.y=3;
 assert(!Follower_BeginAdjacentTrainerYield(1));
 assert(walks==0);
}
static void test_adjacent_no_follower(void) {
 reset();gObjectEvents[2].active=FALSE;
 gObjectEvents[1].currentCoords.x=4;
 gObjectEvents[1].currentCoords.y=3;
 assert(!Follower_BeginAdjacentTrainerYield(1));
}
int main(void) {test_route11_adjacent();test_trainer_task_zero_step();test_no_contact();test_blocked_does_not_stall();test_adjacent_no_follower();puts("PASS: Route 11 zero-approach Trainer contact yields safely");return 0;}
'''
# The extracted task routine runs inside the same native movement harness.
harness = harness.replace('static void finish_follower_step(void)', trainer_engage_fn+'\nstatic void finish_follower_step(void)')
with tempfile.TemporaryDirectory() as d:
 c=Path(d)/'route11.c'; exe=Path(d)/'route11'
 c.write_text(harness)
 subprocess.run(['gcc','-std=gnu11','-O0','-Wall','-Wextra','-Werror','-Wno-unused-parameter','-Wno-misleading-indentation','-Wno-unused-function',str(c),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('PASS: zero-step source wiring, Route 11 config and compiled follower choreography')
