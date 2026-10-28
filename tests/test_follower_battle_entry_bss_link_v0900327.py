#!/usr/bin/env python3
"""Regression for the FireRed custom LD script dropping module .data.

The follower entry token must be zero-initialized and link from .bss;
this test intentionally checks the compiled ELF object section and fails for
v0.9.0.3.26's nonzero PARTY_SIZE initializer.
"""
import pathlib
import re
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
source = (ROOT / "src/following_pokemon.c").read_text()
linker = (ROOT / "ld_script.txt").read_text()

assert "src/following_pokemon.o(.text);" in linker
assert "src/following_pokemon.o(.rodata);" in linker
assert "src/following_pokemon.o" in (ROOT / "sym_bss.txt").read_text()

match = re.search(r"^static u8 sBattleEntryFollowerPartyIndex\s*(?:=\s*[^;]+)?;", source, re.MULTILINE)
assert match is not None, "Missing battle entry state"
declaration = match.group(0)
assert declaration == "static u8 sBattleEntryFollowerPartyIndex;", (
    "Battle entry state must be zero-initialized: initialized .data is discarded by FireRed's custom linker"
)
assert "sBattleEntryFollowerPartyIndex = 0;" in source
assert "sBattleEntryFollowerPartyIndex = partyIndex + 1;" in source
assert "(partyIndex + 1) == sBattleEntryFollowerPartyIndex" in source

with tempfile.TemporaryDirectory() as temp:
    temp = pathlib.Path(temp)
    fixture = temp / "section.c"
    obj = temp / "section.o"
    fixture.write_text("typedef unsigned char u8;\n"
                       + declaration + "\n"
                       + "u8 *get_state_address(void) {return &sBattleEntryFollowerPartyIndex;}\n")
    subprocess.run(["gcc", "-O2", "-std=gnu89", "-c", str(fixture), "-o", str(obj)], check=True)
    result = subprocess.run(["nm", str(obj)], capture_output=True, text=True, check=True)
    matches = [line.split() for line in result.stdout.splitlines() if line.endswith(" sBattleEntryFollowerPartyIndex")]
    assert len(matches) == 1, result.stdout
    assert matches[0][-2] in ("b", "B"), (
        "Follower battle entry state was allocated in initialized .data instead of .bss: " + result.stdout
    )

print("PASS: following_pokemon battle-entry token is in .bss and preserves party indexes 0..5")
