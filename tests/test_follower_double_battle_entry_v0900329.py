#!/usr/bin/env python3
"""v0.9.0.3.29: compiled regression of follower + partner Double Battle intro.

Tests real extracted logic against a tiny fake controller environment, not an ARM ROM.
"""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
field = (ROOT / 'src/following_pokemon.c').read_text()
player = (ROOT / 'src/battle_controller_player.c').read_text()
battle_main = (ROOT / 'src/battle_main.c').read_text()


def extract(src, name):
    match = re.search(r'\b(?:static\s+)?(?:void|bool8)\s+' + name + r'\s*\([^;]*?\)\s*\{', src)
    assert match, f'missing {name}'
    depth = 0
    for j in range(match.end() - 1, len(src)):
        if src[j] == '{':
            depth += 1
        elif src[j] == '}':
            depth -= 1
            if depth == 0:
                return src[match.start():j + 1]
    raise AssertionError(f'unclosed {name}')


assert 'if (IsDoubleBattle() && !(gBattleTypeFlags & BATTLE_TYPE_MULTI))' in extract(player, 'Task_StartFollowerBattleEntry')
assert 'gActiveBattler = partnerId;' in extract(player, 'Task_StartFollowerBattleEntry')
assert 'StartSendOutAnim(partnerId, FALSE);' in extract(player, 'Task_StartFollowerBattleEntry')
assert 'gActiveBattler = savedActiveBattler;' in extract(player, 'Task_StartFollowerBattleEntry')
assert 'gBattleSpritesDataPtr->healthBoxesData[gActiveBattler ^ BIT_FLANK].ballAnimActive' in extract(player, 'Intro_TryShinyAnimShowHealthbox')
assert 'gBattleTypeFlags & BATTLE_TYPE_MULTI' in extract(player, 'Task_StartSendOutAnim')
# The stock introduction normally initializes both team members via one task.
assert 'gActiveBattler ^= BIT_FLANK;' in extract(player, 'Task_StartSendOutAnim')
assert 'BtlController_EmitIntroTrainerBallThrow(0);' in extract(battle_main, 'BattleIntroPlayerSendsOutMonAnimation')

# Actual follower snapshot should support a Double Battle, even if the engine
# has already promoted the battle type before we reach CreateBattleStartTask.
assert 'BATTLE_TYPE_DOUBLE' not in extract(field, 'Follower_PrepareBattleEntry')

