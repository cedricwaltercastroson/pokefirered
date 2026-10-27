# FireRed Followers v0.9.0.3.17 — Faster, Seamless Day/Night Fade (TEST)

**Base:** v0.9.0.3.16 30/30 Day/Night Timing. **Status:** Candidate, not a locked stable release. The last user-locked stable build is v0.9.0.3.12 and is unchanged.

## What changes

- **Automatic day/night visual fade shortened from roughly 2 seconds to roughly 1 second total**, using FireRed's existing native two-stage fade (delay 5 → delay 1). The screen fades to black, switches BG palettes immediately, and fades back in, without a deliberate wait while black.
- The world and character animations continue during the fade. Player input remains briefly guarded during the swap, so scripts/warps cannot hijack the field palette engine mid-transition. This is a fast visual transition, **not a fully walkable crossfade**.

- **30 minutes of active gameplay DAY, then 30 minutes NIGHT**, repeated (60 minutes per complete cycle).
- In-game day is still **08:00–19:59**; night is **20:00–07:59**. Only the accelerated clock speed changes: two virtual minutes per five real gameplay seconds.
- **Preserves the v0.9.0.3.15 weather edge-case fix.** A stale `weatherChangeComplete` flag no longer blocks the lighting switch when `currWeather == nextWeather`. A *real* weather or palette-processing transition still postpones the fade safely.
- Pallet Town bed `Sleep Until Day/Night` still positions the virtual clock at 08:00/20:00 and resumes the normal automatic cycle. The existing saved event variable `VAR_SLEEP_TIME_OFFSET` continues to hold virtual-minute offsets; recorded gameplay hours are not changed and no save-block layout is changed.
- Saves from the 60/60 version remain readable; on upgrading, the selected ambient time can immediately differ because the same recorded playtime now maps to a twice-as-fast clock. Use the bed once to resynchronize to your desired period.
- The virtual-minute resolution of the saved bed offset means a sleep-initiated boundary can be off by roughly 1–2 real seconds. This does not accumulate, and normal clock thresholds without a bed adjustment are exact.

## Preserved and unchanged

- **Exact approved Pokémon Crystal-inspired nighttime palette assets**, and original daytime palettes.
- Outdoor Kanto and all four outdoor Safari Zone day/night coverage; interiors and special-effect areas protected.
- The same native fade-out/swap/fade-in at **approximately one second**, with no separate black-screen hold.
- Quest Log grayscale/sepia ownership and nighttime's seamless first-frame return after save/load.
- Night tall-grass field-effect palette; active weather and 20% combined precipitation selection (17% rain, 3% storm); Safari outdoor weather.
- Pallet Town bed interaction, fade, Pokémon Center healing jingle and saved time selection.
- Pokémon League scripted follower behavior, Hall of Fame pre-machine recall, and Route 21 stationary Mew roamer.
- No continuous global VBlank palette filter; no modifications to pinned third-party dependencies.

## Build (Cygwin, existing agbcc setup)

Use either the complete-source ZIP by itself, or extract the incremental patch over **v0.9.0.3.16** only. Do not apply this patch over v0.9.0.3.12, v0.9.0.3.15 or any other version.

```sh
make -j"$(nproc)" firered_rev1
```

The generated ROM should be `pokefirered_rev1.gba` if your existing build setup is configured correctly. A full ARM/GBA build was not available in the development environment for this candidate.

## In-game acceptance tests

1. **Back up the `.sav`.** Sleep Until Day in the upstairs Pallet Town bedroom. Exit outside. After roughly 30 minutes of active gameplay, confirm a smooth ~1-second fade into NIGHT, with no stuck controls. After another 30 minutes, confirm DAY.
2. Sleep Until Night and ensure the reverse happens after roughly 30 minutes. Save/reload to confirm the chosen time survives Quest Log playback **without a daylight flash**.
3. Test a day/night boundary in rainy weather and in the Safari Zone. A genuine weather transition can briefly defer the lighting fade; a stale `weatherChangeComplete` flag must not block it indefinitely.
4. Recheck followers through League scripted movement and Hall of Fame recall; test night tall grass and water effects; confirm the Mew roamer remains unchanged.
5. Verify daytime colours, night palettes, and special interiors/caves remain correct.

## Test results

- Compiled C tests of clock thresholds (30/60/90 minutes), fade ownership, stale-weather completion, Quest Log first-frame palettes, and saved bed-clock phase passed.
- The inherited v0.9.0.3.16 clock harness confirms 30/30 timing, persisted bed choice and long-playtime modulo arithmetic.
- A new v0.9.0.3.17 fade regression verifies the exact faster fade setting, continuous native two-stage ownership, proper input release, and absence of black-screen wait.
- Source suite has five longstanding follower assertions previously failing in v0.9.0.3.15; they are unrelated to the clock change. Full native ARM link and in-game shortened-fade testing remain unverified.

**Documentation policy:** This is the only Markdown file in the source archive.


---

# Current stable release — FireRed Followers v0.9.0.3.18

