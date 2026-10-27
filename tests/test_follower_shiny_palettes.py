#!/usr/bin/env python3
"""Asset-format and animation-table sanity checks; not visual approval."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'graphics/object_events/pics/followers'
SHINY = BASE / 'shiny'
header = (ROOT / 'src/data/follower_shiny_palettes.h').read_text()
normal_header = (ROOT / 'src/data/follower_art.h').read_text()
normal = re.findall(r'^\s*\[\d+\] = \{sFollowerFrames_[A-Z0-9_]+, sFollowerPal_[A-Z0-9_]+\},', normal_header, re.M)
assert len(normal) == 151, f'expected 151 base sprite definitions, got {len(normal)}'
assert len(re.findall(r'INCBIN_U16\("graphics/object_events/pics/followers/shiny/[^\"]+\.gbapal"\)',header))==151
assert len(re.findall(r'^\s*\[\d+\] = sFollowerShinyPal_',header,re.M)) == 151
normal_png={p.stem for p in BASE.glob('*.png')}
shiny_pal={p.stem for p in SHINY.glob('*.pal')}
shiny_bin={p.stem for p in SHINY.glob('*.gbapal')}
assert len(normal_png)==151 and shiny_pal==normal_png and shiny_bin==normal_png
for name in normal_png:
    src = (SHINY / f'{name}.pal').read_bytes()
    assert b'\r\n' in src and src.replace(b'\r\n',b'').find(b'\n') == -1, name
    lines = src.decode('ascii').splitlines()
    assert lines[:3]==['JASC-PAL','0100','16'] and len(lines)==19, name
    cols = [[int(i) for i in line.split()] for line in lines[3:]]
    assert all(len(c)==3 and all(0<=v<=255 for v in c) for c in cols), name
    gfx = (SHINY / f'{name}.gbapal').read_bytes()
    assert len(gfx)==32, name
    for i,c in enumerate(cols):
        pal=int.from_bytes(gfx[i*2:i*2+2],'little')
        expect=sum(((v*31+127)//255) << shift for v,shift in zip(c,(0,5,10)))
        assert pal==expect, (name,i,pal,expect)
    # Existing walking artwork is not replaced by shiny-specific frame sheets.
    assert (BASE / f'{name}.4bpp').exists(), name

assert 'sFollowerAnimsUniqueEast' in (ROOT / 'src/following_pokemon.c').read_text()
assert 'FOLLOWER_FORCE_SHINY_PREVIEW 0' in (ROOT / 'include/following_pokemon.h').read_text()
print('PASS: 151 paired shiny palettes, GBA 4bpp frames, and preview default off')
