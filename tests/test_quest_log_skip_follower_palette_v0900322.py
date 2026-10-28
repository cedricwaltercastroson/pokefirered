#!/usr/bin/env python3
"""Quest Log B-skip palette + OAM readiness regression, v0.9.0.3.22.

The prior .21 fix tested only palette buffer refresh with an initialized
sprite, so it missed both deferred creation and a wrong OAM paletteNum.
"""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
quest = (root / 'src/quest_log.c').read_text()
overworld = (root / 'src/overworld.c').read_text()
follower = (root / 'src/following_pokemon.c').read_text()
header = (root / 'include/following_pokemon.h').read_text()

assert 'if (skipped)\n            Follower_ArmQuestLogSkipPaletteRepair();' in quest
assert 'void Follower_RepairQuestLogSkipSpriteSlot(void);' in header
assert 'void Follower_RepairQuestLogSkipPalette(void);' in header

basic = overworld.split('static void OverworldBasic(void)\n{',1)[1].split('\n}',1)[0]
assert basic.index('Follower_RepairQuestLogSkipSpriteSlot();') < basic.index('BuildOamBuffer();')
assert basic.index('UpdatePaletteFade();') < basic.index('Follower_RepairQuestLogSkipPalette();')
assert overworld.count('Follower_RepairQuestLogSkipSpriteSlot();') == 1
assert overworld.count('Follower_RepairQuestLogSkipPalette();') == 1

ready = follower.split('static bool8 Follower_GetQuestLogSkipSpriteId(u8 *spriteId)',1)[1].split('void Follower_ArmQuestLogSkipPaletteRepair',1)[0]
assert 'sFollowerSpecies == SPECIES_NONE' in ready
assert 'gObjectEvents[followerId].spriteId >= MAX_SPRITES' in ready
assert 'gObjectEvents[followerId].graphicsId != OBJ_EVENT_GFX_FOLLOWER' in ready
assert 'sQuestLogSkipPaletteFrames = 0' not in ready
repair = follower.split('void Follower_RepairQuestLogSkipPalette(void)\n{',1)[1].split('\n}',1)[0]
assert repair.index('if (!Follower_GetQuestLogSkipSpriteId(&spriteId))') < repair.index('sQuestLogSkipPaletteFrames--;')
assert repair.index('gSprites[spriteId].oam.paletteNum = FOLLOWER_PALETTE_SLOT;') < repair.index('Follower_RefreshPalette();')

# Compile actual follower C module against the established host mocks and
# execute the new missing-sprite / wrong-OAM / bounded-work scenarios.
subprocess.run(['bash','tests/run_follower_smoke.sh'],cwd=root,check=True)
print('PASS: B-skip requires ready sprite, fixes OAM slot before OAM copy and restores colours')
