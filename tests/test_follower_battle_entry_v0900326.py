#!/usr/bin/env python3
"""v0.9.0.3.26: compile real eligibility/slide C functions with GBA state mocked.

Tests do not represent an ARM ROM build or an emulator visual test.
"""
import pathlib
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
field = (ROOT / 'src/following_pokemon.c').read_text()
setup = (ROOT / 'src/battle_setup.c').read_text()
player = (ROOT / 'src/battle_controller_player.c').read_text()
header = (ROOT / 'include/following_pokemon.h').read_text()


def function(text, name):
    import re
    m = re.search(r'\b(?:void|bool8)\s+' + re.escape(name) + r'\s*\([^)]*\)\s*\{', text)
    assert m, f'Missing {name}'
    depth = 0
    for i in range(m.end() - 1, len(text)):
        if text[i] == '{':
            depth += 1
        elif text[i] == '}':
            depth -= 1
            if depth == 0:
                return text[m.start():i + 1]
    raise AssertionError(f'Incomplete {name}')


# Integration checks: one-time snapshot before battle transition, only initial
# trainer-sendout changes, never the ordinary switches or Pokeball machinery.
assert 'Follower_PrepareBattleEntry();\n    taskId = CreateTask(Task_BattleStart, 1);' in setup
assert 'void Follower_PrepareBattleEntry(void);' in header
assert 'bool8 Follower_TakeBattleEntrySlide(u8 partyIndex);' in header
assert 'sBattleEntryFollowerPartyIndex = 0;' in field.split('void Follower_ResetForMapLoad(void)', 1)[1].split('static bool8 Follower_TryEnqueue', 1)[0]
assert player.index('Follower_TakeBattleEntrySlide(gBattlerPartyIndexes[gActiveBattler])') > player.index('static void PlayerHandleIntroTrainerBallThrow(void)\n{')
assert 'StartSendOutAnim(gActiveBattler, gBattleBufferA[gActiveBattler][2]);' in player  # normal switches
assert 'gSprites[monSpriteId].callback = SpriteCB_FollowerBattleSlideIn;' in player
assert 'gBattleControllerData[battlerId] = CreateInvisibleSpriteWithCallback(SpriteCallbackDummy);' in player
assert 'gBattlerControllerFuncs[battlerId] = Intro_WaitForFollowerBattleSlide;' in player
assert 'gBattlerControllerFuncs[gActiveBattler] = Intro_TryShinyAnimShowHealthbox;' in player
assert 'DoPokeballSendOutAnimation(0, POKEBALL_PLAYER_SENDOUT);' in player

