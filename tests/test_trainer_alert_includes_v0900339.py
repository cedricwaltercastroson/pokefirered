#!/usr/bin/env python3
"""v0.9.0.3.39: ensure Trainer alert callbacks have visible declarations.

The v38 patch declared its functions in following_pokemon.h but omitted that
header from trainer_see.c. agbcc treats the resulting implicit declarations
as fatal warnings. This runs an actual C89 host syntax check with that warning
promoted to an error; it does not substitute for an ARM ROM build.
"""
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / 'src/trainer_see.c').read_text()
header = (ROOT / 'include/following_pokemon.h').read_text()
implementation = (ROOT / 'src/following_pokemon.c').read_text()

assert re.search(r'^#include "following_pokemon.h"$', source, re.M), 'trainer_see.c must include follower prototypes'
for name in ('Follower_BeginTrainerAlertYield', 'Follower_WaitTrainerAlertYield'):
    assert re.search(r'^bool8 ' + name + r'\(u8 trainerId\);$', header, re.M), name
    assert re.search(r'^bool8 ' + name + r'\(u8 trainerId\)\s*\{', implementation, re.M), name
    assert len(re.findall(r'\b' + name + r'\s*\(', source)) == 1, name

cmd = [
    'gcc', '-std=gnu89', '-fsyntax-only', '-Werror=implicit-function-declaration',
    '-Wno-builtin-declaration-mismatch', '-Wno-pointer-to-int-cast',
    '-Wno-int-to-pointer-cast', '-D__CYGWIN__', '-DFIRERED=1',
    '-DREVISION=1', '-DENGLISH=1', '-DMODERN=0',
    '-iquote', 'include', '-Itools/agbcc', '-Itools/agbcc/include',
    'src/trainer_see.c',
]
subprocess.run(cmd, cwd=ROOT, check=True)
print('PASS: Trainer alert declarations visible in C89 translation unit')
