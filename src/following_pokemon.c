#include "global.h"
#include "gflib.h"
#include "pokemon.h"
#include "battle.h"
#include "string_util.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "field_effect.h"
#include "field_weather.h"
#include "fieldmap.h"
#include "metatile_behavior.h"
#include "palette.h"
#include "blend_palette.h"
#include "overworld.h"
#include "quest_log.h"
#include "script.h"
#include "sound.h"
#include "following_pokemon.h"
#include "constants/event_object_movement.h"
#include "constants/event_objects.h"
#include "constants/maps.h"
#include "constants/quest_log.h"
#include "constants/species.h"
#include "constants/songs.h"
#include "constants/weather.h"

// Kanto follower sprites are the six-frame, 32x32 indexed overworld sheets
// imported from the provided Emerald follower branch. This is a source-level
// reimplementation of the basic follower loop for FireRed, NOT a copy of
// Emerald's save block, field scripts, or movement-state structures.

extern const struct OamData gObjectEventBaseOam_32x32;
extern const struct SubspriteTable gObjectEventSpriteOamTables_32x32[];
extern const union AffineAnimCmd *const gDummySpriteAffineAnimTable[];

struct FollowerArt
{
    const struct SpriteFrameImage *frames;
    const u16 *palette;
};

#include "data/follower_art.h"
#include "data/follower_shiny_palettes.h"

// The supplied images have 0/1=South, 2/3=North, 4/5=West,
// and horizontal mirroring is used for East. Keep animation indexes
// compatible with FireRed's object-event movement actions.
#define FOLLOW_FACE(name, frame, flip) \
static const union AnimCmd sFollowFace##name[] = {ANIMCMD_FRAME(frame, 12, .hFlip = flip), ANIMCMD_END};
#define FOLLOW_WALK(name, idle, step, flip, delay) \
static const union AnimCmd sFollowWalk##name##delay[] = {ANIMCMD_FRAME(idle, delay, .hFlip = flip), ANIMCMD_FRAME(step, delay, .hFlip = flip), ANIMCMD_JUMP(0)};
FOLLOW_FACE(South, 0, FALSE)
FOLLOW_FACE(North, 2, FALSE)
FOLLOW_FACE(West, 4, FALSE)
FOLLOW_FACE(East, 4, TRUE)
FOLLOW_WALK(South, 0, 1, FALSE, 8)
FOLLOW_WALK(North, 2, 3, FALSE, 8)
FOLLOW_WALK(West, 4, 5, FALSE, 8)
FOLLOW_WALK(East, 4, 5, TRUE, 8)
FOLLOW_WALK(South, 0, 1, FALSE, 4)
FOLLOW_WALK(North, 2, 3, FALSE, 4)
FOLLOW_WALK(West, 4, 5, FALSE, 4)
FOLLOW_WALK(East, 4, 5, TRUE, 4)
FOLLOW_WALK(South, 0, 1, FALSE, 2)
FOLLOW_WALK(North, 2, 3, FALSE, 2)
FOLLOW_WALK(West, 4, 5, FALSE, 2)
FOLLOW_WALK(East, 4, 5, TRUE, 2)

// Dedicated east artwork uses frame 6 while standing still and frame 7 only
// during movement. Earlier versions cycled both frames continuously when idle,
// which made these five species appear to walk or twitch during dialogue.
static const union AnimCmd sFollowUniqueEastFace[] =
{
    ANIMCMD_FRAME(6, 12), ANIMCMD_END,
};
static const union AnimCmd sFollowUniqueEastWalk8[] =
{
    ANIMCMD_FRAME(6, 8), ANIMCMD_FRAME(7, 8), ANIMCMD_JUMP(0),
};
static const union AnimCmd sFollowUniqueEastWalk4[] =
{
    ANIMCMD_FRAME(6, 4), ANIMCMD_FRAME(7, 4), ANIMCMD_JUMP(0),
};
static const union AnimCmd sFollowUniqueEastWalk2[] =
{
    ANIMCMD_FRAME(6, 2), ANIMCMD_FRAME(7, 2), ANIMCMD_JUMP(0),
};

// Native FireRed Rocket Hideout spin actions request sprite animations
// 0x18..0x1B. The first 24 entries above are indexed 0x00..0x17, so
// add actual four-direction rotations at the exact native indices.
// Normal sheets mirror West to draw East; five special sheets use frame 6.
// Match Red's spin order and 2-frame pacing from object_event_anims.h.
#define FOLLOW_SPIN(name, south, north, west, east, eastFlip) \
static const union AnimCmd sFollowerSpin##name##Down[] = { \
    ANIMCMD_FRAME(south, 2), ANIMCMD_FRAME(east, 2, .hFlip = eastFlip), \
    ANIMCMD_FRAME(north, 2), ANIMCMD_FRAME(west, 2), ANIMCMD_LOOP(1), ANIMCMD_END }; \
static const union AnimCmd sFollowerSpin##name##Up[] = { \
    ANIMCMD_FRAME(north, 2), ANIMCMD_FRAME(west, 2), \
    ANIMCMD_FRAME(south, 2), ANIMCMD_FRAME(east, 2, .hFlip = eastFlip), ANIMCMD_LOOP(1), ANIMCMD_END }; \
static const union AnimCmd sFollowerSpin##name##Left[] = { \
    ANIMCMD_FRAME(west, 2), ANIMCMD_FRAME(south, 2), \
    ANIMCMD_FRAME(east, 2, .hFlip = eastFlip), ANIMCMD_FRAME(north, 2), ANIMCMD_LOOP(1), ANIMCMD_END }; \
static const union AnimCmd sFollowerSpin##name##Right[] = { \
    ANIMCMD_FRAME(east, 2, .hFlip = eastFlip), ANIMCMD_FRAME(north, 2), \
    ANIMCMD_FRAME(west, 2), ANIMCMD_FRAME(south, 2), ANIMCMD_LOOP(1), ANIMCMD_END };
FOLLOW_SPIN(Mirrored, 0, 2, 4, 4, TRUE)
FOLLOW_SPIN(Unique, 0, 2, 4, 6, FALSE)

static const union AnimCmd *const sFollowerAnimsUniqueEast[] =
{
    sFollowFaceSouth, sFollowFaceNorth, sFollowFaceWest, sFollowUniqueEastFace,
    sFollowWalkSouth8, sFollowWalkNorth8, sFollowWalkWest8, sFollowUniqueEastWalk8,
    sFollowWalkSouth4, sFollowWalkNorth4, sFollowWalkWest4, sFollowUniqueEastWalk4,
    sFollowWalkSouth2, sFollowWalkNorth2, sFollowWalkWest2, sFollowUniqueEastWalk2,
    sFollowWalkSouth2, sFollowWalkNorth2, sFollowWalkWest2, sFollowUniqueEastWalk2,
    sFollowWalkSouth4, sFollowWalkNorth4, sFollowWalkWest4, sFollowUniqueEastWalk4,
    sFollowerSpinUniqueDown, sFollowerSpinUniqueUp,
    sFollowerSpinUniqueLeft, sFollowerSpinUniqueRight,
};

static const union AnimCmd *const sFollowerAnims[] =
{
    sFollowFaceSouth, sFollowFaceNorth, sFollowFaceWest, sFollowFaceEast,
    sFollowWalkSouth8, sFollowWalkNorth8, sFollowWalkWest8, sFollowWalkEast8,
    sFollowWalkSouth4, sFollowWalkNorth4, sFollowWalkWest4, sFollowWalkEast4,
    sFollowWalkSouth2, sFollowWalkNorth2, sFollowWalkWest2, sFollowWalkEast2,
    sFollowWalkSouth2, sFollowWalkNorth2, sFollowWalkWest2, sFollowWalkEast2,
    sFollowWalkSouth4, sFollowWalkNorth4, sFollowWalkWest4, sFollowWalkEast4,
    sFollowerSpinMirroredDown, sFollowerSpinMirroredUp,
    sFollowerSpinMirroredLeft, sFollowerSpinMirroredRight,
};

// Slot 11 is reserved as a dedicated following-Pokemon OBJ palette.
// Existing map objects normally use the shared 0..10 palette slots.
#define FOLLOWER_PALETTE_SLOT 11

// A short trail of the player's actual visited tiles. The player may finish
// several steps while an earlier follower movement animation is still running.
// Unlike v0.3, we never interrupt a held movement to catch up around corners.
#define FOLLOWER_TRAIL_LENGTH 12

// Emerald FollowingPokemon_Gen1-3 offsets the sprite horizontally by 8 px
// when facing east/west. A 32x32 sheet otherwise visually overlaps the
// trainer even when their object events are correctly one tile apart.
#define FOLLOWER_SIDE_SPACING_PIXELS 8

struct FollowerTrailEntry
{
    s16 x;
    s16 y;
    // A Jump2 destination, rather than an ordinary tile to walk onto.
    bool8 jump;
};

static struct FollowerTrailEntry sTrail[FOLLOWER_TRAIL_LENGTH];
static u8 sTrailHead;
static u8 sTrailCount;
// FireRed's Jump2 first moves currentCoords onto the ledge tile, then to the
// landing tile halfway through the animation. Neither is an ordinary step.
// Save the landing point and queue a follower Jump2 only once the player
// VACATES that tile, maintaining separation rather than overlapping sprites.
static bool8 sPlayerJumpInProgress;
static bool8 sFollowerJumpPending;
static s16 sPlayerJumpLandingX;
static s16 sPlayerJumpLandingY;
static u16 sFollowerSpecies;
static bool8 sFollowerShiny;
// Battle rendering replaces field object sprites, so capture follower status
// while still on the overworld. Zero means no eligible visible follower;
// eligible party indexes are stored as index + 1 to keep this state in BSS.
// The custom FireRed linker script does not map our module's initialized .data.
static u8 sBattleEntryFollowerPartyIndex;
static bool8 sPaletteRefreshAfterLoad;
// The B-skip Quest Log path can return to the overworld before the follower's
// dynamic sprite is ready. Count *ready, settled* frames, not wall-clock frames:
// an early missing follower must not silently cancel its palette repair.
#define FOLLOWER_QUEST_LOG_SKIP_PALETTE_FRAMES 4
static u8 sQuestLogSkipPaletteFrames;

// Only set while the field loader is priming the follower before fade-in.
// An ordinary field reload can try an adjacent free tile, but a battle return
// must NEVER relocate the follower sideways or ahead of the player.
static bool8 sPreDisplaySpawn;
// The battle engine rebuilds overworld sprites without necessarily resetting
// the saved object-event coordinates. Resynchronise the follower under the
// return-to-field fade, not on its first visible update after a trainer battle.
static bool8 sBattleReturnPending;
// Post-battle illusion: unlike ordinary warps, defer the reappearance until
// the player completes a genuine, controllable field step. This deliberately
// keeps the follower absent during every battle fade/palette rebuild frame.
// The coordinates track the LAST settled player tile, not an arbitrary side
// tile, so the follower returns on the tile the player actually vacated.
static bool8 sBattleRecallAwaitStep;
// All local Trainer Battles (including Gym rematches, ordinary Singles and
// Doubles) use the League's safe rear-tile return after dialogue/fade.
// Preserve the historic state name for compatibility with existing logic.
static bool8 sGymRematchReturnPending;
// Ordinary wild encounters use the same settled, safe rear-tile return.
// Scripted legendary and Safari battles keep their own existing callbacks.
static bool8 sWildBattleReturnPending;
// Door entries and indoor-to-indoor warps retain the approved Emerald handoff.
// Outdoor BUILDING exits use a separate Safari-style deferred first-step
// handoff: the scripted southward door-exit motion is NOT a player step.
static bool8 sDoorWarpRecall;
static bool8 sDoorRecallHasAnchor;
static bool8 sDoorRecallExpectsExitStep;
// Preserve this latch across ResetObjectEvents and map load. Only a real
// controllable step after building exit can materialise the follower.
static bool8 sDoorRecallOutdoorExitPending;
// Only the destination warp-exit task can permit the first voluntary step.
static bool8 sDoorRecallExitReady;
// Fly/field Teleport -> immediate building entry -> exit: keep a one-time
// handoff across the two map resets until the follower has been restored.
static bool8 sInterruptedTravelBuildingEntry;
static s16 sDoorRecallAnchorX;
static s16 sDoorRecallAnchorY;
static u8 sDoorRecallMapNum;
static u8 sDoorRecallMapGroup;
static s16 sBattleRecallAnchorX;
static s16 sBattleRecallAnchorY;
static u8 sBattleRecallMapNum;
static u8 sBattleRecallMapGroup;
// During the first battle-return frames the overworld fade and weather tasks
// can modify OBJ palettes *after* Follower_Update runs. Stabilize slot 11 at
// the end of the frame, before palette RAM is transferred to the GBA screen.
#define FOLLOWER_BATTLE_PALETTE_GUARD_FRAMES 90
static u8 sBattlePaletteGuardFrames;
static u8 sBattlePaletteSettledFrames;
// A stair warp offsets the player's sprite independently of tile coordinates.
// Keep the follower recalled until the stair exit animation has finished,
// so it cannot appear beside an offset player midway through the stairs.
// Do not reset across ResetObjectEvents: that occurs between stair maps.
static bool8 sStairWarpRecall;
// Sabrina's Gym and other warp-pad teleports reload the overworld even when
// the destination is another tile on the SAME map. Keep the follower hidden
// until the arriving player's teleport/fade animation fully releases control.
static bool8 sWarpPadRecall;
// After arrival, only restore behind the player on a safe non-pad tile.
// If blocked, track actual player movement without spawning at the warp pad.
static bool8 sWarpPadAwaitRelease;
static bool8 sWarpPadHasAnchor;
static s16 sWarpPadAnchorX;
static s16 sWarpPadAnchorY;
static u8 sWarpPadAnchorMapNum;
static u8 sWarpPadAnchorMapGroup;
// Armed only if an actual visible follower was recalled for the warp.
// Consume on successful post-warp respawn, never on the warp callback alone.
static bool8 sWarpPadReleaseSoundPending;
// Shared deferred-return state for Safari outdoor boundary warps and all
// non-overworld fade warps (caves, dungeons and indoor rooms). No generic
// adjacent-tile fallback is permitted; return on a safe player-vacated tile.
static bool8 sSafariWarpAwaitStep;
static bool8 sSafariWarpHasAnchor;
static s16 sSafariWarpAnchorX;
static s16 sSafariWarpAnchorY;
static u8 sSafariWarpAnchorMapNum;
static u8 sSafariWarpAnchorMapGroup;
// Dig, Escape Rope, and field-move Teleport share one scripted travel handoff.
// Preserve across map-load resets and release ONLY after the destination's
// landing animation relinquishes control and the player takes a safe step.
static bool8 sFieldTravelRecallActive;
static bool8 sFieldTravelAwaitStep;
static bool8 sFieldTravelHasAnchor;
static bool8 sFieldTravelReleaseSoundPending;
static s16 sFieldTravelAnchorX;
static s16 sFieldTravelAnchorY;
static u8 sFieldTravelAnchorMapNum;
static u8 sFieldTravelAnchorMapGroup;
// Rocket Hideout forced spinning can outpace the delayed follower trail.
// Recall it for the entire spin sequence, then spawn only behind the player
// after the STOP_SPINNING tile has completed its movement. No recall sound:
// this is a movement presentation rule, not the Nurse Joy healing script.
static bool8 sSpinTileRecall;
// The STOP_SPINNING tile is the last safe anchor before manual movement.
// A held D-pad can immediately start another step without an idle frame;
// keep this anchor until the follower actually spawns on the vacated tile.
static bool8 sSpinStopHasAnchor;
static s16 sSpinStopX;
static s16 sSpinStopY;
static s16 sLastPlayerX;
static s16 sLastPlayerY;
static u8 sLastMapNum;
static u8 sLastMapGroup;
static bool8 sHasLastPosition;
static bool8 sAwaitingSafeSpawn;
static bool8 sInteractionActive;
// Mount/dismount animations clear avatar flags before the action is complete.
// Defer reappearing until the player is fully on solid ground and controllable.
static bool8 sResumeFromSpecialMovement;
// Armed only when a visible follower was recalled for bicycle travel.
// Consume on successful post-dismount spawn (not when the bike flag clears).
static bool8 sBikeReleaseSoundPending;
// The SURFING avatar flag is cleared at shore-jump START, long before the
// field dismount task finishes. The task completion callback explicitly arms
// this deferred return, independently of an idle frame or released D-pad.
static bool8 sSurfReturnAwaitStep;
static bool8 sSurfReturnHasAnchor;
static bool8 sSurfReleaseSoundPending;
static s16 sSurfReturnAnchorX;
static s16 sSurfReturnAnchorY;
static u8 sSurfReturnAnchorMapNum;
static u8 sSurfReturnAnchorMapGroup;
// Fly temporarily returns the avatar to ON_FOOT before the bird departs;
// suppress follower re-creation until the destination map is loaded.
static bool8 sFlyRecallActive;
// Preserve the post-Fly release across ResetObjectEvents on arrival. The
// follower must not simply be created under the destination map's fade;
// restore on the FIRST genuine player-controlled step from the landing tile.
static bool8 sFlyReturnAwaitStep;
static bool8 sFlyReturnHasAnchor;
static bool8 sFlyReleaseSoundPending;
static s16 sFlyReturnAnchorX;
static s16 sFlyReturnAnchorY;
static u8 sFlyReturnAnchorMapNum;
static u8 sFlyReturnAnchorMapGroup;
// Emerald recalls the follower before the Nurse Joy healing animation. Keep
// the follower recalled until FireRed's whole nurse conversation finishes,
// so it never pops back into view during Nurse Joy's closing dialogue.
#define FOLLOWER_HEAL_IDLE 0
#define FOLLOWER_HEAL_IN_PROGRESS 1
#define FOLLOWER_HEAL_WAIT_FOR_RELEASE 2
static u8 sHealingRecallState;
// Arm only after Nurse Joy releases control; consume on successful spawn.
static bool8 sHealingReleaseSoundPending;
// Cutscene applymovement on the PLAYER is distinct from an ordinary NPC
// conversation. Hide the dynamic follower while the script steers the player;
// do not reappear until the scene has released both script contexts.
#define FOLLOWER_SCRIPT_IDLE 0
#define FOLLOWER_SCRIPT_MOVING 1
#define FOLLOWER_SCRIPT_WAIT_FOR_RELEASE 2
static u8 sScriptedMovementState;
// League entry, Champion and Hall of Fame scripts physically walk the player.
// Their dynamic follower must trace those steps rather than be recalled until
// an unrelated manual step. Non-League cutscenes retain their old recall policy.
static bool8 sLeagueScriptFollowing;
// Ordinary cutscene walking uses the existing follower trail, not a recall.
static bool8 sGeneralScriptFollowing;
// NPCs and Trainer tasks NEVER wait for this state. It affects the follower only.
static bool8 sFollowerYieldHoldTrail;
static s16 sFollowerYieldPlayerX;
static s16 sFollowerYieldPlayerY;
// Recall is reserved for the Hall of Fame machine ceremony, after Red and
// the follower finish walking in. Suppress respawn until the next map load.
static bool8 sHallOfFameRecallActive;

