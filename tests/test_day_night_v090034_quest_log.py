"""Regression: Quest Log owns the palette during replay and saved-game scene."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
s=(r/'src/day_night.c').read_text()
q=(r/'src/quest_log.c').read_text()
w=(r/'src/overworld.c').read_text()
assert '#include "quest_log.h"' in s
assert '#include "overworld.h"' in s
fn=s.split('void DayNight_ApplyOverworldPalette(void)',1)[1]
for token in ('gQuestLogState == QL_STATE_PLAYBACK',
              'gQuestLogState == QL_STATE_PLAYBACK_LAST',
              'gGlobalFieldTintMode != QL_TINT_NONE'):
    assert token in fn, token
assert fn.index('gQuestLogState == QL_STATE_PLAYBACK') < fn.index('DynamicOverworldWeather_IsEligibleMap()')
assert 'gGlobalFieldTintMode = QL_TINT_GRAYSCALE;' in w
assert 'gGlobalFieldTintMode = QL_TINT_BACKUP_GRAYSCALE;' in w
assert 'gQuestLogState = QL_STATE_PLAYBACK_LAST;' in q
vblank=w.split('static void VBlankCB_Field(void)\n{',1)[1].split('\n}',1)[0]
assert vblank.index('TransferPlttBuffer();') < vblank.index('DayNight_ApplyOverworldPalette();')
assert 'src/day_night.o(.text)' in (r/'ld_script.txt').read_text()
print('PASS: quest log replay/final-scene palette guard runs before night transform')
