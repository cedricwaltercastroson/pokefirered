#!/usr/bin/env python3
"""Static and host-C regression for v0.9.0.3.20 anime-inspired rematch parties."""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/battle_main.c').read_text()
species_defs = (root / 'include/constants/species.h').read_text()
constants = {
    name: int(value)
    for name, value in re.findall(r'^#define (SPECIES_\w+)\s+(\d+)\s*$', species_defs, re.M)
}
start = source.index('static const u16 sGymRematchSpecies[8][4][6] = {')
end = source.index('\n};', start) + len('\n};')
matrix = source[start:end]
rows = []
for line in matrix.splitlines():
    found = re.findall(r'SPECIES_[A-Z0-9_]+', line)
    if found:
        assert len(found) == 6, ('Incomplete team', line)
        assert all(s in constants and 1 <= constants[s] <= 151 for s in found), line
        assert len(set(found)) == 6, ('Duplicate species', line)
        rows.append(found)
assert len(rows) == 32, len(rows)
teams = [rows[i*4:(i+1)*4] for i in range(8)]
assert all(len(set(tuple(team) for team in group)) == 4 for group in teams)
ace = ['ONIX','GYARADOS','RAICHU','GLOOM','VENOMOTH','KADABRA','MAGMAR','RHYDON']
for i, group in enumerate(teams):
    # Anime ace on highest-level slot; early Misty ace is Starmie,
    # early Koga ace is Golbat, until their evolved/final teams unlock.
    expected_ace = ('SPECIES_'+ace[i])
    if i in (1,4):
        assert group[2][5] == group[3][5] == expected_ace
    else:
        assert all(t[5] == expected_ace for t in group)

# Preserve distinctive anime-owned off-type species, without using Mewtwo.
assert all('SPECIES_VULPIX' in t for t in teams[0])
assert all('SPECIES_SCYTHER' in t for t in teams[4])
assert all('SPECIES_RHYDON' in t for t in teams[6])
for t in teams[7]:
    assert 'SPECIES_PERSIAN' in t and 'SPECIES_KINGLER' in t
assert 'SPECIES_MACHAMP' in teams[7][3]
assert 'SPECIES_MEWTWO' not in matrix

# This is the only target change to the rematch builder; save tiers and wild
# isolation remain the same as v0.9.0.3.19.
start_fn = source.index('static u8 CreateGymRematchParty(')
end_fn = source.index('static u8 CreateNPCTrainerParty(', start_fn)
body = source[start_fn:end_fn]
assert 'VarGet(VAR_GYM_REMATCH_BROCK + gym)' in body
assert 'if (tier > 3)' in body
assert 'u8 level = 49 + tier * 3 + i;' in body
assert '(gBattleTypeFlags & BATTLE_TYPE_TRAINER)' in source[end_fn:end_fn+900]
assert 'trainerNum >= TRAINER_GYM_REMATCH_BROCK' in source[end_fn:end_fn+900]
assert 'trainerNum <= TRAINER_GYM_REMATCH_GIOVANNI' in source[end_fn:end_fn+900]
assert [(49 + t*3, 54 + t*3) for t in range(4)] == [(49,54),(52,57),(55,60),(58,63)]

# Compile and run a real host-side stub exercising all 32 parties against
# the unchanged C rematch generator, including persisted-tier clamp.
gcc = shutil.which('gcc')
if gcc:
    defs = '\n'.join(f'#define {s} {v}' for s,v in constants.items())
    c = f'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char u8;
typedef unsigned short u16;
#define FALSE 0
#define OT_ID_RANDOM_NO_SHINY 0
#define VAR_GYM_REMATCH_BROCK 100
#define TRAINER_GYM_REMATCH_BROCK 1000
{defs}
struct Pokemon {{ u16 species; u8 level; }};
static struct Pokemon gParty[6];
static u16 currentTier;
static u16 VarGet(u16 var) {{ return currentTier; }}
static void ZeroEnemyPartyMons(void) {{ memset(gParty, 0, sizeof(gParty)); }}
static void CreateMon(struct Pokemon *p, u16 species, u8 level, int iv,
                      int fixed, int personality, int ot, int seed) {{
    p->species=species; p->level=level;
}}
{matrix}
{body}
int main(void) {{
 u16 gym, tier, mon;
 for (gym=0;gym<8;gym++) {{
   for (tier=0;tier<4;tier++) {{
     currentTier=tier;
     if(CreateGymRematchParty(gParty,TRAINER_GYM_REMATCH_BROCK+gym)!=6) return 1;
     for(mon=0;mon<6;mon++) {{
       if(!gParty[mon].species || gParty[mon].species>151) return 2;
       if(gParty[mon].level != 49+tier*3+mon) return 3;
     }}
   }}
 }}
 currentTier=65535;
 CreateGymRematchParty(gParty,TRAINER_GYM_REMATCH_BROCK);
 if (gParty[0].level!=58 || gParty[5].level!=63) return 4;
 puts("PASS: 8 leaders x 4 tier C runtime; Kanto roster; 49-63 levels; saved-tier clamp");
 return 0;
}}
'''
    with tempfile.TemporaryDirectory() as td:
        src = Path(td) / 'rematch.c'
        out = Path(td) / 'rematch'
        src.write_text(c)
        subprocess.run([gcc,'-std=gnu89','-Wall','-Wextra','-Wno-unused-parameter','-Werror','-o',str(out),str(src)],check=True,text=True)
        subprocess.run([str(out)],check=True)
print('PASS: 32 unique teams, 151-only, anime signatures and off-type exceptions, tier formula and battle flag isolation')