static bool8 Follower_IsLeagueScriptMap(void)
{
    u16 mapId = gSaveBlock1Ptr->location.mapNum
              | (gSaveBlock1Ptr->location.mapGroup << 8);
    switch (mapId)
    {
    case MAP_POKEMON_LEAGUE_LORELEIS_ROOM:
    case MAP_POKEMON_LEAGUE_BRUNOS_ROOM:
    case MAP_POKEMON_LEAGUE_AGATHAS_ROOM:
    case MAP_POKEMON_LEAGUE_LANCES_ROOM:
    case MAP_POKEMON_LEAGUE_CHAMPIONS_ROOM:
    case MAP_POKEMON_LEAGUE_HALL_OF_FAME:
        return TRUE;
    }
    return FALSE;
}
// Seamless connections keep the same object sprite. Camera coordinate changes
// must be mirrored in the follower's path history on the same frame.

static const u8 sFollowerUnknownName[] = _("POKEMON");
static const u8 sFollowerTextFainted[] = _("{STR_VAR_1} is exhausted.\nIt needs some rest.");
static const u8 sFollowerTextStatus[] = _("{STR_VAR_1} does not look\nvery well.");
static const u8 sFollowerTextHurt[] = _("{STR_VAR_1} seems tired.\nIt could use some rest.");
static const u8 sFollowerTextDelighted[] = _("{STR_VAR_1} is thrilled to\nbe with you!");
static const u8 sFollowerTextHappy[] = _("{STR_VAR_1} is happily\nfollowing you.");
static const u8 sFollowerTextCurious[] = _("{STR_VAR_1} looks around\ncuriously.");
static const u8 sFollowerTextReserved[] = _("{STR_VAR_1} seems a little\nshy around you.");

// The script's follower-specific face turn needs the v0.5.3 sprite offset.
static void Follower_ApplyHorizontalSpacing(const struct ObjectEvent *follower);

static void Follower_ClearTrail(void)
{
    sTrailHead = 0;
    sTrailCount = 0;
    sPlayerJumpInProgress = FALSE;
    sFollowerJumpPending = FALSE;
}

// Called from ResetObjectEvents before FireRed clears the map's sprite array.
// This is essential when a warp reloads the SAME map number and group.
void Follower_ResetForMapLoad(void)
{
    Follower_ClearTrail();
    sHasLastPosition = FALSE;
    sAwaitingSafeSpawn = FALSE;
    sInteractionActive = FALSE;
    sResumeFromSpecialMovement = FALSE;
    sBikeReleaseSoundPending = FALSE;
    sSurfReturnAwaitStep = FALSE;
    sSurfReturnHasAnchor = FALSE;
    sSurfReleaseSoundPending = FALSE;
    sFlyRecallActive = FALSE;
    sFlyReturnHasAnchor = FALSE;
    // Do not reset sFlyReturnAwaitStep or sFlyReleaseSoundPending here:
    // ResetObjectEvents runs during the actual Fly destination map load.
    sHealingRecallState = FOLLOWER_HEAL_IDLE;
    sHealingReleaseSoundPending = FALSE;
    sScriptedMovementState = FOLLOWER_SCRIPT_IDLE;
    sLeagueScriptFollowing = FALSE;
    sGeneralScriptFollowing = FALSE;
    sFollowerYieldHoldTrail = FALSE;
    sHallOfFameRecallActive = FALSE;
    sFollowerSpecies = SPECIES_NONE;
    sFollowerShiny = FALSE;
    sBattleEntryFollowerPartyIndex = 0;
    sPaletteRefreshAfterLoad = TRUE;
    sQuestLogSkipPaletteFrames = 0;
    sBattlePaletteGuardFrames = 0;
    sBattlePaletteSettledFrames = 0;
    sBattleRecallAwaitStep = FALSE;
    // Preserve the selected gym return through the battle-to-field reload,
    // but never carry it into an unrelated map transition.
    if (!sBattleReturnPending)
    {
        sGymRematchReturnPending = FALSE;
        sWildBattleReturnPending = FALSE;
    }
    // Door recalls must survive ResetObjectEvents, but old-map coordinates
    // must not leak into the new map. PrimeForOverworldDisplay anchors once
    // the destination player object has valid coordinates.
    sDoorRecallHasAnchor = FALSE;
    sDoorRecallExpectsExitStep = FALSE;
    // Preserve sDoorRecallOutdoorExitPending: the outdoor destination map
    // is loaded after the indoor departure has already recalled the follower.
    sPreDisplaySpawn = FALSE;
    sSpinTileRecall = FALSE;
    sSpinStopHasAnchor = FALSE;
    sWarpPadHasAnchor = FALSE;
    sSafariWarpHasAnchor = FALSE;
    sFieldTravelHasAnchor = FALSE;
    // Do not reset sFieldTravelRecallActive / sFieldTravelAwaitStep or the
    // pending sound here: ResetObjectEvents executes during Dig/Rope/Teleport.
    // Preserve sSafariWarpAwaitStep across ResetObjectEvents on arrival.
    // Retain both stair and warp-pad recall across ResetObjectEvents; Sabrina
    // teleports between coordinates in the same map as well as across reloads.
}

static bool8 Follower_TryEnqueue(s16 x, s16 y, bool8 jump)
{
    u8 tail;
    if (sTrailCount == FOLLOWER_TRAIL_LENGTH)
        return FALSE;
    tail = (sTrailHead + sTrailCount) % FOLLOWER_TRAIL_LENGTH;
    sTrail[tail].x = x;
    sTrail[tail].y = y;
    sTrail[tail].jump = jump;
    sTrailCount++;
    return TRUE;
}

static void Follower_Dequeue(void)
{
    if (sTrailCount != 0)
    {
        sTrailHead = (sTrailHead + 1) % FOLLOWER_TRAIL_LENGTH;
        sTrailCount--;
    }
}

// The Quest Log replays NPC snapshots, then restores the saved game's object
// state without a normal map warp. Its final callback must reconcile the
// dynamic follower explicitly: an old doorway/travel latch or old playback
// position must not strand it after control returns to the player.
// This runs ONCE when Quest Log playback finishes; no frame-loop polling.
void Follower_ResumeAfterQuestLog(void)
{
    u8 playerId = gPlayerAvatar.objectEventId;

    if (QL_IS_PLAYBACK_STATE || playerId >= OBJECT_EVENTS_COUNT
     || !gObjectEvents[playerId].active || !gObjectEvents[playerId].isPlayer)
        return;

    // Playback is complete. The saved location is now authoritative, not
    // the previous session's in-flight travel, warp, battle or script state.
    sFlyRecallActive = FALSE;
    sFlyReturnAwaitStep = FALSE;
    sFlyReturnHasAnchor = FALSE;
    sFlyReleaseSoundPending = FALSE;
    sFieldTravelRecallActive = FALSE;
    sFieldTravelAwaitStep = FALSE;
    sFieldTravelHasAnchor = FALSE;
    sFieldTravelReleaseSoundPending = FALSE;
    sDoorWarpRecall = FALSE;
    sDoorRecallOutdoorExitPending = FALSE;
    sDoorRecallExitReady = FALSE;
    sDoorRecallHasAnchor = FALSE;
    sDoorRecallExpectsExitStep = FALSE;
    sInterruptedTravelBuildingEntry = FALSE;
    sSafariWarpAwaitStep = FALSE;
    sSafariWarpHasAnchor = FALSE;
    sBattleReturnPending = FALSE;
    sBattleRecallAwaitStep = FALSE;
    sGymRematchReturnPending = FALSE;
    sWildBattleReturnPending = FALSE;
    sWarpPadRecall = FALSE;
    sWarpPadAwaitRelease = FALSE;
    sWarpPadHasAnchor = FALSE;
    sWarpPadReleaseSoundPending = FALSE;
    sStairWarpRecall = FALSE;
    sSpinTileRecall = FALSE;
    sSpinStopHasAnchor = FALSE;
    sSurfReturnAwaitStep = FALSE;
    sSurfReturnHasAnchor = FALSE;
    sSurfReleaseSoundPending = FALSE;
    sResumeFromSpecialMovement = FALSE;
    sHealingRecallState = FOLLOWER_HEAL_IDLE;
    sHealingReleaseSoundPending = FALSE;
    sBikeReleaseSoundPending = FALSE;
    sScriptedMovementState = FOLLOWER_SCRIPT_IDLE;
    sLeagueScriptFollowing = FALSE;
    sGeneralScriptFollowing = FALSE;
    sFollowerYieldHoldTrail = FALSE;
    sHallOfFameRecallActive = FALSE;
    sPreDisplaySpawn = FALSE;
    sBattlePaletteGuardFrames = 0;
    sBattlePaletteSettledFrames = 0;
    sInteractionActive = FALSE;
    Follower_ClearTrail();
    sHasLastPosition = FALSE;
    sAwaitingSafeSpawn = FALSE;
    sPaletteRefreshAfterLoad = TRUE;

    // Prefer the true tile behind the player. If that tile is obstructed,
    // the existing delayed-spawn logic waits for the next vacated safe tile;
    // it never puts the Pokemon on the player, a door, or an arbitrary side.
    Follower_Update(&gObjectEvents[playerId]);
}

static bool8 Follower_IsSafeSpawnTile(s16 x, s16 y)
{
    u8 otherObjectId;
    if (MapGridIsImpassableAt(x, y) || GetMapBorderIdAt(x, y) == CONNECTION_INVALID)
        return FALSE;
    if (MetatileBehavior_IsSurfable(MapGridGetMetatileBehaviorAt(x, y)))
        return FALSE;
    otherObjectId = GetObjectEventIdByXY(x, y);
    // The follower must NEVER spawn or teleport onto the player tile. A
    // Quest Log snapshot can initialise the player with DIR_NONE, which in
    // older versions produced no coordinate shift and allowed an overlap.
    if (otherObjectId != OBJECT_EVENTS_COUNT)
        return FALSE;
    return TRUE;
}

// Emerald can place the invisible follower on the door threshold and reveal
// it as it walks through. Some FireRed doors mark their threshold impassable;
// allow these *recognized door metatiles* as the vacated follower entry tile
// while still rejecting walls, water, NPCs and map borders.
static bool8 Follower_IsSafeDoorReturnTile(s16 x, s16 y)
{
    u8 behavior = MapGridGetMetatileBehaviorAt(x, y);
    if (GetMapBorderIdAt(x, y) == CONNECTION_INVALID
     || MetatileBehavior_IsSurfable(behavior)
     || GetObjectEventIdByXY(x, y) != OBJECT_EVENTS_COUNT)
        return FALSE;
    if (!MapGridIsImpassableAt(x, y))
        return TRUE;
    return MetatileBehavior_IsWarpDoor(behavior)
        || MetatileBehavior_IsWarpDoor_2(behavior)
        || MetatileBehavior_IsNonAnimDoor(behavior);
}

// Viridian Gym's exterior has a one-tile-wide landing between the exit
// warp at (36, 10) and an impassable strip at y=12 (ViridianCity map.json
// and layout). A follower drawn at the warp or its immediate landing visually
// stacks with the entrance facade. The map grid adds seven border tiles to
// map.json coordinates. Leave the scripted exit and player movement untouched;
// use the first later player-vacated walkway tile instead.
static bool8 Follower_IsViridianGymExteriorThreshold(u8 mapNum, u8 mapGroup,
                                                    s16 x, s16 y)
{
    return mapNum == MAP_NUM(VIRIDIAN_CITY)
        && mapGroup == MAP_GROUP(VIRIDIAN_CITY)
        && x == 36 + 7
        && (y == 10 + 7 || y == 11 + 7);
}

// Route 5 Day Care has the same raised, one-tile-deep exterior landing as
// Viridian Gym. The exterior door warp is (23, 25) and the passable porch
// below it is (23, 26); the next row is impassable (Route5 map/layout).
// Never respawn the follower on either door/facade tile. Wait until the player
// vacates a normal walkway tile, without altering the player's exit motion.
static bool8 Follower_IsRoute5DayCareExteriorThreshold(u8 mapNum, u8 mapGroup,
                                                       s16 x, s16 y)
{
    return mapNum == MAP_NUM(ROUTE5)
        && mapGroup == MAP_GROUP(ROUTE5)
        && x == 23 + 7
        && (y == 25 + 7 || y == 26 + 7);
}

// Choose a safe adjacent tile without changing the player's movement trail.
// This fallback is for ordinary field loading only, NOT battle returns or
// Quest Log replay. Battle returns must preserve rear-only placement, or wait
// until the player vacates a tile to follow.
static bool8 Follower_FindAdjacentTile(const struct ObjectEvent *player, s16 *x, s16 *y)
{
    const u8 directions[] = {DIR_SOUTH, DIR_NORTH, DIR_WEST, DIR_EAST};
    u8 preferred = GetOppositeDirection(player->facingDirection);
    u8 i;

    if (preferred >= DIR_SOUTH && preferred <= DIR_EAST)
    {
        *x = player->currentCoords.x;
        *y = player->currentCoords.y;
        MoveCoords(preferred, x, y);
        if (Follower_IsSafeSpawnTile(*x, *y))
            return TRUE;
    }
    for (i = 0; i < ARRAY_COUNT(directions); i++)
    {
        if (directions[i] == preferred)
            continue;
        *x = player->currentCoords.x;
        *y = player->currentCoords.y;
        MoveCoords(directions[i], x, y);
        if (Follower_IsSafeSpawnTile(*x, *y))
            return TRUE;
    }
    return FALSE;
}

static u8 Follower_DirectionTo(s16 dx, s16 dy)
{
    if (dy == 0 && dx > 0 && dx <= 2)
        return DIR_EAST;
    if (dy == 0 && dx < 0 && dx >= -2)
        return DIR_WEST;
    if (dx == 0 && dy > 0 && dy <= 2)
        return DIR_SOUTH;
    if (dx == 0 && dy < 0 && dy >= -2)
        return DIR_NORTH;
    return DIR_NONE;
}

// A 2-tile change is NOT necessarily a ledge jump (scripts also relocate the
// player by two tiles). FireRed checks the intermediate ledge metatile with
// GetLedgeJumpDirection before starting the player's Jump2 action. Require
// that same evidence before asking the follower to jump across it.
static bool8 Follower_IsLedgeJump(s16 fromX, s16 fromY, s16 toX, s16 toY)
{
    s16 dx = toX - fromX;
    s16 dy = toY - fromY;
    s16 ledgeX = fromX;
    s16 ledgeY = fromY;
    u8 direction = Follower_DirectionTo(dx, dy);

    if (direction == DIR_NONE)
        return FALSE;
    if ((dx != 2 && dx != -2) && (dy != 2 && dy != -2))
        return FALSE;

    MoveCoords(direction, &ledgeX, &ledgeY);
    return GetLedgeJumpDirection(ledgeX, ledgeY, direction) == direction;
}

const struct ObjectEventGraphicsInfo *Follower_GetGraphicsInfo(void)
{
    // Use a single reusable graphics-info structure; its frame table is
    // selected before the follower is spawned, and stays fixed until respawn.
    static struct ObjectEventGraphicsInfo info;
    info.tileTag = SPRITE_INVALID_TAG;
    info.paletteTag = SPRITE_INVALID_TAG;
    info.reflectionPaletteTag = 0x11FF; // FireRed internal OBJ_EVENT_PAL_TAG_NONE
    info.size = 512;
    info.width = 32;
    info.height = 32;
    info.paletteSlot = FOLLOWER_PALETTE_SLOT;
    info.shadowSize = 1;
    info.inanimate = FALSE;
    info.disableReflectionPaletteLoad = TRUE;
    info.tracks = 0;
    info.oam = &gObjectEventBaseOam_32x32;
    info.subspriteTables = gObjectEventSpriteOamTables_32x32;
    // Five Kanto sheets have unique east-facing frames instead of a mirror.
    if (sFollowerSpecies == SPECIES_FARFETCHD || sFollowerSpecies == SPECIES_KINGLER ||
        sFollowerSpecies == SPECIES_CUBONE || sFollowerSpecies == SPECIES_MAROWAK ||
        sFollowerSpecies == SPECIES_WEEZING)
        info.anims = sFollowerAnimsUniqueEast;
    else
        info.anims = sFollowerAnims;
    info.images = sFollowerArt[sFollowerSpecies].frames;
    info.affineAnims = gDummySpriteAffineAnimTable;
    return &info;
}

static u8 Follower_GetLeadPartyIndex(void)
{
    u8 i;
    for (i = 0; i < gPlayerPartyCount; i++)
    {
        struct Pokemon *mon = &gPlayerParty[i];
        u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);
        if (species >= SPECIES_BULBASAUR && species <= SPECIES_MEW
         && !GetMonData(mon, MON_DATA_IS_EGG, NULL))
            return i;
    }
    return PARTY_SIZE;
}

static u16 Follower_GetLeadSpecies(bool8 *isShiny)
{
    u8 slot = Follower_GetLeadPartyIndex();
    *isShiny = FALSE;
    if (slot == PARTY_SIZE)
        return SPECIES_NONE;
#if FOLLOWER_FORCE_SHINY_PREVIEW
    *isShiny = TRUE; // DEBUG ONLY: preview the shiny palette with any starter.
#else
    *isShiny = IsMonShiny(&gPlayerParty[slot]);
#endif
    return GetMonData(&gPlayerParty[slot], MON_DATA_SPECIES, NULL);
}

// The follower is the first non-Egg Kanto Pokémon in the party, which is not
// always the first battle-ready Pokémon. Record its party slot only when its
// overworld sprite actually exists and is visible before battle transitions.
// Double Battles can be promoted after this snapshot; the player controller
// handles the second Pokémon separately when its normal send-out begins.
void Follower_PrepareBattleEntry(void)
{
    u8 objectId;
    u8 partyIndex;
    struct ObjectEvent *follower;

    sBattleEntryFollowerPartyIndex = 0;
    if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_MULTI
                          | BATTLE_TYPE_SAFARI | BATTLE_TYPE_OLD_MAN_TUTORIAL
                          | BATTLE_TYPE_POKEDUDE | BATTLE_TYPE_BATTLE_TOWER
                          | BATTLE_TYPE_RECORDED | BATTLE_TYPE_INGAME_PARTNER))
        return;

    partyIndex = Follower_GetLeadPartyIndex();
    if (partyIndex >= PARTY_SIZE || sFollowerSpecies == SPECIES_NONE
     || GetMonData(&gPlayerParty[partyIndex], MON_DATA_HP) == 0
     || GetMonData(&gPlayerParty[partyIndex], MON_DATA_SPECIES) != sFollowerSpecies)
        return;

    if (TryGetObjectEventIdByLocalIdAndMap(FOLLOWER_LOCAL_ID,
                                          gSaveBlock1Ptr->location.mapNum,
                                          gSaveBlock1Ptr->location.mapGroup,
                                          &objectId))
        return;
    if (objectId >= OBJECT_EVENTS_COUNT)
        return;

    follower = &gObjectEvents[objectId];
    if (!follower->active || follower->invisible
     || follower->graphicsId != OBJ_EVENT_GFX_FOLLOWER
     || follower->spriteId >= MAX_SPRITES
     || gSprites[follower->spriteId].invisible)
        return;

    sBattleEntryFollowerPartyIndex = partyIndex + 1;
}

