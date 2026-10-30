#!/usr/bin/env python3
"""Exercise the Oak-lab first-battle follower entrance and script-safe return."""
from pathlib import Path
import re
import subprocess
import tempfile
import hashlib

root = Path(__file__).resolve().parents[1]
oak = (root / 'src/battle_controller_oak_old_man.c').read_text()
follower = (root / 'src/following_pokemon.c').read_text()
setup = (root / 'src/battle_setup.c').read_text()
header = (root / 'include/following_pokemon.h').read_text()
labfile = 'data/maps/PalletTown_ProfessorOaksLab/scripts.inc'
lab = (root / labfile).read_bytes()
assert hashlib.sha256(lab).hexdigest() == '3ad06964c0d71e8bba9b8b1e6d3b5b6ad0d46f6ce6f15db727810c3a1804b215', 'Oak/Blue original lab movement scripts must be unchanged'


def function(source, name):
    m = re.search(r'\b(?:static\s+)?(?:void|bool8)\s+' + name + r'\s*\([^;]*?\)\s*\{', source)
    assert m, name
    depth = 0
    for i in range(m.end()-1, len(source)):
        if source[i] == '{': depth += 1
        elif source[i] == '}':
            depth -= 1
            if depth == 0: return source[m.start():i+1]
    raise AssertionError(name)

assert 'else if (gBattleTypeFlags & (BATTLE_TYPE_OLD_MAN_TUTORIAL | BATTLE_TYPE_FIRST_BATTLE))' in (root/'src/battle_controllers.c').read_text()
assert 'gBattleTypeFlags |= BATTLE_TYPE_FIRST_BATTLE;' in setup
assert 'Follower_TakeBattleEntrySlide(gBattlerPartyIndexes[gActiveBattler])' in function(oak,'OakOldManHandleIntroTrainerBallThrow')
assert 'gBattlerControllerFuncs[battlerId] = Intro_WaitForOakLabFollowerBattleSlide;' in oak
assert 'gBattlerControllerFuncs[gActiveBattler] = Intro_TryShinyAnimShowHealthbox;' in function(oak,'Intro_WaitForOakLabFollowerBattleSlide')
assert 'gBattleControllerData[battlerId] = CreateInvisibleSpriteWithCallback(SpriteCallbackDummy);' in oak
assert 'gTasks[taskId].data[2]++ < 18' in oak
assert 'sprite->x2 += 4;' in oak
assert 'gSprites[monSpriteId].x2 = -96;' in oak
assert 'if (gBattleTypeFlags & BATTLE_TYPE_FIRST_BATTLE)' in function(follower,'Follower_PrepareBattleEntry')
assert 'sBattleEntryFollowerPartyIndex = partyIndex + 1;' in function(follower,'Follower_PrepareBattleEntry')
assert 'void Follower_NotifyOakLabRivalBattleReturn(void);' in header
assert setup.count('Follower_NotifyOakLabRivalBattleReturn();') == 2
assert 'sOakLabRivalReturnPending = TRUE;' in function(follower,'Follower_NotifyOakLabRivalBattleReturn')
assert 'sOakLabRivalReturnPending && sBattleRecallAwaitStep' in function(follower,'Follower_BeginScriptedPlayerMovement')
assert 'sOakLabRivalReturnPending = FALSE;' in function(follower,'Follower_NotifyBattleReturn')
assert 'sOakLabRivalReturnPending = FALSE;' in function(follower,'Follower_ResetForMapLoad')

