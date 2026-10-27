#!/usr/bin/env python3
"""Static v0.5 integration checks; does not replace an ARM build."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
field = (root / 'src/field_control_avatar.c').read_text()
start = field.index('static const u8 *GetInteractedObjectEventScript(struct MapPosition *position, u8 metatileBehavior, u8 direction)\n{')
end = field.index('static const u8 *GetInteractedBackgroundEventScript(', start)
body = field[start:end]
assert body.index('gObjectEvents[objectEventId].localId == FOLLOWER_LOCAL_ID') < body.index('GetObjectEventScriptPointerByObjectEventId(objectEventId)'), 'Follower could reach missing template'
assert 'gSelectedObjectEvent = objectEventId;' in body
assert 'return EventScript_FollowerTalk;' in body
script = (root / 'data/scripts/follower_interaction.inc').read_text()
expected = ['EventScript_FollowerTalk::', '\tlock', '\tspecial Follower_PrepareInteraction', '\tmsgbox gStringVar4', '\tspecial Follower_EndInteraction', '\trelease', '\tend']
positions = [script.index(x) for x in expected]
assert positions == sorted(positions), 'Interaction script steps out of order'
assert '\tfaceplayer' not in script, 'Generic faceplayer can be lost to an active follower held movement'
follower_code = (root / 'src/following_pokemon.c').read_text()
prepare = follower_code.split('u16 Follower_PrepareInteraction(void)', 1)[1].split('u16 Follower_EndInteraction(void)', 1)[0]
for token in ('gSelectedObjectEvent < OBJECT_EVENTS_COUNT', 'ObjectEventClearHeldMovementIfActive(follower)', 'ObjectEventTurn(follower, GetOppositeDirection(GetPlayerFacingDirection()))', 'Follower_ApplyHorizontalSpacing(follower)', 'FreezeObjectEvent(follower)'):
    assert token in prepare, f'Missing follower-specific interaction turn: {token}'
assert 'data/scripts/follower_interaction.inc' in (root / 'data/event_scripts.s').read_text()
assert 'extern const u8 EventScript_FollowerTalk[];' in (root / 'include/event_scripts.h').read_text()
specials = (root / 'data/specials.inc').read_text()
assert specials.index('def_special Follower_PrepareInteraction') < specials.index('def_special Follower_EndInteraction') < specials.index('gSpecialsEnd::')
assert specials.splitlines().count('\tdef_special Follower_PrepareInteraction') == 1
assert specials.splitlines().count('\tdef_special Follower_EndInteraction') == 1
header = (root / 'include/following_pokemon.h').read_text()
for name in ('Follower_PrepareInteraction', 'Follower_EndInteraction'):
    assert name in header
print('PASS: v0.5 follower interaction script and safe dynamic event lookup')
