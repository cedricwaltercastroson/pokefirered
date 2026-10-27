#ifndef GUARD_FOLLOWING_POKEMON_H
#define GUARD_FOLLOWING_POKEMON_H

#include "global.h"

// FireRed proof-of-concept: dynamic Kanto follower. 152 is deliberately a single
// graphics id, not a range of 151 ids (the graphics field is only one byte).
#define OBJ_EVENT_GFX_FOLLOWER 152
#define FOLLOWER_LOCAL_ID 0xFE

// Build-time palette preview only. Keep 0 for normal gameplay.
#define FOLLOWER_FORCE_SHINY_PREVIEW 0

struct ObjectEventGraphicsInfo;
struct ObjectEvent;
const struct ObjectEventGraphicsInfo *Follower_GetGraphicsInfo(void);
void Follower_Update(struct ObjectEvent *player);
void Follower_ResetForMapLoad(void);
void Follower_BeginMapConnectionTransition(void);
void Follower_FinishMapConnectionTransition(void);
void Follower_RebaseForCameraUpdate(s16 dx, s16 dy);
void Follower_RefreshPalette(void);
// FireRed field-script specials for facing the follower and displaying dialogue.
u16 Follower_PrepareInteraction(void);
u16 Follower_PlayInteractionCry(void);
u16 Follower_EndInteraction(void);
// Pokemon Center healing scripts; the return value of Begin is TRUE only
// when a visible follower existed and the short recall pause is warranted.
u16 Follower_BeginHealingRecall(void);
u16 Follower_EndHealingRecall(void);
// Recall immediately before Hall of Fame machine recording animation.
u16 Follower_BeginHallOfFameRecall(void);
// Engine hooks for scripted PLAYER applymovement / release. This avoids
// treating the follower as a normal map NPC during cutscene choreography.
void Follower_BeginScriptedPlayerMovement(void);
void Follower_EndScriptedPlayerMovement(void);
// Fly takeoff recalls before the avatar temporarily becomes ON_FOOT.
void Follower_BeginFlyRecall(void);
// Bicycle item callback completes on-foot transition explicitly.
void Follower_AfterBicycleDismount(void);
void Follower_AfterSurfDismount(void);
// Shared Dig / Escape Rope / field-move Teleport transition hooks.
void Follower_BeginFieldTravelRecall(void);
void Follower_EndFieldTravelRecall(void);
// Pre-display hook for returning from battles and other field loads.
void Follower_PrimeForOverworldDisplay(void);
// Post-battle only: correct the final palette buffer before it reaches OBJ RAM.
void Follower_StabilizeBattleReturnPalette(void);
void Follower_StabilizeBattleReturnSpriteSlot(void);
void Follower_NotifyBattleReturn(void);
void Follower_BeginDoorWarp(void);
void Follower_BeginBuildingExitWarp(void);
void Follower_EndBuildingExitWarp(void);
void Follower_BeginRearOnlyWarp(void);
void Follower_BeginStairWarp(void);
void Follower_EndStairWarp(void);
void Follower_BeginSafariZoneWarp(void);
void Follower_BeginWarpPadRecall(void);
void Follower_EndWarpPadRecall(void);
void RemoveFollowerObjectEvent(void);
void RemoveFollowerObjectEventById(u8 objectEventId);
bool8 Follower_IsCollisionExempt(const struct ObjectEvent *a, const struct ObjectEvent *b);

#endif
