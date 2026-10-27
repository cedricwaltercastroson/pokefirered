#!/usr/bin/env python3
"""Asset and animation index audit for v0.6.1; not an emulator or GBA build.

This checks the static frame references that the FireRed object-event engine
will use for facing/walking, and PNG/<gba 4bpp> frame capacity for all 151.
"""
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[1]
SRC = (ROOT / "src/following_pokemon.c").read_text()
ART = (ROOT / "src/data/follower_art.h").read_text()
ASSETS = ROOT / "graphics/object_events/pics/followers"

unique_east = {"FARFETCHD", "KINGLER", "CUBONE", "MAROWAK", "WEEZING"}
frames = {}
for species, definition in re.findall(
    r"static const struct SpriteFrameImage sFollowerFrames_([A-Z0-9_]+)\[\] = \{(.*?)\n\};",
    ART, re.S
):
    numbers = [int(n) for n in re.findall(r"overworld_frame\([^,]+, 4, 4, (\d+)\)", definition)]
    assert numbers == list(range(len(numbers))), (species, numbers)
    frames[species] = len(numbers)

assert len(frames) == 151, len(frames)
assert {name for name, count in frames.items() if count == 8} == unique_east
assert sum(n == 6 for n in frames.values()) == 146
assert set(frames.values()) == {6, 8}

# Check the real source sprite dimension and compiled 4bpp frame allocation.
for species, count in frames.items():
    # Source paths are named in the INCBIN declarations, so this works even
    # for species with underscores in their C names (e.g. MR_MIME).
    match = re.search(
        rf'sFollowerPic_{species}\[\] = INCBIN_U32\("([^\"]+)"\)', ART
    )
    assert match, species
    filename = match.group(1)
    png_path = ROOT / filename.replace('.4bpp', '.png')
    bin_path = ROOT / filename
    assert png_path.is_file() and bin_path.is_file(), species
    png = png_path.read_bytes()
    assert png[:8] == b'\x89PNG\r\n\x1a\n', species
    width, height = struct.unpack('>II', png[16:24])
    assert (width, height) == (32, 32 * count), (species, width, height)
    assert bin_path.stat().st_size == 512 * count, species

# Unique right-facing art: frame 6 is the *still* east pose; frame 7 is
# movement-only. Previously face east alternated both frames indefinitely.
face = SRC.split('static const union AnimCmd sFollowUniqueEastFace[] =', 1)[1].split('};', 1)[0]
assert 'ANIMCMD_FRAME(6, 12)' in face
assert 'ANIMCMD_END' in face
assert 'ANIMCMD_FRAME(7,' not in face
assert 'ANIMCMD_JUMP' not in face
for speed in (8, 4, 2):
    motion = SRC.split(f'static const union AnimCmd sFollowUniqueEastWalk{speed}[] =', 1)[1].split('};', 1)[0]
    assert f'ANIMCMD_FRAME(6, {speed})' in motion
    assert f'ANIMCMD_FRAME(7, {speed})' in motion

# FireRed indexes animations in 6 groups of four directions: facing,
# normal walk, fast, faster, fastest and jump/run. Each table must hold all
# 24 walk/facing entries followed by 4 SPIN action entries 0x18..0x1B.
# Only walking indices ending in 3 use dedicated east frames.
for array in ('sFollowerAnims', 'sFollowerAnimsUniqueEast'):
    definition = SRC.split(f'static const union AnimCmd *const {array}[] =', 1)[1].split('};', 1)[0]
    entries = re.findall(r'\bsFollow\w+\b', definition)
    assert len(entries) == 28, (array, len(entries))
    for i, symbol in enumerate(entries[:24]):
        if i % 4 == 3:
            if array == 'sFollowerAnimsUniqueEast':
                assert 'UniqueEast' in symbol, (i, symbol)
            else:
                assert 'East' in symbol and 'UniqueEast' not in symbol, (i, symbol)
        else:
            assert 'East' not in symbol, (i, symbol)

    for suffix, symbol in zip(('Down','Up','Left','Right'), entries[24:]):
        assert symbol.endswith(suffix), (array, suffix, symbol)
assert 'FOLLOWER_SIDE_SPACING_PIXELS 8' in SRC
assert 'void Follower_FinishMapConnectionTransition(void)' in SRC
assert 'ObjectEventTurn(follower, GetOppositeDirection(GetPlayerFacingDirection()))' in SRC
print('PASS: 151 sprites; 24 walking + 4 spinning animations; dedicated east idle pose')
