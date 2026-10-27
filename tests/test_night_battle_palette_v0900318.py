"""Source regression: 0.9.0.3.18 modifies only terrain BG, not battle UI/OBJ."""
from pathlib import Path
s = (Path(__file__).resolve().parents[1] / 'src/battle_bg.c').read_text()
f = (Path(__file__).resolve().parents[1] / 'src/fieldmap.c').read_text()
assert s.count('TimeOfDay_ApplyNightBattleTerrainPalette();') == 2
assert 'if ((gBattleTypeFlags & BATTLE_TYPE_LINK) || !TimeOfDay_IsNightForFieldEffects())' in s
assert 'for (i = 0x20; i < 0x50; i++)' in s
assert 'gPlttBufferUnfaded[i] = colour;' in s
assert 'gPlttBufferFaded[i] = colour;' in s
assert 'LoadCompressedPalette(sBattleTerrainTable[terrain].palette, 0x20, 0x60);\n    TimeOfDay_ApplyNightBattleTerrainPalette();' in s
assert 'LoadCompressedPalette(sBattleTerrainTable[battleScene].palette, 0x20, 0x60);\n        TimeOfDay_ApplyNightBattleTerrainPalette();' in s
assert 'return TimeOfDay_GetDesiredState() == TOD_NIGHT;' in f
assert 'seconds % 3600' in f
print('PASS: nighttime battle palette routing, palette bank isolation, clock sharing')
