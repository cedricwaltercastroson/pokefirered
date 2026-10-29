#!/usr/bin/env python3
"""v41: compile/run the real offscreen-Trainer camera states and alert cancellation.

Host-only simulation: validates source state-machine ordering, not ROM timing.
"""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
TRAINER = (ROOT / 'src/trainer_see.c').read_text()
FOLLOWER = (ROOT / 'src/following_pokemon.c').read_text()
HEADER = (ROOT / 'include/following_pokemon.h').read_text()


def extract(src, name):
    matches = list(re.finditer(r'(?:static\s+)?(?:bool8|void)\s+' + re.escape(name) + r'\s*\(', src))
    assert matches, name
    start = matches[-1].start()
    brace = src.index('{', matches[-1].end())
    level = 0
    for i in range(brace, len(src)):
        if src[i] == '{':
            level += 1
        elif src[i] == '}':
            level -= 1
            if level == 0:
                return src[start:i + 1]
    raise AssertionError(name)

up = extract(TRAINER, 'TrainerSeeFunc_OffscreenAboveTrainerCameraObjMoveUp')
down = extract(TRAINER, 'TrainerSeeFunc_OffscreenAboveTrainerCameraObjMoveDown')
cancel = extract(FOLLOWER, 'Follower_CancelTrainerAlertYield')
assert 'void Follower_CancelTrainerAlertYield(u8 trainerId);' in HEADER
assert up.index('FieldEffectStart(FLDEFF_EXCLAMATION_MARK_ICON)') < up.index('Follower_BeginTrainerAlertYield')
assert down.index('Follower_WaitTrainerAlertYield') < down.index('FieldEffectActiveListContains')
assert 'Follower_CancelTrainerAlertYield((u8)(trainerObj - gObjectEvents))' in TRAINER

code = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef unsigned char u8;
typedef short s16;
typedef u8 bool8;
#define TRUE 1
#define FALSE 0
#define DIR_NORTH 2
#define DIR_SOUTH 1
#define OBJ_EVENT_ID_CAMERA 127
#define FLDEFF_EXCLAMATION_MARK_ICON 3
#define tFuncId data[0]
#define tTrainerRange data[3]
#define tData5 data[5]
struct ObjectEvent { u8 active, spriteId; };
struct Task { s16 data[16]; };
struct Save { struct { u8 mapNum, mapGroup; } location; } saved, *gSaveBlock1Ptr=&saved;
struct ObjectEvent gObjectEvents[16];
static u8 sTrainerAlertYieldActive, sNpcYieldInProgress, sNpcYieldHoldTrail, sTrainerAlertVisualHold, sNpcYieldObjectId;
static int iconActive, followerAvailable, pendingFollowerFrames, beginCount, waitCount;
static int cameraUpSteps, cameraDownSteps, cameraRemoved, followedSprite;
static void TryGetObjectEventIdByLocalIdAndMap(u8 id, u8 map, u8 group, u8 *output) { (void)id; (void)map; (void)group; *output=3; }
static u8 ObjectEventIsMovementOverridden(struct ObjectEvent *o) { (void)o; return 0; }
static u8 ObjectEventClearHeldMovementIfFinished(struct ObjectEvent *o) { (void)o; return 1; }
static u8 ObjectEventSetHeldMovement(struct ObjectEvent *o, u8 action) {
 (void)o; if(action==DIR_NORTH)cameraUpSteps++; else if(action==DIR_SOUTH)cameraDownSteps++;
 return 0;
}
static u8 GetWalkFastMovementAction(u8 direction) {return direction;}
static void ObjectEventGetLocalIdAndMap(struct ObjectEvent *o, u8 *id, u8 *map, u8 *group) {
 (void)o;*id=1;*map=2;*group=3;
}
static u8 gFieldEffectArguments[3];
static void FieldEffectStart(u8 effect) { (void)effect; iconActive=1; }
static u8 FieldEffectActiveListContains(u8 effect) { (void)effect; return iconActive; }
static void CameraObjectSetFollowedObjectId(int id) {followedSprite=id;}
static int GetPlayerAvatarObjectId(void) {return 0;}
static void RemoveObjectEventByLocalIdAndMap(u8 id, u8 map, u8 group) { (void)id; (void)map; (void)group; cameraRemoved++;}
static bool8 Follower_BeginTrainerAlertYield(u8 trainerId) {
 assert(trainerId==1);
 beginCount++;
 if(!followerAvailable) return FALSE;
 pendingFollowerFrames=3;
 return TRUE;
}
static bool8 Follower_WaitTrainerAlertYield(u8 trainerId) {
 assert(trainerId==1);
 waitCount++;
 if(pendingFollowerFrames>0) {pendingFollowerFrames--; return TRUE;}
 return FALSE;
}
''' + '\n' + up + '\n' + down + '\n' + cancel + r'''
static void reset(int available) {
 memset(gObjectEvents,0,sizeof(gObjectEvents));
 followerAvailable=available;
 gObjectEvents[1].active=1;
 gObjectEvents[3].active=1;
 iconActive=pendingFollowerFrames=beginCount=waitCount=0;
 cameraUpSteps=cameraDownSteps=cameraRemoved=followedSprite=0;
 sTrainerAlertYieldActive=sNpcYieldInProgress=sNpcYieldHoldTrail=sTrainerAlertVisualHold=sNpcYieldObjectId=0;
}
static void test_offscreen(int hasFollower) {
 struct Task t={{0}};
 int i;
 reset(hasFollower);
 t.tTrainerRange=4;
 t.tFuncId=13;
 for(i=0;i<3;i++) {
  assert(!TrainerSeeFunc_OffscreenAboveTrainerCameraObjMoveUp(0,&t,&gObjectEvents[1]));
 }
 assert(cameraUpSteps==3);
 assert(!TrainerSeeFunc_OffscreenAboveTrainerCameraObjMoveUp(0,&t,&gObjectEvents[1]));
 assert(t.tFuncId==14 && iconActive && beginCount==1 && cameraDownSteps==0);
 // Before the icon finishes, a pending follower sidestep must hold camera.
 for(i=0;i<(hasFollower?3:0);i++) {
  assert(!TrainerSeeFunc_OffscreenAboveTrainerCameraObjMoveDown(0,&t,&gObjectEvents[1]));
  assert(cameraDownSteps==0 && t.tFuncId==14);
 }
 assert(!TrainerSeeFunc_OffscreenAboveTrainerCameraObjMoveDown(0,&t,&gObjectEvents[1]));
 assert(cameraDownSteps==0); // icon is still showing
 iconActive=0;
 for(i=0;i<3;i++) {
  assert(!TrainerSeeFunc_OffscreenAboveTrainerCameraObjMoveDown(0,&t,&gObjectEvents[1]));
 }
 assert(cameraDownSteps==3 && cameraRemoved==0);
 assert(!TrainerSeeFunc_OffscreenAboveTrainerCameraObjMoveDown(0,&t,&gObjectEvents[1]));
 assert(cameraRemoved==1 && t.tFuncId==2 && followedSprite==0);
}
static void test_cancel_owner_isolation(void) {
 reset(1);
 sTrainerAlertYieldActive=1;
 sNpcYieldInProgress=1;
 sNpcYieldHoldTrail=1;
 sTrainerAlertVisualHold=1;
 sNpcYieldObjectId=1;
 Follower_CancelTrainerAlertYield(2);
 assert(sTrainerAlertYieldActive && sNpcYieldInProgress && sNpcYieldHoldTrail);
 Follower_CancelTrainerAlertYield(1);
 assert(!sTrainerAlertYieldActive && !sNpcYieldInProgress && !sNpcYieldHoldTrail && !sTrainerAlertVisualHold);
 // Do not cancel another scripted NPC whose yield is now active.
 sNpcYieldInProgress=1;sNpcYieldObjectId=2;
 Follower_CancelTrainerAlertYield(1);
 assert(sNpcYieldInProgress);
}
int main(void) {
 test_offscreen(1);test_offscreen(0);test_cancel_owner_isolation();
 puts("PASS: offscreen camera alert one-tile handoff, no follower, and canceled alert state");
 return 0;
}
'''
with tempfile.TemporaryDirectory() as tmp:
    c = Path(tmp) / 'offscreen.c'
    exe = Path(tmp) / 'offscreen'
    c.write_text(code)
    subprocess.run(['gcc', '-std=gnu89', '-O0', '-Wall', '-Wextra', '-Werror',
                    '-Wno-unused-parameter', '-Wno-unused-function', str(c), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('PASS: source hooks at offscreen exclamation and canceled-trainer cleanup')
