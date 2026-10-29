"""v0.9.0.3.35: two-tile visual clearance for scripted and sighted NPCs.

Host-side regression, not proof of in-game sprite rendering.
"""
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
source = (root / 'src/following_pokemon.c').read_text()
assert 'static u8 sNpcYieldExtraDirection;' in source
assert 'sNpcYieldExtraDirection = i < 4 ? choice : DIR_NONE;' in source
assert 'if (sNpcYieldExtraDirection != DIR_NONE)' in source
assert 'for (i = 0; i < 8; i++)' in source
assert 'Follower_ClearTrail();' in source
# Both tests compile the REAL extracted C yield function, and exercise a
# two-step rear approach plus the one-step narrow-corridor fallback.
for script in ('test_cutscene_choreography_v0900332.py',
               'test_trainer_sight_follower_yield_v0900334.py'):
    subprocess.run([sys.executable, str(root / 'tests' / script)], cwd=root, check=True)
print('PASS: two-tile visual clearance and fallback regressions')