# Compile and execute actual extracted eligibility and slide functions.
ctype = '''
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
struct ObjectEvent { int active, invisible, graphicsId; u8 spriteId; };
struct Sprite { int x2, invisible; u16 data[8]; void (*callback)(struct Sprite *); };
struct Pokemon { int species, hp; };
struct SaveBlock1 { struct { u8 mapNum,mapGroup; } location; };
struct SaveBlock1 save; struct SaveBlock1 *gSaveBlock1Ptr = &save;
struct Pokemon gPlayerParty[PARTY_SIZE];
struct ObjectEvent gObjectEvents[OBJECT_EVENTS_COUNT];
struct Sprite gSprites[MAX_SPRITES];
u8 gPlayerPartyCount = 3; u32 gBattleTypeFlags = 0;
static u16 sFollowerSpecies = 25;
static u8 sBattleEntryFollowerPartyIndex;
static int objectVisible = 1;
static int cryCount = 0;
static int lastCrySpecies = 0;
static u8 Follower_GetLeadPartyIndex(void) {
    for (u8 i=0;i<gPlayerPartyCount;i++)
        if (gPlayerParty[i].species > 0 && gPlayerParty[i].species <= 151)
            return i;
    return PARTY_SIZE;
}
static u32 GetMonData(struct Pokemon *p, int field) {
    return field == MON_DATA_HP ? p->hp : p->species;
}
static bool8 TryGetObjectEventIdByLocalIdAndMap(u8 id, u8 m, u8 g, u8 *out) {
    if (!objectVisible) return TRUE;
    *out = 2; return FALSE;
}
static void SpriteCallbackDummy(struct Sprite *sprite) { }
static void PlayCry3(u16 species, int pan, int mode) {
    cryCount++; lastCrySpecies = species;
    assert(pan == -25 && mode == 0);
}
'''
source = ctype + '\n' + '\n'.join(function(field, n) for n in ['Follower_PrepareBattleEntry', 'Follower_TakeBattleEntrySlide']) + '\n' + function(player, 'SpriteCB_FollowerBattleSlideIn') + '''
int main(void) {
    gObjectEvents[2].active = TRUE;
    gObjectEvents[2].spriteId = 7;
    gObjectEvents[2].graphicsId = OBJ_EVENT_GFX_FOLLOWER;
    gPlayerParty[0].species = 25; gPlayerParty[0].hp = 75;
    gPlayerParty[1].species = 6; gPlayerParty[1].hp = 40;

    Follower_PrepareBattleEntry();
    assert(Follower_TakeBattleEntrySlide(0) == TRUE);
    assert(Follower_TakeBattleEntrySlide(0) == FALSE); // once only, not switches

    Follower_PrepareBattleEntry();
    assert(Follower_TakeBattleEntrySlide(1) == FALSE); // different lead

    gPlayerParty[0].hp = 0;
    Follower_PrepareBattleEntry();
    assert(Follower_TakeBattleEntrySlide(0) == FALSE); // fainted follower
    gPlayerParty[0].hp = 75;

    objectVisible = 0;
    Follower_PrepareBattleEntry();
    assert(Follower_TakeBattleEntrySlide(0) == FALSE);
    objectVisible = 1;

    gSprites[7].invisible = TRUE;
    Follower_PrepareBattleEntry();
    assert(Follower_TakeBattleEntrySlide(0) == FALSE);
    gSprites[7].invisible = FALSE;

    gBattleTypeFlags = BATTLE_TYPE_DOUBLE;
    Follower_PrepareBattleEntry();
    assert(Follower_TakeBattleEntrySlide(0) == TRUE); // lead slides; partner uses its own Poké Ball
    gBattleTypeFlags = BATTLE_TYPE_LINK;
    Follower_PrepareBattleEntry();
    assert(Follower_TakeBattleEntrySlide(0) == FALSE);
    gBattleTypeFlags = 0;

    // The last slot must work with zero as the unarmed sentinel.
    gPlayerPartyCount = PARTY_SIZE;
    for (int i = 0; i < 5; i++) gPlayerParty[i].species = 0;
    gPlayerParty[5].species = 25; gPlayerParty[5].hp = 75;
    Follower_PrepareBattleEntry();
    assert(Follower_TakeBattleEntrySlide(5) == TRUE);
    assert(Follower_TakeBattleEntrySlide(5) == FALSE);
    gPlayerPartyCount = 3;
    gPlayerParty[0].species = 25;

    // A previous valid capture must be invalidated by a new no-follower battle.
    Follower_PrepareBattleEntry();
    objectVisible = 0;
    Follower_PrepareBattleEntry();
    assert(Follower_TakeBattleEntrySlide(0) == FALSE);

    struct Sprite mon = {0};
    mon.x2 = -96; mon.data[2] = 25;
    for (int i=0;i<23;i++) {
        SpriteCB_FollowerBattleSlideIn(&mon);
        assert(mon.x2 < 0 && cryCount == 0);
    }
    SpriteCB_FollowerBattleSlideIn(&mon);
    assert(mon.x2 == 0 && cryCount == 1 && lastCrySpecies == 25);
    assert(mon.callback == SpriteCallbackDummy);
    return 0;
}
'''
with tempfile.TemporaryDirectory() as d:
    src = pathlib.Path(d) / 'check.c'
    exe = pathlib.Path(d) / 'check'
    src.write_text(source)
    subprocess.run(['gcc','-std=gnu11','-Wall','-Wextra','-Werror','-Wno-unused-parameter',str(src),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS: visible matching lead only; no switch/link/multi; slide 24 frames, cry once')
