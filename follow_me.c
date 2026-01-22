#include "global.h"
#include "follow_me.h"
#include "event_object_movement.h"
#include "field_door.h"
#include "field_effect.h"
#include "field_effect_helpers.h"
#include "field_player_avatar.h"
#include "field_control_avatar.h"
#include "field_screen_effect.h"
#include "field_weather.h"
#include "fieldmap.h"
//#include "fldeff_misc.h"
#include "item.h"
#include "task.h"
#include "metatile_behavior.h"
#include "overworld.h"
#include "script.h"
#include "event_data.h"
#include "sound.h"
#include "trig.h"
#include "metatile_behavior.h"
#include "constants/event_object_movement.h"
#include "constants/event_objects.h"
#include "constants/songs.h"
#include "constants/map_types.h"
#include "constants/field_effects.h"
#include "constants/metatile_behaviors.h"
#include "event_object_lock.h"
//#include "event_objects.h"
#include "field_screen.h"
#include "field_fadetransition.h"

//static
extern struct ScriptContext sScriptContext1;
extern void MovementType_None(struct Sprite *);

/*
    -FollowMe_StairsMoveHook ?
    -FollowMe_WarpStairsEndHook ?
*/

/*
Known Issues:
    -follower gets messed up if you go into a map with a maximum number of event objects
        -inherits incorrect palette, may get directionally confused
*/

// Defines
#define PLAYER_AVATAR_FLAG_BIKE    PLAYER_AVATAR_FLAG_MACH_BIKE | PLAYER_AVATAR_FLAG_ACRO_BIKE

struct FollowerSpriteGraphics
{
    u16 normalId;
    u16 machBikeId;
    u16 acroBikeId;
    u16 surfId;
    u16 underwaterId;
};

// EWRAM
//EWRAM_DATA struct Follower gSaveBlock1Ptr->follower = {0};

// Function Declarations
static u8 GetFollowerMapObjId(void);
static u16 GetFollowerSprite(void);
static bool8 TryGetFollowerObjectEvent(struct ObjectEvent **outFollower);
static void TryUpdateFollowerSpriteUnderwater(void);
static void Task_ReallowPlayerMovement(u8 taskId);
static u8 DetermineFollowerDirection(struct ObjectEvent* player, struct ObjectEvent* follower);
static void PlayerLogCoordinates(struct ObjectEvent* player);
static u8 DetermineFollowerState(struct ObjectEvent* follower, u8 state, u8 direction);
static bool8 IsStateMovement(u8 state);
static u8 ReturnFollowerDelayedState(u8 direction);
static void SetSurfJump(void);
static void Task_BindSurfBlobToFollower(u8 taskId);
static void SetUpSurfBlobFieldEffect(struct ObjectEvent* npc);
static void SetSurfDismount(void);
static void Task_FinishSurfDismount(u8 taskId);
static void Task_FollowerOutOfDoor(u8 taskId);
static void Task_FollowerHandleIndoorStairs(u8 taskId);
static void Task_FollowerHandleEscalator(u8 taskId);
static void Task_FollowerHandleEscalatorFinish(u8 taskId);
static void CalculateFollowerEscalatorTrajectoryUp(struct Task *task);
static void CalculateFollowerEscalatorTrajectoryDown(struct Task *task);
static void TurnNPCIntoFollower(u8 localId, u16 followerFlags);


/**
 * ::ACIMUT::
 * 2022/04/15
 * - Expandir luego los gfx.
 * - ajustar luego.
 */

// Const Data
static const struct FollowerSpriteGraphics gFollowerAlternateSprites[] =
{
    //FORMAT:
    //{WALKING/RUNNING SPRITE ID, MACH BIKE SPRITE ID, ACRO BIKE SPRITE ID, SURFING SPRITE ID},
    [0] = 
    {
        .normalId = OBJ_EVENT_GFX_GREEN_NORMAL,//OBJ_EVENT_GFX_RIVAL_MAY_NORMAL,
        .machBikeId = OBJ_EVENT_GFX_GREEN_BIKE,//OBJ_EVENT_GFX_RIVAL_MAY_MACH_BIKE,
        .acroBikeId = OBJ_EVENT_GFX_GREEN_BIKE,//OBJ_EVENT_GFX_RIVAL_MAY_ACRO_BIKE,
        .surfId = OBJ_EVENT_GFX_GREEN_SURF,//OBJ_EVENT_GFX_RIVAL_MAY_SURFING,
        .underwaterId = OBJ_EVENT_GFX_GREEN_SURF,//OBJ_EVENT_GFX_MAY_UNDERWATER,
    },

};

//movido de script.c
u8* ReadWord(u8 index)
{
    struct ScriptContext *ctx = &sScriptContext1;

    return (T1_READ_PTR(&ctx->data[index]));
}


// Functions
static bool8 TryGetFollowerObjectEvent(struct ObjectEvent **outFollower)
{
    u8 objId;

    if (!gSaveBlock1Ptr->follower.inProgress)
        return FALSE;

    objId = gSaveBlock1Ptr->follower.objId;

    if (objId >= OBJECT_EVENTS_COUNT)
        return FALSE;

    if (!gObjectEvents[objId].active || gObjectEvents[objId].isPlayer)
        return FALSE;

    // Safety: only accept the dedicated follower slot marker.
    if (gObjectEvents[objId].localId != OBJ_EVENT_ID_FOLLOWER)
        return FALSE;

    *outFollower = &gObjectEvents[objId];
    return TRUE;
}

u8 GetFollowerObjectId(void)
{
    struct ObjectEvent *follower;

    if (!TryGetFollowerObjectEvent(&follower))
        return OBJECT_EVENTS_COUNT;

    return gSaveBlock1Ptr->follower.objId;
}

u8 GetFollowerLocalId(void)
{
    if (!gSaveBlock1Ptr->follower.inProgress)
        return 0;

    return OBJ_EVENT_ID_FOLLOWER;
}

const u8* GetFollowerScriptPointer(void)
{
    if (!gSaveBlock1Ptr->follower.inProgress)
        return NULL;

    return gSaveBlock1Ptr->follower.script;
}

void HideFollower(void)
{
    struct ObjectEvent *follower;

    if (!TryGetFollowerObjectEvent(&follower))
        return;

    if (gSaveBlock1Ptr->follower.createSurfBlob == 2 || gSaveBlock1Ptr->follower.createSurfBlob == 3)
    {
        SetSurfBlob_BobState(follower->fieldEffectSpriteId, 2);
        DestroySprite(&gSprites[follower->fieldEffectSpriteId]);
        follower->fieldEffectSpriteId = 0; //Unbind
    }

    follower->invisible = TRUE;
}

/*
void IsFollowerStoppingRockClimb(void)
{
    gSpecialVar_Result = FALSE;
    if (!gSaveBlock1Ptr->follower.inProgress)
        return;
    gSpecialVar_Result = (gSaveBlock1Ptr->follower.flags & FOLLOWER_FLAG_CAN_ROCK_CLIMB) == 0;
}
*/

void FollowMe_SetIndicatorToComeOutDoor(void)
{
    if (gSaveBlock1Ptr->follower.inProgress)
        gSaveBlock1Ptr->follower.comeOutDoorStairs = 1;
}

void FollowMe_SetIndicatorToRecreateSurfBlob(void)
{
    if (gSaveBlock1Ptr->follower.inProgress)
        gSaveBlock1Ptr->follower.createSurfBlob = 2;
}

void FollowMe_TryRemoveFollowerOnWhiteOut(void)
{
    if (gSaveBlock1Ptr->follower.inProgress)
    {
        if (gSaveBlock1Ptr->follower.flags & FOLLOWER_FLAG_CLEAR_ON_WHITE_OUT)
            gSaveBlock1Ptr->follower.inProgress = FALSE;
        else
            FollowMe_WarpSetEnd();
    }
}

static u8 GetFollowerMapObjId(void)
{
    return GetFollowerObjectId();
}

static u16 GetFollowerSprite(void)
{
    u32 i;

    switch (gSaveBlock1Ptr->follower.currentSprite)
    {
    case FOLLOWER_SPRITE_INDEX_MACH_BIKE:
        for (i = 0; i < NELEMS(gFollowerAlternateSprites); i++)
        {
            if (gFollowerAlternateSprites[i].normalId == gSaveBlock1Ptr->follower.graphicsId)
                return gFollowerAlternateSprites[i].machBikeId;
        }
        break;
    case FOLLOWER_SPRITE_INDEX_ACRO_BIKE:
        for (i = 0; i < NELEMS(gFollowerAlternateSprites); i++)
        {
            if (gFollowerAlternateSprites[i].normalId == gSaveBlock1Ptr->follower.graphicsId)
                return gFollowerAlternateSprites[i].acroBikeId;
        }
        break;
    case FOLLOWER_SPRITE_INDEX_SURF:
        for (i = 0; i < NELEMS(gFollowerAlternateSprites); i++)
        {
            if (gFollowerAlternateSprites[i].normalId == gSaveBlock1Ptr->follower.graphicsId)
                return gFollowerAlternateSprites[i].surfId;
        }
        break;
    case FOLLOWER_SPRITE_INDEX_UNDERWATER:
        for (i = 0; i < NELEMS(gFollowerAlternateSprites); i++)
        {
            if (gFollowerAlternateSprites[i].normalId == gSaveBlock1Ptr->follower.graphicsId)
                return gFollowerAlternateSprites[i].underwaterId;
        }
        break;
    }

    return gSaveBlock1Ptr->follower.graphicsId;
}

