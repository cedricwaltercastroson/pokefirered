"""Historical v0.8.8 spin-animation assets audit.

The original spin-follow logic was replaced by v0.8.9 recall-during-spin.
The added four animation entries remain available and correctly indexed, but
must no longer be dispatched by the live follower's movement trail.
"""
from pathlib import Path
import re
s = Path('src/following_pokemon.c').read_text()
engine = Path('src/event_object_movement.c').read_text()
for table in ('sFollowerAnimsUniqueEast', 'sFollowerAnims'):
    block = s.split('static const union AnimCmd *const ' + table + '[] =', 1)[1].split('};', 1)[0]
    items = re.findall(r'sFollow(?:erSpin)?[A-Za-z0-9]+', block)
    assert len(items) == 28, (table, len(items))
    assert all(items[24 + i].endswith(suffix) for i, suffix in enumerate(['Down', 'Up', 'Left', 'Right']))
assert 'FOLLOW_SPIN(Unique, 0, 2, 4, 6, FALSE)' in s
assert 'FOLLOW_SPIN(Mirrored, 0, 2, 4, 4, TRUE)' in s
assert 'return gUnknown_83A6465[direction];' in engine
assert '[DIR_SOUTH]     = 0x18' in engine
assert '[DIR_EAST]      = 0x1B' in engine
assert 'movementAction = GetSpinMovementAction(direction);' not in s
print('v0.8.8 native spin asset compatibility retained, live spin dispatch disabled')
