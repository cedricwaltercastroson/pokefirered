#!/usr/bin/env python3
"""v0.9.0.3.28: verify actual trainer and follower slide callbacks at frame boundaries.

Runs on host GCC with only battle-engine APIs mocked; does not claim a GBA
build or emulator visual verification.
"""
import pathlib
import re
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
player = (ROOT / "src/battle_controller_player.c").read_text()

def extract(name):
    match = re.search(r'\bvoid\s+' + re.escape(name) + r'\s*\([^)]*\)\s*\{', player)
    assert match, f"missing function {name}"
    depth = 0
    for offset in range(match.end() - 1, len(player)):
        if player[offset] == '{':
            depth += 1
        elif player[offset] == '}':
            depth -= 1
            if depth == 0:
                return player[match.start():offset + 1]
    raise AssertionError(f"incomplete function {name}")

assert 'if (Follower_TakeBattleEntrySlide(gBattlerPartyIndexes[gActiveBattler]))' in player
assert 'gSprites[monSpriteId].x2 = -96;' in player
assert 'gSprites[monSpriteId].callback = SpriteCB_FollowerBattleSlideIn;' in player
assert 'gBattlerControllerFuncs[battlerId] = Intro_WaitForFollowerBattleSlide;' in player
assert 'gBattlerControllerFuncs[gActiveBattler] = Intro_TryShinyAnimShowHealthbox;' in player

harness = r'''
#include <assert.h>
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
#define FALSE 0
#define MON_DATA_SPECIES 0
#define BATTLE_TYPE_MULTI 0x40
#define BIT_FLANK 2
struct Sprite;
typedef void (*SpriteCallback)(struct Sprite *);
struct Sprite { int x2, invisible; int data[8]; struct { int paletteNum; } oam; SpriteCallback callback; };
struct Task { int data[16]; };
struct Pokemon { int species; };
struct Template { int unused; };
struct Sprite gSprites[64];
struct Task gTasks[8];
struct Pokemon gPlayerParty[6];
struct Template gMultiuseSpriteTemplate;
u8 gBattlerSpriteIds[4] = {1,0,0,0};
u8 gBattlerPartyIndexes[4];
u8 gBattleControllerData[4];
u8 gBattleBufferA[4][8];
u8 gActiveBattler;
uint32_t gBattleTypeFlags;
u8 gBattleMonForms[4];
void (*gBattlerControllerFuncs[4])(void);
static int freedTrainer, createdMon, destroyedTask, cryCount;
static int crySpecies;
static int IsDoubleBattle(void) { return 0; }
static void StartSendOutAnim(u8 b, int arg) { (void)b; (void)arg; assert(0 && "no second send-out for single battle"); }
static void SpriteCallbackDummy(struct Sprite *sprite) {(void)sprite;}
static void Intro_WaitForFollowerBattleSlide(void) {}
static u16 GetMonData(struct Pokemon *pokemon, int field) { assert(field == MON_DATA_SPECIES); return pokemon->species; }
static void FreeSpriteOamMatrix(struct Sprite *sprite) {(void)sprite;}
static u16 GetSpritePaletteTagByPaletteNum(int num) {(void)num; return 0;}
static void FreeSpritePaletteByTag(u16 tag) {(void)tag;}
static void DestroySprite(struct Sprite *sprite) {assert(sprite == &gSprites[1]); freedTrainer++;}
static void BattleLoadPlayerMonSpriteGfx(struct Pokemon *mon, u8 battler) {(void)mon; assert(battler == 0);}
static u8 GetBattlerPosition(u8 battler) {return battler;}
static void SetMultiuseSpriteTemplateToPokemon(u16 species, u8 position) {(void)position; assert(species == 25);}
static int GetBattlerSpriteCoord(u8 battler, int coord) {(void)battler; assert(coord == 2); return 40;}
static int GetBattlerSpriteDefault_Y(u8 battler) {(void)battler; return 64;}
static int GetBattlerSpriteSubpriority(u8 battler) {(void)battler; return 1;}
static u8 CreateSprite(struct Template *t, int x, int y, int pri) {(void)t; (void)x; (void)y; (void)pri; createdMon++; return 4;}
static void StartSpriteAnim(struct Sprite *sprite, int anim) {(void)sprite; (void)anim;}
static u8 CreateInvisibleSpriteWithCallback(SpriteCallback cb) {(void)cb; return 9;}
static void DestroyTask(u8 id) {assert(id == 0); destroyedTask++;}
static void PlayCry3(u16 species, int pan, int mode) {assert(pan == -25 && mode == 0); cryCount++; crySpecies = species;}
void SpriteCB_FollowerBattleSlideIn(struct Sprite *sprite);
''' + extract('Task_StartFollowerBattleEntry') + '\n' + extract('SpriteCB_FollowerBattleSlideIn') + r'''
int main(void) {
    gPlayerParty[0].species = 25;
    gTasks[0].data[0] = 0;
    gTasks[0].data[1] = 1;
    gSprites[1].x2 = 0;
    for (int i = 0; i < 18; i++) {
        Task_StartFollowerBattleEntry(0);
        assert(gSprites[1].x2 == -8 * (i + 1));
        assert(freedTrainer == 0 && createdMon == 0);
    }
    assert(gSprites[1].x2 == -144);
    Task_StartFollowerBattleEntry(0);
    assert(freedTrainer == 1 && createdMon == 1 && destroyedTask == 1);
    assert(gBattlerSpriteIds[0] == 4);
    assert(gSprites[4].x2 == -96);
    assert(gSprites[4].callback == SpriteCB_FollowerBattleSlideIn);
    assert(gBattlerControllerFuncs[0] == Intro_WaitForFollowerBattleSlide);
    for (int i = 0; i < 23; i++) {
        SpriteCB_FollowerBattleSlideIn(&gSprites[4]);
        assert(gSprites[4].x2 == -96 + 4 * (i + 1));
        assert(gSprites[4].x2 < 0);
        assert(cryCount == 0);
    }
    SpriteCB_FollowerBattleSlideIn(&gSprites[4]);
    assert(gSprites[4].x2 == 0 && cryCount == 1 && crySpecies == 25);
    assert(gSprites[4].callback == SpriteCallbackDummy);
    return 0;
}
'''

with tempfile.TemporaryDirectory() as tempdir:
    source = pathlib.Path(tempdir) / 'test.c'
    binary = pathlib.Path(tempdir) / 'test'
    source.write_text(harness)
    subprocess.run(['gcc', '-std=gnu11', '-Wall', '-Wextra', '-Werror',
                    str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('PASS: trainer 18 frames/144 px, follower 24 frames/96 px, cry once at finish')