static void TryUpdateFollowerSpriteUnderwater(void)
{
    struct ObjectEvent *follower;

    if (gMapHeader.mapType != MAP_TYPE_UNDERWATER)
        return;

    if (!TryGetFollowerObjectEvent(&follower))
        return;

    SetFollowerSprite(FOLLOWER_SPRITE_INDEX_UNDERWATER);

    if (!TryGetFollowerObjectEvent(&follower))
        return;

    follower->fieldEffectSpriteId = StartUnderwaterSurfBlobBobbing(follower->spriteId);
}

//Actual Follow Me
void FollowMe(struct ObjectEvent* npc, u8 state, bool8 ignoreScriptActive)
{
    struct ObjectEvent* player = &gObjectEvents[gPlayerAvatar.objectEventId];
    struct ObjectEvent* follower;
    u8 dir;
    u8 newState;
    u8 taskId;

    if (player != npc) //Only when the player moves
        return;
    else if (!gSaveBlock1Ptr->follower.inProgress)
        return;
    else if (!TryGetFollowerObjectEvent(&follower))
        return;
    else if (ScriptContext2_IsEnabled() && !ignoreScriptActive)
        return; //Don't follow during a script


    // fix post-surf jump
    if ((gSaveBlock1Ptr->follower.currentSprite == FOLLOWER_SPRITE_INDEX_SURF) && !(gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_SURFING) && follower->fieldEffectSpriteId == 0)
    {
        SetFollowerSprite(FOLLOWER_SPRITE_INDEX_NORMAL);
        if (!TryGetFollowerObjectEvent(&follower))
            return;
        gSaveBlock1Ptr->follower.createSurfBlob = 0;
    }

    //Check if state would cause hidden follower to reappear
    if (IsStateMovement(state) && gSaveBlock1Ptr->follower.warpEnd)
    {
        gSaveBlock1Ptr->follower.warpEnd = 0;

        if (gSaveBlock1Ptr->follower.comeOutDoorStairs == 1)
        {
            gPlayerAvatar.preventStep = TRUE;
            taskId = CreateTask(Task_FollowerOutOfDoor, 1);
            gTasks[taskId].data[0] = 0;
            gTasks[taskId].data[2] = follower->currentCoords.x;
            gTasks[taskId].data[3] = follower->currentCoords.y;
            goto RESET;
        }
        else if (gSaveBlock1Ptr->follower.comeOutDoorStairs == 2)
        {
            gSaveBlock1Ptr->follower.comeOutDoorStairs = 0;
        }

        follower->invisible = FALSE;
        MoveObjectEventToMapCoords(follower, player->currentCoords.x, player->currentCoords.y);
        ObjectEventTurn(follower, player->facingDirection); //The follower should be facing the same direction as the player when it comes out of hiding

        if (gSaveBlock1Ptr->follower.createSurfBlob == 2) //Recreate surf blob
        {
            SetUpSurfBlobFieldEffect(follower);
            follower->fieldEffectSpriteId = FieldEffectStart(FLDEFF_SURF_BLOB);
            SetSurfBlob_BobState(follower->fieldEffectSpriteId, 1);
        }
        else
        {
            TryUpdateFollowerSpriteUnderwater();
        }
    }

    dir = DetermineFollowerDirection(player, follower);

    if (dir == DIR_NONE)
        goto RESET;

    newState = DetermineFollowerState(follower, state, dir);
    if (newState == MOVEMENT_INVALID)
        goto RESET;

    if (gSaveBlock1Ptr->follower.createSurfBlob == 1) //Get on Surf Blob
    {
        gSaveBlock1Ptr->follower.createSurfBlob = 2;
        gPlayerAvatar.preventStep = TRUE; //Wait for finish
        SetSurfJump();
        goto RESET;
    }
    else if (gSaveBlock1Ptr->follower.createSurfBlob == 3) //Get off Surf Blob
    {
        gSaveBlock1Ptr->follower.createSurfBlob = 0;
        gPlayerAvatar.preventStep = TRUE; //Wait for finish
        SetSurfDismount();
        goto RESET;
    }

    ObjectEventClearHeldMovementIfActive(follower);
    ObjectEventSetHeldMovement(follower, newState);
    PlayerLogCoordinates(player);

    switch (newState) 
    {
    case MOVEMENT_ACTION_JUMP_2_DOWN ... MOVEMENT_ACTION_JUMP_2_RIGHT:
        CreateTask(Task_ReallowPlayerMovement, 1); //Synchronize movements on stairs and ledges
        gPlayerAvatar.preventStep = TRUE;   //allow follower to catch up
    }

RESET:
    ObjectEventClearHeldMovementIfFinished(follower);
}

static void Task_ReallowPlayerMovement(u8 taskId)
{
    struct ObjectEvent *follower;
    bool8 animStatus;

    if (!TryGetFollowerObjectEvent(&follower))
    {
        gPlayerAvatar.preventStep = FALSE;
        DestroyTask(taskId);
        return;
    }

    animStatus = ObjectEventClearHeldMovementIfFinished(follower);
    if (animStatus == 0)
    {
        if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_DASH)
        && ObjectEventClearHeldMovementIfFinished(&gObjectEvents[gPlayerAvatar.objectEventId]))
            SetPlayerAvatarTransitionFlags(PLAYER_AVATAR_FLAG_ON_FOOT); //Temporarily stop running
        return;
    }

    gPlayerAvatar.preventStep = FALSE;
    DestroyTask(taskId);
}

static u8 DetermineFollowerDirection(struct ObjectEvent* player, struct ObjectEvent* follower)
{
    //Move the follower towards the player
    s8 delta_x = follower->currentCoords.x - player->currentCoords.x;
    s8 delta_y = follower->currentCoords.y - player->currentCoords.y;

    if (delta_x < 0)
        return DIR_EAST;
    else if (delta_x > 0)
        return DIR_WEST;

    if (delta_y < 0)
        return DIR_SOUTH;
    else if (delta_y > 0)
        return DIR_NORTH;

    return DIR_NONE;
}

static void PlayerLogCoordinates(struct ObjectEvent* player)
{
    gSaveBlock1Ptr->follower.log.x = player->currentCoords.x;
    gSaveBlock1Ptr->follower.log.y = player->currentCoords.y;
}