# Compile and execute the *actual extracted C animation task and controller
# handoff*, not a reimplementation. Keep tutorial-specific callback isolated.
h = r'''
#include <assert.h>
#include <stdint.h>
typedef unsigned char u8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned int u32;
typedef unsigned char bool8;
#define TRUE 1
#define FALSE 0
#define MON_DATA_SPECIES 1
#define SpriteCallbackDummy spriteDummy
#define GetBattlerPosition(x) (x)
#define GetBattlerSpriteCoord(x,y) (80)
#define GetBattlerSpriteDefault_Y(x) (80)
#define GetBattlerSpriteSubpriority(x) (3)
struct Sprite;
typedef void (*SpriteFn)(struct Sprite*);
struct Sprite { s16 x2; s16 data[8]; u8 invisible; struct {u8 paletteNum;} oam; SpriteFn callback; };
struct Task { s16 data[16]; };
struct Pokemon {u16 species;};
static struct Sprite gSprites[16];
static struct Task gTasks[8];
static struct Pokemon gPlayerParty[6];
static u8 gBattlerSpriteIds[4], gBattlerPartyIndexes[4], gBattleMonForms[4], gBattleControllerData[4], gActiveBattler;
static void (*gBattlerControllerFuncs[4])(void);
static int gMultiuseSpriteTemplate, destroyedTrainer, taskDestroyed, cried, nextSprite=4;
static void spriteDummy(struct Sprite* p) {(void)p;}
static void Intro_TryShinyAnimShowHealthbox(void) {}
static void BattleLoadPlayerMonSpriteGfx(struct Pokemon* p,u8 b){(void)p;(void)b;}
static u16 GetMonData(struct Pokemon* p,int type){(void)type;return p->species;}
static void SetMultiuseSpriteTemplateToPokemon(u16 s,int pos){(void)s;(void)pos;}
static u8 CreateSprite(int* t,int x,int y,int p){(void)t;(void)x;(void)y;(void)p;return nextSprite++;}
static void StartSpriteAnim(struct Sprite* s,u8 a){(void)s;(void)a;}
static u8 CreateInvisibleSpriteWithCallback(SpriteFn f){(void)f;return 12;}
static void FreeSpriteOamMatrix(struct Sprite* p){(void)p;}
static u16 GetSpritePaletteTagByPaletteNum(u8 n){return n;}
static void FreeSpritePaletteByTag(u16 tag){(void)tag;}
static void DestroySprite(struct Sprite* p){ if(p==&gSprites[2])destroyedTrainer++; }
static void DestroyTask(u8 id){(void)id;taskDestroyed++;}
static void PlayCry3(u16 species,int x,int y){(void)species;(void)x;(void)y;cried++;}
static void Task_StartOakLabFollowerBattleEntry(u8 taskId);
static void SpriteCB_OakLabFollowerBattleSlideIn(struct Sprite *sprite);
static void Intro_WaitForOakLabFollowerBattleSlide(void);
'''
body = '\n\n'.join(function(oak,x) for x in ('Task_StartOakLabFollowerBattleEntry', 'SpriteCB_OakLabFollowerBattleSlideIn', 'Intro_WaitForOakLabFollowerBattleSlide'))
main = r'''
int main(void)
{
    int i;
    gBattlerPartyIndexes[0] = 0;
    gPlayerParty[0].species = 1;
    gBattlerSpriteIds[0] = 2;
    gTasks[0].data[0] = 0;
    gTasks[0].data[1] = 2;
    for (i=0;i<18;i++) {
        Task_StartOakLabFollowerBattleEntry(0);
        assert(destroyedTrainer == 0);
    }
    assert(gSprites[2].x2 == -144);
    Task_StartOakLabFollowerBattleEntry(0);
    assert(destroyedTrainer == 1 && taskDestroyed == 1);
    assert(gBattlerSpriteIds[0] == 4 && gSprites[4].x2 == -96);
    assert(gSprites[4].invisible == FALSE && gBattleControllerData[0] == 12);
    assert(gBattlerControllerFuncs[0] == Intro_WaitForOakLabFollowerBattleSlide);
    for (i=0;i<24;i++) gSprites[4].callback(&gSprites[4]);
    assert(cried == 1 && gSprites[4].x2 == 0);
    Intro_WaitForOakLabFollowerBattleSlide();
    assert(gBattlerControllerFuncs[0] == Intro_TryShinyAnimShowHealthbox);
    return 0;
}
'''
with tempfile.TemporaryDirectory() as d:
    c = Path(d)/'first_battle.c'; exe=Path(d)/'first_battle'
    c.write_text(h + body + main)
    subprocess.run(['gcc','-std=gnu89','-Wall','-Wextra','-Werror',str(c),'-o',str(exe)],check=True)
    subprocess.run([str(exe)], check=True)