**Status: USER-TESTED AND LOCKED.** The preceding v0.9.0.3.17 section is retained above as the historical release notes for that candidate; its old “candidate” label and build notes do **not** describe the current v0.9.0.3.18 release. The v0.9.0.3.18 Night Battle Palette candidate was subsequently reported working perfectly in-game and approved as the new stable baseline. Future changes should start from a copy of v0.9.0.3.18, not edit the archived stable source in place.

## Consolidated mod features through v0.9.0.3.18

### Following Pokémon and movement

- Following Pokémon companion behaviour integrated into the FireRed overworld, with dedicated movement, interaction, animation and shiny-palette handling.
- Follower restoration and positioning across map transitions, normal building entrances/exits, doors, stairs, spinning tiles and other special movement/warp cases.
- Special handling for transitions involving battles, healing, scripted cutscenes, Quest Log playback, Safari Zone travel and Pokémon League sequences.
- Companion recall/release handling around Fly, Teleport, Escape Rope and other field-travel effects, including delayed return so the follower does not appear in front of the player or during a transition.
- Surf/return and ledge/weather edge-case handling, plus follower cry/audio and interaction integration.
- Pokémon League scripted follower movement and a pre-Hall-of-Fame-machine follower recall.

### World lighting and in-game time

- Pokémon Crystal-inspired **nighttime overworld palettes** for eligible outdoor Kanto locations; original daytime colours retained.
- Night lighting supported across all four outdoor Safari Zone areas; protected indoor/special-effect maps retain appropriate original handling.
- **30 real gameplay minutes of DAY, then 30 minutes of NIGHT**, repeating every 60 minutes. The accelerated virtual clock uses 08:00–19:59 for day and 20:00–07:59 for night.
- Faster, native two-stage automatic day/night fade (approximately **one second total**), with character/world animation continuing while player control is briefly guarded for the palette swap.
- Corrected night rendering for tall-grass field effects; preserves active weather effects and Quest Log grayscale/sepia ownership.
- Save/load and Quest Log recovery safeguards against brief incorrect daytime/nighttime palette flashes.
- Weather-transition edge-case correction prevents a stale completion flag from indefinitely blocking a time-of-day lighting change.

### Night battle palette — added in v0.9.0.3.18

- Outdoor **wild and trainer battles** receive a subdued, cooler, darker nighttime **battle-terrain background** treatment when the existing night eligibility rules permit it.
- Both normal and staged battle terrain palette-loading paths are covered.
- Daytime, indoor and link battles retain their original colours; battle text, interface, menu frames, HP bars and Pokémon/trainer sprite OBJ palettes are not deliberately recoloured.
- Treatment is applied only to battle background palette banks **2–4**, using R 18/31, G 23/31 and B 27/31 scaling. It is a separate Crystal-inspired grade, **not** a byte-for-byte use of the overworld's nighttime palette artwork.
- Battle lighting follows the existing 30/30 cycle and sleep-set virtual time, without adding a second clock implementation.

### Dynamic weather and world events

- Outdoor dynamic precipitation selection with **20% combined rain/storm** chance (**17% rain, 3% storm**), and Safari Zone outdoor weather handling.
- Weather/lighting transition coordination, including safeguards for active weather palette processing.
- **Stationary Route 21 Mew roaming encounter** as incorporated in the prior release line.

### Pallet Town sleep interaction

- Upstairs Pallet Town bed offers **Sleep Until Day / Sleep Until Night**.
- Bed interaction includes transition effects and the Pokémon Center healing jingle, setting virtual time to **08:00 / 20:00** while preserving the continuing accelerated cycle.
- Selected phase persists through the existing saved `VAR_SLEEP_TIME_OFFSET`; no new save-block layout is introduced.

## Stability, compatibility and packaging rules

- **Locked baseline:** `v0.9.0.3.18`. **Next development version:** `v0.9.0.3.19`.
- Never overwrite or silently modify the locked v0.9.0.3.18 source archive. Build and test successors in separate copies.
- **Default deliverable: patch-only ZIP** containing modified/new files and a short change summary. Supply a complete source archive **only when explicitly requested**.
- Apply patches only to the exact base version they identify; do not apply a later patch to an older release out of order.
- Keep established followers, day/night timing and fade, original daytime palettes, Quest Log protections, weather effects and night battle palette behaviour intact unless a future change expressly targets them.
- Respect pinned dependencies; no needless third-party/vendor modifications.

## Current validation status

- **User acceptance:** v0.9.0.3.18 night battle implementation was confirmed working in-game and explicitly locked as stable.
- Previous development-stage source/static regression checks covered follower systems, day/night timing, night grass, weather boundaries, sleep phase persistence, Safari, Quest Log, Hall of Fame recall and battle palette wiring.
- The earlier candidate's source-test success is **not** a claim that a full independent ARM build or exhaustive emulator regression suite was performed in this environment. No new ROM compile is required for this documentation-only README patch.

## Development notes

This appended feature inventory describes systems integrated in the present source line; it is not a promise that every edge case has been independently tested. Consult `tests/` for available source/runtime checks. Earlier release-specific notes above have been preserved for traceability.