#define RETURN_STATE(state, dir) return newState == MOVEMENT_INVALID ? state + (dir - 1) : ReturnFollowerDelayedState(dir - 1);
static u8 DetermineFollowerState(struct ObjectEvent* follower, u8 state, u8 direction)
{
    u8 newState = MOVEMENT_INVALID;
    #if SIDEWAYS_STAIRS_IMPLEMENTED == TRUE
        u8 collision = COLLISION_NONE;
        s16 followerX = follower->currentCoords.x;
        s16 followerY = follower->currentCoords.y;
        u8 currentBehavior = MapGridGetMetatileBehaviorAt(followerX, followerY);
        u8 nextBehavior;

        MoveCoords(direction, &followerX, &followerY);
        nextBehavior = MapGridGetMetatileBehaviorAt(followerX, followerY);
    #endif

    if (IsStateMovement(state) && gSaveBlock1Ptr->follower.delayedState)
        newState = gSaveBlock1Ptr->follower.delayedState + direction;

    //Clear ice tile stuff
    follower->disableAnim = FALSE; //follower->field1 &= 0xFB;

    #if SIDEWAYS_STAIRS_IMPLEMENTED == TRUE
        // clear overwrite movement
        follower->directionOverwrite = DIR_NONE;

        //sideways stairs checks
        collision = GetSidewaysStairsCollision(follower, direction, currentBehavior, nextBehavior, collision);
        switch (collision)
        {
        case COLLISION_SIDEWAYS_STAIRS_TO_LEFT:
            follower->directionOverwrite = GetLeftSideStairsDirection(direction);
            break;
        case COLLISION_SIDEWAYS_STAIRS_TO_RIGHT:
            follower->directionOverwrite = GetRightSideStairsDirection(direction);
            break;
        }
    #endif

    switch (state) 
    {
    case MOVEMENT_ACTION_WALK_SLOW_DOWN ... MOVEMENT_ACTION_WALK_SLOW_RIGHT:
        // Slow walk
        RETURN_STATE(MOVEMENT_ACTION_WALK_SLOW_DOWN, direction);

    case MOVEMENT_ACTION_WALK_NORMAL_DOWN ... MOVEMENT_ACTION_WALK_NORMAL_RIGHT:
        // normal walk
        RETURN_STATE(MOVEMENT_ACTION_WALK_NORMAL_DOWN, direction);

    case MOVEMENT_ACTION_JUMP_2_DOWN ... MOVEMENT_ACTION_JUMP_2_RIGHT:
        // Ledge jump
        if (((newState - direction) >= MOVEMENT_ACTION_JUMP_2_DOWN && (newState - direction) <= MOVEMENT_ACTION_JUMP_2_RIGHT)
        ||  ((newState - direction) >= 0x84 && (newState - direction) <= 0x87)) //Previously jumped
        {
            newState = MOVEMENT_INVALID;
            RETURN_STATE(MOVEMENT_ACTION_JUMP_2_DOWN, direction); //Jump right away
        }

        gSaveBlock1Ptr->follower.delayedState = MOVEMENT_ACTION_JUMP_2_DOWN;
        RETURN_STATE(MOVEMENT_ACTION_WALK_NORMAL_DOWN, direction);

    case MOVEMENT_ACTION_WALK_FAST_DOWN ... MOVEMENT_ACTION_WALK_FAST_RIGHT:
        // Handle player on waterfall
        if (PlayerIsUnderWaterfall(&gObjectEvents[gPlayerAvatar.objectEventId]) && (state == MOVEMENT_ACTION_WALK_FAST_UP))
            return MOVEMENT_INVALID;

        //Handle ice tile (some walking animation) -  Set a bit to freeze the follower's animation
        if (MetatileBehavior_IsIce(follower->currentMetatileBehavior) || MetatileBehavior_IsUnknownMovement48(follower->currentMetatileBehavior))
            follower->disableAnim = TRUE;

        RETURN_STATE(MOVEMENT_ACTION_WALK_FAST_DOWN, direction);

    case MOVEMENT_ACTION_WALK_FASTEST_DOWN ... MOVEMENT_ACTION_WALK_FASTEST_RIGHT:
        // mach bike
        if (MetatileBehavior_IsIce(follower->currentMetatileBehavior) || MetatileBehavior_IsUnknownMovement48(follower->currentMetatileBehavior))
            follower->disableAnim = TRUE;

        RETURN_STATE(MOVEMENT_ACTION_WALK_FASTEST_DOWN, direction);

    // acro bike
    case MOVEMENT_ACTION_RIDE_WATER_CURRENT_DOWN ... MOVEMENT_ACTION_RIDE_WATER_CURRENT_RIGHT:
        // Handle player on waterfall
        if (PlayerIsUnderWaterfall(&gObjectEvents[gPlayerAvatar.objectEventId]) && IsPlayerSurfingNorth())
        //if (PlayerIsUnderWaterfall(&gObjectEvents[gPlayerAvatar.objectEventId]) && (state == MOVEMENT_ACTION_RIDE_WATER_CURRENT_DOWN))
            return MOVEMENT_INVALID;

        RETURN_STATE(MOVEMENT_ACTION_RIDE_WATER_CURRENT_DOWN, direction);  //regular movement
    case MOVEMENT_ACTION_ACRO_WHEELIE_FACE_DOWN ... MOVEMENT_ACTION_ACRO_WHEELIE_FACE_RIGHT:
        RETURN_STATE(MOVEMENT_ACTION_ACRO_WHEELIE_FACE_DOWN, direction);
    case MOVEMENT_ACTION_ACRO_POP_WHEELIE_DOWN ... MOVEMENT_ACTION_ACRO_POP_WHEELIE_RIGHT:
        RETURN_STATE(MOVEMENT_ACTION_ACRO_POP_WHEELIE_DOWN, direction);
    case MOVEMENT_ACTION_ACRO_END_WHEELIE_FACE_DOWN ... MOVEMENT_ACTION_ACRO_END_WHEELIE_FACE_RIGHT:
        RETURN_STATE(MOVEMENT_ACTION_ACRO_END_WHEELIE_FACE_DOWN, direction);
    case MOVEMENT_ACTION_ACRO_WHEELIE_HOP_FACE_DOWN ... MOVEMENT_ACTION_ACRO_WHEELIE_HOP_FACE_RIGHT:
        RETURN_STATE(MOVEMENT_ACTION_ACRO_WHEELIE_HOP_FACE_DOWN, direction);
    case MOVEMENT_ACTION_ACRO_WHEELIE_HOP_DOWN ... MOVEMENT_ACTION_ACRO_WHEELIE_HOP_RIGHT:
        RETURN_STATE(MOVEMENT_ACTION_ACRO_WHEELIE_HOP_DOWN, direction);
    case MOVEMENT_ACTION_ACRO_WHEELIE_JUMP_DOWN ... MOVEMENT_ACTION_ACRO_WHEELIE_JUMP_RIGHT:
        RETURN_STATE(MOVEMENT_ACTION_ACRO_WHEELIE_JUMP_DOWN, direction);
    case MOVEMENT_ACTION_ACRO_WHEELIE_IN_PLACE_DOWN ... MOVEMENT_ACTION_ACRO_WHEELIE_IN_PLACE_RIGHT:
        RETURN_STATE(MOVEMENT_ACTION_ACRO_WHEELIE_IN_PLACE_DOWN, direction);
    case MOVEMENT_ACTION_ACRO_POP_WHEELIE_MOVE_DOWN ... MOVEMENT_ACTION_ACRO_POP_WHEELIE_MOVE_RIGHT:
        RETURN_STATE(MOVEMENT_ACTION_ACRO_POP_WHEELIE_MOVE_DOWN, direction);
    case MOVEMENT_ACTION_ACRO_WHEELIE_MOVE_DOWN ... MOVEMENT_ACTION_ACRO_WHEELIE_MOVE_RIGHT:
        RETURN_STATE(MOVEMENT_ACTION_ACRO_WHEELIE_MOVE_DOWN, direction);
    //case MOVEMENT_ACTION_ACRO_END_WHEELIE_MOVE_DOWN ... MOVEMENT_ACTION_ACRO_END_WHEELIE_MOVE_RIGHT:
    //    RETURN_STATE(MOVEMENT_ACTION_ACRO_END_WHEELIE_MOVE_DOWN, direction);

    // Sliding
    case MOVEMENT_ACTION_SLIDE_DOWN ... MOVEMENT_ACTION_SLIDE_RIGHT:
        RETURN_STATE(MOVEMENT_ACTION_SLIDE_DOWN, direction);
    case MOVEMENT_ACTION_PLAYER_RUN_DOWN ... MOVEMENT_ACTION_PLAYER_RUN_RIGHT:
        //Running frames
        if (gSaveBlock1Ptr->follower.flags & FOLLOWER_FLAG_HAS_RUNNING_FRAMES)
            RETURN_STATE(MOVEMENT_ACTION_PLAYER_RUN_DOWN, direction);

        RETURN_STATE(MOVEMENT_ACTION_WALK_FAST_DOWN, direction);
    case MOVEMENT_ACTION_JUMP_SPECIAL_DOWN ... MOVEMENT_ACTION_JUMP_SPECIAL_RIGHT:
        gSaveBlock1Ptr->follower.delayedState = MOVEMENT_ACTION_JUMP_SPECIAL_DOWN;
        RETURN_STATE(MOVEMENT_ACTION_WALK_NORMAL_DOWN, direction);
    case MOVEMENT_ACTION_JUMP_DOWN ... MOVEMENT_ACTION_JUMP_RIGHT:
        gSaveBlock1Ptr->follower.delayedState = MOVEMENT_ACTION_JUMP_DOWN;
        RETURN_STATE(MOVEMENT_ACTION_WALK_NORMAL_DOWN, direction);

    // run slow
    #ifdef MOVEMENT_ACTION_RUN_DOWN_SLOW
    case MOVEMENT_ACTION_RUN_DOWN_SLOW ... MOVEMENT_ACTION_RUN_RIGHT_SLOW:
        if (gSaveBlock1Ptr->follower.flags & FOLLOWER_FLAG_HAS_RUNNING_FRAMES)
            RETURN_STATE(MOVEMENT_ACTION_RUN_DOWN_SLOW, direction);

        RETURN_STATE(MOVEMENT_ACTION_WALK_NORMAL_DOWN, direction);
    #endif

    default:
        return MOVEMENT_INVALID;
    }

    return newState;
}

