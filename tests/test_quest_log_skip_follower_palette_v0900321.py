#!/usr/bin/env python3
"""Legacy v0.9.0.3.21 skip wiring check.

The v0.9.0.3.21 host-only helper was incomplete: it assumed an initialized
follower and did not validate the live sprite's OBJ palette assignment.
The real-module runtime regression now lives in the v0.9.0.3.22 test.
"""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
quest = (root / 'src/quest_log.c').read_text()
overworld = (root / 'src/overworld.c').read_text()
assert 'if (skipped)\n            Follower_ArmQuestLogSkipPaletteRepair();' in quest
assert 'Follower_RepairQuestLogSkipPalette();' in overworld
subprocess.run(['python', 'tests/test_quest_log_skip_follower_palette_v0900322.py'], cwd=root, check=True)
print('PASS: legacy .21 skip wiring retained; .22 real-module regression supersedes synthetic harness')
