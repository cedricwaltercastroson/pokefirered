#!/usr/bin/env python3
"""Source-order checks for the v0.8.14 post-battle palette guard."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
overworld = (root / 'src/overworld.c').read_text()
follower = (root / 'src/following_pokemon.c').read_text()
header = (root / 'include/following_pokemon.h').read_text()

assert 'Follower_StabilizeBattleReturnSpriteSlot' in header
assert 'Follower_StabilizeBattleReturnPalette' in header
segment = overworld.split('static void OverworldBasic(void)', 1)[1].split('void CB2_OverworldBasic', 1)[0]
assert segment.index('AnimateSprites();') < segment.index('Follower_StabilizeBattleReturnSpriteSlot();') < segment.index('BuildOamBuffer();')
assert segment.index('UpdatePaletteFade();') < segment.index('Follower_StabilizeBattleReturnPalette();') < segment.index('UpdateTilesetAnimations();')
assert 'FOLLOWER_BATTLE_PALETTE_GUARD_FRAMES 90' in follower
assert 'sBattlePaletteGuardFrames = 0; // No visible follower needs guarding.' in follower
assert 'BlendPalette(0x100 + FOLLOWER_PALETTE_SLOT * 16' in follower
assert 'gPaletteFade.blendColor' in follower
assert 'UpdateSpritePaletteWithWeather(FOLLOWER_PALETTE_SLOT);' in follower
assert 'gGlobalFieldTintMode != QL_TINT_NONE' in follower
# This must remain a brief post-battle-only repair, not a global recolouring.
prime = follower.split('void Follower_PrimeForOverworldDisplay(void)', 1)[1].split('void Follower_BeginMapConnectionTransition', 1)[0]
assert 'if (sBattleReturnPending && !sStairWarpRecall)' in prime
assert 'sBattlePaletteGuardFrames = 0; // No visible follower needs guarding.' in prime
assert 'sBattlePaletteGuardFrames = 0;' in follower.split('void Follower_ResetForMapLoad(void)', 1)[1].split('static bool8 Follower_TryEnqueue', 1)[0]
# No changes to existing v0.8.11 palette-ownership protections.
engine = (root / 'src/event_object_movement.c').read_text()
assert 'gReservedSpritePaletteCount = 12' in engine
assert 'graphicsInfo->paletteSlot == 10 && objectEvent->graphicsId != OBJ_EVENT_GFX_FOLLOWER' in engine
print('PASS: v0.8.14 fade/OAM call ordering, disabled palette guard during invisible battle recall, v0.8.11 slot ownership')
