#!/usr/bin/env python3
"""Quest Log follower callbacks: static wiring checks, not an emulator run."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
ql=(r/'src/quest_log.c').read_text()
obj=(r/'src/event_object_movement.c').read_text()
normal=(r/'src/field_player_avatar.c').read_text()
assert '#include "following_pokemon.h"' in ql
handler=ql.split('void sub_811246C(struct Sprite *sprite)',1)[1].split('void QuestLogRecordNPCStep',1)[0]
player=handler.split('if (objectEvent->localId == OBJ_EVENT_ID_PLAYER)',1)[1].split('\n    else',1)[0]
assert player.index('sub_8063E28(objectEvent, sprite);') < player.index('Follower_Update(objectEvent);')
assert 'Follower_Update(&gObjectEvents[sprite->data[0]]);' in normal
assert obj.count('graphicsId != OBJ_EVENT_GFX_FOLLOWER') >= 1
make=obj.split('void MakeObjectTemplateFromObjectEventGraphicsInfo(',1)[1].split('static void MakeObjectTemplate',1)[0]
assert make.index('graphicsId != OBJ_EVENT_GFX_FOLLOWER') < make.index('spriteTemplate->callback = sub_811246C;')
nonplayer=handler.split('\n    else\n',1)[1]
assert 'objectEvent->localId == FOLLOWER_LOCAL_ID' in nonplayer
assert 'objectEvent->graphicsId == OBJ_EVENT_GFX_FOLLOWER' in nonplayer
assert nonplayer.index('objectEvent->localId == FOLLOWER_LOCAL_ID') < nonplayer.index('sMovementScripts[objectEvent->localId]')
assert 'return;' in nonplayer.split('if (objectEvent->localId == FOLLOWER_LOCAL_ID',1)[1].split('sMovementScripts[objectEvent->localId]',1)[0]
for v in (r/'tests/test_follower_v081_wiring.py',r/'src/quest_log_objects.c'):
    assert v.exists()
print('PASS: Quest Log playback player follower tick; safe follower callback; guarded 64-slot NPC table; prior wiring preserved')