bool8 Follower_TakeBattleEntrySlide(u8 partyIndex)
{
    bool8 eligible = (partyIndex < PARTY_SIZE
                   && (partyIndex + 1) == sBattleEntryFollowerPartyIndex
                   && !(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_MULTI | BATTLE_TYPE_INGAME_PARTNER)));
    // Always consume the snapshot, including if a different Pokémon is sent.
    sBattleEntryFollowerPartyIndex = 0;
    return eligible;
}

// Only the dedicated follower OBJ palette slot changes for shiny Pokémon.
// The artwork, animation frames and map objects stay exactly the same.
static const u16 *Follower_GetSelectedPalette(void)
{
    if (sFollowerShiny)
        return sFollowerShinyPalettes[sFollowerSpecies];
    return sFollowerArt[sFollowerSpecies].palette;
}

// FireRed can restore OBJ palettes after the follower is first spawned when
// continuing a saved game (notably after the Quest Log's grayscale recap).
// Check the UNFADED copy: it should match the selected normal/shiny artwork
// once ordinary, untinted overworld play resumes. Do not fight fades or the
// intentionally grayscale/sepia Quest Log while either is still active.
static bool8 Follower_PaletteMatchesSelected(void)
{
    const u16 *expected = Follower_GetSelectedPalette();
    const u16 *loaded = &gPlttBufferUnfaded[0x100 + FOLLOWER_PALETTE_SLOT * 16];
    u8 i;

    for (i = 0; i < 16; i++)
    {
        if (loaded[i] != expected[i])
            return FALSE;
    }
    return TRUE;
}

static void Follower_RestorePaletteAfterLoad(void)
{
    if (gPaletteFade.active || !IsWeatherNotFadingIn() || gGlobalFieldTintMode != QL_TINT_NONE)
        return;

    // The one-shot refresh also repairs the FADED buffer when only that
    // buffer was changed during a loading/fade transition. Later calls repair
    // the palette only if another system overwrites our reserved OBJ slot.
    if (sPaletteRefreshAfterLoad || !Follower_PaletteMatchesSelected())
    {
        Follower_RefreshPalette();
        sPaletteRefreshAfterLoad = FALSE;
    }
}

// Called by the interaction script after the map event has been selected.
// The current follower is dynamic and has no map-event template.
// Use the same party selection as its sprite graphics to avoid name mismatch.
u16 Follower_PrepareInteraction(void)
{
    u8 slot = Follower_GetLeadPartyIndex();
    const u8 *text = sFollowerTextCurious;
    u16 hp, maxHp, friendship;
    struct Pokemon *mon;

    // FireRed's standard "faceplayer" script action can be ignored when the
    // follower still has a held movement queued. The event's "lock" command
    // has already waited for the selected object to stop moving; now finish
    // its old movement, turn its sprite immediately, and freeze that pose
    // throughout dialogue. Do not alter the object-event tile coordinates or
    // the player's queued path, so following resumes as before on release.
    sInteractionActive = TRUE;
    if (gSelectedObjectEvent < OBJECT_EVENTS_COUNT)
    {
        struct ObjectEvent *follower = &gObjectEvents[gSelectedObjectEvent];
        if (follower->active && !follower->isPlayer
         && follower->localId == FOLLOWER_LOCAL_ID
         && follower->graphicsId == OBJ_EVENT_GFX_FOLLOWER
         && follower->spriteId < MAX_SPRITES
         && !follower->singleMovementActive)
        {
            ObjectEventClearHeldMovementIfActive(follower);
            ObjectEventTurn(follower, GetOppositeDirection(GetPlayerFacingDirection()));
            Follower_ApplyHorizontalSpacing(follower);
            FreezeObjectEvent(follower);
        }
    }
    if (slot == PARTY_SIZE)
    {
        // Defensive fallback if the party changes as interaction begins.
        StringCopy(gStringVar1, sFollowerUnknownName);
    }
    else
    {
        mon = &gPlayerParty[slot];
        GetMonData(mon, MON_DATA_NICKNAME, gStringVar1);
        hp = GetMonData(mon, MON_DATA_HP, NULL);
        maxHp = GetMonData(mon, MON_DATA_MAX_HP, NULL);
        friendship = GetMonData(mon, MON_DATA_FRIENDSHIP, NULL);
        if (hp == 0)
            text = sFollowerTextFainted;
        else if (GetMonData(mon, MON_DATA_STATUS, NULL) != 0)
            text = sFollowerTextStatus;
        else if (maxHp != 0 && hp * 4 < maxHp)
            text = sFollowerTextHurt;
        else if (friendship >= 220)
            text = sFollowerTextDelighted;
        else if (friendship >= 140)
            text = sFollowerTextHappy;
        else if (friendship < 70)
            text = sFollowerTextReserved;
    }
    StringExpandPlaceholders(gStringVar4, text);
    return 0;
}

// Called exclusively by EventScript_FollowerTalk after the follower has been
// selected and turned towards the player. Playing the species cry here keeps
// it tied to an intentional A-button interaction (never map loads, walks,
// door transitions, healing release or battle returns). A normal and a shiny
// follower share their species cry.
u16 Follower_PlayInteractionCry(void)
{
    u8 slot;
    u16 species;
    struct ObjectEvent *follower;

    if (!sInteractionActive || gSelectedObjectEvent >= OBJECT_EVENTS_COUNT)
        return FALSE;

    follower = &gObjectEvents[gSelectedObjectEvent];
    if (!follower->active || follower->isPlayer
     || follower->localId != FOLLOWER_LOCAL_ID
     || follower->graphicsId != OBJ_EVENT_GFX_FOLLOWER)
        return FALSE;

    slot = Follower_GetLeadPartyIndex();
    if (slot == PARTY_SIZE)
        return FALSE;

    species = GetMonData(&gPlayerParty[slot], MON_DATA_SPECIES, NULL);
    if (species == SPECIES_NONE || species != sFollowerSpecies)
        return FALSE;

    // Standard FireRed field-cry mode; no palette or shiny-dependent audio.
    PlayCry7(species, 0);
    return TRUE;
}

// Always release the movement pause before unlocking the player.
u16 Follower_EndInteraction(void)
{
    sInteractionActive = FALSE;
    return 0;
}

static void Follower_Remove(void)
{
    sInteractionActive = FALSE;
    sFollowerYieldHoldTrail = FALSE; // Do not carry an old sidestep through recalls.
    // Do NOT use RemoveObjectEventByLocalIdAndMap here: it attempts to set a
    // map-template flag, and dynamically spawned followers have no template.
    RemoveFollowerObjectEvent();
}

// Script special called only after the player accepted Nurse Joy's healing
// offer. Return TRUE to request Emerald's short recall pause. The existing
// sprite is removed safely (without treating it as a map NPC), and the
// follow loop remains disabled throughout the Pokemon Center healing effect.
u16 Follower_BeginHealingRecall(void)
{
    u8 objectEventId;
    bool8 visible = FALSE;
    u8 mapNum = gSaveBlock1Ptr->location.mapNum;
    u8 mapGroup = gSaveBlock1Ptr->location.mapGroup;

    if (!TryGetObjectEventIdByLocalIdAndMap(FOLLOWER_LOCAL_ID, mapNum,
                                            mapGroup, &objectEventId)
     && objectEventId < OBJECT_EVENTS_COUNT
     && gObjectEvents[objectEventId].active)
        visible = TRUE;

    sBattleRecallAwaitStep = FALSE; // Healing's own recall owns this transition.
    // The Nurse Joy recall must never inherit an unfinished doorway latch.
    sDoorWarpRecall = FALSE;
    sDoorRecallOutdoorExitPending = FALSE;
    sDoorRecallExitReady = FALSE;
    sDoorRecallHasAnchor = FALSE;
    sDoorRecallExpectsExitStep = FALSE;
    sHealingRecallState = FOLLOWER_HEAL_IN_PROGRESS;
    sHealingReleaseSoundPending = FALSE;
    sBikeReleaseSoundPending = FALSE;
    Follower_ClearTrail();
    sHasLastPosition = FALSE;
    sAwaitingSafeSpawn = FALSE;
    // Clear any stale-map clones as well as the visible dynamic object.
    Follower_Remove();
    if (visible)
        PlaySE(SE_BALL_TRADE);
    return visible;
}

// The party is now healed, but the nurse's remaining text and bow still run.
// Defer respawn until the script context finishes, rather than materialising
// a Pokemon halfway through the nurse's dialogue.
u16 Follower_EndHealingRecall(void)
{
    if (sHealingRecallState == FOLLOWER_HEAL_IN_PROGRESS)
        sHealingRecallState = FOLLOWER_HEAL_WAIT_FOR_RELEASE;
    return 0;
}

// Called only if FireRed actually accepted an applymovement on the player.
// The follower has no map-event template, so remove it via the dynamic-object
// path instead of trying to make the cutscene script move or freeze it.
void Follower_BeginScriptedPlayerMovement(void)
{
    // These Pokemon League cutscenes contain real, consecutive player steps.
    // FireRed lockall freezes object events, but the dynamic follower should
    // keep following the player on their actual vacated tiles. The previous
    // unconditional recall hid it for the whole corridor and (by arming the
    // Safari rear-only latch below) could strand it after the final dialogue.
    if (Follower_IsLeagueScriptMap() && !QL_IS_PLAYBACK_STATE
     && !sStairWarpRecall && !sFlyRecallActive && !sFieldTravelRecallActive)
    {
        sLeagueScriptFollowing = TRUE;
        sScriptedMovementState = FOLLOWER_SCRIPT_IDLE;
        // A league door/corridor arrival is now owned by the scripted path.
        // These inherited warp/postbattle latches must not suppress its steps.
        sBattleRecallAwaitStep = FALSE;
        sSafariWarpAwaitStep = FALSE;
        sSafariWarpHasAnchor = FALSE;
        sDoorWarpRecall = FALSE;
        sDoorRecallHasAnchor = FALSE;
        sDoorRecallOutdoorExitPending = FALSE;
        sDoorRecallExitReady = FALSE;
        sDoorRecallExpectsExitStep = FALSE;
        return;
    }
    // Facing and ordinary scripted walking must not remove the follower.
    // Special travel, healing, Quest Log and Hall of Fame effects keep their
    // dedicated recall/warp state. Only follow if a visible follower exists.
    if (!QL_IS_PLAYBACK_STATE && !sHallOfFameRecallActive
     && !sStairWarpRecall && !sFlyRecallActive && !sFieldTravelRecallActive
     && !sWarpPadRecall && !sDoorWarpRecall && !sSpinTileRecall
     && sHealingRecallState == FOLLOWER_HEAL_IDLE)
    {
        u8 id;
        if (!TryGetObjectEventIdByLocalIdAndMap(FOLLOWER_LOCAL_ID,
                gSaveBlock1Ptr->location.mapNum,
                gSaveBlock1Ptr->location.mapGroup, &id)
         && id < OBJECT_EVENTS_COUNT && gObjectEvents[id].active
         && !gObjectEvents[id].invisible)
        {
            sGeneralScriptFollowing = TRUE;
            sScriptedMovementState = FOLLOWER_SCRIPT_IDLE;
            UnfreezeObjectEvent(&gObjectEvents[id]);
            return;
        }
    }
    // If a cutscene takes over before the first post-battle step, the
    // normal scripted-movement recall replaces the battle-specific latch.
    sBattleRecallAwaitStep = FALSE;
    sGymRematchReturnPending = FALSE;
    sWildBattleReturnPending = FALSE;
    sScriptedMovementState = FOLLOWER_SCRIPT_MOVING;
    Follower_ClearTrail();
    sHasLastPosition = FALSE;
    sAwaitingSafeSpawn = FALSE;
    Follower_Remove();
}

// The event movement task is destroyed on release/releaseall (also used by
// normal NPC scripts). Wait until the entire cutscene has returned control
// before restoring the follower on a safe overworld tile.
void Follower_EndScriptedPlayerMovement(void)
{
    if (sGeneralScriptFollowing)
    {
        sGeneralScriptFollowing = FALSE;
        return;
    }
    // A League sequence already used the ordinary path-trailing code. Never
    // arm deferred rear-only spawn here: doing so makes the Pokemon vanish
    // after the text until the player walks again.
    if (sLeagueScriptFollowing)
    {
        sLeagueScriptFollowing = FALSE;
        return;
    }
    if (sScriptedMovementState == FOLLOWER_SCRIPT_MOVING)
        sScriptedMovementState = FOLLOWER_SCRIPT_WAIT_FOR_RELEASE;
}

// The Hall of Fame's Record effect begins after Oak congratulates Red.
// The League follower is allowed to follow scripted entry movement normally,
// then is recalled immediately before the machine inserts the Pokeball.
// Suppress the normal dynamic-follower respawn for the rest of this ceremony.
u16 Follower_BeginHallOfFameRecall(void)
{
    u8 id;
    bool8 visible = FALSE;

    if ((gSaveBlock1Ptr->location.mapNum
      | (gSaveBlock1Ptr->location.mapGroup << 8)) != MAP_POKEMON_LEAGUE_HALL_OF_FAME)
        return FALSE;

    if (sHallOfFameRecallActive)
        return FALSE;

    if (!TryGetObjectEventIdByLocalIdAndMap(FOLLOWER_LOCAL_ID,
            gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup, &id)
     && id < OBJECT_EVENTS_COUNT && gObjectEvents[id].active
     && !gObjectEvents[id].invisible)
        visible = TRUE;

    sHallOfFameRecallActive = TRUE;
    sLeagueScriptFollowing = FALSE;
    sGeneralScriptFollowing = FALSE;
    sFollowerYieldHoldTrail = FALSE;
    sScriptedMovementState = FOLLOWER_SCRIPT_IDLE;
    sBattleRecallAwaitStep = FALSE;
    sGymRematchReturnPending = FALSE;
    sWildBattleReturnPending = FALSE;
    sAwaitingSafeSpawn = FALSE;
    sHasLastPosition = FALSE;
    Follower_ClearTrail();
    Follower_Remove();
    if (visible)
        PlaySE(SE_BALL_TRADE);
    return visible;
}

// Called at Fly takeoff while the current field map and follower still exist.
// Fly switches the avatar to ON_FOOT before its departure animation, so the
// normal bike/surf flag check alone cannot keep the follower recalled.
void Follower_BeginFlyRecall(void)
{
    u8 followerId;
    bool8 visible = FALSE;

    if (!TryGetObjectEventIdByLocalIdAndMap(FOLLOWER_LOCAL_ID,
                                           gSaveBlock1Ptr->location.mapNum,
                                           gSaveBlock1Ptr->location.mapGroup,
                                           &followerId)
     && followerId < OBJECT_EVENTS_COUNT
     && gObjectEvents[followerId].active
     && !gObjectEvents[followerId].invisible)
        visible = TRUE;

    if (!sFlyRecallActive)
        sFlyReleaseSoundPending = visible;
    sFlyRecallActive = TRUE;
    sFlyReturnAwaitStep = TRUE;
    sFlyReturnHasAnchor = FALSE;
    sBikeReleaseSoundPending = FALSE;
    sBattleRecallAwaitStep = FALSE;
    Follower_ClearTrail();
    sHasLastPosition = FALSE;
    sAwaitingSafeSpawn = FALSE;
    // Remove stale follower objects too, but never play audio for an object
    // that was already absent or hidden by another recall sequence.
    Follower_Remove();
    if (visible)
        PlaySE(SE_BALL_TRADE);
}

// FireRed's bicycle is toggled by an item-use task, not a generic field
// movement task. That callback explicitly releases ScriptContext2 after the
// normal player graphics have been selected. The old catch-up latch could
// remain set despite a completed bike dismount, indefinitely hiding the mon.
// Clear only the bicycle latch here; Surf still uses its landing safeguards.
void Follower_AfterBicycleDismount(void)
{
    if (!(gPlayerAvatar.flags & (PLAYER_AVATAR_FLAG_MACH_BIKE | PLAYER_AVATAR_FLAG_ACRO_BIKE
                               | PLAYER_AVATAR_FLAG_SURFING | PLAYER_AVATAR_FLAG_UNDERWATER)))
        sResumeFromSpecialMovement = FALSE;
}

// Called at the END of Task_WaitStopSurfing, after the shore-jump animation
// and script lock have finished. The engine clears SURFING at task creation,
// which makes a flag-only return unreliable (especially with a held D-pad).
void Follower_AfterSurfDismount(void)
{
    struct ObjectEvent *player = &gObjectEvents[gPlayerAvatar.objectEventId];

    if (!player->active || !(gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_ON_FOOT))
        return;
    sResumeFromSpecialMovement = FALSE;
    sSurfReturnAwaitStep = TRUE;
    sSurfReturnHasAnchor = TRUE;
    sSurfReturnAnchorX = player->currentCoords.x;
    sSurfReturnAnchorY = player->currentCoords.y;
    sSurfReturnAnchorMapNum = gSaveBlock1Ptr->location.mapNum;
    sSurfReturnAnchorMapGroup = gSaveBlock1Ptr->location.mapGroup;
}

// Called by the battle setup callbacks only after a battle is going to return
// to the field (not on whiteout). Keep this latch across ResumeMap, which
// resets sprites but may preserve gObjectEvents and follower runtime state.
void Follower_NotifyBattleReturn(void)
{
    sFollowerYieldHoldTrail = FALSE; // Fresh battle return owns follower position.
    sBattleReturnPending = TRUE;
    sGymRematchReturnPending = FALSE;
    sWildBattleReturnPending = FALSE;
}

void Follower_NotifyGymRematchReturn(void)
{
    Follower_NotifyBattleReturn();
    sGymRematchReturnPending = TRUE;
}

// Standard local single and rematch Trainers use the same fade-safe rear-tile
// policy as Gym leaders and Double Trainers. The historical latch name is
// retained so older save/transition code and existing regressions stay intact.
void Follower_NotifyTrainerBattleReturn(void)
{
    Follower_NotifyGymRematchReturn();
}

// Local Double Trainer Battles share the same safe rear-tile return as
// ordinary Trainers and Gym rematches; scripted/link callbacks remain separate.
void Follower_NotifyDoubleTrainerBattleReturn(void)
{
    Follower_NotifyGymRematchReturn();
}

void Follower_NotifyWildBattleReturn(void)
{
    Follower_NotifyBattleReturn();
    sWildBattleReturnPending = TRUE;
}

// A player can enter a Pokemon Center immediately after Fly or field-move
// Teleport, before the first voluntary destination step has released the
// follower. A subsequent warp must replace that OLD arrival handoff, not
// stack its pending latch with the new indoor/outdoor warp release.
// Do not cancel a travel effect still in flight: its own warp/map load is
// part of the same journey, and the end callback must remain authoritative.
static void Follower_CancelCompletedTravelHandoffForWarp(void)
{
    if ((sFlyReturnAwaitStep && !sFlyRecallActive)
     || (sFieldTravelAwaitStep && !sFieldTravelRecallActive))
        sInterruptedTravelBuildingEntry = TRUE;

    if (sFlyReturnAwaitStep && !sFlyRecallActive)
    {
        sFlyReturnAwaitStep = FALSE;
        sFlyReturnHasAnchor = FALSE;
        sFlyReleaseSoundPending = FALSE;
    }
    if (sFieldTravelAwaitStep && !sFieldTravelRecallActive)
    {
        sFieldTravelAwaitStep = FALSE;
        sFieldTravelHasAnchor = FALSE;
        sFieldTravelReleaseSoundPending = FALSE;
    }
}