static bool8 IsStateMovement(u8 state)
{
    switch (state) 
    {
    case MOVEMENT_ACTION_FACE_DOWN:
    case MOVEMENT_ACTION_FACE_UP:
    case MOVEMENT_ACTION_FACE_LEFT:
    case MOVEMENT_ACTION_FACE_RIGHT:
    //case MOVEMENT_ACTION_FACE_DOWN_FAST:
    //case MOVEMENT_ACTION_FACE_UP_FAST:
    //case MOVEMENT_ACTION_FACE_LEFT_FAST:
    //case MOVEMENT_ACTION_FACE_RIGHT_FAST:
    case MOVEMENT_ACTION_DELAY_1:
    case MOVEMENT_ACTION_DELAY_2:
    case MOVEMENT_ACTION_DELAY_4:
    case MOVEMENT_ACTION_DELAY_8:
    case MOVEMENT_ACTION_DELAY_16:
    case MOVEMENT_ACTION_FACE_PLAYER:
    case MOVEMENT_ACTION_FACE_AWAY_PLAYER:
    case MOVEMENT_ACTION_LOCK_FACING_DIRECTION:
    case MOVEMENT_ACTION_UNLOCK_FACING_DIRECTION:
    case MOVEMENT_ACTION_SET_INVISIBLE:
    case MOVEMENT_ACTION_SET_VISIBLE:
    case MOVEMENT_ACTION_EMOTE_EXCLAMATION_MARK:
    case MOVEMENT_ACTION_EMOTE_QUESTION_MARK:
    //Scase MOVEMENT_ACTION_EMOTE_HEART:
    //case MOVEMENT_ACTION_EMOTE_CROSS:
    //case MOVEMENT_ACTION_EMOTE_DOUBLE_EXCLAMATION_MARK:
    //case MOVEMENT_ACTION_EMOTE_HAPPY:
    case MOVEMENT_ACTION_WALK_IN_PLACE_NORMAL_DOWN:
    case MOVEMENT_ACTION_WALK_IN_PLACE_NORMAL_UP:
    case MOVEMENT_ACTION_WALK_IN_PLACE_NORMAL_LEFT:
    case MOVEMENT_ACTION_WALK_IN_PLACE_NORMAL_RIGHT:
    case MOVEMENT_ACTION_WALK_IN_PLACE_FAST_DOWN:
    case MOVEMENT_ACTION_WALK_IN_PLACE_FAST_UP:
    case MOVEMENT_ACTION_WALK_IN_PLACE_FAST_LEFT:
    case MOVEMENT_ACTION_WALK_IN_PLACE_FAST_RIGHT:
    case MOVEMENT_ACTION_WALK_IN_PLACE_FASTEST_DOWN:
    case MOVEMENT_ACTION_WALK_IN_PLACE_FASTEST_UP:
    case MOVEMENT_ACTION_WALK_IN_PLACE_FASTEST_LEFT:
    case MOVEMENT_ACTION_WALK_IN_PLACE_FASTEST_RIGHT:
    case MOVEMENT_ACTION_JUMP_IN_PLACE_DOWN:
    case MOVEMENT_ACTION_JUMP_IN_PLACE_UP:
    case MOVEMENT_ACTION_JUMP_IN_PLACE_LEFT:
    case MOVEMENT_ACTION_JUMP_IN_PLACE_RIGHT:
    case MOVEMENT_ACTION_JUMP_IN_PLACE_DOWN_UP:
    case MOVEMENT_ACTION_JUMP_IN_PLACE_UP_DOWN:
    case MOVEMENT_ACTION_JUMP_IN_PLACE_LEFT_RIGHT:
    case MOVEMENT_ACTION_JUMP_IN_PLACE_RIGHT_LEFT:
        return FALSE;
    }

    return TRUE;
}

static u8 ReturnFollowerDelayedState(u8 direction)
{
    u8 newState = gSaveBlock1Ptr->follower.delayedState;
    gSaveBlock1Ptr->follower.delayedState = 0;

    /*
    #ifdef MOVEMENT_ACTION_WALK_STAIRS_DIAGONAL_UP_LEFT
    switch (newState) 
    {
    case MOVEMENT_ACTION_WALK_STAIRS_DIAGONAL_UP_LEFT ... MOVEMENT_ACTION_WALK_STAIRS_DIAGONAL_DOWN_RIGHT:
    case MOVEMENT_ACTION_WALK_STAIRS_DIAGONAL_UP_LEFT_RUNNING ... MOVEMENT_ACTION_WALK_STAIRS_DIAGONAL_DOWN_RIGHT_RUNNING:
    case MOVEMENT_ACTION_RIDE_WATER_CURRENT_UP_LEFT ... MOVEMENT_ACTION_RIDE_WATER_CURRENT_DOWN_RIGHT:
        return newState; //Each its own movement, so don't modify direction
    }
    #endif
    */

    return newState + direction;
}

#define LEDGE_FRAMES_MULTIPLIER 2

//extern void (**stepspeeds[5])(struct Sprite*, u8);
//extern const u16 stepspeed_seq_length[5];
void FollowMe_Ledges(struct ObjectEvent* npc, struct Sprite* sprite, u16* ledgeFramesTbl)
{
    u8 speed;
    u16 frameCount;
    u8 currentFrame;
    struct ObjectEvent* follower;

    if (!TryGetFollowerObjectEvent(&follower))
        return;

    if (follower == npc)
        speed = gPlayerAvatar.runningState ? 3 : 1;
    else
        speed = 0;

    //Calculate the frames for the jump
    frameCount = GetMiniStepCount(speed) * LEDGE_FRAMES_MULTIPLIER;   //in event_object_movement.c
    ledgeFramesTbl[sprite->data[4]] = frameCount;

    //Call the step shifter
    currentFrame = sprite->data[6] / LEDGE_FRAMES_MULTIPLIER;
    //stepspeeds[speed][currentFrame](sprite, sprite->data[3]);
    RunMiniStep(sprite, speed, currentFrame);   //in event_object_movement.c
}

bool8 FollowMe_IsCollisionExempt(struct ObjectEvent* obstacle, struct ObjectEvent* collider)
{
    struct ObjectEvent* follower;
    struct ObjectEvent* player = &gObjectEvents[gPlayerAvatar.objectEventId];

    if (!TryGetFollowerObjectEvent(&follower))
        return FALSE;

    if (obstacle == follower && collider == player)
        return TRUE;

    return FALSE;
}

void FollowMe_FollowerToWater(void)
{
    if (!gSaveBlock1Ptr->follower.inProgress)
        return;

    //Prepare for making the follower do the jump and spawn the surf head
    //right in front of the follower's location.
    FollowMe(&gObjectEvents[gPlayerAvatar.objectEventId], MOVEMENT_ACTION_JUMP_DOWN, TRUE);
    gSaveBlock1Ptr->follower.createSurfBlob = 1;
}

void FollowMe_BindToSurbBlobOnReloadScreen(void)
{
    struct ObjectEvent* follower;

    if (!TryGetFollowerObjectEvent(&follower))
        return;

    TryUpdateFollowerSpriteUnderwater();
    if (!TryGetFollowerObjectEvent(&follower))
        return;

    if (gSaveBlock1Ptr->follower.createSurfBlob != 2 && gSaveBlock1Ptr->follower.createSurfBlob != 3)
        return;

    //Spawn surfhead under follower
    SetUpSurfBlobFieldEffect(follower);
    follower->fieldEffectSpriteId = FieldEffectStart(FLDEFF_SURF_BLOB);
    SetSurfBlob_BobState(follower->fieldEffectSpriteId, 1);
}

static void SetSurfJump(void)
{
    struct ObjectEvent* follower;
    u8 direction;
    u8 jumpState;

    if (!TryGetFollowerObjectEvent(&follower))
        return;

    //Reset NPC movement bits
    ObjectEventClearHeldMovement(follower);

    //Jump animation according to direction
    direction = DetermineFollowerDirection(&gObjectEvents[gPlayerAvatar.objectEventId], follower);
    jumpState = GetJumpMovementAction(direction);
    SetUpSurfBlobFieldEffect(follower);

    //Adjust surf head spawn location infront of follower
    switch (direction) 
    {
    case DIR_SOUTH:
        gFieldEffectArguments[1]++; //effect_y
        break;
    case DIR_NORTH:
        gFieldEffectArguments[1]--;
        break;
    case DIR_WEST:
        gFieldEffectArguments[0]--; //effect_x
        break;
    default: //DIR_EAST
        gFieldEffectArguments[0]++;
    };

    //Execute, store sprite ID in fieldEffectSpriteId and bind surf blob
    follower->fieldEffectSpriteId = FieldEffectStart(FLDEFF_SURF_BLOB);
    CreateTask(Task_BindSurfBlobToFollower, 0x1);
    SetFollowerSprite(FOLLOWER_SPRITE_INDEX_SURF);

    if (!TryGetFollowerObjectEvent(&follower))
        return;
    ObjectEventSetHeldMovement(follower, jumpState);
}

