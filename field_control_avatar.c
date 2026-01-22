#include "global.h"
#include "battle_setup.h"
#include "bike.h"
#include "coord_event_weather.h"
#include "daycare.h"
//#include "faraway_island.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "event_scripts.h"
#include "fieldmap.h"
#include "field_control_avatar.h"
#include "field_player_avatar.h"
#include "field_poison.h"
#include "field_screen_effect.h"
#include "field_specials.h"
//#include "fldeff_misc.h"
#include "item_menu.h"
#include "link.h"
//#include "match_call.h"
#include "metatile_behavior.h"
#include "overworld.h"
#include "pokemon.h"
#include "safari_zone.h"
#include "script.h"
//#include "secret_base.h"
#include "sound.h"
#include "start_menu.h"
#include "trainer_see.h"
//#include "trainer_hill.h"
#include "wild_encounter.h"
#include "follow_me.h"
#include "constants/event_bg.h"
#include "constants/event_objects.h"
//#include "constants/field_poison.h"
#include "constants/map_types.h"
#include "constants/songs.h"
//#include "constants/trainer_hill.h"


/**
 * ::ACIMUT::
 * 2022/04/14
 * - Cambio de función correspondiente a fire red.
 * - No es llamada en otra parte de la inyección.
 * - Hacer hook a esta función
 */

const u8 *GetInteractedObjectEventScript(struct MapPosition *position, u8 metatileBehavior, u8 direction)
{
    u8 objectEventId;
    const u8 *script;

    objectEventId = GetObjectEventIdByXYZ(position->x, position->y, position->height);
    if (objectEventId == OBJECT_EVENTS_COUNT || gObjectEvents[objectEventId].localId == OBJ_EVENT_ID_PLAYER)
    {
        if (MetatileBehavior_IsCounter(metatileBehavior) != TRUE)
            return NULL;

        // Look for an object event on the other side of the counter.
        objectEventId = GetObjectEventIdByXYZ(position->x + gDirectionToVectors[direction].x, position->y + gDirectionToVectors[direction].y, position->height);
        if (objectEventId == OBJECT_EVENTS_COUNT || gObjectEvents[objectEventId].localId == OBJ_EVENT_ID_PLAYER)
            return NULL;
    }

    if (InUnionRoom() == TRUE && !ObjectEventCheckHeldMovementStatus(&gObjectEvents[objectEventId]))
        return NULL;

    gSelectedObjectEvent = objectEventId;
    {
        u8 followerId = GetFollowerObjectId();
        if (objectEventId == followerId)
            gSpecialVar_LastTalked = OBJ_EVENT_ID_FOLLOWER;
        else
            gSpecialVar_LastTalked = gObjectEvents[objectEventId].localId;
    }
    gSpecialVar_Facing = direction;

    //if (InTrainerHill() == TRUE)
    //    script = GetTrainerHillTrainerScript();
    //else 
    {
        u8 followerId = GetFollowerObjectId();
        if (objectEventId == followerId)
            script = GetFollowerScriptPointer();
        else
            script = GetObjectEventScriptPointerByObjectEventId(objectEventId);
    }

    script = GetRamScript(gSpecialVar_LastTalked, script);
    return script;
}

/**
 * ::ACIMUT::
 * 2022/04/14
 * - Cambio de función correspondiente a fire red.
 * - No es llamada en otra parte de la inyección.
 * - Hacer hook a esta función
 */

const u8 *GetInteractedWaterScript(struct MapPosition *unused1, u8 metatileBehavior, u8 direction)
{
    if (MetatileBehavior_IsSemiDeepWater(metatileBehavior) == TRUE &&PartyHasMonWithSurf() == TRUE)
        return EventScript_CurrentTooFast;
    if (FlagGet(FLAG_BADGE05_GET) == TRUE && PartyHasMonWithSurf() == TRUE && IsPlayerFacingSurfableFishableWater() == TRUE && CheckFollowerFlag(FOLLOWER_FLAG_CAN_SURF))
        return EventScript_UseSurf;

    if (MetatileBehavior_IsWaterfall(metatileBehavior) == TRUE && CheckFollowerFlag(FOLLOWER_FLAG_CAN_WATERFALL))
    {
        if (FlagGet(FLAG_BADGE07_GET) == TRUE && IsPlayerSurfingNorth() == TRUE)
            return EventScript_Waterfall;
        else
            return EventScript_CantUseWaterfall;
    }
    return NULL;
}

/**
 * ::ACIMUT::
 * 2022/04/15
 * - Función no existe en FR
 * - No es llamada en otra parte de la inyección.
 * - No es necesario portearlo.
 */

/*
//static 
extern u8 TrySetDiveWarp(void);

extern const u8 EventScript_UseDive[];
extern const u8 EventScript_UseDiveUnderwater[];

bool32 TrySetupDiveDownScript(void)
{
    if (!CheckFollowerFlag(FOLLOWER_FLAG_CAN_DIVE))
        return FALSE;

    if (FlagGet(FLAG_BADGE07_GET) && TrySetDiveWarp() == 2)
    {
        ScriptContext1_SetupScript(EventScript_UseDive);
        return TRUE;
    }
    return FALSE;
}
*/

/**
 * ::ACIMUT::
 * 2022/04/15
 * - Función no existe en FR
 * - No es llamada en otra parte de la inyección.
 * - No es necesario portearlo.
 */

/*
bool32 TrySetupDiveEmergeScript(void)
{
    if (!CheckFollowerFlag(FOLLOWER_FLAG_CAN_DIVE))
        return FALSE;

    if (FlagGet(FLAG_BADGE07_GET) && gMapHeader.mapType == MAP_TYPE_UNDERWATER && TrySetDiveWarp() == 1)
    {
        ScriptContext1_SetupScript(EventScript_UseDiveUnderwater);
        return TRUE;
    }
    return FALSE;
}
*/

