#!/usr/bin/env python3
"""v0.8.22: follower palette stays an ordinary OBJ in horizontal fog."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
code = (root / 'src/following_pokemon.c').read_text()
weather = (root / 'src/field_weather.c').read_text()

# The observed Tower bug is on maps with horizontal fog, not indoor shade.
for floor in ('3F', '4F', '5F', '6F', '7F'):
    assert 'WEATHER_FOG_HORIZONTAL' in (root / f'data/maps/PokemonTower_{floor}/header.inc').read_text()

# FireRed's generic dynamically loaded palette path both fog-whitens idle
# sprites and registers them for special fog whitening on fade-in.
assert 'MarkFogSpritePalToLighten(paletteIndex)' in weather
assert 'BlendPalette(paletteIndex, 16, 12, RGB(28, 31, 28))' in weather

refresh = code.split('void Follower_RefreshPalette(void)\n{', 1)[1].split('\n}\n', 1)[0]
assert 'Follower_GetSelectedPalette()' in refresh  # normal/shiny still sourced correctly
assert 'ApplyGlobalFieldPaletteTint(FOLLOWER_PALETTE_SLOT)' in refresh
assert 'if (GetCurrentWeather() == WEATHER_FOG_HORIZONTAL)' in refresh
fog_branch = refresh.split('if (GetCurrentWeather() == WEATHER_FOG_HORIZONTAL)', 1)[1].split('\n    else\n', 1)[0]
assert 'UpdateSpritePaletteWithWeather' not in fog_branch
assert 'WEATHER_PAL_STATE_SCREEN_FADING_IN' in fog_branch
assert '16 - gWeatherPtr->fadeScreenCounter' in fog_branch
assert 'gWeatherPtr->fadeDestColor' in fog_branch
assert 'WEATHER_PAL_STATE_SCREEN_FADING_OUT' in fog_branch
assert 'gPaletteFade.blendColor' in fog_branch
assert 'UpdateSpritePaletteWithWeather(FOLLOWER_PALETTE_SLOT)' in refresh.split('\n    else\n', 1)[1]

# Battle-return repair must not reapply the incorrect fog whitening.
guard = code.split('void Follower_StabilizeBattleReturnPalette(void)\n{', 1)[1].split('\n}\n', 1)[0]
assert 'GetCurrentWeather() != WEATHER_FOG_HORIZONTAL' in guard
assert 'UpdateSpritePaletteWithWeather(FOLLOWER_PALETTE_SLOT)' in guard

# Dynamic sprites retain their stable palette slot, regardless of weather.
assert '#define FOLLOWER_PALETTE_SLOT 11' in code
assert 'info.paletteSlot = FOLLOWER_PALETTE_SLOT' in code

print('PASS: v0.8.22 horizontal fog palette treatment, fade handling, shiny and battle guard')