static void Task_BindSurfBlobToFollower(u8 taskId)
{
    struct ObjectEvent* npc;
    bool8 animStatus;

    if (!TryGetFollowerObjectEvent(&npc))
    {
        gPlayerAvatar.preventStep = FALSE;
        DestroyTask(taskId);
        return;
    }

    animStatus = ObjectEventClearHeldMovementIfFinished(npc); //Wait jump animation
    if (animStatus == 0)
        return;

    //Bind objs
    SetSurfBlob_BobState(npc->fieldEffectSpriteId, 0x1);
    UnfreezeObjectEvents();
    DestroyTask(taskId);
    gPlayerAvatar.preventStep = FALSE; //Player can move again
    return;
}

static void SetUpSurfBlobFieldEffect(struct ObjectEvent* npc)
{
    //Set up gFieldEffectArguments for execution
    gFieldEffectArguments[0] = npc->currentCoords.x;     //effect_x
    gFieldEffectArguments[1] = npc->currentCoords.y;    //effect_y
    gFieldEffectArguments[2] = GetFollowerObjectId();    //objId
}

void PrepareFollowerDismountSurf(void)
{
    if (!gSaveBlock1Ptr->follower.inProgress)
        return;

    FollowMe(&gObjectEvents[gPlayerAvatar.objectEventId], MOVEMENT_ACTION_WALK_NORMAL_DOWN, TRUE);
    gSaveBlock1Ptr->follower.createSurfBlob = 3;
}

static void SetSurfDismount(void)
{
    struct ObjectEvent* follower;
    u8 direction;
    u8 jumpState;
    u8 task;

    if (!TryGetFollowerObjectEvent(&follower))
        return;

    ObjectEventClearHeldMovement(follower);

    //Jump animation according to direction
    direction = DetermineFollowerDirection(&gObjectEvents[gPlayerAvatar.objectEventId], follower);
    jumpState = GetJumpMovementAction(direction);

    //Unbind and destroy Surf Blob
    task = CreateTask(Task_FinishSurfDismount, 1);
    gTasks[task].data[0] = follower->fieldEffectSpriteId;
    SetSurfBlob_BobState(follower->fieldEffectSpriteId, 2);
    follower->fieldEffectSpriteId = 0; //Unbind
    FollowMe_HandleSprite();

    if (!TryGetFollowerObjectEvent(&follower))
        return;
    ObjectEventSetHeldMovement(follower, jumpState);
}

static void Task_FinishSurfDismount(u8 taskId)
{
    struct ObjectEvent* npc;
    bool8 animStatus;

    if (!TryGetFollowerObjectEvent(&npc))
    {
        DestroyTask(taskId);
        gPlayerAvatar.preventStep = FALSE;
        return;
    }

    animStatus = ObjectEventClearHeldMovementIfFinished(npc); //Wait animation

    if (animStatus == 0)
    {
        if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_DASH) && ObjectEventClearHeldMovementIfFinished(&gObjectEvents[gPlayerAvatar.objectEventId]))
            SetPlayerAvatarTransitionFlags(PLAYER_AVATAR_FLAG_ON_FOOT); //Temporarily stop running

        return;
    }

    DestroySprite(&gSprites[gTasks[taskId].data[0]]);
    UnfreezeObjectEvents();
    DestroyTask(taskId);
    gPlayerAvatar.preventStep = FALSE;
}

/**
 * ::ACIMUT::
 * 2022/04/15
 * - static
 * - No es llamada en otra parte de la inyección.
 * - Repuntear.
 */

void Task_DoDoorWarp(u8 taskId)
{
    struct Task *task = &gTasks[taskId];
    s16 *x = &task->data[2];
    s16 *y = &task->data[3];
    u8 playerObjId = gPlayerAvatar.objectEventId;
    u8 followerObjId = GetFollowerObjectId();

    switch (task->data[0])
    {
    case 0:
        if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_DASH))
            SetPlayerAvatarTransitionFlags(PLAYER_AVATAR_FLAG_ON_FOOT); //Stop running

        gSaveBlock1Ptr->follower.comeOutDoorStairs = 0; //Just in case came out and when right back in
        FreezeObjectEvents();
        PlayerGetDestCoords(x, y);
        PlaySE(GetDoorSoundEffect(*x, *y - 1));
        task->data[1] = FieldAnimateDoorOpen(*x, *y - 1);
        task->data[0] = 1;
        break;
    case 1:
        if (task->data[1] < 0 || gTasks[task->data[1]].isActive != TRUE)
        {
            ObjectEventClearHeldMovementIfActive(&gObjectEvents[playerObjId]);
            ObjectEventSetHeldMovement(&gObjectEvents[playerObjId], MOVEMENT_ACTION_WALK_NORMAL_UP);

            if (gSaveBlock1Ptr->follower.inProgress && !gObjectEvents[followerObjId].invisible)
            {
                u8 newState = DetermineFollowerState(&gObjectEvents[followerObjId], MOVEMENT_ACTION_WALK_NORMAL_UP,
                                                    DetermineFollowerDirection(&gObjectEvents[playerObjId], &gObjectEvents[followerObjId]));
                ObjectEventClearHeldMovementIfActive(&gObjectEvents[followerObjId]);
                ObjectEventSetHeldMovement(&gObjectEvents[followerObjId], newState);
            }

            task->data[0] = 2;
        }
        break;
    case 2:
        if (walkrun_is_standing_still())
        {
            if (!gSaveBlock1Ptr->follower.inProgress || gObjectEvents[followerObjId].invisible) //Don't close door on follower
                task->data[1] = FieldAnimateDoorClose(*x, *y - 1);
            ObjectEventClearHeldMovementIfFinished(&gObjectEvents[playerObjId]);
            SetPlayerVisibility(0);
            task->data[0] = 3;
        }
        break;
    case 3:
        if (task->data[1] < 0 || gTasks[task->data[1]].isActive != TRUE)
        {
            task->data[0] = 4;
        }
        break;
    case 4:
        if (gSaveBlock1Ptr->follower.inProgress)
        {
            ObjectEventClearHeldMovementIfActive(&gObjectEvents[followerObjId]);
            ObjectEventSetHeldMovement(&gObjectEvents[followerObjId], MOVEMENT_ACTION_WALK_NORMAL_UP);
        }

        TryFadeOutOldMapMusic();
        WarpFadeOutScreen();
        PlayRainStoppingSoundEffect();
        task->data[0] = 0;
        task->func = Task_Teleport2Warp;
        break;
    case 5:
        TryFadeOutOldMapMusic();
        PlayRainStoppingSoundEffect();
        task->data[0] = 0;
        task->func = Task_Teleport2Warp;
        break;
    }
}

static u8 GetPlayerFaceToDoorDirection(struct ObjectEvent* player, struct ObjectEvent* follower)
{
    s16 delta_x = player->currentCoords.x - follower->currentCoords.x;

    if (delta_x < 0)
        return DIR_EAST;
    else if (delta_x > 0)
        return DIR_WEST;

    return DIR_NORTH;
}

static void Task_FollowerOutOfDoor(u8 taskId)
{
    struct ObjectEvent *follower;
    struct ObjectEvent *player = &gObjectEvents[gPlayerAvatar.objectEventId];
    struct Task *task = &gTasks[taskId];
    s16 *x = &task->data[2];
    s16 *y = &task->data[3];

    if (!TryGetFollowerObjectEvent(&follower))
    {
        gPlayerAvatar.preventStep = FALSE;
        DestroyTask(taskId);
        return;
    }

    //if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_DASH) && ObjectEventClearHeldMovementIfFinished(player))
        //SetPlayerAvatarTransitionFlags(PLAYER_AVATAR_FLAG_ON_FOOT); //Temporarily stop running

    if (ObjectEventClearHeldMovementIfFinished(player))
        ObjectEventTurn(player, GetPlayerFaceToDoorDirection(player, follower)); //The player should face towards the follow as the exit the door

    switch (task->data[0])
    {
    case 0:
        FreezeObjectEvents();
        task->data[1] = FieldAnimateDoorOpen(follower->currentCoords.x, follower->currentCoords.y);
        if (task->data[1] != -1)
            PlaySE(GetDoorSoundEffect(*x, *y)); //only play SE for animating doors

        task->data[0] = 1;
        break;
    case 1:
        if (task->data[1] < 0 || gTasks[task->data[1]].isActive != TRUE) //if Door isn't still opening
        {
            follower->invisible = FALSE;
            ObjectEventTurn(follower, DIR_SOUTH); //The follower should be facing down when it comes out the door
            follower->singleMovementActive = FALSE;
            follower->heldMovementActive = FALSE;
            ObjectEventSetHeldMovement(follower, MOVEMENT_ACTION_WALK_NORMAL_DOWN); //follower step down
            task->data[0] = 2;
        }
        break;
    case 2:
        if (ObjectEventClearHeldMovementIfFinished(follower))
        {
            task->data[1] = FieldAnimateDoorClose(*x, *y);
            task->data[0] = 3;
        }
        break;
    case 3:
        if (task->data[1] < 0 || gTasks[task->data[1]].isActive != TRUE) //Door is closed
        {
            UnfreezeObjectEvents();
            task->data[0] = 4;
        }
        break;
    case 4:
        FollowMe_HandleSprite();
        gSaveBlock1Ptr->follower.comeOutDoorStairs = 0;
        gPlayerAvatar.preventStep = FALSE; //Player can move again
        DestroyTask(taskId);
        break;
    }
}