// Emerald-style ordinary door handoff. The follower stays hidden across the
// warp, but the destination's automatic exit step counts as movement; the
// player must NOT take an extra manual step merely to reveal the follower.
// Preserve the approved independent stair/healing/spin handoffs.
void Follower_BeginDoorWarp(void)
{
    if (QL_IS_PLAYBACK_STATE || sStairWarpRecall)
        return;
    Follower_CancelCompletedTravelHandoffForWarp();
    // If the player left the previous map before its first safe step,
    // the NEW building-exit handoff takes ownership of the release.
    sSafariWarpAwaitStep = FALSE;
    sSafariWarpHasAnchor = FALSE;
    sBikeReleaseSoundPending = FALSE;
    sDoorWarpRecall = TRUE;
    sDoorRecallHasAnchor = FALSE;
    sDoorRecallExpectsExitStep = FALSE;
    sDoorRecallOutdoorExitPending = FALSE;
    sDoorRecallExitReady = FALSE;
    Follower_ClearTrail();
    sHasLastPosition = FALSE;
    sAwaitingSafeSpawn = FALSE;
    Follower_Remove();
}

// Called only when a building opens onto an outdoor town or route. A normal
// south-facing door exit automatically walks one tile DOWN; do not count that
// scripted motion as the Safari-style player step which releases the follower.
void Follower_BeginBuildingExitWarp(void)
{
    if (QL_IS_PLAYBACK_STATE || sStairWarpRecall)
        return;
    Follower_BeginDoorWarp();
    sDoorRecallOutdoorExitPending = TRUE;
    sDoorRecallExitReady = FALSE;
}

// Called by the destination warp-exit task AFTER it has finished any
// automatic southward door step and relinquished scripted field control.
// This captures the final doorway position, not a coordinate from the fade.
void Follower_EndBuildingExitWarp(void)
{
    u8 playerId = gPlayerAvatar.objectEventId;

    if (!sDoorWarpRecall || !sDoorRecallOutdoorExitPending || sDoorRecallExitReady)
        return;
    if (playerId >= OBJECT_EVENTS_COUNT || !gObjectEvents[playerId].active)
        return;
    sDoorRecallAnchorX = gObjectEvents[playerId].currentCoords.x;
    sDoorRecallAnchorY = gObjectEvents[playerId].currentCoords.y;
    sDoorRecallMapNum = gSaveBlock1Ptr->location.mapNum;
    sDoorRecallMapGroup = gSaveBlock1Ptr->location.mapGroup;
    sDoorRecallHasAnchor = TRUE;
    sDoorRecallExitReady = TRUE;
}

// Stair transitions use a separate player-sprite pixel animation in FireRed.
// Treat the entire stair warp as one scripted transition, as for a doorway.
void Follower_BeginStairWarp(void)
{
    sBikeReleaseSoundPending = FALSE;
    sDoorWarpRecall = FALSE; // Stairs retain their established recall rules.
    sDoorRecallOutdoorExitPending = FALSE;
    sDoorRecallExitReady = FALSE;
    sDoorRecallHasAnchor = FALSE;
    sDoorRecallExpectsExitStep = FALSE;
    sBattleRecallAwaitStep = FALSE; // Stairs have their own exit recall.
    sStairWarpRecall = TRUE;
    Follower_ClearTrail();
    sHasLastPosition = FALSE;
    sAwaitingSafeSpawn = FALSE;
    Follower_Remove();
}

// Called after the exit task has reset the player's pixel offsets and
// released field control. Rear-only spawning resumes on the next update.
void Follower_EndStairWarp(void)
{
    if (sStairWarpRecall)
    {
        sStairWarpRecall = FALSE;
        Follower_ClearTrail();
        sHasLastPosition = FALSE;
        sAwaitingSafeSpawn = FALSE;
    }
}

// Warp pads use a teleport-out/teleport-in animation rather than the normal
// stair exit. Hide the follower before the departure animation starts; the
// source map's sprite will never remain visible during the transition.
void Follower_BeginWarpPadRecall(void)
{
    u8 objectId;
    bool8 visible = FALSE;

    if (!TryGetObjectEventIdByLocalIdAndMap(FOLLOWER_LOCAL_ID,
                                           gSaveBlock1Ptr->location.mapNum,
                                           gSaveBlock1Ptr->location.mapGroup,
                                           &objectId)
     && objectId < OBJECT_EVENTS_COUNT
     && gObjectEvents[objectId].active
     && !gObjectEvents[objectId].invisible)
        visible = TRUE;

    // A second warp can begin before the pending release has materialised.
    // Do not erase an earlier one-shot sound latch just because that follower
    // is already hidden (also guards redundant warp-start callbacks).
    if (!sWarpPadRecall && !sWarpPadAwaitRelease)
        sWarpPadReleaseSoundPending = visible;
    sWarpPadRecall = TRUE;
    sWarpPadAwaitRelease = FALSE;
    sWarpPadHasAnchor = FALSE;
    sBikeReleaseSoundPending = FALSE;
    sDoorWarpRecall = FALSE;
    sDoorRecallOutdoorExitPending = FALSE;
    sDoorRecallExitReady = FALSE;
    sDoorRecallHasAnchor = FALSE;
    sDoorRecallExpectsExitStep = FALSE;
    sBattleRecallAwaitStep = FALSE;
    Follower_ClearTrail();
    sHasLastPosition = FALSE;
    sAwaitingSafeSpawn = FALSE;
    Follower_Remove();
    if (visible)
        PlaySE(SE_BALL_TRADE);
}

// Only the destination teleport-in task may release this latch, after the
// fade and player animation have completed and ScriptContext2 is disabled.
void Follower_EndWarpPadRecall(void)
{
    if (sWarpPadRecall)
    {
        sWarpPadRecall = FALSE;
        sWarpPadAwaitRelease = TRUE;
        sWarpPadHasAnchor = FALSE;
        Follower_ClearTrail();
        sHasLastPosition = FALSE;
        sAwaitingSafeSpawn = FALSE;
    }
}

// Shared rear-only handoff for all non-overworld warps (caves, interiors,
// Victory Road) and Safari outdoor boundary warps. A transition's destination
// fade must never choose an arbitrary front/side adjacent tile.
void Follower_BeginRearOnlyWarp(void)
{
    // Quest Log playback reconstructs its own movement and follower state.
    if (QL_IS_PLAYBACK_STATE)
        return;
    Follower_CancelCompletedTravelHandoffForWarp();
    sSafariWarpAwaitStep = TRUE;
    sSafariWarpHasAnchor = FALSE;
    sDoorWarpRecall = FALSE;
    sDoorRecallOutdoorExitPending = FALSE;
    sDoorRecallExitReady = FALSE;
    sDoorRecallHasAnchor = FALSE;
    Follower_Remove();
    Follower_ClearTrail();
    sHasLastPosition = FALSE;
    sAwaitingSafeSpawn = FALSE;
}

void Follower_BeginSafariZoneWarp(void)
{
    Follower_BeginRearOnlyWarp();
}

// Called by the Dig / Escape Rope / field Teleport departure task only.
// A repeated callback must never replay the recall sound or lose an earlier
// pending release. Keep separate from Sabrina's warp-pad path.
void Follower_BeginFieldTravelRecall(void)
{
    u8 objectId;
    bool8 visible = FALSE;

    if (QL_IS_PLAYBACK_STATE)
        return;
    if (!TryGetObjectEventIdByLocalIdAndMap(FOLLOWER_LOCAL_ID,
                                            gSaveBlock1Ptr->location.mapNum,
                                            gSaveBlock1Ptr->location.mapGroup,
                                            &objectId)
     && objectId < OBJECT_EVENTS_COUNT
     && gObjectEvents[objectId].active && !gObjectEvents[objectId].invisible)
        visible = TRUE;

    if (!sFieldTravelRecallActive && !sFieldTravelAwaitStep)
        sFieldTravelReleaseSoundPending = visible;
    sFieldTravelRecallActive = TRUE;
    sFieldTravelAwaitStep = FALSE;
    sFieldTravelHasAnchor = FALSE;
    sBikeReleaseSoundPending = FALSE;
    sDoorWarpRecall = FALSE;
    sDoorRecallOutdoorExitPending = FALSE;
    sDoorRecallExitReady = FALSE;
    sDoorRecallHasAnchor = FALSE;
    sDoorRecallExpectsExitStep = FALSE;
    Follower_Remove();
    Follower_ClearTrail();
    sHasLastPosition = FALSE;
    sAwaitingSafeSpawn = FALSE;
    if (visible)
        PlaySE(SE_BALL_TRADE);
}

// Called at the exact END of each arrival task, not when the fade starts.
// Follower_Update handles the later first safe player-controlled step.
void Follower_EndFieldTravelRecall(void)
{
    if (!sFieldTravelRecallActive)
        return;
    sFieldTravelRecallActive = FALSE;
    sFieldTravelAwaitStep = TRUE;
    sFieldTravelHasAnchor = FALSE;
    Follower_ClearTrail();
    sHasLastPosition = FALSE;
    sAwaitingSafeSpawn = FALSE;
}

// The field return loader calls this after it has restored map objects and
// palettes but before rendering the first visible overworld frame. Create
// the follower while the display is still black rather than on the first
// visible player-sprite callback (the after-battle pop-in).
void Follower_PrimeForOverworldDisplay(void)
{
    u8 playerId = gPlayerAvatar.objectEventId;
    if (playerId >= OBJECT_EVENTS_COUNT)
        return;
    if (!gObjectEvents[playerId].active || !gObjectEvents[playerId].isPlayer)
        return;
    if (sFieldTravelRecallActive || sFieldTravelAwaitStep)
    {
        // Neither the pre-display fade nor the Dig/Rope/Teleport landing
        // animation is an allowed spawn point.
        Follower_Remove();
        Follower_ClearTrail();
        sHasLastPosition = FALSE;
        sAwaitingSafeSpawn = FALSE;
        sFieldTravelHasAnchor = FALSE;
        sPreDisplaySpawn = FALSE;
        return;
    }
    if (sSafariWarpAwaitStep)
    {
        struct ObjectEvent *player = &gObjectEvents[playerId];
        sSafariWarpAnchorX = player->currentCoords.x;
        sSafariWarpAnchorY = player->currentCoords.y;
        sSafariWarpAnchorMapNum = gSaveBlock1Ptr->location.mapNum;
        sSafariWarpAnchorMapGroup = gSaveBlock1Ptr->location.mapGroup;
        sSafariWarpHasAnchor = TRUE;
        Follower_Remove();
        Follower_ClearTrail();
        sHasLastPosition = FALSE;
        sAwaitingSafeSpawn = FALSE;
        sPreDisplaySpawn = FALSE;
        return;
    }
    if (sFlyReturnAwaitStep)
    {
        // Fly has reached its destination, but the trainer may still be in
        // the landing animation. Even the black-frame pre-display callback
        // must not make the follower visible on the arrival tile.
        struct ObjectEvent *player = &gObjectEvents[playerId];
        sFlyReturnAnchorX = player->currentCoords.x;
        sFlyReturnAnchorY = player->currentCoords.y;
        sFlyReturnAnchorMapNum = gSaveBlock1Ptr->location.mapNum;
        sFlyReturnAnchorMapGroup = gSaveBlock1Ptr->location.mapGroup;
        sFlyReturnHasAnchor = TRUE;
        Follower_Remove();
        Follower_ClearTrail();
        sHasLastPosition = FALSE;
        sAwaitingSafeSpawn = FALSE;
        sPreDisplaySpawn = FALSE;
        sBattleReturnPending = FALSE;
        return;
    }
    if (sDoorWarpRecall && !QL_IS_PLAYBACK_STATE)
    {
        // Emerald's warpEnd flag is set before the map reset and its follower
        // position is re-based on the destination player object. Do likewise
        // here, before FireRed runs its automatic doorway exit movement.
        struct ObjectEvent *player = &gObjectEvents[playerId];
        sDoorRecallAnchorX = player->currentCoords.x;
        sDoorRecallAnchorY = player->currentCoords.y;
        sDoorRecallMapNum = gSaveBlock1Ptr->location.mapNum;
        sDoorRecallMapGroup = gSaveBlock1Ptr->location.mapGroup;
        sDoorRecallHasAnchor = TRUE;
        // Matches the destination's FieldCB_DefaultWarpExit branch: only a
        // warp door or a cave door has an automatic tile step in FireRed.
        // The generic fade-only exit does not, and must not stay invisible
        // until the player voluntarily walks again.
        {
            u8 behavior = MapGridGetMetatileBehaviorAt(player->currentCoords.x,
                                                       player->currentCoords.y);
            sDoorRecallExpectsExitStep = MetatileBehavior_IsWarpDoor_2(behavior)
                                      || MetatileBehavior_IsNonAnimDoor(behavior);
        }
        Follower_Remove();
        Follower_ClearTrail();
        sHasLastPosition = FALSE;
        sPreDisplaySpawn = FALSE;
        return;
    }
    if (sBattleReturnPending && !sStairWarpRecall)
    {
        struct ObjectEvent *player = &gObjectEvents[playerId];

        // Unlike a normal map load, battle return reconstructs the field and
        // weather palettes in several stages. Rebuilding the follower before
        // the fade (v0.8.13), or repairing slot 11 each frame (v0.8.14),
        // still exposed a few gray frames in-game. Use the existing Nurse Joy
        // / stair-exit recall idiom instead: do not draw a follower at all
        // until the PLAYER finishes the next genuine controllable tile step.
        Follower_Remove();
        Follower_ClearTrail();
        sHasLastPosition = FALSE;
        sAwaitingSafeSpawn = FALSE;
        sBattleRecallAwaitStep = TRUE;
        sBattleRecallAnchorX = player->currentCoords.x;
        sBattleRecallAnchorY = player->currentCoords.y;
        sBattleRecallMapNum = gSaveBlock1Ptr->location.mapNum;
        sBattleRecallMapGroup = gSaveBlock1Ptr->location.mapGroup;
        sBattlePaletteGuardFrames = 0; // No visible follower needs guarding.
        sBattlePaletteSettledFrames = 0;
    }
    else
    {
        // Preserve the standard pre-display placement for ALL other map
        // loads, Quest Log playback and stair warp state. No new delay there.
        sPreDisplaySpawn = TRUE;
        Follower_Update(&gObjectEvents[playerId]);
        sPreDisplaySpawn = FALSE;
    }
    sBattleReturnPending = FALSE;
}

// Emerald moves the existing follower object through connected maps instead of
// removing/recreating its sprite. The object and its movement history must
// survive FireRed's LoadMapFromCameraTransition, too.
void Follower_BeginMapConnectionTransition(void)
{
    // Do not invalidate sHasLastPosition or clear sTrail: the camera will
    // translate both player and follower into destination-map coordinates.
    // This entry hook is kept for compatibility with earlier integration.
}

// Called after ApplyCurrentWarp, before FireRed rebases object coordinates.
// Retain the closest valid follower instance; older builds could leave clones.
void Follower_FinishMapConnectionTransition(void)
{
    u8 i;
    u8 keep = OBJECT_EVENTS_COUNT;
    s32 bestDistance = 0x7FFFFFFF;
    u8 mapNum = gSaveBlock1Ptr->location.mapNum;
    u8 mapGroup = gSaveBlock1Ptr->location.mapGroup;
    const struct ObjectEvent *player = &gObjectEvents[gPlayerAvatar.objectEventId];

    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        struct ObjectEvent *objectEvent = &gObjectEvents[i];
        s32 dx, dy, distance;
        if (!objectEvent->active || objectEvent->isPlayer
         || objectEvent->localId != FOLLOWER_LOCAL_ID
         || objectEvent->graphicsId != OBJ_EVENT_GFX_FOLLOWER)
            continue;

        dx = objectEvent->currentCoords.x - player->currentCoords.x;
        dy = objectEvent->currentCoords.y - player->currentCoords.y;
        if (dx < 0) dx = -dx;
        if (dy < 0) dy = -dy;
        distance = dx + dy;
        if (distance < bestDistance)
        {
            bestDistance = distance;
            keep = i;
        }
    }

    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        struct ObjectEvent *objectEvent = &gObjectEvents[i];
        if (!objectEvent->active || objectEvent->isPlayer
         || objectEvent->localId != FOLLOWER_LOCAL_ID
         || objectEvent->graphicsId != OBJ_EVENT_GFX_FOLLOWER)
            continue;

        if (i == keep)
        {
            // This local ID is not an engine special-case; a lookup also
            // compares the map group/number, so update both immediately.
            objectEvent->mapNum = mapNum;
            objectEvent->mapGroup = mapGroup;
        }
        else
        {
            RemoveFollowerObjectEventById(i);
        }
    }

    if (sHasLastPosition)
    {
        sLastMapNum = mapNum;
        sLastMapGroup = mapGroup;
    }
}

// gCamera.x/y are subtracted from all event coordinates by FireRed's camera.
// Rebase the cached player tile and EVERY queued tile by the identical delta.
// Otherwise the very next step is interpreted as a teleport across the map.
void Follower_RebaseForCameraUpdate(s16 dx, s16 dy)
{
    u8 i;
    // Camera rebases also happen while the battle follower is still recalled.
    // Keep its prior-tile anchor in the same map coordinate space as player.
    if (sBattleRecallAwaitStep)
    {
        sBattleRecallAnchorX -= dx;
        sBattleRecallAnchorY -= dy;
    }
    if (sFieldTravelAwaitStep && sFieldTravelHasAnchor)
    {
        sFieldTravelAnchorX -= dx;
        sFieldTravelAnchorY -= dy;
    }
    if (sFlyReturnAwaitStep && sFlyReturnHasAnchor)
    {
        sFlyReturnAnchorX -= dx;
        sFlyReturnAnchorY -= dy;
    }
    if (sSafariWarpAwaitStep && sSafariWarpHasAnchor)
    {
        sSafariWarpAnchorX -= dx;
        sSafariWarpAnchorY -= dy;
    }
    if (sDoorWarpRecall && sDoorRecallHasAnchor)
    {
        sDoorRecallAnchorX -= dx;
        sDoorRecallAnchorY -= dy;
    }
    if (sWarpPadAwaitRelease && sWarpPadHasAnchor)
    {
        sWarpPadAnchorX -= dx;
        sWarpPadAnchorY -= dy;
    }
    if (!sHasLastPosition)
        return;

    sLastPlayerX -= dx;
    sLastPlayerY -= dy;
    if (sPlayerJumpInProgress)
    {
        sPlayerJumpLandingX -= dx;
        sPlayerJumpLandingY -= dy;
    }
    for (i = 0; i < sTrailCount; i++)
    {
        u8 index = (sTrailHead + i) % FOLLOWER_TRAIL_LENGTH;
        sTrail[index].x -= dx;
        sTrail[index].y -= dy;
    }
}

