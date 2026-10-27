"""v0.9.0.3.6.2 safety and functional test of the actual C toggle function."""
from pathlib import Path
import re
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
field=(root/'src/fieldmap.c').read_text()
overworld=(root/'src/overworld.c').read_text()
assert '#define PALLET_NIGHT_PREVIEW_MODE 3' in field
assert 'L_BUTTON | R_BUTTON | SELECT_BUTTON' in field
assert 'PalletTown_TryToggleLivePreview(gMain.newKeys, gMain.heldKeys)' in overworld
assert 'gMain.newKeys &= ~(L_BUTTON | R_BUTTON | SELECT_BUTTON)' in overworld
assert 'gPlttBufferFaded[' not in field[field.index('bool8 PalletTown_TryToggleLivePreview'):field.index('static void LoadTilesetPalette')]
assert 'gGlobalFieldTintMode != QL_TINT_NONE' in field
assert 'gQuestLogState == QL_STATE_PLAYBACK_LAST' in field
assert 'ScriptContext2_IsEnabled()' in field
assert 'gWeatherPtr->palProcessingState != WEATHER_PAL_STATE_IDLE' in field
assert 'ApplyWeatherGammaShiftToPal(i)' in field
assert not (root/'src/day_night.c').exists()
assert [x for x in root.rglob('*.md') if x.name!='README.md']==[]

# Compile unchanged extracted production functions against stand-in field context.
def extract(name):
    m=re.search(r'(?:static )?bool8 '+name+r'\([^)]*\)\s*\{',field)
    assert m, name
    st=m.start(); start=m.end()-1; depth=0
    for i in range(start,len(field)):
        if field[i]=='{': depth+=1
        elif field[i]=='}':
            depth-=1
            if depth==0: return field[st:i+1]
    raise ValueError('Unclosed '+name)

c="""
#include <assert.h>
#include <stdint.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef u8 bool8;
#define TRUE 1
#define FALSE 0
#define PALLET_NIGHT_PREVIEW_MODE 3
#define L_BUTTON 0x200
#define R_BUTTON 0x100
#define SELECT_BUTTON 0x4
#define QL_TINT_NONE 0
#define QL_STATE_PLAYBACK 2
#define QL_STATE_PLAYBACK_LAST 3
#define PALLET_TOWN 1
#define MAP_GROUP(x) 3
#define MAP_NUM(x) 4
#define WEATHER_PAL_STATE_IDLE 3
#define NUM_PALS_TOTAL 13
struct Loc {u8 mapGroup, mapNum;};
struct Save {struct Loc location;};
struct Save2 {u16 playTimeHours; u8 playTimeMinutes, playTimeSeconds;};
struct MapLayout {int placeholder;};
struct Map {struct MapLayout *mapLayout;};
struct Weather {u8 palProcessingState, weatherChangeComplete;};
static struct Save save1 = {{3,4}};
static struct Save2 save2;
static struct MapLayout layout;
static struct Weather weather = {3,1};
static struct Save *gSaveBlock1Ptr=&save1;
static struct Save2 *gSaveBlock2Ptr=&save2;
static struct Map gMapHeader;
static struct Weather *gWeatherPtr=&weather;
static struct {u8 active;} gPaletteFade;
static u8 gGlobalFieldTintMode,gQuestLogState, scriptBusy;
static bool8 sPalletLiveDemoNight = FALSE;
static int reloads,gammas;
static bool8 ScriptContext2_IsEnabled(void) {return scriptBusy;}
static void LoadMapTilesetPalettes(struct MapLayout *arg) { assert(arg==&layout); reloads++; }
static void ApplyWeatherGammaShiftToPal(u8 index) {assert(index<13);gammas++;}
"""+extract('PalletTown_IsNightAtSeconds')+'\n'+extract('PalletTown_ShouldLoadNightPalettes')+'\n'+extract('PalletTown_TryToggleLivePreview')+"""
int main(void) {
    const u16 chord=L_BUTTON|R_BUTTON|SELECT_BUTTON;
    gMapHeader.mapLayout=&layout;
    assert(!PalletTown_ShouldLoadNightPalettes());
    assert(!PalletTown_TryToggleLivePreview(0,chord)); // no new keys
    assert(!PalletTown_TryToggleLivePreview(L_BUTTON,L_BUTTON)); // incomplete chord
    assert(PalletTown_TryToggleLivePreview(SELECT_BUTTON,chord));
    assert(sPalletLiveDemoNight && PalletTown_ShouldLoadNightPalettes());
    assert(reloads==1 && gammas==13);
    assert(PalletTown_TryToggleLivePreview(R_BUTTON,chord));
    assert(!sPalletLiveDemoNight && !PalletTown_ShouldLoadNightPalettes());
    assert(reloads==2 && gammas==26);
    gPaletteFade.active=1;
    assert(!PalletTown_TryToggleLivePreview(R_BUTTON,chord));
    gPaletteFade.active=0;
    scriptBusy=1;
    assert(!PalletTown_TryToggleLivePreview(R_BUTTON,chord));
    scriptBusy=0;
    gQuestLogState=QL_STATE_PLAYBACK_LAST;
    assert(!PalletTown_TryToggleLivePreview(R_BUTTON,chord));
    gQuestLogState=0;
    gGlobalFieldTintMode=1;
    assert(!PalletTown_TryToggleLivePreview(R_BUTTON,chord));
    gGlobalFieldTintMode=0;
    weather.palProcessingState=1;
    assert(!PalletTown_TryToggleLivePreview(R_BUTTON,chord));
    weather.palProcessingState=3;
    weather.weatherChangeComplete=0;
    assert(!PalletTown_TryToggleLivePreview(R_BUTTON,chord));
    weather.weatherChangeComplete=1;
    save1.location.mapNum=5;
    assert(!PalletTown_TryToggleLivePreview(R_BUTTON,chord));
    assert(!PalletTown_ShouldLoadNightPalettes());
    save1.location.mapNum=4;
    assert(PalletTown_TryToggleLivePreview(SELECT_BUTTON,chord));
    assert(sPalletLiveDemoNight);
    save1.location.mapNum=5;
    assert(!PalletTown_ShouldLoadNightPalettes());
    assert(!sPalletLiveDemoNight); // leaving Pallet resets demo
    return 0;
}
"""
with tempfile.TemporaryDirectory() as d:
    cfile=Path(d)/'test.c'; cfile.write_text(c)
    subprocess.run(['gcc','-Wall','-Wextra','-Werror','-std=c11',str(cfile),'-o',str(Path(d)/'test')],check=True)
    subprocess.run([str(Path(d)/'test')],check=True)
print('PASS: live toggle, palette reloads, weather gamma, Quest Log guard, fade/script locks, map isolation, reset outside Pallet')