void StairsMoveFollower(void)
{
    if (!gSaveBlock1Ptr->follower.inProgress)
        return;

    if (!FuncIsActiveTask(Task_FollowerHandleIndoorStairs) && gSaveBlock1Ptr->follower.comeOutDoorStairs != 2)
        CreateTask(Task_FollowerHandleIndoorStairs, 1);
}

static void Task_FollowerHandleIndoorStairs(u8 taskId)
{
    struct ObjectEvent* follower;
    struct ObjectEvent* player = &gObjectEvents[gPlayerAvatar.objectEventId];
    struct Task *task = &gTasks[taskId];

    if (!TryGetFollowerObjectEvent(&follower))
    {
        gPlayerAvatar.preventStep = FALSE;
        DestroyTask(taskId);
        return;
    }

    if (!TryGetFollowerObjectEvent(&follower))
    {
        DestroyTask(taskId);
        return;
    }

    switch (task->data[0])
    {
    case 0:
        gSaveBlock1Ptr->follower.comeOutDoorStairs = 2; //So the task doesn't get created more than once
        ObjectEventClearHeldMovementIfActive(follower);
        ObjectEventSetHeldMovement(follower, DetermineFollowerState(follower, MOVEMENT_ACTION_WALK_NORMAL_DOWN, DetermineFollowerDirection(player, follower)));
        task->data[0]++;
        break;
    case 1:
        if (ObjectEventClearHeldMovementIfFinished(follower))
        {
            ObjectEventSetHeldMovement(follower, DetermineFollowerState(follower, MOVEMENT_ACTION_WALK_NORMAL_DOWN, player->movementDirection));
            DestroyTask(taskId);
        }
        break;
    }
}

void EscalatorMoveFollower(u8 movementType)
{
    u8 taskId;

    if (!gSaveBlock1Ptr->follower.inProgress)
        return;

    taskId = CreateTask(Task_FollowerHandleEscalator, 1);
    gTasks[taskId].data[1] = movementType;
}

static void Task_FollowerHandleEscalator(u8 taskId)
{
    struct ObjectEvent* follower;
    struct ObjectEvent* player = &gObjectEvents[gPlayerAvatar.objectEventId];

    ObjectEventClearHeldMovementIfActive(follower);
    ObjectEventSetHeldMovement(follower, DetermineFollowerState(follower, MOVEMENT_ACTION_WALK_NORMAL_DOWN, DetermineFollowerDirection(player, follower)));
    DestroyTask(taskId);
}

void EscalatorMoveFollowerFinish(void)
{
    if (!gSaveBlock1Ptr->follower.inProgress)
        return;

    CreateTask(Task_FollowerHandleEscalatorFinish, 1);
}

static void Task_FollowerHandleEscalatorFinish(u8 taskId)
{
    s16 x, y;
    struct ObjectEvent* follower;
    struct ObjectEvent* player = &gObjectEvents[gPlayerAvatar.objectEventId];
    struct Sprite* sprite = &gSprites[follower->spriteId];
    struct Task *task = &gTasks[taskId];

    switch (task->data[0])
    {
    case 0:
        MoveObjectEventToMapCoords(follower, player->currentCoords.x, player->currentCoords.y);
        PlayerGetDestCoords(&x, &y);
        task->data[2] = MapGridGetMetatileBehaviorAt(x, y);
        task->data[7] = 0;
        task->data[0]++;
        break;
    case 1:
        if (task->data[7]++ < 0x20) //Wait half a second before revealing the follower
            break;

        task->data[0]++;
        task->data[1] = 16;
        CalculateFollowerEscalatorTrajectoryUp(task);
        gSaveBlock1Ptr->follower.warpEnd = 0;
        gPlayerAvatar.preventStep = TRUE;
        ObjectEventSetHeldMovement(follower, GetFaceDirectionMovementAction(DIR_EAST));
        if (task->data[2] == 0x6b)
            task->data[0] = 4;
        break;
    case 2:
        follower->invisible = FALSE;
        CalculateFollowerEscalatorTrajectoryDown(task);
        task->data[0]++;
        break;
    case 3:
        CalculateFollowerEscalatorTrajectoryDown(task);
        task->data[2]++;
        if (task->data[2] & 1)
        {
            task->data[1]--;
        }

        if (task->data[1] == 0)
        {
            sprite->x2 = 0;
            sprite->y2 = 0;
            task->data[0] = 6;
        }
        break;
    case 4:
        follower->invisible = FALSE;
        CalculateFollowerEscalatorTrajectoryUp(task);
        task->data[0]++;
        break;
    case 5:
        CalculateFollowerEscalatorTrajectoryUp(task);
        task->data[2]++;
        if (task->data[2] & 1)
        {
            task->data[1]--;
        }

        if (task->data[1] == 0)
        {
            sprite->x2 = 0;
            sprite->y2 = 0;
            task->data[0]++;
        }
        break;
    case 6:
        if (ObjectEventClearHeldMovementIfFinished(follower))
        {
            gPlayerAvatar.preventStep = FALSE;
            DestroyTask(taskId);
        }
    }
}

static void CalculateFollowerEscalatorTrajectoryDown(struct Task *task)
{
    struct ObjectEvent *follower;
    struct Sprite *sprite;

    if (!TryGetFollowerObjectEvent(&follower))
        return;

    sprite = &gSprites[follower->spriteId];
    sprite->x2 = Cos(0x84, task->data[1]);
    sprite->y2 = Sin(0x94, task->data[1]);
}

static void CalculateFollowerEscalatorTrajectoryUp(struct Task *task)
{
    struct ObjectEvent *follower;
    struct Sprite *sprite;

    if (!TryGetFollowerObjectEvent(&follower))
        return;

    sprite = &gSprites[follower->spriteId];
    sprite->x2 = Cos(0x7c, task->data[1]);
    sprite->y2 = Sin(0x76, task->data[1]);
}

bool8 FollowerCanBike(void)
{
    if (!gSaveBlock1Ptr->follower.inProgress)
        return TRUE;
    else if (gSaveBlock1Ptr->follower.flags & FOLLOWER_FLAG_CAN_BIKE)
        return TRUE;
    else
        return FALSE;
}

void FollowMe_HandleBike(void)
{
    if (gSaveBlock1Ptr->follower.currentSprite == FOLLOWER_SPRITE_INDEX_SURF) //Follower is surfing
        return; //Sprite will automatically be adjusted when they finish surfing

    if (gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_MACH_BIKE && FollowerCanBike() && gSaveBlock1Ptr->follower.comeOutDoorStairs != 1) //Coming out door
        SetFollowerSprite(FOLLOWER_SPRITE_INDEX_MACH_BIKE); //Mach Bike on
    else if (gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_ACRO_BIKE && FollowerCanBike() && gSaveBlock1Ptr->follower.comeOutDoorStairs != 1) //Coming out door
        SetFollowerSprite(FOLLOWER_SPRITE_INDEX_ACRO_BIKE); //Acro Bike on
    else
        SetFollowerSprite(FOLLOWER_SPRITE_INDEX_NORMAL);
}

void FollowMe_HandleSprite(void)
{
    if (!gSaveBlock1Ptr->follower.inProgress)
        return;

    // Ensure origin NPC is hidden while follower is active (CFRU-like).
    FollowMe_HideOriginNpcOnCurrentMap();

    if (gSaveBlock1Ptr->follower.flags & FOLLOWER_FLAG_CAN_BIKE)
    {
        if (gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_MACH_BIKE)
            SetFollowerSprite(FOLLOWER_SPRITE_INDEX_MACH_BIKE);
        else if (gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_ACRO_BIKE)
            SetFollowerSprite(FOLLOWER_SPRITE_INDEX_ACRO_BIKE);
    }
    else if (gMapHeader.mapType == MAP_TYPE_UNDERWATER)
    {
        TryUpdateFollowerSpriteUnderwater();
    }
    else
    {
        SetFollowerSprite(FOLLOWER_SPRITE_INDEX_NORMAL);
    }
}