// Secondary tileset / weather operations can touch the sprite palette during
// connection loading. Restore the selected follower palette afterwards.
void Follower_RefreshPalette(void)
{
    if (sFollowerSpecies == SPECIES_NONE)
        return;

    LoadPalette(Follower_GetSelectedPalette(),
                0x100 + FOLLOWER_PALETTE_SLOT * 16, 32);
    ApplyGlobalFieldPaletteTint(FOLLOWER_PALETTE_SLOT);
    // The follower is an ordinary overworld object, not a fog particle.
    // The generic weather palette helper gives newly loaded palettes a heavy
    // white blend in horizontal fog (and registers them in the fog-lightened
    // sprite list during fade-in). That washes out this reserved, dynamically
    // refreshed palette in Pokemon Tower while the trainer/NPCs stay opaque.
    if (GetCurrentWeather() == WEATHER_FOG_HORIZONTAL)
    {
        // Maintain weather fade timing without registering this ordinary OBJ
        // palette in the special fog-sprite lightening list.
        if (gWeatherPtr->palProcessingState == WEATHER_PAL_STATE_SCREEN_FADING_IN)
        {
            BlendPalette(0x100 + FOLLOWER_PALETTE_SLOT * 16, 16,
                         16 - gWeatherPtr->fadeScreenCounter,
                         gWeatherPtr->fadeDestColor);
        }
        else if (gWeatherPtr->palProcessingState == WEATHER_PAL_STATE_SCREEN_FADING_OUT)
        {
            BlendPalette(0x100 + FOLLOWER_PALETTE_SLOT * 16, 16,
                         gPaletteFade.y, gPaletteFade.blendColor);
        }
        // In settled fog LoadPalette supplies the correct normal/shiny colors;
        // the weather effect is rendered by separate fog sprites.
    }
    else
    {
        UpdateSpritePaletteWithWeather(FOLLOWER_PALETTE_SLOT);
    }
}

// Quest Log B-skip returns to field control quickly, sometimes before its
// dynamic follower sprite is initialized. Only repair an actual active sprite.
// A missing/not-yet-ready sprite must NOT consume or cancel the repair.
static bool8 Follower_GetQuestLogSkipSpriteId(u8 *spriteId)
{
    u8 followerId;

    if (sQuestLogSkipPaletteFrames == 0 || QL_IS_PLAYBACK_STATE
     || gGlobalFieldTintMode != QL_TINT_NONE
     || gPaletteFade.active || !IsWeatherNotFadingIn())
        return FALSE;
    if (sFollowerSpecies == SPECIES_NONE
     || TryGetObjectEventIdByLocalIdAndMap(FOLLOWER_LOCAL_ID,
                                           gSaveBlock1Ptr->location.mapNum,
                                           gSaveBlock1Ptr->location.mapGroup,
                                           &followerId)
     || followerId >= OBJECT_EVENTS_COUNT
     || !gObjectEvents[followerId].active
     || gObjectEvents[followerId].graphicsId != OBJ_EVENT_GFX_FOLLOWER
     || gObjectEvents[followerId].spriteId >= MAX_SPRITES)
        return FALSE;

    *spriteId = gObjectEvents[followerId].spriteId;
    return TRUE;
}

void Follower_ArmQuestLogSkipPaletteRepair(void)
{
    sQuestLogSkipPaletteFrames = FOLLOWER_QUEST_LOG_SKIP_PALETTE_FRAMES;
}

// OAM's palette number is separate from the palette's colour buffers. The
// previous repairs restored colours in slot 11 but never verified that the
// follower *draws* from slot 11. Fix this before BuildOamBuffer copies OAM.
void Follower_RepairQuestLogSkipSpriteSlot(void)
{
    u8 spriteId;
    if (Follower_GetQuestLogSkipSpriteId(&spriteId))
        gSprites[spriteId].oam.paletteNum = FOLLOWER_PALETTE_SLOT;
}

// Run after UpdatePaletteFade and before VBlank. Only ready/visible followers
// count toward the four settled palette repairs. There is no work after the
// handoff completes, and the ordinary Quest Log playback path is untouched.
void Follower_RepairQuestLogSkipPalette(void)
{
    u8 spriteId;
    if (!Follower_GetQuestLogSkipSpriteId(&spriteId))
        return;

    gSprites[spriteId].oam.paletteNum = FOLLOWER_PALETTE_SLOT;
    Follower_RefreshPalette();
    sPaletteRefreshAfterLoad = FALSE;
    sQuestLogSkipPaletteFrames--;
}

// Object OAM is copied by BuildOamBuffer before the final palette processing.
// Fix the sprite's own slot in that same frame, not one frame later.
void Follower_StabilizeBattleReturnSpriteSlot(void)
{
    u8 followerId;
    if (sBattlePaletteGuardFrames == 0)
        return;
    if (TryGetObjectEventIdByLocalIdAndMap(FOLLOWER_LOCAL_ID,
                                            gSaveBlock1Ptr->location.mapNum,
                                            gSaveBlock1Ptr->location.mapGroup,
                                            &followerId)
     || followerId >= OBJECT_EVENTS_COUNT
     || !gObjectEvents[followerId].active
     || gObjectEvents[followerId].graphicsId != OBJ_EVENT_GFX_FOLLOWER)
        return;
    if (gObjectEvents[followerId].spriteId < MAX_SPRITES)
        gSprites[gObjectEvents[followerId].spriteId].oam.paletteNum = FOLLOWER_PALETTE_SLOT;
}

// Called by OverworldBasic AFTER the engine's UpdatePaletteFade and BEFORE
// the hardware palette transfer. Unlike Follower_RefreshPalette this does not
// reset the faded palette to full brightness in the middle of a transition.
// It is enabled only for a short period after battle; Pokecenter healing,
// Rocket spin recall, ordinary map loads, and the Quest Log are unaffected.
void Follower_StabilizeBattleReturnPalette(void)
{
    u8 followerId;
    u8 i;
    const u16 *selected;
    u16 *unfaded;

    if (sBattlePaletteGuardFrames == 0)
        return;
    sBattlePaletteGuardFrames--;

    // Quest Log grayscaling/sepia is deliberate; do not force full colour.
    if (gGlobalFieldTintMode != QL_TINT_NONE || sFollowerSpecies == SPECIES_NONE)
        return;
    if (TryGetObjectEventIdByLocalIdAndMap(FOLLOWER_LOCAL_ID,
                                            gSaveBlock1Ptr->location.mapNum,
                                            gSaveBlock1Ptr->location.mapGroup,
                                            &followerId)
     || followerId >= OBJECT_EVENTS_COUNT
     || !gObjectEvents[followerId].active
     || gObjectEvents[followerId].graphicsId != OBJ_EVENT_GFX_FOLLOWER)
        return;

    selected = Follower_GetSelectedPalette();
    unfaded = &gPlttBufferUnfaded[0x100 + FOLLOWER_PALETTE_SLOT * 16];
    for (i = 0; i < 16; i++)
    {
        if (unfaded[i] != selected[i])
            unfaded[i] = selected[i];
    }

    // This runs after UpdatePaletteFade. Rebuild the 16 displayed colours
    // from their selected normal/shiny source using the SAME coefficient as
    // the engine's normal field fade (black or white). This repairs a bad
    // faded buffer without causing the full-bright flash LoadPalette would.
    if (gPaletteFade.active && gPaletteFade.mode == 0 /* NORMAL_FADE (palette.c private enum) */
     && (gPaletteFade_selectedPalettes & (1u << (16 + FOLLOWER_PALETTE_SLOT))))
    {
        BlendPalette(0x100 + FOLLOWER_PALETTE_SLOT * 16, 16,
                     gPaletteFade.y, gPaletteFade.blendColor);
    }
    else if (!gPaletteFade.active && IsWeatherNotFadingIn())
    {
        // Horizontal fog's special palette whitening is for weather sprites,
        // not our ordinary follower. Non-fog weather retains normal gamma.
        if (GetCurrentWeather() != WEATHER_FOG_HORIZONTAL)
            UpdateSpritePaletteWithWeather(FOLLOWER_PALETTE_SLOT);
    }

    if (!gPaletteFade.active && IsWeatherNotFadingIn())
    {
        if (++sBattlePaletteSettledFrames >= 4)
            sBattlePaletteGuardFrames = 0;
    }
    else
    {
        sBattlePaletteSettledFrames = 0;
    }
}

// Called on every player-sprite update, even when the player is stationary.
// This lets pending trail steps drain after the player stops running.
// Apply Emerald's render-only horizontal spacing. Never shift tile coords:
// those must remain on the player's actual path for collisions and warps.
static void Follower_ApplyHorizontalSpacing(const struct ObjectEvent *follower)
{
    struct Sprite *sprite;
    if (follower->spriteId >= MAX_SPRITES)
        return;

    sprite = &gSprites[follower->spriteId];
    // Render the alert sidestep on its actual map tile, without drawing
    // the follower 8 pixels back into the Trainer's approach line.
    if (sFollowerYieldHoldTrail && gPlayerAvatar.objectEventId < OBJECT_EVENTS_COUNT)
    {
        const struct ObjectEvent *player = &gObjectEvents[gPlayerAvatar.objectEventId];
        if (player->active && player->currentCoords.x == sFollowerYieldPlayerX
         && player->currentCoords.y == sFollowerYieldPlayerY)
        {
            sprite->x2 = 0;
            return;
        }
    }
    if (follower->facingDirection == DIR_EAST)
        sprite->x2 = -FOLLOWER_SIDE_SPACING_PIXELS;
    else if (follower->facingDirection == DIR_WEST)
        sprite->x2 = FOLLOWER_SIDE_SPACING_PIXELS;
    else
        sprite->x2 = 0;
}

// FireRed's spin arrows force movement until STOP_SPINNING. Tracking each
// intermediate step with native spin actions caused snap/catch-up artifacts.
// The follower is instead hidden for the entire forced spin sequence.
static bool8 Follower_IsSpinOrStopTile(u8 behavior, bool8 stop)
{
    if (stop)
        return MetatileBehavior_IsStopSpinning(behavior);
    return MetatileBehavior_IsSpinTile(behavior);
}

static void Follower_DrainTrail(struct ObjectEvent *follower)
{
    struct FollowerTrailEntry *next;
    s16 dx, dy;
    u8 direction;
    u8 movementAction;

    if (!sTrailCount)
        return;

    if (ObjectEventIsHeldMovementActive(follower))
    {
        if (!ObjectEventClearHeldMovementIfFinished(follower))
            return;
    }
    if (ObjectEventIsMovementOverridden(follower))
        return;

    next = &sTrail[sTrailHead];
    // During replay the recorded player can reverse direction or pause on
    // a previously visited tile. Never drain a historical trail step INTO
    // the player's current tile, even if the recording has no more movement.
    if (QL_IS_PLAYBACK_STATE && gPlayerAvatar.objectEventId < OBJECT_EVENTS_COUNT
     && next->x == gObjectEvents[gPlayerAvatar.objectEventId].currentCoords.x
     && next->y == gObjectEvents[gPlayerAvatar.objectEventId].currentCoords.y)
    {
        Follower_Dequeue();
        return;
    }
    dx = next->x - follower->currentCoords.x;
    dy = next->y - follower->currentCoords.y;
    if (dx == 0 && dy == 0)
    {
        Follower_Dequeue();
        return;
    }

    // Following during scripted walking never sends the follower into a
    // stationary NPC. The NPC itself is never delayed or rerouted.
    if (sGeneralScriptFollowing && !Follower_IsSafeSpawnTile(next->x, next->y))
        return;

    direction = Follower_DirectionTo(dx, dy);
    if (direction == DIR_NONE)
    {
        // A scripted relocation or discontinuity. Clear an obsolete trail
        // rather than trying to animate diagonally or across the map.
        Follower_ClearTrail();
        if (Follower_IsSafeSpawnTile(sLastPlayerX, sLastPlayerY))
            MoveObjectEventToMapCoords(follower, sLastPlayerX, sLastPlayerY);
        return;
    }

    // Dispatch the player's recorded jump as a native two-tile action.
    // The arrival tile is added when the player leaves it, *not* when the
    // player's first half-jump crosses onto the ledge metatile.
    if (next->jump)
    {
        if (!Follower_IsLedgeJump(follower->currentCoords.x, follower->currentCoords.y,
                                 next->x, next->y))
        {
            Follower_ClearTrail();
            return;
        }
        movementAction = GetJump2MovementAction(direction);
    }
    else if (dx == 2 || dx == -2 || dy == 2 || dy == -2)
    {
        // Never walk over a missing tile when the trail was interrupted.
        Follower_ClearTrail();
        return;
    }
    else if (sTrailCount >= 2)
        // Catch up when running/cornering rather than opening a multi-tile gap.
        movementAction = GetWalkFastestMovementAction(direction);
    else if (gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_DASH)
        // Keep pace with player running rather than walking at normal speed.
        movementAction = GetWalkFastMovementAction(direction);
    else
        movementAction = GetWalkNormalMovementAction(direction);

    // Only dequeue AFTER the engine accepted this action. Do not clobber an
    // in-progress animation, which caused corner-cutting and snaps in v0.3.
    if (!ObjectEventSetHeldMovement(follower, movementAction))
    {
        // The player uses SE_LEDGE for Jump2. Emit the same hop sound only
        // when the follower's delayed native Jump2 movement is accepted;
        // ordinary steps, blocked jumps and rejected movement stay silent.
        if (next->jump)
            PlaySE(SE_LEDGE);
        Follower_Dequeue();
    }
}


// Observer-only notification for NPC movement. It MUST NOT return a value
// to, pause, reroute, or otherwise change the NPC's original engine task.
// Only the follower gets a held movement, at most one tile.
static bool8 Follower_YieldForNpc(u8 npcId, u8 direction, bool8 alert)
{
    struct ObjectEvent *npc, *follower, *player;
    u8 id, dirs[4], i, best = DIR_NONE;
    s16 fx, fy, nx, ny, tx, ty, px, py, targetX, targetY;
    s16 score, bestScore = -32767;
    bool8 inLine;
    bool8 oakEnteringChampionAisle;

    if (direction < DIR_SOUTH || direction > DIR_EAST || npcId >= OBJECT_EVENTS_COUNT
     || !gObjectEvents[npcId].active || gObjectEvents[npcId].isPlayer
     || gPlayerAvatar.objectEventId >= OBJECT_EVENTS_COUNT
     || QL_IS_PLAYBACK_STATE || sHallOfFameRecallActive
     || sHealingRecallState != FOLLOWER_HEAL_IDLE
     || sStairWarpRecall || sFlyRecallActive || sFieldTravelRecallActive
     || sWarpPadRecall || sDoorWarpRecall || sSpinTileRecall)
        return FALSE;
    if (TryGetObjectEventIdByLocalIdAndMap(FOLLOWER_LOCAL_ID,
            gSaveBlock1Ptr->location.mapNum,
            gSaveBlock1Ptr->location.mapGroup, &id) || id >= OBJECT_EVENTS_COUNT)
        return FALSE;
    follower = &gObjectEvents[id];
    npc = &gObjectEvents[npcId];
    player = &gObjectEvents[gPlayerAvatar.objectEventId];
    if (!follower->active || follower->invisible || follower->spriteId >= MAX_SPRITES
     || !player->active || sFollowerYieldHoldTrail
     || (ObjectEventIsHeldMovementActive(follower)
      && !ObjectEventClearHeldMovementIfFinished(follower)))
        return FALSE;

    fx = follower->currentCoords.x;
    fy = follower->currentCoords.y;
    tx = npc->currentCoords.x;
    ty = npc->currentCoords.y;
    px = player->currentCoords.x;
    py = player->currentCoords.y;
    targetX = tx;
    targetY = ty;
    MoveCoords(direction, &targetX, &targetY);
    inLine = (tx == px && fx == tx
           && fy >= (ty < py ? ty : py) && fy <= (ty > py ? ty : py))
          || (ty == py && fy == ty
           && fx >= (tx < px ? tx : px) && fx <= (tx > px ? tx : px));
    if (alert)
    {
        if (!inLine && !(fx == targetX && fy == targetY))
            return FALSE;
    }
    else if (fx != targetX || fy != targetY)
        return FALSE;

    // Oak's unchanged Champion Room entrance walks north through (6, 10),
    // then turns WEST into (5, 10). A westward follower yield at (6, 10)
    // would put the Pokemon on Oak's NEXT tile. Select the east aisle only
    // for this exact encounter; Oak's script, timing and position are untouched.
    // Object-event coordinates include the seven-tile map border.
    oakEnteringChampionAisle = !alert && direction == DIR_NORTH
        && npc->localId == 2  // LOCALID_PROF_OAK in ChampionsRoom scripts.inc
        && gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(POKEMON_LEAGUE_CHAMPIONS_ROOM)
        && gSaveBlock1Ptr->location.mapNum == MAP_NUM(POKEMON_LEAGUE_CHAMPIONS_ROOM)
        && fx == 6 + 7 && fy == 10 + 7
        && tx == 6 + 7 && ty == 11 + 7;

    // Prefer a perpendicular sidestep, never a second tile or pixel offset.
    if (direction == DIR_NORTH || direction == DIR_SOUTH)
    {
        dirs[0] = oakEnteringChampionAisle ? DIR_EAST : DIR_WEST;
        dirs[1] = oakEnteringChampionAisle ? DIR_WEST : DIR_EAST;
        dirs[2] = DIR_NORTH; dirs[3] = DIR_SOUTH;
    }
    else
    {
        dirs[0] = DIR_NORTH; dirs[1] = DIR_SOUTH;
        dirs[2] = DIR_WEST; dirs[3] = DIR_EAST;
    }
    for (i = 0; i < 4; i++)
    {
        nx = fx;
        ny = fy;
        MoveCoords(dirs[i], &nx, &ny);
        if (!Follower_IsSafeSpawnTile(nx, ny)
         || MetatileBehavior_IsWarpPad(MapGridGetMetatileBehaviorAt(nx, ny)))
            continue;
        // West is Oak's next scripted tile; never use it as fallback when
        // the east aisle is obstructed. Do not interfere with Oak instead.
        if (oakEnteringChampionAisle && nx == fx - 1 && ny == fy)
            continue;
        // A Trainer's route is fixed. Never yield onto its sight line.
        if ((nx == px && ny == py) || (nx == tx && ny == ty)
         || (nx == targetX && ny == targetY)
         || (alert && ((tx == px && nx == tx
              && ny >= (ty < py ? ty : py) && ny <= (ty > py ? ty : py))
           || (ty == py && ny == ty
              && nx >= (tx < px ? tx : px) && nx <= (tx > px ? tx : px)))))
            continue;
        score = (nx > px ? nx - px : px - nx)
              + (ny > py ? ny - py : py - ny);
        if (score > bestScore)
        {
            bestScore = score;
            best = dirs[i];
        }
    }
    if (best == DIR_NONE)
        return FALSE; // NPC continues normally even in cramped scenes.
    UnfreezeObjectEvent(follower);
    if (ObjectEventSetHeldMovement(follower, GetWalkFastMovementAction(best)))
        return FALSE;
    sFollowerYieldHoldTrail = TRUE;
    sFollowerYieldPlayerX = px;
    sFollowerYieldPlayerY = py;
    Follower_ClearTrail();
    return TRUE;
}

