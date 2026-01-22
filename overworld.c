#include "global.h"
#include "overworld.h"
//#include "battle_pyramid.h"
#include "battle_setup.h"
#include "berry.h"
#include "bg.h"
#include "cable_club.h"
//#include "clock.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "event_scripts.h"
#include "field_camera.h"
#include "field_control_avatar.h"
#include "field_effect.h"
#include "field_message_box.h"
#include "field_player_avatar.h"
#include "field_screen_effect.h"
//#include "field_special_scene.h"
#include "field_specials.h"
#include "field_tasks.h"
#include "field_weather.h"
#include "fieldmap.h"
#include "fldeff.h"
#include "gpu_regs.h"
#include "heal_location.h"
#include "gba/io_reg.h"
#include "link.h"
#include "link_rfu.h"
#include "load_save.h"
#include "main.h"
#include "malloc.h"
#include "m4a.h"
#include "map_name_popup.h"
//#include "match_call.h"
#include "menu.h"
#include "metatile_behavior.h"
//#include "mirage_tower.h"
#include "money.h"
#include "new_game.h"
#include "palette.h"
#include "play_time.h"
#include "random.h"
#include "roamer.h"
//#include "rotating_gate.h"
#include "safari_zone.h"
#include "save.h"
#include "save_location.h"
#include "script.h"
#include "script_pokemon_util.h"
//#include "secret_base.h"
#include "sound.h"
#include "start_menu.h"
#include "task.h"
#include "tileset_anims.h"
//#include "time_events.h"
//#include "trainer_hill.h"
#include "trainer_pokemon_sprites.h"
//#include "tv.h"
#include "scanline_effect.h"
#include "wild_encounter.h"
//#include "frontier_util.h"
#include "follow_me.h"
#include "constants/abilities.h"
#include "constants/layouts.h"
#include "constants/map_types.h"
#include "constants/region_map_sections.h"
#include "constants/songs.h"
//#include "constants/trainer_hill.h"
#include "constants/weather.h"
#include "constants/event_object_movement.h"

#include "quest_log.h"
#include "event_object_lock.h"
#include "help_system.h"

//static
extern bool8 map_post_load_hook_exec(void);
extern void sub_8056F08(void);
extern void sub_8057114(void);
extern void ReloadObjectsAndRunReturnToFieldMapScript(void);
extern void ResumeMap(bool32 a0);
extern void InitOverworldBgs(void);

struct InitialPlayerAvatarState
{
    u8 transitionFlags;
    u8 direction;
    bool8 unk2;
};

extern struct InitialPlayerAvatarState *GetInitialPlayerAvatarState(void);


/**
 * ::ACIMUT::
 * 2022/04/03
 * 
 * - Función en emerald.
 *      bool8 IsPlayerStandingStill(void)
 * 
 * - Función equivalente en fire red:
 *      bool8 walkrun_is_standing_still(void)
 * 
 * - Se encuentra en el siguiente archivo:
 *      event_object_lock.c
 * 
 * - Reemplazado.
 */

/**
 * ::ACIMUT::
 * 2022/04/03
 * 
 * - Cambio de nombre correspondiente a fire red.
 * - Cambio de función correspondiente a fire red.
 * - No es llamada en otra parte de la inyección.
 * - Es posible hacer hook a toda esta función.
 */

//static 
void Overworld_ResetStateAfterWhitingOut(void)
{
    ResetInitialPlayerAvatarState();
    FlagClear(FLAG_SYS_ON_CYCLING_ROAD);
    VarSet(VAR_MAP_SCENE_ROUTE16, 0);
    FlagClear(FLAG_SYS_CRUISE_MODE);
    FlagClear(FLAG_SYS_SAFARI_MODE);
    VarSet(VAR_MAP_SCENE_FUCHSIA_CITY_SAFARI_ZONE_ENTRANCE, 0);
    FlagClear(FLAG_SYS_USE_STRENGTH);
    FlagClear(FLAG_SYS_FLASH_ACTIVE);
    FlagClear(FLAG_SYS_QL_DEPARTED);
    VarSet(VAR_QL_ENTRANCE, 0);

    FollowMe_TryRemoveFollowerOnWhiteOut();
}

