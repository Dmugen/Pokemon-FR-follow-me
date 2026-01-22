#include "global.h"
#include "battle.h"
#include "link.h"
#include "event_data.h"
#include "constants/songs.h"
#include "m4a.h"
#include "roamer.h"
#include "follow_me.h"

/**
 * ::ACIMUT::
 * 2022/04/15
 * - static
 * - No es llamada en otra parte de la inyección.
 * - Repuntear.
 */

void ReturnFromBattleToOverworld(void)
{
    if (!(gBattleTypeFlags & BATTLE_TYPE_LINK))
    {
        RandomlyGivePartyPokerus(gPlayerParty);
        PartySpreadPokerus(gPlayerParty);
    }
    if (gBattleTypeFlags & BATTLE_TYPE_LINK && gReceivedRemoteLinkPlayers != 0)
        return;
    gSpecialVar_Result = gBattleOutcome;
    gMain.inBattle = FALSE;
    gMain.callback1 = gPreBattleCallback1;
    if (gBattleTypeFlags & BATTLE_TYPE_ROAMER)
    {
        UpdateRoamerHPStatus(&gEnemyParty[0]);
//#ifndef BUGFIX
//        if ((gBattleOutcome & B_OUTCOME_WON) || gBattleOutcome == B_OUTCOME_CAUGHT)
//#else
        if ((gBattleOutcome == B_OUTCOME_WON) || gBattleOutcome == B_OUTCOME_CAUGHT) // Bug: When Roar is used by roamer, gBattleOutcome is B_OUTCOME_PLAYER_TELEPORTED (5).
//#endif                                                                               // & with B_OUTCOME_WON (1) will return TRUE and deactivates the roamer.
            SetRoamerInactive();
    }

    m4aSongNumStop(SE_LOW_HEALTH);
    SetMainCallback2(gMain.savedCallback);

// if you experience the follower de-syncing with the player after battle, set POST_BATTLE_FOLLOWER_FIX to TRUE in include/constants/global.h
#if POST_BATTLE_FOLLOWER_FIX
    FollowMe_WarpSetEnd();
    {
        u8 followerId = GetFollowerObjectId();
        if (followerId != OBJECT_EVENTS_COUNT)
            gObjectEvents[followerId].invisible = TRUE;
    }
#endif
}



