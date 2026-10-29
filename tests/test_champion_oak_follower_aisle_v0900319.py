#!/usr/bin/env python3
"""Source-level geometry regression for the Champion Room / Oak entrance.

Run from a FireRed checkout: python tests/test_champion_oak_follower_aisle_v0900319.py
This does NOT emulate sprite animation or replace an in-game visual check.
"""
import json
import os
import re
import struct
from pathlib import Path

PROJECT = Path(__file__).resolve().parents[1]
# For maintainers testing a patch before extraction over the target repository.
BASE = Path(os.environ.get("FIRERED_BASE_SOURCE", PROJECT))
SCRIPT = (PROJECT / "data/maps/PokemonLeague_ChampionsRoom/scripts.inc").read_text()
MAP = json.loads((BASE / "data/maps/PokemonLeague_ChampionsRoom/map.json").read_text())
LAYOUTS = json.loads((BASE / "data/layouts/layouts.json").read_text())["layouts"]
LAYOUT = next(layout for layout in LAYOUTS if layout["id"] == MAP["layout"])
MAP_BYTES = (BASE / LAYOUT["blockdata_filepath"]).read_bytes()
TILES = struct.unpack("<" + "H" * (len(MAP_BYTES) // 2), MAP_BYTES)
WIDTH, HEIGHT = LAYOUT["width"], LAYOUT["height"]


def movement(label):
    marker = label + "::"
    assert SCRIPT.count(marker) == 1, marker
    block = SCRIPT.split(marker, 1)[1].split("step_end", 1)[0]
    return [line.strip() for line in block.splitlines() if line.strip() and not line.lstrip().startswith("@") and not line.lstrip().endswith("::")]


def walk(start, instructions):
    x, y = start
    points = []
    steps = {"walk_left": (-1, 0), "walk_right": (1, 0), "walk_up": (0, -1), "walk_down": (0, 1)}
    for command in instructions:
        if command in steps:
            dx, dy = steps[command]
            x, y = x + dx, y + dy
            points.append((x, y))
    return points


player_entry = movement("PokemonLeague_ChampionsRoom_Movement_PlayerEnter")
player_warp = MAP["warp_events"][0]
player_final = walk((player_warp["x"], player_warp["y"]), player_entry)[-1]
assert player_final == (6, 9), player_final
follower_tile = (6, 10)  # A one-tile trail behind Red after the scripted entrance.

npc_events = MAP["object_events"]
oak_start = next((o["x"], o["y"]) for o in npc_events if o["graphics_id"] == "OBJ_EVENT_GFX_PROF_OAK")
rival_tile = next((o["x"], o["y"]) for o in npc_events if o["graphics_id"] == "OBJ_EVENT_GFX_BLUE")
oak_script = movement("PokemonLeague_ChampionsRoom_Movement_OakEnter")
oak_path = walk(oak_start, oak_script)
assert len(oak_path) == 8, oak_path  # Keep original pacing, same number of walking steps.
assert oak_path[-1] == (5, 8), oak_path[-1]  # Preserve Oak's scene dialogue position.
# In v38, restore Oak's original FireRed movement script. The shared
# scripted-NPC guard now yields the follower before Oak crosses (6, 10).
assert oak_path == [(6, 14), (6, 13), (6, 12), (6, 11),
                    (6, 10), (5, 10), (5, 9), (5, 8)], oak_path
assert follower_tile in oak_path
assert not ({player_final, rival_tile} & set(oak_path)), oak_path
assert "Follower_DeferNpcScriptStep(objEventId, nextMoveActionId)" in (PROJECT / "src/script_movement.c").read_text()

# Check the destination tiles from the actual map layout, not guessed terrain.
for x, y in oak_path:
    assert 0 <= x < WIDTH and 0 <= y < HEIGHT, (x, y)
    tile = TILES[y * WIDTH + x]
    assert (tile & 0x0C00) == 0, (x, y, hex(tile))  # Impassability bits.

# The Hall of Fame route and the existing player/rival choreography are untouched.
assert walk(player_final, movement("PokemonLeague_ChampionsRoom_Movement_PlayerExit"))[-1] == (6, 2)
assert walk((5, 8), movement("PokemonLeague_ChampionsRoom_Movement_OakExit"))[-1] == (6, 2)
assert "warp MAP_POKEMON_LEAGUE_HALL_OF_FAME, 255, 5, 12" in SCRIPT
assert "applymovement OBJ_EVENT_ID_PLAYER, PokemonLeague_ChampionsRoom_Movement_PlayerWatchOakEnter" in SCRIPT
assert "applymovement LOCALID_RIVAL, PokemonLeague_ChampionsRoom_Movement_RivalWatchOakEnter" in SCRIPT
assert "applymovement LOCALID_PROF_OAK, PokemonLeague_ChampionsRoom_Movement_OakEnter" in SCRIPT
print("PASS: restored original Oak route uses shared scripted-NPC follower yielding")