void Follower_ObserveTrainerAlert(u8 trainerId)
{
    if (trainerId < OBJECT_EVENTS_COUNT)
        Follower_YieldForNpc(trainerId, gObjectEvents[trainerId].facingDirection, TRUE);
}

// For ordinary scripted NPC steps, observe only the action about to execute.
// Return nothing to the caller and never alter the NPC's movement script.
void Follower_ObserveNpcScriptStep(u8 npcId, u8 action)
{
    u8 direction = DIR_NONE;
    if (action >= MOVEMENT_ACTION_WALK_SLOW_DOWN && action <= MOVEMENT_ACTION_WALK_SLOW_RIGHT)
        direction = action - MOVEMENT_ACTION_WALK_SLOW_DOWN + DIR_SOUTH;
    else if (action >= MOVEMENT_ACTION_WALK_NORMAL_DOWN && action <= MOVEMENT_ACTION_WALK_NORMAL_RIGHT)
        direction = action - MOVEMENT_ACTION_WALK_NORMAL_DOWN + DIR_SOUTH;
    else if (action >= MOVEMENT_ACTION_WALK_FAST_DOWN && action <= MOVEMENT_ACTION_WALK_FAST_RIGHT)
        direction = action - MOVEMENT_ACTION_WALK_FAST_DOWN + DIR_SOUTH;
    else if (action >= MOVEMENT_ACTION_WALK_FASTEST_DOWN && action <= MOVEMENT_ACTION_WALK_FASTEST_RIGHT)
        direction = action - MOVEMENT_ACTION_WALK_FASTEST_DOWN + DIR_SOUTH;
    if (direction != DIR_NONE)
        Follower_YieldForNpc(npcId, direction, FALSE);
}

bool8 Follower_IsCollisionExempt(const struct ObjectEvent *a, const struct ObjectEvent *b)
{
    // Neither the player nor scripted NPCs should be trapped by the follower.
    return (a->active && b->active && (a->localId == FOLLOWER_LOCAL_ID || b->localId == FOLLOWER_LOCAL_ID));
}