void SetFollowerSprite(u8 spriteIndex)
{
    u8 oldSpriteId;
    u8 newSpriteId;
    u8 followerObjId;
    u16 newGraphicsId;
    struct ObjectEventTemplate clone;
    struct ObjectEvent backupFollower;
    struct ObjectEvent *follower;

    if (!TryGetFollowerObjectEvent(&follower))
        return;

    if (gSaveBlock1Ptr->follower.currentSprite == spriteIndex)
        return;

    // Save sprite
    followerObjId = gSaveBlock1Ptr->follower.objId;
    gSaveBlock1Ptr->follower.currentSprite = spriteIndex;
    oldSpriteId = follower->spriteId;
    newGraphicsId = GetFollowerSprite();

    // Reload the entire event object.
    // It would usually be enough just to change the sprite Id, but if the original
    // sprite and the new sprite have different palettes, the palette would need to
    // be reloaded.
    backupFollower = *follower;
    backupFollower.graphicsId = newGraphicsId;
    DestroySprite(&gSprites[oldSpriteId]);
    RemoveObjectEvent(&gObjectEvents[followerObjId]);

    clone = *GetObjectEventTemplateByLocalIdAndMap(gSaveBlock1Ptr->follower.map.id,
                                                  gSaveBlock1Ptr->follower.map.number,
                                                  gSaveBlock1Ptr->follower.map.group);
    clone.localId = OBJ_EVENT_ID_FOLLOWER;
    clone.flagId = 0;
    clone.script = NULL;
    clone.graphicsId = newGraphicsId;
    clone.x = backupFollower.currentCoords.x - MAP_OFFSET;
    clone.y = backupFollower.currentCoords.y - MAP_OFFSET;
    clone.elevation = backupFollower.currentElevation;
    clone.movementType = MOVEMENT_TYPE_NONE;

    gSaveBlock1Ptr->follower.objId = TrySpawnObjectEventTemplate(&clone,
                                                                 gSaveBlock1Ptr->location.mapNum,
                                                                 gSaveBlock1Ptr->location.mapGroup,
                                                                 clone.x, clone.y);

    if (gSaveBlock1Ptr->follower.objId == OBJECT_EVENTS_COUNT)
    {
        gSaveBlock1Ptr->follower.inProgress = FALSE;
        return;
    }

    if (!TryGetFollowerObjectEvent(&follower))
        return;

    newSpriteId = follower->spriteId;
    *follower = backupFollower;
    follower->spriteId = newSpriteId;
    MoveObjectEventToMapCoords(follower, follower->currentCoords.x, follower->currentCoords.y);
    ObjectEventTurn(follower, follower->facingDirection);
}

void FollowMe_WarpSetEnd(void)
{
    struct ObjectEvent *player;
    struct ObjectEvent *follower;
    u8 toY;

    if (!TryGetFollowerObjectEvent(&follower))
        return;

    player = &gObjectEvents[gPlayerAvatar.objectEventId];

    gSaveBlock1Ptr->follower.warpEnd = 1;
    PlayerLogCoordinates(player);

    toY = gSaveBlock1Ptr->follower.comeOutDoorStairs == 1 ? (player->currentCoords.y - 1) : player->currentCoords.y;
    MoveObjectEventToMapCoords(follower, player->currentCoords.x, toY);

    follower->facingDirection = player->facingDirection;
    follower->movementDirection = player->movementDirection;
}

void CreateFollowerAvatar(void)
{
    struct ObjectEvent *player;
    struct ObjectEvent *follower;
    struct ObjectEventTemplate clone;

    if (!gSaveBlock1Ptr->follower.inProgress)
        return;

    // CFRU-like: if a follower object already exists on this map (e.g. the NPC was converted in-place
    // or objects were not fully reset), reuse it and remove any duplicates.
    {
        u8 objId;
        u8 existingFollower = OBJECT_EVENTS_COUNT;
        for (objId = 0; objId < OBJECT_EVENTS_COUNT; objId++)
        {
            if (!gObjectEvents[objId].active || gObjectEvents[objId].isPlayer)
                continue;

            if (gObjectEvents[objId].localId == OBJ_EVENT_ID_FOLLOWER)
            {
                if (existingFollower == OBJECT_EVENTS_COUNT)
                    existingFollower = objId;
                else
                    RemoveObjectEvent(&gObjectEvents[objId]);
            }
        }

        if (existingFollower != OBJECT_EVENTS_COUNT)
        {
            gSaveBlock1Ptr->follower.objId = existingFollower;
            gObjectEvents[existingFollower].movementType = MOVEMENT_TYPE_NONE;
            if (gObjectEvents[existingFollower].spriteId < MAX_SPRITES)
                gSprites[gObjectEvents[existingFollower].spriteId].callback = MovementType_None;
            return;
        }
    }

    player = &gObjectEvents[gPlayerAvatar.objectEventId];

    clone = *GetObjectEventTemplateByLocalIdAndMap(gSaveBlock1Ptr->follower.map.id,
                                                  gSaveBlock1Ptr->follower.map.number,
                                                  gSaveBlock1Ptr->follower.map.group);

    // Spawn a dedicated follower event object for this map (single follower, no per-map NPC requirement)
    clone.localId = OBJ_EVENT_ID_FOLLOWER;
    clone.flagId = 0;
    clone.script = NULL;
    clone.graphicsId = GetFollowerSprite();
    clone.x = player->currentCoords.x - MAP_OFFSET;
    clone.y = player->currentCoords.y - MAP_OFFSET;
    clone.elevation = player->currentElevation;
    clone.movementType = MOVEMENT_TYPE_NONE;

    gSaveBlock1Ptr->follower.objId = TrySpawnObjectEventTemplate(&clone,
                                                                 gSaveBlock1Ptr->location.mapNum,
                                                                 gSaveBlock1Ptr->location.mapGroup,
                                                                 clone.x, clone.y);

    if (gSaveBlock1Ptr->follower.objId == OBJECT_EVENTS_COUNT)
    {
        // Cancel the following because couldn't load sprite (too many objects on this map)
        gSaveBlock1Ptr->follower.inProgress = FALSE;
        return;
    }

    follower = &gObjectEvents[gSaveBlock1Ptr->follower.objId];
    follower->localId = OBJ_EVENT_ID_FOLLOWER;
    follower->movementType = MOVEMENT_TYPE_NONE;

    if (gMapHeader.mapType == MAP_TYPE_UNDERWATER)
        gSaveBlock1Ptr->follower.createSurfBlob = 0;

    follower->invisible = TRUE;
    ObjectEventTurn(follower, player->facingDirection);
}

// CFRU-like behavior: If the follower originated from an NPC on a specific map, hide that
// original NPC whenever that origin map is (re)loaded while the follower is active.
// This prevents seeing both the follower and the original NPC at the same time.
void FollowMe_HideOriginNpcOnCurrentMap(void)
{
    u8 objId;

    if (!gSaveBlock1Ptr->follower.inProgress)
        return;

    // Only hide on the origin map (where the NPC follower was recruited).
    if (gSaveBlock1Ptr->location.mapNum != gSaveBlock1Ptr->follower.map.number
     || gSaveBlock1Ptr->location.mapGroup != gSaveBlock1Ptr->follower.map.group)
        return;

    if (gSaveBlock1Ptr->follower.map.id == 0)
        return;

    for (objId = 0; objId < OBJECT_EVENTS_COUNT; objId++)
    {
        if (!gObjectEvents[objId].active || gObjectEvents[objId].isPlayer)
            continue;

        // Remove the original NPC (by its original localId). The follower uses localId 0xFE.
        if (gObjectEvents[objId].localId == gSaveBlock1Ptr->follower.map.id)
        {
            RemoveObjectEvent(&gObjectEvents[objId]);
            return;
        }
    }
}