harness = r'''
#include <assert.h>
#include <stdint.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
typedef uint8_t bool8;
#define TRUE 1
#define FALSE 0
#define PARTY_SIZE 6
#define SPECIES_NONE 0
#define OBJ_EVENT_GFX_FOLLOWER 152
#define FOLLOWER_LOCAL_ID 254
#define OBJECT_EVENTS_COUNT 16
#define MAX_SPRITES 64
#define MON_DATA_SPECIES 0
#define MON_DATA_HP 1
#define BATTLE_TYPE_LINK (1<<1)
#define BATTLE_TYPE_DOUBLE (1<<0)
#define BATTLE_TYPE_MULTI (1<<6)
#define BATTLE_TYPE_SAFARI (1<<7)
#define BATTLE_TYPE_OLD_MAN_TUTORIAL (1<<9)
#define BATTLE_TYPE_POKEDUDE (1<<16)
#define BATTLE_TYPE_BATTLE_TOWER (1<<8)
#define BATTLE_TYPE_RECORDED (1<<24)
#define BATTLE_TYPE_INGAME_PARTNER (1<<22)
#define BIT_FLANK 2
#define B_POSITION_PLAYER_LEFT 0
#define B_POSITION_PLAYER_RIGHT 2
#define HEALTHBOX_ALL 0

struct Pokemon { u16 species; u16 hp; };
struct ObjectEvent { int active, invisible, graphicsId; u8 spriteId; };
struct SaveBlock1 { struct { u8 mapNum, mapGroup; } location; };
struct Sprite { int x, y, x2, invisible; int subpriority; struct {u8 paletteNum;} oam; int data[8]; void (*callback)(struct Sprite *); };
struct Task { int data[16]; };
struct BattleHealthData { bool8 ballAnimActive; };
struct BattleSpriteData { struct BattleHealthData healthBoxesData[4]; };
struct SpriteTemplate {int placeholder;};
static struct SaveBlock1 save;
static struct SaveBlock1 *gSaveBlock1Ptr = &save;
static struct Pokemon gPlayerParty[PARTY_SIZE];
static struct ObjectEvent gObjectEvents[OBJECT_EVENTS_COUNT];
static struct Sprite gSprites[MAX_SPRITES];
static struct Task gTasks[16];
static struct SpriteTemplate gMultiuseSpriteTemplate;
static struct BattleSpriteData battleData;
static struct BattleSpriteData *gBattleSpritesDataPtr = &battleData;
static u8 gPlayerPartyCount = 3;
static u32 gBattleTypeFlags;
static u8 gActiveBattler;
static u8 gBattlerPartyIndexes[4];
static u8 gBattlerSpriteIds[4];
static u8 gBattleControllerData[4];
static u8 gBattleMonForms[4];
static u8 gBattleBufferA[4][8];
static void (*gBattlerControllerFuncs[4])(void);
static u16 sFollowerSpecies = 25;
static u8 sBattleEntryFollowerPartyIndex;
static int objectVisible=1, partnerBallCount=0, partnerGfxCount=0, destroyedTrainers=0, sentinelCount=0;
static int lastBallBattler=-1, lastBallActive=-1, lastBallPartyIndex=-1;
static int battleSpriteCount=0, lastSpriteSpecies=-1;
static void SpriteCallbackDummy(struct Sprite *s) {(void)s;}
static void SpriteCB_FollowerBattleSlideIn(struct Sprite *s){s->x2 += 4; if(s->x2 >= 0) {s->x2=0; s->callback=SpriteCallbackDummy;}}
static void Intro_WaitForFollowerBattleSlide(void){}
static void Intro_TryShinyAnimShowHealthbox(void){}
static u8 Follower_GetLeadPartyIndex(void){for(u8 i=0;i<gPlayerPartyCount;i++) if(gPlayerParty[i].species && gPlayerParty[i].species <= 151)return i; return PARTY_SIZE;}
static u32 GetMonData(struct Pokemon *mon, int attr){return attr == MON_DATA_SPECIES ? mon->species : mon->hp;}
static bool8 TryGetObjectEventIdByLocalIdAndMap(u8 a,u8 b,u8 c,u8 *result){(void)a;(void)b;(void)c;if(!objectVisible) return TRUE; *result=2; return FALSE;}
static bool8 IsDoubleBattle(void){return !!(gBattleTypeFlags & BATTLE_TYPE_DOUBLE);}
static int GetSpritePaletteTagByPaletteNum(u8 n){return n;}
static void FreeSpritePaletteByTag(int tag){(void)tag;}
static void FreeSpriteOamMatrix(struct Sprite *s){(void)s;}
static void DestroySprite(struct Sprite *s){(void)s; destroyedTrainers++;}
static void BattleLoadPlayerMonSpriteGfx(struct Pokemon *mon, u8 battler){(void)mon; if(battler==BIT_FLANK)partnerGfxCount++;}
static void SetMultiuseSpriteTemplateToPokemon(u16 species, int pos){(void)pos;lastSpriteSpecies=species;}
static int GetBattlerPosition(u8 battler){return battler;}
static int GetBattlerSpriteCoord(u8 battler,int coord){(void)battler;(void)coord;return 120;}
static int GetBattlerSpriteDefault_Y(u8 battler){(void)battler;return 80;}
static int GetBattlerSpriteSubpriority(u8 battler){(void)battler;return 5;}
static u8 CreateSprite(struct SpriteTemplate *tpl,int x,int y,int sub){(void)tpl;(void)x;(void)y;(void)sub;return 20+battleSpriteCount++;}
static void StartSpriteAnim(struct Sprite *s, int anim){(void)s;(void)anim;}
static u8 CreateInvisibleSpriteWithCallback(void(*cb)(struct Sprite*)){(void)cb;return 60+sentinelCount++;}
static void StartSendOutAnim(u8 battler, bool8 dontClear){
 (void)dontClear;
 assert(battler==BIT_FLANK);
 partnerBallCount++;
 lastBallBattler=battler;
 lastBallActive=gActiveBattler;
 lastBallPartyIndex=gBattleBufferA[battler][1];
 gBattlerSpriteIds[battler]=30;
 gBattleControllerData[battler]=59;
 battleData.healthBoxesData[battler].ballAnimActive=TRUE;
}
static void DestroyTask(u8 taskId){gTasks[taskId].data[15]=-1;}
'''

actual = '\n\n'.join([extract(field, 'Follower_PrepareBattleEntry'),
                        extract(field, 'Follower_TakeBattleEntrySlide'),
                        extract(player, 'Task_StartFollowerBattleEntry')])

