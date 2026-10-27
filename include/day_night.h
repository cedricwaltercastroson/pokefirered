#ifndef GUARD_DAY_NIGHT_H
#define GUARD_DAY_NIGHT_H

// v0.9.0: Outdoor palette-only day/night lighting, derived from saved playtime.
// Called only from the overworld VBlank callback after TransferPlttBuffer().
void DayNight_ApplyOverworldPalette(void);

#endif // GUARD_DAY_NIGHT_H