/**
 * ::ACIMUT::
 * 2022/04/03
 * 
 * - Cambio de función correspondiente a fire red.
 * - No es llamada en otra parte de la inyección.
 * - Es posible hacer hook a toda esta función.
 */

//static 
void DoCB1_Overworld(u16 newKeys, u16 heldKeys)
{
    struct FieldInput fieldInput;

    sub_8112B3C();
    UpdatePlayerAvatarTransitionState();
    FieldClearPlayerInput(&fieldInput);
    FieldGetPlayerInput(&fieldInput, newKeys, heldKeys);
    FieldInput_HandleCancelSignpost(&fieldInput);
    if (!ScriptContext2_IsEnabled())
    {
        if (ProcessPlayerFieldInput(&fieldInput) == TRUE)
        {
            if (gQuestLogPlaybackState == 2)
                sub_81127F8(&gInputToStoreInQuestLogMaybe);
            ScriptContext2_Enable();
            DismissMapNamePopup();
        }
        else
        {
            player_step(fieldInput.dpadDirection, newKeys, heldKeys);
        }
    }
    RunQuestLogCB();

    // if stop running but keep holding B -> fix follower frame
    if (PlayerHasFollower() && IsPlayerOnFoot() && walkrun_is_standing_still())
    {
        u8 followerId = GetFollowerObjectId();
        if (followerId != OBJECT_EVENTS_COUNT)
            ObjectEventSetHeldMovement(&gObjectEvents[followerId], GetFaceDirectionAnimNum(gObjectEvents[followerId].facingDirection));
    }

    // CFRU-like: continuously enforce hiding the origin NPC while a follower is active.
    FollowMe_HideOriginNpcOnCurrentMap();
}

/**
 * ::ACIMUT::
 * 2022/04/03
 * 
 * - Esta función en FR se conoce como:
 *      static bool32 sub_8056CD8(u8 *state)
 * - Cambio de función correspondiente a fire red.
 * - No es llamada en otra parte de la inyección.
 * - Es posible hacer hook a toda esta función.
 */

//static 
bool32 ReturnToFieldLocal(u8 *state)
{
    switch (*state)
    {
    case 0:
        InitOverworldBgs();
        QuestLog_InitPalettesBackup();
        ResumeMap(FALSE);
        ReloadObjectsAndRunReturnToFieldMapScript();
        // CFRU-like: after any object reload, ensure the origin NPC is hidden while the follower is active.
        // This fixes the "duplicate OW" that can appear after closing menus/battles where objects are reloaded.
        FollowMe_HideOriginNpcOnCurrentMap();
        sub_8057114();
        (*state)++;
        break;
    case 1:
        sub_8056F08();
        SetHelpContextForMap();
        FollowMe_BindToSurbBlobOnReloadScreen();
        (*state)++;
        break;
    case 2:
        if (map_post_load_hook_exec())
            (*state)++;
        break;
    case 3:
        // Final safety: hide origin NPC after post-load scripts finish.
        FollowMe_HideOriginNpcOnCurrentMap();
        return TRUE;
    }
    return FALSE;
}


/**
 * ::ACIMUT::
 * 2022/04/03
 * 
 * - Esta función en FR se conoce como:
 *      static void mli4_mapscripts_and_other(void)
 * - Cambio de función correspondiente a fire red.
 * - No es llamada en otra parte de la inyección.
 * - Es posible hacer hook a toda esta función.
 */


//static 
void InitObjectEventsLocal(void)
{
    s16 x, y;
    struct InitialPlayerAvatarState *player;
    gTotalCameraPixelOffsetX = 0;
    gTotalCameraPixelOffsetY = 0;
    ResetObjectEvents();
    GetCameraFocusCoords(&x, &y);
    player = GetInitialPlayerAvatarState();
    InitPlayerAvatar(x, y, player->direction, gSaveBlock2Ptr->playerGender);
    SetPlayerAvatarTransitionFlags(player->transitionFlags);
    ResetInitialPlayerAvatarState();
    TrySpawnObjectEvents(0, 0);

    // CFRU-like: if the follower originated from an NPC on this map, hide that original NPC
    // so only the follower remains (prevents duplicate overworld sprites).
    FollowMe_HideOriginNpcOnCurrentMap();

    TryRunOnWarpIntoMapScript();

    FollowMe_HandleSprite();
}





