#!/usr/bin/env python3
"""Entry-door sequencing regression: delayed recall, with validated exits untouched."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
field = (root / 'src/field_fadetransition.c').read_text()
fol = (root / 'src/following_pokemon.c').read_text()
entrance = field.split('void DoDoorWarp(void)', 1)[1].split('void DoTeleport2Warp(void)', 1)[0]
task = field.split('static void Task_DoorWarp(u8 taskId)\n{', 1)[1].split('static void Task_StairWarp(', 1)[0]
case0 = task.split('    case 0:', 1)[1].split('    case 1:', 1)[0]
case1 = task.split('    case 1:', 1)[1].split('    case 2:', 1)[0]
case2 = task.split('    case 2:', 1)[1].split('    case 3:', 1)[0]
assert 'Follower_BeginDoorWarp()' not in entrance, 'Premature recall before entrance animation'
assert 'FreezeObjectEvents()' in case0, 'Normal NPC freeze should be retained'
assert 'UnfreezeObjectEvent(&gObjectEvents[followerId])' in case1
assert case1.index('UnfreezeObjectEvent(&gObjectEvents[followerId])') < case1.index('MOVEMENT_ACTION_WALK_NORMAL_UP')
assert 'ObjectEventIsHeldMovementActive(&gObjectEvents[followerId])' in case2
assert '!gObjectEvents[followerId].heldMovementFinished' in case2
assert case2.index('Follower_BeginRearOnlyWarp()') < case2.index('SetPlayerVisibility(FALSE)')
assert case2.index('ObjectEventIsHeldMovementActive(&gObjectEvents[followerId])') < case2.index('Follower_BeginRearOnlyWarp()')
# v0.8.27 revises exit timing but preserves the original door-entry animation.
assert 'Follower_BeginBuildingExitWarp();' in field.split('void DoWarp(void)', 1)[1].split('void DoDiveWarp(', 1)[0]
assert 'Follower_BeginRearOnlyWarp();' in field.split('void DoWarp(void)', 1)[1].split('void DoDiveWarp(', 1)[0]
assert 'Follower_BeginStairWarp()' in field.split('void DoStairWarp(', 1)[1].split('void DoDoorWarp(', 1)[0]
assert 'void Follower_BeginHealingRecall' not in fol or 'sHealingRecallState' in fol
assert 'sSpinTileRecall' in fol
assert 'sBattleRecallAwaitStep' in fol
print('PASS natural door-entry sequencing with v0.8.27 deferred destination release')