run = r'''
static void resetScene(int isDouble){
 partnerBallCount=0; partnerGfxCount=0; lastBallActive=-1; lastBallBattler=-1;
 lastBallPartyIndex=-1; battleSpriteCount=0; sentinelCount=0; destroyedTrainers=0;
 for(int i=0;i<4;i++){ gBattlerSpriteIds[i]=0; gBattleControllerData[i]=0; gBattlerControllerFuncs[i]=0;}
 for(int i=0;i<16;i++) for(int j=0;j<16;j++)gTasks[i].data[j]=0;
 for(int i=0;i<64;i++){gSprites[i].callback=SpriteCallbackDummy;gSprites[i].x2=0;gSprites[i].invisible=FALSE;}
 for(int i=0;i<4;i++)battleData.healthBoxesData[i].ballAnimActive=FALSE;
 gBattleTypeFlags=isDouble?BATTLE_TYPE_DOUBLE:0;
 gActiveBattler=0;
 gPlayerParty[0].species=25; gPlayerParty[0].hp=99;
 gPlayerParty[1].species=6; gPlayerParty[1].hp=99;
 gPlayerPartyCount=2;
 gObjectEvents[2].active=TRUE;gObjectEvents[2].spriteId=7;
 gObjectEvents[2].graphicsId=OBJ_EVENT_GFX_FOLLOWER;
 gBattlerPartyIndexes[0]=0;gBattlerPartyIndexes[2]=1;
 gBattlerSpriteIds[0]=1;
 gTasks[0].data[0]=0;gTasks[0].data[1]=1;gTasks[0].data[2]=0;
}
int main(void){
 (void)gBattleSpritesDataPtr;
 // Battle flagged double before overworld snapshot: still arm the follower.
 resetScene(1);
 Follower_PrepareBattleEntry();
 assert(Follower_TakeBattleEntrySlide(0)==TRUE);
 Follower_PrepareBattleEntry();
 assert(Follower_TakeBattleEntrySlide(0)==TRUE);
 for(int i=0;i<19;i++) Task_StartFollowerBattleEntry(0);
 assert(partnerBallCount==1 && partnerGfxCount==1);
 assert(lastBallBattler==2 && lastBallActive==2 && lastBallPartyIndex==1);
 assert(gActiveBattler==0);
 assert(gBattlerSpriteIds[0]==20 && gBattlerSpriteIds[2]==30);
 assert(gBattleControllerData[0]>=60 && gBattleControllerData[2]==59);
 assert(gSprites[gBattlerSpriteIds[0]].x2==-96);
 assert(gBattlerControllerFuncs[0]==Intro_WaitForFollowerBattleSlide);
 assert(battleData.healthBoxesData[2].ballAnimActive==TRUE);
 assert(gTasks[0].data[15]==-1);

 // Already-begun battle promoted to double AFTER follower snapshot.
 resetScene(0);
 Follower_PrepareBattleEntry();
 assert(Follower_TakeBattleEntrySlide(0));
 gBattleTypeFlags=BATTLE_TYPE_DOUBLE;
 for(int i=0;i<19;i++) Task_StartFollowerBattleEntry(0);
 assert(partnerBallCount==1 && partnerGfxCount==1);

 // In a normal SINGLE battle, the follower still slides, with no second send-out.
 resetScene(0);
 Follower_PrepareBattleEntry();
 assert(Follower_TakeBattleEntrySlide(0));
 for(int i=0;i<19;i++) Task_StartFollowerBattleEntry(0);
 assert(partnerBallCount==0 && partnerGfxCount==0);
 assert(gBattlerSpriteIds[0]==20);

 // Link and multi battles must keep their stock intro, not the follower slide.
 resetScene(1);
 gBattleTypeFlags|=BATTLE_TYPE_LINK;
 Follower_PrepareBattleEntry();
 assert(!Follower_TakeBattleEntrySlide(0));
 resetScene(1);
 gBattleTypeFlags|=BATTLE_TYPE_MULTI;
 Follower_PrepareBattleEntry();
 assert(!Follower_TakeBattleEntrySlide(0));
 // If battle flags change after the snapshot, multi must still stay stock.
 resetScene(0);
 Follower_PrepareBattleEntry();
 gBattleTypeFlags = BATTLE_TYPE_DOUBLE | BATTLE_TYPE_MULTI;
 assert(!Follower_TakeBattleEntrySlide(0));
 return 0;
}
'''
with tempfile.TemporaryDirectory() as td:
    src = Path(td) / 'follower_doubles.c'
    exe = Path(td) / 'follower_doubles'
    src.write_text(harness + actual + run)
    subprocess.run(['gcc', '-std=gnu11', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function', str(src), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('PASS: real extracted double-battle follower/partner controller paths compiled and executed')