void Follower_Update(struct ObjectEvent *player)
{
    struct ObjectEvent *follower;
    u16 species;
    bool8 shiny;
    u8 objectId;
    u8 mapNum, mapGroup;
    u8 direction;
    s16 x, y, dx, dy;
    bool8 exists;
    bool8 newMap;
    bool8 usingSpecialMovement;
    bool8 specialTransitionPending;
    bool8 playerStartedLedge;
    bool8 playerFinishedLedge;
    bool8 jumpTrailEntry;
    u8 playerMetatileBehavior;
    u8 gridMetatileBehavior;
    bool8 spinStopTile;
    bool8 gridSpinTile;
    bool8 gridStopTile;
    bool8 spinRecallFirstSpawn = FALSE;
    bool8 battleRecallFirstSpawn = FALSE;
    bool8 doorRecallFirstSpawn = FALSE;
    bool8 warpPadRecallFirstSpawn = FALSE;
    bool8 flyReturnFirstSpawn = FALSE;
    bool8 surfReturnFirstSpawn = FALSE;
    bool8 safariWarpFirstSpawn = FALSE;
    bool8 fieldTravelFirstSpawn = FALSE;

    if (!player->active || !player->isPlayer)
        return;

    mapNum = gSaveBlock1Ptr->location.mapNum;
    mapGroup = gSaveBlock1Ptr->location.mapGroup;
    exists = !TryGetObjectEventIdByLocalIdAndMap(FOLLOWER_LOCAL_ID, mapNum, mapGroup, &objectId);
    species = Follower_GetLeadSpecies(&shiny);
    newMap = !sHasLastPosition || mapNum != sLastMapNum || mapGroup != sLastMapGroup;

    // The Hall of Fame machine must register Pokeballs, not a visible
    // follower. The ceremony is script-locked; don't respawn after recall.
    // ResetForMapLoad clears this guard for ordinary post-League gameplay.
    if (sHallOfFameRecallActive)
    {
        if (exists)
            Follower_Remove();
        return;
    }

    // lockall freezes all non-player objects before applymovement executes.
    // Only the League's intentional cutscene-follow path may unfreeze its
    // follower; ordinary conversation, healing and other scripted movements
    // keep their original freeze/recall rules.
    if ((sLeagueScriptFollowing || sGeneralScriptFollowing)
     && exists && objectId < OBJECT_EVENTS_COUNT)
        UnfreezeObjectEvent(&gObjectEvents[objectId]);

    // The stair warp manipulates only the player sprite's pixel offsets.
    // Never place a grid-aligned follower beside an animated stair player.
    if (sStairWarpRecall)
    {
        if (exists)
            Follower_Remove();
        Follower_ClearTrail();
        sHasLastPosition = FALSE;
        sAwaitingSafeSpawn = FALSE;
        return;
    }

    if (sWarpPadRecall)
    {
        // ResetObjectEvents runs between teleport-out and teleport-in even
        // for Sabrina's Gym's same-map pads. Do not permit a pre-fade spawn.
        if (exists)
            Follower_Remove();
        Follower_ClearTrail();
        sHasLastPosition = FALSE;
        sAwaitingSafeSpawn = FALSE;
        return;
    }

    if (sFieldTravelRecallActive)
    {
        if (exists)
            Follower_Remove();
        Follower_ClearTrail();
        sHasLastPosition = FALSE;
        sAwaitingSafeSpawn = FALSE;
        return;
    }

    if (sFlyRecallActive)
    {
        if (exists)
            Follower_Remove();
        Follower_ClearTrail();
        sHasLastPosition = FALSE;
        sAwaitingSafeSpawn = FALSE;
        return;
    }

    // Use the actual grid tile as the primary signal. With the D-pad held,
    // currentMetatileBehavior can lag behind currentCoords by a frame; treating
    // that stale cached arrow as a new spin would re-arm recall forever.
    playerMetatileBehavior = player->currentMetatileBehavior;
    gridMetatileBehavior = MapGridGetMetatileBehaviorAt(player->currentCoords.x,
                                                       player->currentCoords.y);
    gridSpinTile = Follower_IsSpinOrStopTile(gridMetatileBehavior, FALSE);
    gridStopTile = Follower_IsSpinOrStopTile(gridMetatileBehavior, TRUE);
    spinStopTile = gridStopTile || Follower_IsSpinOrStopTile(playerMetatileBehavior, TRUE);
    if (!gridStopTile
     && (gridSpinTile
      || ((gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_FORCED)
       && (Follower_IsSpinOrStopTile(playerMetatileBehavior, FALSE)
        || Follower_IsSpinOrStopTile(gPlayerAvatar.lastSpinTile, FALSE)))))
    {
        if (!sSpinTileRecall)
            sSpinStopHasAnchor = FALSE;
        sSpinTileRecall = TRUE;
        // Forced arrow movement is not an ordinary player-controlled step.
        sBattleRecallAwaitStep = FALSE;
    }

    if (sSpinTileRecall)
    {
        // Record the stop tile while the player is on it, even if its held
        // forced-movement action is still animating. A direct move off STOP
        // should materialise the follower on this newly vacated tile.
        if (gridStopTile)
        {
            sSpinStopHasAnchor = TRUE;
            sSpinStopX = player->currentCoords.x;
            sSpinStopY = player->currentCoords.y;
        }
        else if (!sSpinStopHasAnchor
              && MetatileBehavior_IsStopSpinning(
                   MapGridGetMetatileBehaviorAt(player->previousCoords.x,
                                                player->previousCoords.y)))
        {
            // If a frame skipped the stop tile, the engine still records it
            // as previousCoords when the first controlled step starts.
            sSpinStopHasAnchor = TRUE;
            sSpinStopX = player->previousCoords.x;
            sSpinStopY = player->previousCoords.y;
        }

        // Unlike the old idle-only gate, accept the *start* of the first
        // genuine controlled step after STOP. This works with uninterrupted
        // D-pad input, T_TILE_TRANSITION, and active held walking animation.
        // Require a neighbouring destination: scripted teleports and skips
        // cannot cause a forward/sideways follower spawn.
        if (sSpinStopHasAnchor
         && !(gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_FORCED)
         && !gridSpinTile && !gridStopTile
         && !ScriptContext1_IsScriptSetUp() && !ScriptContext2_IsEnabled()
         && !gPlayerAvatar.preventStep
         && ((player->currentCoords.x == sSpinStopX
           && (player->currentCoords.y == sSpinStopY + 1
            || player->currentCoords.y == sSpinStopY - 1))
          || (player->currentCoords.y == sSpinStopY
           && (player->currentCoords.x == sSpinStopX + 1
            || player->currentCoords.x == sSpinStopX - 1))))
        {
            if (Follower_IsSafeSpawnTile(sSpinStopX, sSpinStopY))
                spinRecallFirstSpawn = TRUE;
            else
            {
                // A map object can occupy the stop tile. Remember the next
                // vacated tile rather than using an arbitrary side position.
                sSpinStopX = player->currentCoords.x;
                sSpinStopY = player->currentCoords.y;
            }
        }
        else if (sSpinStopHasAnchor
              && !(gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_FORCED)
              && !gridSpinTile && !gridStopTile
              && (player->currentCoords.x != sSpinStopX
               || player->currentCoords.y != sSpinStopY))
        {
            // While a cutscene moves the player, or if two steps were
            // coalesced into one update, discard the obsolete stop position.
            // The next legitimate player step can use this last vacated tile
            // even when the direction key has never been released.
            sSpinStopX = player->currentCoords.x;
            sSpinStopY = player->currentCoords.y;
        }

        // Otherwise keep the sprite recalled for all forced spin frames,
        // the stop animation, dialogue and scripted movement. The old
        // T_TILE_TRANSITION/heldMovement gate is deliberately bypassed ONLY
        // for the validated player-controlled step above.
        if (!spinRecallFirstSpawn
         && ((!spinStopTile && gridSpinTile)
          || (gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_FORCED)
          || gPlayerAvatar.tileTransitionState == T_TILE_TRANSITION
          || (ObjectEventIsHeldMovementActive(player) && !player->heldMovementFinished)
          || ScriptContext1_IsScriptSetUp() || ScriptContext2_IsEnabled()
          || gPlayerAvatar.preventStep))
        {
            if (exists)
                Follower_Remove();
            Follower_ClearTrail();
            sHasLastPosition = FALSE;
            sAwaitingSafeSpawn = FALSE;
            return;
        }
        if (!spinRecallFirstSpawn)
        {
            // Stopped players retain the v0.8.20 immediate rear-tile return.
            // Do not clear a deferred first-step handoff before it succeeds.
            sSpinTileRecall = FALSE;
            sSpinStopHasAnchor = FALSE;
        }
        Follower_ClearTrail();
        sHasLastPosition = FALSE;
        sAwaitingSafeSpawn = FALSE;
    }

    if (sHealingRecallState != FOLLOWER_HEAL_IDLE)
    {
        // A map reload, whiteout, or other forced transition clears the state
        // in Follower_ResetForMapLoad, so recall cannot permanently strand us.
        if (sHealingRecallState == FOLLOWER_HEAL_WAIT_FOR_RELEASE
         && !ScriptContext1_IsScriptSetUp() && !ScriptContext2_IsEnabled())
        {
            sHealingRecallState = FOLLOWER_HEAL_IDLE;
            sHealingReleaseSoundPending = TRUE;
        }
        else
        {
            if (exists)
                Follower_Remove();
            Follower_ClearTrail();
            sHasLastPosition = FALSE;
            sAwaitingSafeSpawn = FALSE;
            return;
        }
    }

    if (sScriptedMovementState != FOLLOWER_SCRIPT_IDLE)
    {
        if (sScriptedMovementState == FOLLOWER_SCRIPT_WAIT_FOR_RELEASE
         && !ScriptContext1_IsScriptSetUp() && !ScriptContext2_IsEnabled()
         && !gPlayerAvatar.preventStep
         && gPlayerAvatar.tileTransitionState != T_TILE_TRANSITION
         && !ObjectEventIsHeldMovementActive(player))
        {
            sScriptedMovementState = FOLLOWER_SCRIPT_IDLE;
        }
        else
        {
            // FireRed may update the player object during a script. Ignore
            // forced paths rather than generating follower steps or snaps.
            if (exists)
                Follower_Remove();
            Follower_ClearTrail();
            sHasLastPosition = FALSE;
            sAwaitingSafeSpawn = FALSE;
            return;
        }
    }

    // We intentionally recall the follower on bikes and on Surf. Also cover
    // underwater and avatar transition flags so it cannot appear partway
    // through the mount/dismount graphics sequence.
    usingSpecialMovement = (gPlayerAvatar.flags & (PLAYER_AVATAR_FLAG_MACH_BIKE
                          | PLAYER_AVATAR_FLAG_ACRO_BIKE | PLAYER_AVATAR_FLAG_SURFING
                          | PLAYER_AVATAR_FLAG_UNDERWATER)) != 0;
    specialTransitionPending = (gPlayerAvatar.transitionFlags &
        (PLAYER_AVATAR_FLAG_MACH_BIKE | PLAYER_AVATAR_FLAG_ACRO_BIKE
        | PLAYER_AVATAR_FLAG_SURFING | PLAYER_AVATAR_FLAG_UNDERWATER)) != 0;
    if (usingSpecialMovement || specialTransitionPending)
    {
        bool8 bicycleTravel = ((gPlayerAvatar.flags | gPlayerAvatar.transitionFlags)
                 & (PLAYER_AVATAR_FLAG_MACH_BIKE | PLAYER_AVATAR_FLAG_ACRO_BIKE)) != 0;
        bool8 surfingTravel = ((gPlayerAvatar.flags | gPlayerAvatar.transitionFlags)
                 & (PLAYER_AVATAR_FLAG_SURFING | PLAYER_AVATAR_FLAG_UNDERWATER)) != 0;

        sResumeFromSpecialMovement = TRUE;
        sBattleRecallAwaitStep = FALSE;
        // Sound is tied to removal of a genuinely visible follower, not to
        // a flag change or a subsequent frame on the Bicycle/Surf.
        if (exists && objectId < OBJECT_EVENTS_COUNT
         && gObjectEvents[objectId].active && !gObjectEvents[objectId].invisible
         && !sPreDisplaySpawn && species != SPECIES_NONE)
        {
            PlaySE(SE_BALL_TRADE);
            if (bicycleTravel && !surfingTravel)
                sBikeReleaseSoundPending = TRUE;
            if (surfingTravel)
                sSurfReleaseSoundPending = TRUE;
        }
        // A subsequent Surf recall should not turn an old bicycle latch into
        // an unrelated release sound when the player eventually comes ashore.
        if (surfingTravel)
            sBikeReleaseSoundPending = FALSE;
    }

    if (species == SPECIES_NONE || usingSpecialMovement || specialTransitionPending
        || player->invisible)
    {
        if (species == SPECIES_NONE)
        {
            sBikeReleaseSoundPending = FALSE;
            sSurfReleaseSoundPending = FALSE;
            sSurfReturnAwaitStep = FALSE;
        }
        if (exists)
            Follower_Remove();
        Follower_ClearTrail();
        sHasLastPosition = FALSE;
        sAwaitingSafeSpawn = FALSE;
        return;
    }

    // FireRed clears SURFING before its dismount task has finished jumping
    // ashore. A bike transition can likewise clear its flag while the player's
    // sprite is still animating. Never materialise the follower on that frame
    // or on a surfable tile; resume only after normal field control returns.
    if (sResumeFromSpecialMovement)
    {
        if (gPlayerAvatar.preventStep || ScriptContext2_IsEnabled()
            || gPlayerAvatar.tileTransitionState == T_TILE_TRANSITION
            || ObjectEventIsHeldMovementActive(player)
            || MetatileBehavior_IsSurfable(MapGridGetMetatileBehaviorAt(player->currentCoords.x,
                                                                       player->currentCoords.y)))
            return;
        sResumeFromSpecialMovement = FALSE;
    }

    if (sSurfReturnAwaitStep)
    {
        // Surf shore-jump animation has ended, but a continuous held D-pad
        // can start another player-controlled step immediately. Use the
        // landing tile as the anchor rather than waiting for a frame idle.
        if (exists)
        {
            Follower_Remove();
            exists = FALSE;
        }
        if (species == SPECIES_NONE)
        {
            sSurfReturnAwaitStep = FALSE;
            sSurfReturnHasAnchor = FALSE;
            sSurfReleaseSoundPending = FALSE;
            return;
        }
        if (sPreDisplaySpawn || gPaletteFade.active || !IsWeatherNotFadingIn()
         || player->invisible)
            return;
        if (ScriptContext1_IsScriptSetUp() || ScriptContext2_IsEnabled()
         || gPlayerAvatar.preventStep || (gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_FORCED))
        {
            // Re-anchor any additional scripted movement after landing.
            sSurfReturnAnchorX = player->currentCoords.x;
            sSurfReturnAnchorY = player->currentCoords.y;
            sSurfReturnAnchorMapNum = mapNum;
            sSurfReturnAnchorMapGroup = mapGroup;
            sSurfReturnHasAnchor = TRUE;
            return;
        }
        if (!sSurfReturnHasAnchor || sSurfReturnAnchorMapNum != mapNum
         || sSurfReturnAnchorMapGroup != mapGroup)
        {
            sSurfReturnAnchorX = player->currentCoords.x;
            sSurfReturnAnchorY = player->currentCoords.y;
            sSurfReturnAnchorMapNum = mapNum;
            sSurfReturnAnchorMapGroup = mapGroup;
            sSurfReturnHasAnchor = TRUE;
            return;
        }
        if (player->currentCoords.x == sSurfReturnAnchorX
         && player->currentCoords.y == sSurfReturnAnchorY)
            return;
        if (!((player->currentCoords.x == sSurfReturnAnchorX
            && (player->currentCoords.y == sSurfReturnAnchorY + 1
             || player->currentCoords.y == sSurfReturnAnchorY - 1))
           || (player->currentCoords.y == sSurfReturnAnchorY
            && (player->currentCoords.x == sSurfReturnAnchorX + 1
             || player->currentCoords.x == sSurfReturnAnchorX - 1))))
        {
            sSurfReturnAnchorX = player->currentCoords.x;
            sSurfReturnAnchorY = player->currentCoords.y;
            return;
        }
        // Never spawn in water or substitute a side/front tile if the
        // vacated land tile becomes blocked; retry after a later step.
        if (!Follower_IsSafeSpawnTile(sSurfReturnAnchorX, sSurfReturnAnchorY))
        {
            sSurfReturnAnchorX = player->currentCoords.x;
            sSurfReturnAnchorY = player->currentCoords.y;
            return;
        }
        surfReturnFirstSpawn = TRUE;
    }

    if (sFieldTravelAwaitStep)
    {
        // Escape Rope and Dig land through the same exit callback; Teleport
        // has a separate callback. Both finish before this can spawn anything.
        if (exists)
        {
            Follower_Remove();
            exists = FALSE;
        }
        if (species == SPECIES_NONE)
        {
            sFieldTravelAwaitStep = FALSE;
            sFieldTravelHasAnchor = FALSE;
            sFieldTravelReleaseSoundPending = FALSE;
            return;
        }
        if (sPreDisplaySpawn || gPaletteFade.active || !IsWeatherNotFadingIn()
         || player->invisible)
            return;
        if (ScriptContext1_IsScriptSetUp() || ScriptContext2_IsEnabled()
         || gPlayerAvatar.preventStep || (gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_FORCED))
        {
            sFieldTravelAnchorX = player->currentCoords.x;
            sFieldTravelAnchorY = player->currentCoords.y;
            sFieldTravelAnchorMapNum = mapNum;
            sFieldTravelAnchorMapGroup = mapGroup;
            sFieldTravelHasAnchor = TRUE;
            return;
        }
        if (!sFieldTravelHasAnchor
         || sFieldTravelAnchorMapNum != mapNum
         || sFieldTravelAnchorMapGroup != mapGroup)
        {
            sFieldTravelAnchorX = player->currentCoords.x;
            sFieldTravelAnchorY = player->currentCoords.y;
            sFieldTravelAnchorMapNum = mapNum;
            sFieldTravelAnchorMapGroup = mapGroup;
            sFieldTravelHasAnchor = TRUE;
            return;
        }
        if (player->currentCoords.x == sFieldTravelAnchorX
         && player->currentCoords.y == sFieldTravelAnchorY)
            return;
        // Only a genuine adjacent step qualifies. Never use a side/front
        // fallback or a scripted coordinate jump.
        if (!((player->currentCoords.x == sFieldTravelAnchorX
            && (player->currentCoords.y == sFieldTravelAnchorY + 1
             || player->currentCoords.y == sFieldTravelAnchorY - 1))
           || (player->currentCoords.y == sFieldTravelAnchorY
            && (player->currentCoords.x == sFieldTravelAnchorX + 1
             || player->currentCoords.x == sFieldTravelAnchorX - 1))))
        {
            sFieldTravelAnchorX = player->currentCoords.x;
            sFieldTravelAnchorY = player->currentCoords.y;
            return;
        }
        if (!Follower_IsSafeSpawnTile(sFieldTravelAnchorX, sFieldTravelAnchorY))
        {
            sFieldTravelAnchorX = player->currentCoords.x;
            sFieldTravelAnchorY = player->currentCoords.y;
            return;
        }
        fieldTravelFirstSpawn = TRUE;
    }

    if (sFlyReturnAwaitStep)
    {
        // Unlike the old v0.8.23 map-reset path, never show the follower
        // immediately on the destination map. Queue a proper release on the
        // tile vacated by the first controllable player movement, even if
        // the directional button is held continuously.
        if (exists)
        {
            Follower_Remove();
            exists = FALSE;
        }
        if (species == SPECIES_NONE)
        {
            sFlyReturnAwaitStep = FALSE;
            sFlyReleaseSoundPending = FALSE;
            sFlyReturnHasAnchor = FALSE;
            return;
        }
        if (sFlyRecallActive || sPreDisplaySpawn || gPaletteFade.active
         || !IsWeatherNotFadingIn() || player->invisible)
            return;
        if (ScriptContext1_IsScriptSetUp() || ScriptContext2_IsEnabled()
         || gPlayerAvatar.preventStep || (gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_FORCED))
        {
            // The Fly landing script can itself alter player coordinates.
            // Continuously anchor those changes rather than mistaking the
            // script's final movement for the player's first voluntary step.
            sFlyReturnAnchorX = player->currentCoords.x;
            sFlyReturnAnchorY = player->currentCoords.y;
            sFlyReturnAnchorMapNum = mapNum;
            sFlyReturnAnchorMapGroup = mapGroup;
            sFlyReturnHasAnchor = TRUE;
            return;
        }
        if (!sFlyReturnHasAnchor
         || sFlyReturnAnchorMapNum != mapNum || sFlyReturnAnchorMapGroup != mapGroup)
        {
            sFlyReturnAnchorX = player->currentCoords.x;
            sFlyReturnAnchorY = player->currentCoords.y;
            sFlyReturnAnchorMapNum = mapNum;
            sFlyReturnAnchorMapGroup = mapGroup;
            sFlyReturnHasAnchor = TRUE;
            return;
        }
        if (player->currentCoords.x == sFlyReturnAnchorX
         && player->currentCoords.y == sFlyReturnAnchorY)
            return;
        // A warp or scripted reposition is not a player step; accept only a
        // one-tile move and never choose a random side/front spawn point.
        if (!((player->currentCoords.x == sFlyReturnAnchorX
            && (player->currentCoords.y == sFlyReturnAnchorY + 1
             || player->currentCoords.y == sFlyReturnAnchorY - 1))
           || (player->currentCoords.y == sFlyReturnAnchorY
            && (player->currentCoords.x == sFlyReturnAnchorX + 1
             || player->currentCoords.x == sFlyReturnAnchorX - 1))))
        {
            sFlyReturnAnchorX = player->currentCoords.x;
            sFlyReturnAnchorY = player->currentCoords.y;
            return;
        }
        if (!Follower_IsSafeSpawnTile(sFlyReturnAnchorX, sFlyReturnAnchorY))
        {
            sFlyReturnAnchorX = player->currentCoords.x;
            sFlyReturnAnchorY = player->currentCoords.y;
            return;
        }
        flyReturnFirstSpawn = TRUE;
    }

    if (sSafariWarpAwaitStep)
    {
        // An outdoor Safari boundary warp can change the player's facing
        // during the destination load. Never guess a new rear direction from
        // that facing and never use the generic adjacent-tile fallback.
        if (exists)
        {
            Follower_Remove();
            exists = FALSE;
        }
        if (species == SPECIES_NONE)
        {
            sSafariWarpAwaitStep = FALSE;
            sSafariWarpHasAnchor = FALSE;
            return;
        }
        if (gPaletteFade.active || !IsWeatherNotFadingIn()
         || ScriptContext1_IsScriptSetUp() || ScriptContext2_IsEnabled()
         || gPlayerAvatar.preventStep || player->invisible
         || (gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_FORCED))
        {
            // Scripted arrival movement must not count as a normal step.
            sSafariWarpAnchorX = player->currentCoords.x;
            sSafariWarpAnchorY = player->currentCoords.y;
            sSafariWarpAnchorMapNum = mapNum;
            sSafariWarpAnchorMapGroup = mapGroup;
            sSafariWarpHasAnchor = TRUE;
            return;
        }
        if (!sSafariWarpHasAnchor
         || mapNum != sSafariWarpAnchorMapNum || mapGroup != sSafariWarpAnchorMapGroup)
        {
            sSafariWarpAnchorX = player->currentCoords.x;
            sSafariWarpAnchorY = player->currentCoords.y;
            sSafariWarpAnchorMapNum = mapNum;
            sSafariWarpAnchorMapGroup = mapGroup;
            sSafariWarpHasAnchor = TRUE;
            return;
        }
        if (player->currentCoords.x == sSafariWarpAnchorX
         && player->currentCoords.y == sSafariWarpAnchorY)
            return;
        // Only accept ONE neighbouring, player-controlled step. A map jump,
        // a second warp, or a blocked old tile must simply move the anchor.
        if (!((player->currentCoords.x == sSafariWarpAnchorX
            && (player->currentCoords.y == sSafariWarpAnchorY + 1
             || player->currentCoords.y == sSafariWarpAnchorY - 1))
           || (player->currentCoords.y == sSafariWarpAnchorY
            && (player->currentCoords.x == sSafariWarpAnchorX + 1
             || player->currentCoords.x == sSafariWarpAnchorX - 1))))
        {
            sSafariWarpAnchorX = player->currentCoords.x;
            sSafariWarpAnchorY = player->currentCoords.y;
            return;
        }
        if (!Follower_IsSafeSpawnTile(sSafariWarpAnchorX, sSafariWarpAnchorY))
        {
            sSafariWarpAnchorX = player->currentCoords.x;
            sSafariWarpAnchorY = player->currentCoords.y;
            return;
        }
        safariWarpFirstSpawn = TRUE;
    }

    if (sWarpPadAwaitRelease)
    {
        if (exists)
        {
            Follower_Remove();
            exists = FALSE;
        }
        if (species == SPECIES_NONE)
        {
            sWarpPadAwaitRelease = FALSE;
            sWarpPadReleaseSoundPending = FALSE;
            return;
        }
        // The pad's own tile is not a safe release location, and the
        // teleport-in task may update player coordinates during its animation.
        if (gPaletteFade.active || !IsWeatherNotFadingIn()
         || ScriptContext1_IsScriptSetUp() || ScriptContext2_IsEnabled()
         || gPlayerAvatar.preventStep || player->invisible
         || (gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_FORCED))
            return;
        if (!sWarpPadHasAnchor
         || mapNum != sWarpPadAnchorMapNum || mapGroup != sWarpPadAnchorMapGroup)
        {
            sWarpPadAnchorX = player->currentCoords.x;
            sWarpPadAnchorY = player->currentCoords.y;
            sWarpPadAnchorMapNum = mapNum;
            sWarpPadAnchorMapGroup = mapGroup;
            sWarpPadHasAnchor = TRUE;
        }
        if (player->currentCoords.x == sWarpPadAnchorX
         && player->currentCoords.y == sWarpPadAnchorY)
        {
            // Standing on the destination pad: a rear tile is acceptable if
            // it is ordinary floor. Do not spawn while the landing animation
            // is still using pixel offsets, or on a second teleport pad.
            if (gPlayerAvatar.tileTransitionState == T_TILE_TRANSITION
             || ObjectEventIsHeldMovementActive(player))
                return;
            x = player->currentCoords.x;
            y = player->currentCoords.y;
            direction = player->facingDirection;
            if (direction < DIR_SOUTH || direction > DIR_EAST)
                return;
            MoveCoords(GetOppositeDirection(direction), &x, &y);
        }
        else
        {
            // A held D-pad may start the first normal tile movement without
            // an idle frame. Use ONLY the vacated anchor, never a side tile.
            // Teleport jumps are not neighbouring steps and must re-anchor.
            if (!((player->currentCoords.x == sWarpPadAnchorX
                && (player->currentCoords.y == sWarpPadAnchorY + 1
                 || player->currentCoords.y == sWarpPadAnchorY - 1))
               || (player->currentCoords.y == sWarpPadAnchorY
                && (player->currentCoords.x == sWarpPadAnchorX + 1
                 || player->currentCoords.x == sWarpPadAnchorX - 1))))
            {
                sWarpPadAnchorX = player->currentCoords.x;
                sWarpPadAnchorY = player->currentCoords.y;
                return;
            }
            x = sWarpPadAnchorX;
            y = sWarpPadAnchorY;
        }
        // Never reveal on another warp pad: it could immediately teleport the
        // player again. Wait for a subsequent safe vacated tile instead.
        if (!Follower_IsSafeSpawnTile(x, y)
         || MetatileBehavior_IsWarpPad(MapGridGetMetatileBehaviorAt(x, y)))
        {
            sWarpPadAnchorX = player->currentCoords.x;
            sWarpPadAnchorY = player->currentCoords.y;
            return;
        }
        sWarpPadAnchorX = x;
        sWarpPadAnchorY = y;
        warpPadRecallFirstSpawn = TRUE;
    }

    if (sDoorWarpRecall)
    {
        // During a warp, existing objects may be loaded before the player's
        // coordinates settle. Do not draw the follower, even when the direct
        // rear tile is free: the exit animation is NOT the user's first step.
        if (QL_IS_PLAYBACK_STATE)
        {
            // Quest Log replay has its own validated spawn/placement policy.
            sDoorWarpRecall = FALSE;
            sDoorRecallOutdoorExitPending = FALSE;
            sDoorRecallExitReady = FALSE;
            sDoorRecallHasAnchor = FALSE;
        }
        else
        {
            if (exists)
            {
                Follower_Remove();
                exists = FALSE;
            }
            if (!sDoorRecallHasAnchor
             || mapNum != sDoorRecallMapNum || mapGroup != sDoorRecallMapGroup)
            {
                // Normal map reloads can be primed before the player object
                // callback; do not bring old-map coordinates across a warp.
                sDoorRecallAnchorX = player->currentCoords.x;
                sDoorRecallAnchorY = player->currentCoords.y;
                sDoorRecallMapNum = mapNum;
                sDoorRecallMapGroup = mapGroup;
                sDoorRecallHasAnchor = TRUE;
                return;
            }
            if (sDoorRecallOutdoorExitPending)
            {
                // Never count the automatic door-exit animation as a real
                // player step, even if it advanced currentCoords before the
                // engine finished releasing the animation/script lock.
                if (!sDoorRecallExitReady)
                {
                    sDoorRecallAnchorX = player->currentCoords.x;
                    sDoorRecallAnchorY = player->currentCoords.y;
                    return;
                }
                // FireRed automatically moves the player SOUTH when leaving
                // an animated building door. The script may advance coords
                // before its exit animation finishes; always re-anchor while
                // field control or the fade still owns that motion.
                if (gPaletteFade.active || !IsWeatherNotFadingIn()
                 || ScriptContext1_IsScriptSetUp() || ScriptContext2_IsEnabled()
                 || gPlayerAvatar.preventStep || player->invisible
                 || (gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_FORCED))
                {
                    sDoorRecallAnchorX = player->currentCoords.x;
                    sDoorRecallAnchorY = player->currentCoords.y;
                    return;
                }
                // A travel arrival interrupted by a Center visit must not
                // materialize the follower ON the building's door graphic.
                // Wait for the player's first real outdoor step, using only
                // the last vacated, non-door tile behind the player.
                if (!doorRecallFirstSpawn
                 && player->currentCoords.x == sDoorRecallAnchorX
                 && player->currentCoords.y == sDoorRecallAnchorY)
                    return; // Normal exits still wait for a manual step.
                // Exactly one adjacent, player-controlled tile movement.
                // Held DOWN begins that step without an intervening idle
                // frame. The vacated tile is above the south-facing player.
                if (!doorRecallFirstSpawn
                 && !((player->currentCoords.x == sDoorRecallAnchorX
                    && (player->currentCoords.y == sDoorRecallAnchorY + 1
                     || player->currentCoords.y == sDoorRecallAnchorY - 1))
                   || (player->currentCoords.y == sDoorRecallAnchorY
                    && (player->currentCoords.x == sDoorRecallAnchorX + 1
                     || player->currentCoords.x == sDoorRecallAnchorX - 1))))
                {
                    sDoorRecallAnchorX = player->currentCoords.x;
                    sDoorRecallAnchorY = player->currentCoords.y;
                    return;
                }
                // Viridian Gym and Route 5 Day Care have narrow building
                // landings. Never place the follower on their door facades or
                // porches; wait for the first safely vacated outdoor walkway
                // tile. All player warp/exit timing remains original.
                if (Follower_IsViridianGymExteriorThreshold(mapNum, mapGroup,
                                                              sDoorRecallAnchorX,
                                                              sDoorRecallAnchorY)
                 || Follower_IsRoute5DayCareExteriorThreshold(mapNum, mapGroup,
                                                                sDoorRecallAnchorX,
                                                                sDoorRecallAnchorY))
                {
                    sDoorRecallAnchorX = player->currentCoords.x;
                    sDoorRecallAnchorY = player->currentCoords.y;
                    return;
                }
                if (sInterruptedTravelBuildingEntry)
                {
                    u8 behavior = MapGridGetMetatileBehaviorAt(sDoorRecallAnchorX,
                                                                sDoorRecallAnchorY);
                    // An outdoor warp threshold may be passable, but drawing
                    // the follower there overlaps the Pokemon Center door.
                    // Require an ordinary empty tile, with no warp-door art.
                    if (!Follower_IsSafeSpawnTile(sDoorRecallAnchorX, sDoorRecallAnchorY)
                     || MetatileBehavior_IsWarpDoor(behavior)
                     || MetatileBehavior_IsWarpDoor_2(behavior)
                     || MetatileBehavior_IsNonAnimDoor(behavior))
                    {
                        sDoorRecallAnchorX = player->currentCoords.x;
                        sDoorRecallAnchorY = player->currentCoords.y;
                        return;
                    }
                }
                else if (!Follower_IsSafeDoorReturnTile(sDoorRecallAnchorX,
                                                        sDoorRecallAnchorY))
                {
                    // Preserve regular door behavior; retry if the tile is
                    // obstructed rather than relocating beside the player.
                    sDoorRecallAnchorX = player->currentCoords.x;
                    sDoorRecallAnchorY = player->currentCoords.y;
                    return;
                }
                doorRecallFirstSpawn = TRUE;
            }
            else
            {
                // Preserve the established entrance / indoor-to-indoor
                // handoff; only building exits onto outdoor maps change.
                if (gPaletteFade.active || !IsWeatherNotFadingIn()
                 || (gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_FORCED))
                    return;
                if (player->currentCoords.x == sDoorRecallAnchorX
                 && player->currentCoords.y == sDoorRecallAnchorY)
                {
                    s16 rearX, rearY;
                    u8 face;
                    if (ScriptContext1_IsScriptSetUp() || ScriptContext2_IsEnabled()
                     || gPlayerAvatar.preventStep
                     || gPlayerAvatar.tileTransitionState == T_TILE_TRANSITION
                     || (ObjectEventIsHeldMovementActive(player) && !player->heldMovementFinished))
                        return;
                    if (sDoorRecallExpectsExitStep)
                        return;
                    rearX = player->currentCoords.x;
                    rearY = player->currentCoords.y;
                    face = player->facingDirection;
                    if (face < DIR_SOUTH || face > DIR_EAST)
                        return;
                    MoveCoords(GetOppositeDirection(face), &rearX, &rearY);
                    if (!Follower_IsSafeDoorReturnTile(rearX, rearY))
                        return;
                    sDoorRecallAnchorX = rearX;
                    sDoorRecallAnchorY = rearY;
                    doorRecallFirstSpawn = TRUE;
                }
                else
                {
                    if (!sDoorRecallExpectsExitStep
                     && (ScriptContext1_IsScriptSetUp() || ScriptContext2_IsEnabled()
                      || gPlayerAvatar.preventStep))
                        return;
                    if (!((player->currentCoords.x == sDoorRecallAnchorX
                        && (player->currentCoords.y == sDoorRecallAnchorY + 1
                         || player->currentCoords.y == sDoorRecallAnchorY - 1))
                       || (player->currentCoords.y == sDoorRecallAnchorY
                        && (player->currentCoords.x == sDoorRecallAnchorX + 1
                         || player->currentCoords.x == sDoorRecallAnchorX - 1))))
                    {
                        sDoorRecallAnchorX = player->currentCoords.x;
                        sDoorRecallAnchorY = player->currentCoords.y;
                        return;
                    }
                    if (!Follower_IsSafeDoorReturnTile(sDoorRecallAnchorX,
                                                       sDoorRecallAnchorY))
                    {
                        sDoorRecallAnchorX = player->currentCoords.x;
                        sDoorRecallAnchorY = player->currentCoords.y;
                        return;
                    }
                    doorRecallFirstSpawn = TRUE;
                }
            }
        }
    }

    if (sBattleRecallAwaitStep)
    {
        // A script can move the player during/after a trainer encounter.
        // Re-anchor while scripted or transitioning so only a later normal
        // PLAYER-controlled step releases the follower; stationary dialogue
        // and frame-by-frame palette fades cannot trigger its appearance.
        if (mapNum != sBattleRecallMapNum || mapGroup != sBattleRecallMapGroup
         || ScriptContext1_IsScriptSetUp() || ScriptContext2_IsEnabled()
         || gPlayerAvatar.preventStep)
        {
            sBattleRecallAnchorX = player->currentCoords.x;
            sBattleRecallAnchorY = player->currentCoords.y;
            sBattleRecallMapNum = mapNum;
            sBattleRecallMapGroup = mapGroup;
            if (exists)
                Follower_Remove();
            return;
        }
        // The player's currentCoords advances when a genuine tile step begins.
        // Restore on that first step, not after its animation has finished:
        // the saved anchor is the tile just vacated by the player. Dialogue,
        // scripted movement and map transitions remain gated above.
        // League, all local Trainer Battles and ordinary wild battles can
        // restore on the rear tile once field control and fades settle.
        // Other encounters retain the first-manual-step fallback.
        if (Follower_IsLeagueScriptMap() || sGymRematchReturnPending || sWildBattleReturnPending)
        {
            if (!QL_IS_PLAYBACK_STATE && !gPaletteFade.active
             && IsWeatherNotFadingIn()
             && gPlayerAvatar.tileTransitionState != T_TILE_TRANSITION
             && !ObjectEventIsHeldMovementActive(player)
             && player->facingDirection >= DIR_SOUTH
             && player->facingDirection <= DIR_EAST)
            {
                x = player->currentCoords.x;
                y = player->currentCoords.y;
                MoveCoords(GetOppositeDirection(player->facingDirection), &x, &y);
                if (Follower_IsSafeSpawnTile(x, y))
                {
                    sBattleRecallAnchorX = x;
                    sBattleRecallAnchorY = y;
                    battleRecallFirstSpawn = TRUE;
                }
            }
        }
        if (!battleRecallFirstSpawn
         && player->currentCoords.x == sBattleRecallAnchorX
         && player->currentCoords.y == sBattleRecallAnchorY)
            return;
        // Outside the League only the actual vacated tile qualifies. In the
        // League, the safe stationary rear tile above is allowed after text.
        if (!Follower_IsSafeSpawnTile(sBattleRecallAnchorX, sBattleRecallAnchorY))
        {
            sBattleRecallAnchorX = player->currentCoords.x;
            sBattleRecallAnchorY = player->currentCoords.y;
            return;
        }
        battleRecallFirstSpawn = TRUE;
    }

    if (!exists || species != sFollowerSpecies || newMap || battleRecallFirstSpawn || doorRecallFirstSpawn || spinRecallFirstSpawn || warpPadRecallFirstSpawn || flyReturnFirstSpawn || surfReturnFirstSpawn || safariWarpFirstSpawn || fieldTravelFirstSpawn)
    {
        // Always sweep the reserved follower ID on resynchronisation. Normal
        // local-ID lookups can hide objects whose map identity is now stale.
        Follower_Remove();
        Follower_ClearTrail();
        sFollowerSpecies = species;
        sFollowerShiny = shiny;

        // Do not spawn a follower inside an impassable warp tile, doorway,
        // water tile, or NPC. Wait until the player takes the first valid step
        // and materialise on the tile they just vacated instead.
        if (fieldTravelFirstSpawn)
        {
            x = sFieldTravelAnchorX;
            y = sFieldTravelAnchorY;
        }
        else if (safariWarpFirstSpawn)
        {
            x = sSafariWarpAnchorX;
            y = sSafariWarpAnchorY;
        }
        else if (spinRecallFirstSpawn)
        {
            x = sSpinStopX;
            y = sSpinStopY;
        }
        else if (surfReturnFirstSpawn)
        {
            x = sSurfReturnAnchorX;
            y = sSurfReturnAnchorY;
        }
        else if (flyReturnFirstSpawn)
        {
            x = sFlyReturnAnchorX;
            y = sFlyReturnAnchorY;
        }
        else if (warpPadRecallFirstSpawn)
        {
            x = sWarpPadAnchorX;
            y = sWarpPadAnchorY;
        }
        else if (doorRecallFirstSpawn)
        {
            x = sDoorRecallAnchorX;
            y = sDoorRecallAnchorY;
        }
        else if (battleRecallFirstSpawn)
        {
            x = sBattleRecallAnchorX;
            y = sBattleRecallAnchorY;
        }
        else
        {
            x = player->currentCoords.x;
            y = player->currentCoords.y;
            direction = GetOppositeDirection(player->facingDirection);
            MoveCoords(direction, &x, &y);
            if (!Follower_IsSafeSpawnTile(x, y))
            {
                // A recorded Quest Log scene can start with the player's back
                // against a doorway or wall, or with an unknown facing. Never
                // use an arbitrary adjacent tile during replay: it may be IN
                // FRONT of the player for a frame before snapping into the trail.
                // Instead, let the delayed-spawn path use the tile vacated by
                // the player's next real step. During a battle return the trainer
                // may occupy the rear tile; do NOT choose a side/front tile just
                // to avoid a delayed spawn. That broke player/follower alignment
                // and facing during post-battle dialogue in v0.8.5.
                if (!QL_IS_PLAYBACK_STATE && sPreDisplaySpawn
                 && !sBattleReturnPending
                 && Follower_FindAdjacentTile(player, &x, &y))
                {
                    sAwaitingSafeSpawn = FALSE;
                }
                else
                {
                    if (!sAwaitingSafeSpawn || newMap
                        || (sLastPlayerX == player->currentCoords.x && sLastPlayerY == player->currentCoords.y))
                    {
                        sLastPlayerX = player->currentCoords.x;
                        sLastPlayerY = player->currentCoords.y;
                        sLastMapNum = mapNum;
                        sLastMapGroup = mapGroup;
                        sHasLastPosition = TRUE;
                        sAwaitingSafeSpawn = TRUE;
                        return;
                    }
                    x = sLastPlayerX;
                    y = sLastPlayerY;
                    if (!Follower_IsSafeSpawnTile(x, y))
                        return;
                }
            }
        }
        sAwaitingSafeSpawn = FALSE;

        objectId = SpawnSpecialObjectEventParameterized(OBJ_EVENT_GFX_FOLLOWER,
                          MOVEMENT_TYPE_FACE_DOWN, FOLLOWER_LOCAL_ID,
                          x, y, player->currentElevation);
        if (objectId != OBJECT_EVENTS_COUNT)
        {
            // A successful spawn completes a deferred interrupted-travel
            // handoff, including normal indoor steps and building exits.
            sInterruptedTravelBuildingEntry = FALSE;
            // Play only on the actual post-healing materialisation, never
            // from the Pokemon Center exit/warp callback.
            if (sHealingReleaseSoundPending)
            {
                PlaySE(SE_BALL_OPEN);
                sHealingReleaseSoundPending = FALSE;
            }
            else if (sBikeReleaseSoundPending)
            {
                // Exactly one release, only after the bicycle's follower
                // sprite really exists again (never merely on dismount).
                PlaySE(SE_BALL_OPEN);
            }
            else if (surfReturnFirstSpawn && sSurfReleaseSoundPending)
            {
                // A successful return after the first shore step, never
                // at the start of the water-to-land animation.
                PlaySE(SE_BALL_OPEN);
            }
            else if (warpPadRecallFirstSpawn && sWarpPadReleaseSoundPending)
            {
                // Do not play from the teleport-in callback. Wait until the
                // follower actually appears on a safe, non-warp rear tile.
                PlaySE(SE_BALL_OPEN);
            }
            else if (fieldTravelFirstSpawn && sFieldTravelReleaseSoundPending)
            {
                PlaySE(SE_BALL_OPEN);
            }
            else if (flyReturnFirstSpawn && sFlyReleaseSoundPending)
            {
                // The same open sound as bicycle dismount/Nurse Joy, never
                // the automatic arrival-map creation from older versions.
                PlaySE(SE_BALL_OPEN);
            }
            sBikeReleaseSoundPending = FALSE;
            if (fieldTravelFirstSpawn)
                sFieldTravelReleaseSoundPending = FALSE;
            if (surfReturnFirstSpawn)
                sSurfReleaseSoundPending = FALSE;
            if (flyReturnFirstSpawn)
                sFlyReleaseSoundPending = FALSE;
            if (warpPadRecallFirstSpawn)
                sWarpPadReleaseSoundPending = FALSE;
            Follower_RefreshPalette();
            // Run another refresh after the load/fade sequence has completed:
            // the field loader or Quest Log may overwrite the palette later.
            sPaletteRefreshAfterLoad = !battleRecallFirstSpawn;
            follower = &gObjectEvents[objectId];
            if (sLeagueScriptFollowing || sGeneralScriptFollowing)
                UnfreezeObjectEvent(follower);
            // An unset replay facing must not become an invalid idle anim.
            direction = player->facingDirection;
            if (direction < DIR_SOUTH || direction > DIR_EAST)
                direction = DIR_SOUTH;
            follower->facingDirection = direction;
            ObjectEventTurn(follower, direction);
            Follower_ApplyHorizontalSpacing(follower);
        }
        sLastPlayerX = player->currentCoords.x;
        sLastPlayerY = player->currentCoords.y;
        sLastMapNum = mapNum;
        sLastMapGroup = mapGroup;
        sHasLastPosition = (objectId != OBJECT_EVENTS_COUNT);
        if (surfReturnFirstSpawn && objectId != OBJECT_EVENTS_COUNT)
        {
            sSurfReturnAwaitStep = FALSE;
            sSurfReturnHasAnchor = FALSE;
        }
        else if (surfReturnFirstSpawn)
        {
            sSurfReturnAnchorX = player->currentCoords.x;
            sSurfReturnAnchorY = player->currentCoords.y;
        }
        if (fieldTravelFirstSpawn && objectId != OBJECT_EVENTS_COUNT)
        {
            sFieldTravelAwaitStep = FALSE;
            sFieldTravelHasAnchor = FALSE;
        }
        else if (fieldTravelFirstSpawn)
        {
            // Allocation failed; try another vacated tile without audio.
            sFieldTravelAnchorX = player->currentCoords.x;
            sFieldTravelAnchorY = player->currentCoords.y;
        }
        if (safariWarpFirstSpawn && objectId != OBJECT_EVENTS_COUNT)
        {
            sSafariWarpAwaitStep = FALSE;
            sSafariWarpHasAnchor = FALSE;
        }
        else if (safariWarpFirstSpawn)
        {
            // No spare sprite slot: try the next player-vacated tile.
            sSafariWarpAnchorX = player->currentCoords.x;
            sSafariWarpAnchorY = player->currentCoords.y;
        }
        if (flyReturnFirstSpawn && objectId != OBJECT_EVENTS_COUNT)
        {
            sFlyReturnAwaitStep = FALSE;
            sFlyReturnHasAnchor = FALSE;
        }
        else if (flyReturnFirstSpawn)
        {
            // Exhausted object-event slots: keep retrying on subsequent
            // genuine steps without playing the release sound early.
            sFlyReturnAnchorX = player->currentCoords.x;
            sFlyReturnAnchorY = player->currentCoords.y;
        }
        if (warpPadRecallFirstSpawn && objectId != OBJECT_EVENTS_COUNT)
        {
            sWarpPadAwaitRelease = FALSE;
            sWarpPadHasAnchor = FALSE;
        }
        else if (warpPadRecallFirstSpawn)
        {
            // No sprite slots: retain the release latch and try again after
            // the next real step, without double-playing either sound.
            sWarpPadAnchorX = player->currentCoords.x;
            sWarpPadAnchorY = player->currentCoords.y;
        }
        if (spinRecallFirstSpawn && objectId != OBJECT_EVENTS_COUNT)
        {
            sSpinTileRecall = FALSE;
            sSpinStopHasAnchor = FALSE;
        }
        else if (spinRecallFirstSpawn)
        {
            // Sprite capacity failure: retry on the next vacated tile.
            sSpinStopX = player->currentCoords.x;
            sSpinStopY = player->currentCoords.y;
        }
        // If spawning fails due to sprite/OAM capacity, retry on a future
        // settled step rather than letting a generic side spawn slip through.
        if (doorRecallFirstSpawn && objectId != OBJECT_EVENTS_COUNT)
        {
            sDoorWarpRecall = FALSE;
            sDoorRecallOutdoorExitPending = FALSE;
            sDoorRecallExitReady = FALSE;
            sDoorRecallHasAnchor = FALSE;
            sDoorRecallExpectsExitStep = FALSE;
        }
        else if (doorRecallFirstSpawn)
        {
            // Sprite capacity failure: retry on a subsequent tile step.
            sDoorRecallAnchorX = player->currentCoords.x;
            sDoorRecallAnchorY = player->currentCoords.y;
        }
        if (battleRecallFirstSpawn && objectId != OBJECT_EVENTS_COUNT)
        {
            sBattleRecallAwaitStep = FALSE;
            sGymRematchReturnPending = FALSE;
            sWildBattleReturnPending = FALSE;
        }
        else if (battleRecallFirstSpawn)
        {
            sBattleRecallAnchorX = player->currentCoords.x;
            sBattleRecallAnchorY = player->currentCoords.y;
        }
        return;
    }

    follower = &gObjectEvents[objectId];
    if (QL_IS_PLAYBACK_STATE
     && follower->currentCoords.x == player->currentCoords.x
     && follower->currentCoords.y == player->currentCoords.y)
    {
        // Repair historical overlap ONLY to the actual rear tile. Moving a
        // replay follower to any available neighbor repeats the front-of-
        // player flash reported in v0.8.3. Hide if rear is obstructed.
        direction = GetOppositeDirection(player->facingDirection);
        x = player->currentCoords.x;
        y = player->currentCoords.y;
        if (direction >= DIR_SOUTH && direction <= DIR_EAST)
            MoveCoords(direction, &x, &y);
        if (direction >= DIR_SOUTH && direction <= DIR_EAST
         && Follower_IsSafeSpawnTile(x, y))
        {
            ObjectEventClearHeldMovementIfActive(follower);
            Follower_ClearTrail();
            MoveObjectEventToMapCoords(follower, x, y);
            sLastPlayerX = player->currentCoords.x;
            sLastPlayerY = player->currentCoords.y;
        }
        else
        {
            // With no unoccupied adjacent tile, hide rather than standing
            // directly over the player; normal updates retry when safe.
            Follower_Remove();
            sHasLastPosition = FALSE;
            return;
        }
    }
    // A party reorder can select a different individual of the same species.
    // Swap its palette in place, without resetting the follower's movement.
    if (sFollowerShiny != shiny)
    {
        sFollowerShiny = shiny;
        Follower_RefreshPalette();
        // If the switch occurs during a fade/tint, repaint after it ends.
        sPaletteRefreshAfterLoad = (gPaletteFade.active || !IsWeatherNotFadingIn()
                                    || gGlobalFieldTintMode != QL_TINT_NONE);
    }
    // Avoid a second palette write inside the pre-display battle hook. That
    // hook owns the initial reload while the field is still hidden.
    if (!sPreDisplaySpawn || !sBattleReturnPending)
        Follower_RestorePaletteAfterLoad();
    dx = player->currentCoords.x - sLastPlayerX;
    dy = player->currentCoords.y - sLastPlayerY;
    if (dx != 0 || dy != 0)
    {
        direction = Follower_DirectionTo(dx, dy);
        playerStartedLedge = FALSE;
        playerFinishedLedge = FALSE;
        jumpTrailEntry = sFollowerJumpPending;

        // Genuine FireRed jumps pass through the intermediate ledge tile.
        // Detect their held Jump2 action and require an actual directional
        // ledge, never just a two-tile position change made by a script.
        if (sPlayerJumpInProgress)
        {
            if (player->currentCoords.x == sPlayerJumpLandingX
             && player->currentCoords.y == sPlayerJumpLandingY)
            {
                playerFinishedLedge = TRUE;
                sPlayerJumpInProgress = FALSE;
                sFollowerJumpPending = TRUE;
            }
        }
        else if ((dx == 2 || dx == -2 || dy == 2 || dy == -2)
              && Follower_IsLedgeJump(sLastPlayerX, sLastPlayerY,
                                      player->currentCoords.x, player->currentCoords.y))
        {
            // Some update schedules only observe the takeoff and landing.
            playerStartedLedge = TRUE;
            sFollowerJumpPending = TRUE;
        }
        else if ((dx == 1 || dx == -1 || dy == 1 || dy == -1)
              && direction != DIR_NONE
              && player->movementActionId == GetJump2MovementAction(direction)
              && GetLedgeJumpDirection(player->currentCoords.x,
                                       player->currentCoords.y, direction) == direction)
        {
            sPlayerJumpLandingX = player->currentCoords.x;
            sPlayerJumpLandingY = player->currentCoords.y;
            MoveCoords(direction, &sPlayerJumpLandingX, &sPlayerJumpLandingY);
            sPlayerJumpInProgress = TRUE;
            playerStartedLedge = TRUE;
        }

        // At the midpoint, skip the *ledge tile*. Ordinary tracking would
        // instruct the follower to walk onto that tile, preventing a jump.
        // At takeoff enqueue the safe tile; after landing enqueue its tile
        // as Jump2 ONLY on the player's next step, like Emerald's delayed
        // jump state. This keeps one-tile separation after the jump.
        if (direction == DIR_NONE
            || ((dx == 2 || dx == -2 || dy == 2 || dy == -2) && !playerStartedLedge)
            || (sPlayerJumpInProgress && !playerStartedLedge)
            || (!playerFinishedLedge
                && !Follower_TryEnqueue(sLastPlayerX, sLastPlayerY, jumpTrailEntry)))
        {
            if (!playerFinishedLedge)
            {
                Follower_ClearTrail();
                ObjectEventClearHeldMovementIfActive(follower);
                if (Follower_IsSafeSpawnTile(sLastPlayerX, sLastPlayerY))
                    MoveObjectEventToMapCoords(follower, sLastPlayerX, sLastPlayerY);
            }
        }
        else if (!playerStartedLedge && !playerFinishedLedge && jumpTrailEntry)
        {
            sFollowerJumpPending = FALSE;
        }
        sLastPlayerX = player->currentCoords.x;
        sLastPlayerY = player->currentCoords.y;
    }

    // A cue can reposition only the follower. Hold its trail until the player
    // changes tiles; do not interfere with ANY Trainer/NPC movement timing.
    if (sFollowerYieldHoldTrail
     && (player->currentCoords.x != sFollowerYieldPlayerX
      || player->currentCoords.y != sFollowerYieldPlayerY))
        sFollowerYieldHoldTrail = FALSE;
    if (!sInteractionActive && !sFollowerYieldHoldTrail)
        Follower_DrainTrail(follower);
    Follower_ApplyHorizontalSpacing(follower);
}
