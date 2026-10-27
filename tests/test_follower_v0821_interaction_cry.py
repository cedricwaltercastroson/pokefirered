#!/usr/bin/env python3
"""v0.8.21 interaction-cry wiring: compile/runtime checks live in host smoke."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
script = (root / 'data/scripts/follower_interaction.inc').read_text()
code = (root / 'src/following_pokemon.c').read_text()
header = (root / 'include/following_pokemon.h').read_text()
specials = (root / 'data/specials.inc').read_text()
smoke = (root / 'tests/follower_runtime_smoke.c').read_text()
field = (root / 'src/field_control_avatar.c').read_text()

commands = ('\tlock', '\tspecial Follower_PrepareInteraction',
            '\tspecial Follower_PlayInteractionCry', '\tmsgbox gStringVar4',
            '\twaitmoncry', '\tspecial Follower_EndInteraction',
            '\trelease', '\tend')
assert [script.index(t) for t in commands] == sorted(script.index(t) for t in commands)
assert script.count('\tspecial Follower_PlayInteractionCry') == 1
assert 'return EventScript_FollowerTalk;' in field
assert 'u16 Follower_PlayInteractionCry(void);' in header
assert specials.splitlines().count('\tdef_special Follower_PlayInteractionCry') == 1
assert specials.index('def_special Follower_PlayInteractionCry') > specials.index('def_special Follower_EndHealingRecall')

cry = code.split('u16 Follower_PlayInteractionCry(void)', 1)[1].split('u16 Follower_EndInteraction(void)', 1)[0]
for check in ('sInteractionActive', 'gSelectedObjectEvent >= OBJECT_EVENTS_COUNT',
              'follower->active', 'follower->isPlayer',
              'follower->localId != FOLLOWER_LOCAL_ID',
              'follower->graphicsId != OBJ_EVENT_GFX_FOLLOWER',
              'Follower_GetLeadPartyIndex()', 'slot == PARTY_SIZE',
              'MON_DATA_SPECIES', 'species != sFollowerSpecies',
              'PlayCry7(species, 0)'):
    assert check in cry, check
assert code.count('PlayCry7(') == 1, 'Do not add cry sounds to other callbacks'
assert 'mockLastCrySpecies == SPECIES_CHARMANDER' in smoke
assert 'mockShiny = TRUE;' in smoke
assert 'mockSpecies = SPECIES_BULBASAUR' in smoke
assert 'mockCryCount == before' in smoke
print('PASS: v0.8.21 cry only on safe explicit interactions')
