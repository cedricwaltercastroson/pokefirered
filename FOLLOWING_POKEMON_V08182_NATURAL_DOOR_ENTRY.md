# FireRed Followers v0.8.18.2 — Natural Door Entry Candidate

**Base:** approved, protected v0.8.18.1 (agbcc syntax hotfix). The v0.8.18.1 archive is unchanged.

## Reason

The animated `DoDoorWarp` called `Follower_BeginDoorWarp()` before the player opened the door. That function destroys the follower object immediately, making it visibly vanish when the player touches the door.

## Change (animated entrance only)

- Do not recall the follower at `DoDoorWarp()` startup.
- The normal door task freezes NPCs as before. At its player-walk-in stage, unfreeze **only** the following Pokemon, so the existing follower movement logic can play the step naturally.
- Let the player walk through the door; keep the follower visible until its own pending movement completes.
- Call `Follower_BeginDoorWarp()` just before the door closes and the player is hidden. The existing map-load recall and destination-map handoff handle the exit side unchanged.
- All edits are contained in `src/field_fadetransition.c`; `src/following_pokemon.c`, sprite palettes and engine exit logic are byte-identical to v0.8.18.1.

## Behaviour / limitations

- Applies to **animated door entrances** (`DoDoorWarp`) only. Generic `DoWarp`, scripted entrances, stair warps and other transitions retain existing logic.
- The follower is still recalled at the end of the entry step, as the door closes; this isn't a full separate animated walk-inside-the-building feature. It avoids the premature disappearance reported in v0.8.18.1.
- The follower may pause briefly to complete its active step before the door closes. This is intentional to prevent half-step popping, rather than a new wait for an extra manual step.

## Validation

- GNU89 syntax check: `src/field_fadetransition.c`
- Existing host follower runtime test.
- All follower source-regression scripts, including a new entry sequencing guard.
- Full agbcc ARM build and emulator behaviour still require testing on the user's Cygwin setup.

## In-game tests

1. Walk into a Pokemon Center door while Charizard is following. Charizard should not disappear **at initial contact**; it should move naturally as the entrance animates, then be recalled when the player is hidden.
2. Exit that Center while continuously holding Down. The existing v0.8.18.1 smooth exit should be unchanged.
3. Repeat with the Poke Mart and ordinary house.
4. Repeat approved stairs, Nurse Joy healing and Rocket Hideout stop-tile recall. These code paths have not been edited.
5. Post-battle flicker / recall is still an open, separate issue; not modified by this patch.