# Compile the exact lab-specific deferred-return guard as a standalone C
# expression, exercise both map identity and the latch's lifetime.
match = re.search(r'if \(sOakLabRivalReturnPending && sBattleRecallAwaitStep\s*&& .*?\)\s*return;',function(follower,'Follower_BeginScriptedPlayerMovement'),flags=re.S)
assert match, 'Missing first-battle follower return state guard'
guard = match.group(0)
cond = r'''
#include <assert.h>
typedef unsigned char bool8;
typedef unsigned char u8;
#define MAP_NUM(x) 8
#define MAP_GROUP(x) 4
#define PALLET_TOWN_PROFESSOR_OAKS_LAB 0
struct Save {struct {u8 mapNum,mapGroup;}location;};
static struct Save saved;
static struct Save *gSaveBlock1Ptr=&saved;
static bool8 sOakLabRivalReturnPending,sBattleRecallAwaitStep;
static int mustPreserve(void){
''' + guard.replace('return;', 'return 1;') + r'''
return 0;
}
int main(void){
 saved.location.mapNum=8; saved.location.mapGroup=4;
 sOakLabRivalReturnPending=1; sBattleRecallAwaitStep=1;
 assert(mustPreserve());
 sOakLabRivalReturnPending=0; assert(!mustPreserve());
 sOakLabRivalReturnPending=1; sBattleRecallAwaitStep=0; assert(!mustPreserve());
 sBattleRecallAwaitStep=1; saved.location.mapNum=9; assert(!mustPreserve());
 saved.location.mapNum=8; saved.location.mapGroup=5; assert(!mustPreserve());
 return 0;
}
'''
with tempfile.TemporaryDirectory() as d:
    c=Path(d)/'lab_guard.c';exe=Path(d)/'lab_guard';c.write_text(cond)
    subprocess.run(['gcc','-std=gnu89','-Wall','-Wextra','-Werror',str(c),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS: lab scripts unchanged; compiled actual tutorial follower 18/24-frame entry and return guard; other maps unaffected')

# The first fight can happen before the follower OBJ exists: verify the real
# snapshot code admits the starter only with FIRST_BATTLE, not other fights.
party_harness = r'''
#include <assert.h>
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned char bool8;
#define PARTY_SIZE 6
#define OBJECT_EVENTS_COUNT 16
#define MAX_SPRITES 16
#define SPECIES_NONE 0
#define BATTLE_TYPE_FIRST_BATTLE (1u << 4)
#define BATTLE_TYPE_LINK (1u << 5)
#define BATTLE_TYPE_MULTI (1u << 6)
#define BATTLE_TYPE_SAFARI (1u << 7)
#define BATTLE_TYPE_OLD_MAN_TUTORIAL (1u << 8)
#define BATTLE_TYPE_POKEDUDE (1u << 9)
#define BATTLE_TYPE_BATTLE_TOWER (1u << 10)
#define BATTLE_TYPE_RECORDED (1u << 11)
#define BATTLE_TYPE_INGAME_PARTNER (1u << 12)
#define MON_DATA_HP 1
#define MON_DATA_SPECIES 2
#define OBJ_EVENT_GFX_FOLLOWER 152
#define FOLLOWER_LOCAL_ID 254
struct Pokemon {u16 species,hp;};
struct ObjectEvent {u8 active,invisible,graphicsId,spriteId;};
struct Sprite {u8 invisible;};
struct Save {struct {u8 mapNum,mapGroup;}location;};
static struct Pokemon gPlayerParty[6];
static struct ObjectEvent gObjectEvents[16];
static struct Sprite gSprites[16];
static struct Save saved;
static struct Save *gSaveBlock1Ptr=&saved;
static u32 gBattleTypeFlags;
static u8 sBattleEntryFollowerPartyIndex, partySlot;
static u16 sFollowerSpecies;
static u8 Follower_GetLeadPartyIndex(void){return partySlot;}
static u16 GetMonData(struct Pokemon *p,int which,...){return which==MON_DATA_HP?p->hp:p->species;}
static bool8 TryGetObjectEventIdByLocalIdAndMap(u8 localId,u8 mapNum,u8 mapGroup,u8 *id)
{(void)localId;(void)mapNum;(void)mapGroup;*id=1;return !gObjectEvents[1].active;}
'''
party_body = function(follower,'Follower_PrepareBattleEntry')
party_main = r'''
int main(void)
{
 partySlot=0;gPlayerParty[0].species=1;gPlayerParty[0].hp=20;
 gBattleTypeFlags=BATTLE_TYPE_FIRST_BATTLE;
 Follower_PrepareBattleEntry();
 assert(sBattleEntryFollowerPartyIndex==1); /* Starter before follower OBJ */
 gBattleTypeFlags=0;
 Follower_PrepareBattleEntry();
 assert(sBattleEntryFollowerPartyIndex==0); /* Other battles require sprite */
 sFollowerSpecies=1;gObjectEvents[1].active=1;
 gObjectEvents[1].graphicsId=OBJ_EVENT_GFX_FOLLOWER;
 gObjectEvents[1].spriteId=2;
 Follower_PrepareBattleEntry();
 assert(sBattleEntryFollowerPartyIndex==1); /* Existing normal visible follower */
 gObjectEvents[1].invisible=1;
 Follower_PrepareBattleEntry();
 assert(sBattleEntryFollowerPartyIndex==0);
 gBattleTypeFlags=BATTLE_TYPE_FIRST_BATTLE;partySlot=PARTY_SIZE;
 Follower_PrepareBattleEntry();
 assert(sBattleEntryFollowerPartyIndex==0); /* Empty party */
 partySlot=0;gPlayerParty[0].hp=0;
 Follower_PrepareBattleEntry();
 assert(sBattleEntryFollowerPartyIndex==0); /* Fainted starter */
 return 0;
}
'''
with tempfile.TemporaryDirectory() as d:
    c=Path(d)/'snapshot.c';exe=Path(d)/'snapshot'
    c.write_text(party_harness+party_body+party_main)
    subprocess.run(['gcc','-std=gnu89','-Wall','-Wextra','-Werror',str(c),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS: compiled initial battle snapshot accepts unseen starter only for tutorial; other battles unchanged')