static void TurnNPCIntoFollower(u8 localId, u16 followerFlags)
{
    struct ObjectEvent* follower;
    u8 eventObjId;
    u8 originalLocalId;
    const u8 *script;

    if (gSaveBlock1Ptr->follower.inProgress)
        return; //Only 1 NPC following at a time

    // 0 means "use last talked" from scripts that don't pass a localId explicitly.
    if (localId == 0)
        localId = (u8)gSpecialVar_LastTalked;

    // Never allow selecting the follower virtual localId.
    if (localId == 0 || localId == OBJ_EVENT_ID_FOLLOWER)
        return;

    for (eventObjId = 0; eventObjId < OBJECT_EVENTS_COUNT; eventObjId++) //For each NPC on the map
    {
        if (!gObjectEvents[eventObjId].active || gObjectEvents[eventObjId].isPlayer)
            continue;

        if (gObjectEvents[eventObjId].localId == localId)
        {
            follower = &gObjectEvents[eventObjId];
            follower->movementType = MOVEMENT_TYPE_NONE; //Doesn't get to move on its own anymore
            gSprites[follower->spriteId].callback = MovementType_None; //MovementType_None
            if (followerFlags & FOLLOWER_FLAG_CUSTOM_FOLLOW_SCRIPT)
                script = (const u8 *)ReadWord(0);
            else
                script = GetObjectEventScriptPointerByObjectEventId(eventObjId);

            originalLocalId = follower->localId;
            follower->localId = OBJ_EVENT_ID_FOLLOWER;
            gSaveBlock1Ptr->follower.inProgress = TRUE;
            gSaveBlock1Ptr->follower.objId = eventObjId;
            gSaveBlock1Ptr->follower.graphicsId = follower->graphicsId;
            gSaveBlock1Ptr->follower.map.id = originalLocalId;
            gSaveBlock1Ptr->follower.map.number = gSaveBlock1Ptr->location.mapNum;
            gSaveBlock1Ptr->follower.map.group = gSaveBlock1Ptr->location.mapGroup;
            gSaveBlock1Ptr->follower.script = script;
            gSaveBlock1Ptr->follower.flags = followerFlags;
            gSaveBlock1Ptr->follower.createSurfBlob = 0;
            gSaveBlock1Ptr->follower.comeOutDoorStairs = 0;

            if (!(gSaveBlock1Ptr->follower.flags & FOLLOWER_FLAG_CAN_BIKE) //Follower can't bike
            &&  TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_BIKE)) //Player on bike
                SetPlayerAvatarTransitionFlags(PLAYER_AVATAR_FLAG_ON_FOOT); //Dismmount Bike

            // CFRU-like safety: ensure origin NPC is not present (no-op in the common case).
            FollowMe_HideOriginNpcOnCurrentMap();
            return;
        }
    }
}

bool8 CheckFollowerFlag(u16 flag)
{
    if (!gSaveBlock1Ptr->follower.inProgress)
        return TRUE;

    if (gSaveBlock1Ptr->follower.flags & flag)
        return TRUE;

    return FALSE;
}

static u8 GetPlayerMapObjId(void)
{
	return gPlayerAvatar.objectEventId;
}

enum
{
	GoDown,
	GoUp,
	GoLeft,
	GoRight
};
void FollowerPositionFix(u8 offset)
{
    u8 playerObjId = GetPlayerMapObjId();
    u8 followerObjid = gSaveBlock1Ptr->follower.objId;
    u16 playerX = gObjectEvents[playerObjId].currentCoords.x;
    u16 playerY = gObjectEvents[playerObjId].currentCoords.y;
    u16 npcX = gObjectEvents[followerObjid].currentCoords.x;
    u16 npcY = gObjectEvents[followerObjid].currentCoords.y;

    gSpecialVar_Result = 0xFFFF;

    if (!gSaveBlock1Ptr->follower.inProgress)
        return;

    if (playerX == npcX)
    {
        if (playerY > npcY)
        {
            if (playerY != npcY + offset) //Player and follower are not 1 tile apart
            {
                if (gSpecialVar_0x8000 == 0)
                    gSpecialVar_Result = GoDown;
                else
                    gObjectEvents[followerObjid].currentCoords.y = playerY - offset;
            }
        }
        else // Player Y <= npcY
        {
            if (playerY != npcY - offset) //Player and follower are not 1 tile apart
            {
                if (gSpecialVar_0x8000 == 0)
                    gSpecialVar_Result = GoUp;
                else
                    gObjectEvents[followerObjid].currentCoords.y = playerY + offset;
            }
        }
    }
    else //playerY == npcY
    {
        if (playerX > npcX)
        {
            if (playerX != npcX + offset) //Player and follower are not 1 tile apart
            {
                if (gSpecialVar_0x8000 == 0)
                    gSpecialVar_Result = GoRight;
                else
                    gObjectEvents[followerObjid].currentCoords.x = playerX - offset;
            }
        }
        else // Player X <= npcX
        {
            if (playerX != npcX - offset) //Player and follower are not 1 tile apart
            {
                if (gSpecialVar_0x8000 == 0)
                    gSpecialVar_Result = GoLeft;
                else
                    gObjectEvents[followerObjid].currentCoords.x = playerX + offset;
            }
        }
    }
}

void FollowerTrainerSightingPositionFix(void)
{
    FollowerPositionFix(1);
}

void FollowerIntoPlayer(void)
{
    FollowerPositionFix(0);
}

bool8 PlayerHasFollower(void)
{
    return gSaveBlock1Ptr->follower.inProgress;
}

bool8 IsPlayerOnFoot(void)
{
    if (gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_ON_FOOT)
        return TRUE;
    else
        return FALSE;
}

bool8 FollowerComingThroughDoor(void)
{
    if (!PlayerHasFollower())
        return FALSE;

    if (gSaveBlock1Ptr->follower.comeOutDoorStairs)
        return TRUE;

    return FALSE;
}

//////////////////SCRIPTING////////////////////
//@Details: Sets up the follow me feature.
//@Input:    local id - NPC to start following player.
//            flags - Follower flags.
void SetUpFollowerSprite(u8 localId, u16 flags)
{
    TurnNPCIntoFollower(localId, flags);
}

//@Details: Ends the follow me feature.
void DestroyFollower(void)
{
    u8 followerObjId;
    u8 originLocalId;
    u8 originMapNum;
    u8 originMapGroup;
    u8 objId;

    if (!gSaveBlock1Ptr->follower.inProgress)
        return;

    // Save origin data so we can restore the NPC immediately if we're still on the origin map.
    originLocalId = gSaveBlock1Ptr->follower.map.id;
    originMapNum = gSaveBlock1Ptr->follower.map.number;
    originMapGroup = gSaveBlock1Ptr->follower.map.group;

    followerObjId = GetFollowerObjectId();
    if (followerObjId != OBJECT_EVENTS_COUNT)
        RemoveObjectEvent(&gObjectEvents[followerObjId]);

    // Clear follower state (do NOT set/clear flags; origin hiding is handled runtime like CFRU).
    gSaveBlock1Ptr->follower.inProgress = FALSE;
    gSaveBlock1Ptr->follower.objId = OBJECT_EVENTS_COUNT;
    gSaveBlock1Ptr->follower.script = NULL;
    gSaveBlock1Ptr->follower.flags = 0;
    gSaveBlock1Ptr->follower.createSurfBlob = 0;
    gSaveBlock1Ptr->follower.comeOutDoorStairs = 0;

    // If we're currently on the origin map, respawn the original NPC immediately from its template.
    if (originLocalId != 0
     && gSaveBlock1Ptr->location.mapNum == originMapNum
     && gSaveBlock1Ptr->location.mapGroup == originMapGroup)
    {
        // Only respawn if it's not already present.
        for (objId = 0; objId < OBJECT_EVENTS_COUNT; objId++)
        {
            if (!gObjectEvents[objId].active || gObjectEvents[objId].isPlayer)
                continue;
            if (gObjectEvents[objId].localId == originLocalId)
                goto clear_origin;
        }

        {
            const struct ObjectEventTemplate *tmpl = GetObjectEventTemplateByLocalIdAndMap(originLocalId, originMapNum, originMapGroup);
            if (tmpl != NULL)
                TrySpawnObjectEventTemplate(tmpl, originMapNum, originMapGroup, tmpl->x, tmpl->y);
        }
    }

clear_origin:
    // Clear origin fields.
    gSaveBlock1Ptr->follower.map.id = 0;
    gSaveBlock1Ptr->follower.map.number = 0;
    gSaveBlock1Ptr->follower.map.group = 0;
}

//@Details: Faces the player and the follower sprite towards each other.
void PlayerFaceFollowerSprite(void)
{
    if (gSaveBlock1Ptr->follower.inProgress)
    {
        u8 playerDirection, followerDirection;
        struct ObjectEvent* player, *follower;

        player = &gObjectEvents[gPlayerAvatar.objectEventId];
        follower = &gObjectEvents[gSaveBlock1Ptr->follower.objId];
        playerDirection = DetermineFollowerDirection(player, follower);
        followerDirection = playerDirection;

        //Flip direction
        switch (playerDirection) 
        {
        case DIR_NORTH:
            playerDirection = DIR_SOUTH;
            followerDirection = DIR_NORTH;
            break;
        case DIR_SOUTH:
            playerDirection = DIR_NORTH;
            followerDirection = DIR_SOUTH;
            break;
        case DIR_WEST:
            playerDirection = DIR_EAST;
            followerDirection = DIR_WEST;
            break;
        case DIR_EAST:
            playerDirection = DIR_WEST;
            followerDirection = DIR_EAST;
            break;
        }

        ObjectEventTurn(player, playerDirection);
        ObjectEventTurn(follower, followerDirection);
    }
}

//@Details: Checks if the player is being followed.
//@Returns: LastResult: 0 if the Player isn't being followed. 1 otherwise.
void CheckPlayerHasFollower(void)
{
    gSpecialVar_Result = gSaveBlock1Ptr->follower.inProgress;
}



